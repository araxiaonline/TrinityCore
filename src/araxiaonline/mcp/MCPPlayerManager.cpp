/*
 * Araxia MCP Player Manager Implementation
 * 
 * Manages multiple AI-controlled player sessions.
 * Each session represents one LLM controlling one player character.
 */

#include "MCPPlayerManager.h"
#include "AraxiaCore.h"
#include "Config.h"
#include "Log.h"
#include "World.h"
#include "WorldSession.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "CharacterCache.h"
#include "Map.h"
#include "MapManager.h"
#include "MotionMaster.h"
#include "SpellMgr.h"
#include "DatabaseEnv.h"
#include "QueryHolder.h"
#include "Chat.h"
#include "CharacterPackets.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>

namespace Araxia
{

// MCPPlayerSession::isOnline() - defined here since Player must be fully defined
bool MCPPlayerSession::isOnline() const
{
    return player != nullptr && player->IsInWorld();
}

// ============================================================================
// Login Query Holder - Prepares DB queries for player login
// ============================================================================

class MCPPlayerLoginQueryHolder : public CharacterDatabaseQueryHolder
{
public:
    MCPPlayerLoginQueryHolder(uint32 accountId, ObjectGuid guid, uint32 sessionId)
        : _accountId(accountId), _guid(guid), _sessionId(sessionId) { }
    
    ObjectGuid GetGuid() const { return _guid; }
    uint32 GetAccountId() const { return _accountId; }
    uint32 GetSessionId() const { return _sessionId; }
    bool Initialize();

private:
    uint32 _accountId;
    ObjectGuid _guid;
    uint32 _sessionId;
};

bool MCPPlayerLoginQueryHolder::Initialize()
{
    SetSize(MAX_PLAYER_LOGIN_QUERY);
    bool res = true;
    ObjectGuid::LowType lowGuid = _guid.GetCounter();

    // Core character data
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_FROM, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_CUSTOMIZATIONS);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_CUSTOMIZATIONS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GROUP_MEMBER);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_GROUP, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_AURAS);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_AURAS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_AURA_EFFECTS);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_AURA_EFFECTS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_AURA_STORED_LOCATIONS);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_AURA_STORED_LOCATIONS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SPELL);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SPELLS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SPELL_FAVORITES);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SPELL_FAVORITES, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_INVENTORY);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_INVENTORY, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_REPUTATION);
    stmt->setUInt64(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_REPUTATION, stmt);

    // Add more queries as needed for full player functionality
    // This is a minimal set for basic operation

    return res;
}

// ============================================================================
// MCPPlayerManager Implementation
// ============================================================================

MCPPlayerManager::MCPPlayerManager() = default;
MCPPlayerManager::~MCPPlayerManager()
{
    // Don't call Shutdown() from destructor during static destruction
    // The mutex may already be destroyed. Shutdown should be called explicitly
    // from AraxiaMCPServer::Shutdown() before process exit.
    // If we get here with sessions still active, they'll leak but at least we won't crash.
    if (_initialized)
    {
        TC_LOG_WARN("araxia.mcp", "[MCPPlayerManager] Destructor called while still initialized - sessions may leak");
    }
}

MCPPlayerManager* MCPPlayerManager::Instance()
{
    static MCPPlayerManager instance;
    return &instance;
}

bool MCPPlayerManager::Initialize()
{
    if (_initialized)
        return true;
    
    // Load configuration
    _maxSessions = sConfigMgr->GetIntDefault("Araxia.MCP.Player.MaxSessions", 10);
    _defaultAccountId = sConfigMgr->GetIntDefault("Araxia.MCP.Player.DefaultAccountId", 0);
    _sessionTimeoutSeconds = sConfigMgr->GetIntDefault("Araxia.MCP.Player.SessionTimeout", 3600);
    
    // Register with AraxiaCore for World::Update callbacks
    sAraxiaCore->RegisterUpdateCallback("MCPPlayerManager", [this](uint32 diff) {
        Update(diff);
    });
    
    TC_LOG_INFO("araxia.mcp", "[MCPPlayerManager] Initialized (maxSessions: {}, defaultAccount: {})",
                _maxSessions, _defaultAccountId);
    
    _initialized = true;
    return true;
}

void MCPPlayerManager::Update(uint32 /*diff*/)
{
    // Process async operations for all bot sessions on the world thread
    std::lock_guard<std::mutex> lock(_sessionMutex);
    
    for (auto& [sessionId, session] : _sessions)
    {
        // Process pending logouts first (must happen on world thread)
        if (session->logoutPending && session->player)
        {
            LogoutInternal(session.get());
            session->logoutPending = false;
        }
        
        // Process login callbacks
        if (session->worldSession && session->loginPending)
        {
            // Process any pending database query callbacks
            TC_LOG_DEBUG("araxia.mcp", "[MCPPlayerManager] Processing callbacks for session {} (loginPending={})", 
                        sessionId, session->loginPending);
            session->worldSession->GetQueryProcessor().ProcessReadyCallbacks();
        }
    }
}

void MCPPlayerManager::Shutdown()
{
    if (!_initialized)
        return;
    
    // Mark as not initialized first to prevent re-entry
    _initialized = false;
    
    // Unregister from AraxiaCore
    sAraxiaCore->UnregisterUpdateCallback("MCPPlayerManager");
    
    TC_LOG_INFO("araxia.mcp", "[MCPPlayerManager] Shutting down, logging out all sessions...");
    
    // Try to lock mutex - if it fails (during static destruction), just proceed without lock
    std::unique_lock<std::mutex> lock(_sessionMutex, std::try_to_lock);
    if (!lock.owns_lock())
    {
        TC_LOG_WARN("araxia.mcp", "[MCPPlayerManager] Could not acquire mutex during shutdown - proceeding anyway");
    }
    for (auto& [id, session] : _sessions)
    {
        if (!session)
            continue;
            
        TC_LOG_INFO("araxia.mcp", "[MCPPlayerManager] Cleaning up session {}", id);
        
        // Always clean up player if it exists
        if (session->player)
        {
            TC_LOG_INFO("araxia.mcp", "[MCPPlayerManager] Removing player from world...");
            
            // Check if player is still in world and has a valid map
            // Use IsInWorld() instead of GetMap() which asserts on null
            if (session->player->IsInWorld())
            {
                // FindMap doesn't assert, returns nullptr if map is gone
                Map* map = sMapMgr->FindMap(session->player->GetMapId(), session->player->GetInstanceId());
                if (map)
                {
                    // RemovePlayerFromMap with remove=true will call DeleteFromWorld
                    map->RemovePlayerFromMap(session->player, true);
                    session->player = nullptr;  // Map deleted it
                }
                else
                {
                    // Map already gone, just clean up the player object
                    TC_LOG_INFO("araxia.mcp", "[MCPPlayerManager] Map already unloaded, cleaning up player directly");
                    ObjectAccessor::RemoveObject(session->player);
                    delete session->player;
                    session->player = nullptr;
                }
            }
            else
            {
                // Not in world, just delete directly
                ObjectAccessor::RemoveObject(session->player);
                delete session->player;
                session->player = nullptr;
            }
        }
        
        // Clean up WorldSession
        if (session->worldSession)
        {
            // Clear the packet capture callback to avoid dangling references
            session->worldSession->SetPacketCaptureCallback(nullptr);
            delete session->worldSession;
            session->worldSession = nullptr;
        }
    }
    _sessions.clear();
    _tokenToSession.clear();
    
    TC_LOG_INFO("araxia.mcp", "[MCPPlayerManager] Shutdown complete");
    _initialized = false;
}

// ============================================================================
// Session Management
// ============================================================================

std::string MCPPlayerManager::GenerateToken()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 32; ++i)
        ss << dis(gen);
    return ss.str();
}

uint32 MCPPlayerManager::CreateSession(const std::string& ownerName)
{
    std::lock_guard<std::mutex> lock(_sessionMutex);
    
    if (_sessions.size() >= _maxSessions)
    {
        TC_LOG_WARN("araxia.mcp", "[MCPPlayerManager] Max sessions reached (%zu)", _sessions.size());
        return 0;
    }
    
    auto session = std::make_unique<MCPPlayerSession>();
    session->sessionId = _nextSessionId++;
    session->sessionToken = GenerateToken();
    session->ownerName = ownerName;
    session->createdAt = time(nullptr);
    session->lastActivity = time(nullptr);
    session->accountId = _defaultAccountId;
    
    uint32 sessionId = session->sessionId;
    _tokenToSession[session->sessionToken] = sessionId;
    _sessions[sessionId] = std::move(session);
    
    TC_LOG_INFO("araxia.mcp", "[MCPPlayerManager] Created session {} for '{}'", 
                sessionId, ownerName);
    
    return sessionId;
}

bool MCPPlayerManager::DestroySession(uint32 sessionId)
{
    std::lock_guard<std::mutex> lock(_sessionMutex);
    
    auto it = _sessions.find(sessionId);
    if (it == _sessions.end())
        return false;
    
    MCPPlayerSession* session = it->second.get();
    
    // Logout player if online
    if (session->isOnline())
    {
        LogoutInternal(session);
    }
    
    // Remove token mapping
    _tokenToSession.erase(session->sessionToken);
    
    TC_LOG_INFO("araxia.mcp", "[MCPPlayerManager] Destroyed session {} ('{}')", 
                sessionId, session->ownerName);
    
    _sessions.erase(it);
    return true;
}

MCPPlayerSession* MCPPlayerManager::GetSession(uint32 sessionId)
{
    std::lock_guard<std::mutex> lock(_sessionMutex);
    auto it = _sessions.find(sessionId);
    return (it != _sessions.end()) ? it->second.get() : nullptr;
}

MCPPlayerSession* MCPPlayerManager::GetSessionByToken(const std::string& token)
{
    std::lock_guard<std::mutex> lock(_sessionMutex);
    auto it = _tokenToSession.find(token);
    if (it == _tokenToSession.end())
        return nullptr;
    
    auto sessionIt = _sessions.find(it->second);
    return (sessionIt != _sessions.end()) ? sessionIt->second.get() : nullptr;
}

std::vector<uint32> MCPPlayerManager::GetActiveSessions() const
{
    std::lock_guard<std::mutex> lock(_sessionMutex);
    std::vector<uint32> result;
    result.reserve(_sessions.size());
    for (const auto& [id, session] : _sessions)
        result.push_back(id);
    return result;
}

size_t MCPPlayerManager::GetSessionCount() const
{
    std::lock_guard<std::mutex> lock(_sessionMutex);
    return _sessions.size();
}

// ============================================================================
// Player Lifecycle
// ============================================================================

WorldSession* MCPPlayerManager::CreateBotSession(uint32 sessionId, uint32 accountId)
{
    // Create a WorldSession with NO socket (nullptr)
    // This is the key to making a headless player work!
    
    WorldSession* session = new WorldSession(
        accountId,                          // Account ID
        std::string("MCPBot"),              // Account name
        0,                                  // Battlenet account ID
        std::string(""),                    // Battlenet account email
        std::shared_ptr<WorldSocket>(),     // Socket = NULL (no network!)
        SEC_GAMEMASTER,                     // Security level - GM for full access
        EXPANSION_THE_WAR_WITHIN,           // Expansion
        0,                                  // Mute time
        std::string("Bot"),                 // OS string
        Minutes(0),                         // Timezone offset
        0,                                  // Client build
        {},                                 // Build variant
        LOCALE_enUS,                        // Locale
        0,                                  // Recruiter
        false                               // Is recruiter
    );
    
    // NOTE: We do NOT add to sWorld - we'll handle updates manually
    // sWorld->AddSession() can cause issues with null-socket sessions
    
    // Set up packet capture callback for this session
    session->SetPacketCaptureCallback([sessionId, this](WorldPacket const& packet) {
        CapturePacket(sessionId, packet);
    });
    
    TC_LOG_INFO("araxia.mcp", "[MCPPlayerManager] Created bot session for account {} with packet capture", accountId);
    
    return session;
}

bool MCPPlayerManager::Login(uint32 sessionId, ObjectGuid playerGuid)
{
    MCPPlayerSession* session = GetSession(sessionId);
    if (!session)
    {
        TC_LOG_ERROR("araxia.mcp", "[MCPPlayerManager] Login failed - session {} not found", sessionId);
        return false;
    }
    
    if (session->isOnline())
    {
        TC_LOG_WARN("araxia.mcp", "[MCPPlayerManager] Session {} already has player online", sessionId);
        return false;
    }
    
    if (session->loginPending)
    {
        TC_LOG_WARN("araxia.mcp", "[MCPPlayerManager] Session {} login already pending", sessionId);
        return false;
    }
    
    // Get account ID for this character
    uint32 accountId = sCharacterCache->GetCharacterAccountIdByGuid(playerGuid);
    if (!accountId)
    {
        TC_LOG_ERROR("araxia.mcp", "[MCPPlayerManager] Character {} not found", playerGuid.ToString());
        return false;
    }
    
    // Use session's account ID if set, otherwise use character's account
    if (session->accountId > 0)
        accountId = session->accountId;
    
    TC_LOG_INFO("araxia.mcp", "[MCPPlayerManager] Session {} initiating login for {} (account {})",
                sessionId, playerGuid.ToString(), accountId);
    
    // Create the WorldSession
    session->worldSession = CreateBotSession(sessionId, accountId);
    if (!session->worldSession)
    {
        TC_LOG_ERROR("araxia.mcp", "[MCPPlayerManager] Failed to create bot session");
        return false;
    }
    
    session->playerGuid = playerGuid;
    session->loginPending = true;
    session->lastActivity = time(nullptr);
    
    // First, query basic character info for caching
    QueryResult result = CharacterDatabase.PQuery(
        "SELECT guid, account, name, race, class, level, map, position_x, position_y, position_z "
        "FROM characters WHERE guid = {}", playerGuid.GetCounter());
    
    if (!result)
    {
        TC_LOG_ERROR("araxia.mcp", "[MCPPlayerManager] Character {} not found in database", 
                    playerGuid.ToString());
        delete session->worldSession;
        session->worldSession = nullptr;
        session->loginPending = false;
        return false;
    }
    
    // Cache basic info
    Field* fields = result->Fetch();
    session->characterName = fields[2].GetString();
    session->accountId = fields[1].GetUInt32();
    session->level = fields[5].GetUInt8();
    session->race = fields[3].GetUInt8();
    session->playerClass = fields[4].GetUInt8();
    session->mapId = fields[6].GetUInt32();
    session->posX = fields[7].GetFloat();
    session->posY = fields[8].GetFloat();
    session->posZ = fields[9].GetFloat();
    
    TC_LOG_INFO("araxia.mcp", "[MCPPlayerManager] Character '{}' found (Level {}, Map {})",
                session->characterName, session->level, session->mapId);
    
    // Phase 1: Proper player initialization using InitializeForBot
    TC_LOG_INFO("araxia.mcp", "[MCPPlayerManager] Initializing player (Phase 1 - proper init)...");
    
    // Create Player object
    session->player = new Player(session->worldSession);
    
    // Initialize using our new bot-specific method (handles GUID, race, class, stats, etc.)
    session->player->InitializeForBot(
        playerGuid,
        session->characterName,
        session->race,
        session->playerClass,
        GENDER_MALE,  // Default, we didn't query gender
        session->level
    );
    
    // Relocate to saved position
    session->player->Relocate(session->posX, session->posY, session->posZ, session->orientation);
    
    // Get/create the map
    Map* map = sMapMgr->CreateMap(session->mapId, session->player);
    if (!map)
    {
        TC_LOG_ERROR("araxia.mcp", "[MCPPlayerManager] Failed to create map {}", session->mapId);
        delete session->player;
        session->player = nullptr;
        session->loginPending = false;
        return true;
    }
    
    // Set map and update position data
    session->player->SetMap(map);
    session->player->UpdatePositionData();
    
    // Clear any phase restrictions so bot is visible to everyone
    session->player->GetPhaseShift().Clear();
    session->player->GetPhaseShift().AddPhase(169, PhaseFlags::None, nullptr);  // Default phase
    
    // Link session to player
    session->worldSession->SetPlayer(session->player);
    
    // Add player to map
    if (!map->AddPlayerToMap(session->player))
    {
        TC_LOG_ERROR("araxia.mcp", "[MCPPlayerManager] Failed to add player to map");
        session->worldSession->SetPlayer(nullptr);
        delete session->player;
        session->player = nullptr;
        session->loginPending = false;
        return true;
    }
    
    // Add to ObjectAccessor so world can find us
    ObjectAccessor::AddObject(session->player);
    
    session->loginPending = false;
    
    TC_LOG_INFO("araxia.mcp", "[MCPPlayerManager] Session {}: Player '{}' initialized and added to world!",
                session->sessionId, session->characterName);
    
    return true;
}

bool MCPPlayerManager::Login(uint32 sessionId, const std::string& characterName)
{
    ObjectGuid guid = sCharacterCache->GetCharacterGuidByName(characterName);
    if (guid.IsEmpty())
    {
        TC_LOG_ERROR("araxia.mcp", "[MCPPlayerManager] Character '{}' not found", characterName);
        return false;
    }
    return Login(sessionId, guid);
}

void MCPPlayerManager::HandleLoginCallback(SQLQueryHolderBase const& holderBase)
{
    TC_LOG_INFO("araxia.mcp", "[MCPPlayerManager] HandleLoginCallback fired!");
    
    MCPPlayerLoginQueryHolder const& holder = static_cast<MCPPlayerLoginQueryHolder const&>(holderBase);
    uint32 sessionId = holder.GetSessionId();
    
    MCPPlayerSession* session = GetSession(sessionId);
    if (!session)
    {
        TC_LOG_ERROR("araxia.mcp", "[MCPPlayerManager] Login callback but session {} gone!", sessionId);
        return;
    }
    
    session->loginPending = false;
    
    if (!session->worldSession)
    {
        TC_LOG_ERROR("araxia.mcp", "[MCPPlayerManager] Login callback but no WorldSession!");
        return;
    }
    
    // Create the Player object
    session->player = new Player(session->worldSession);
    
    // Load player data from database
    if (!session->player->LoadFromDB(session->playerGuid, holder))
    {
        TC_LOG_ERROR("araxia.mcp", "[MCPPlayerManager] Failed to load player from database");
        delete session->player;
        session->player = nullptr;
        delete session->worldSession;
        session->worldSession = nullptr;
        return;
    }
    
    // Initialize motion master
    session->player->GetMotionMaster()->Initialize();
    
    // Add player to map
    Map* map = sMapMgr->CreateMap(session->player->GetMapId(), session->player);
    if (!map || !map->AddPlayerToMap(session->player))
    {
        TC_LOG_ERROR("araxia.mcp", "[MCPPlayerManager] Failed to add player to map {}", session->player->GetMapId());
        
        // Try homebind
        if (!session->player->TeleportTo(session->player->m_homebind))
        {
            TC_LOG_ERROR("araxia.mcp", "[MCPPlayerManager] Failed to teleport to homebind, aborting");
            delete session->player;
            session->player = nullptr;
            delete session->worldSession;
            session->worldSession = nullptr;
            return;
        }
    }
    
    // Add to ObjectAccessor
    ObjectAccessor::AddObject(session->player);
    
    // Mark online in database
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_ONLINE);
    stmt->setUInt64(0, session->player->GetGUID().GetCounter());
    CharacterDatabase.Execute(stmt);
    
    // Set player in session
    session->worldSession->SetPlayer(session->player);
    
    TC_LOG_INFO("araxia.mcp", "[MCPPlayerManager] Session {}: '{}' logged in at ({:.1f}, {:.1f}, {:.1f}) map {}",
                sessionId, session->player->GetName(),
                session->player->GetPositionX(), session->player->GetPositionY(), 
                session->player->GetPositionZ(), session->player->GetMapId());
}

void MCPPlayerManager::Logout(uint32 sessionId)
{
    std::lock_guard<std::mutex> lock(_sessionMutex);
    
    auto it = _sessions.find(sessionId);
    if (it == _sessions.end())
        return;
    
    // Queue logout to happen on world thread during Update()
    // Map operations are not thread-safe
    it->second->logoutPending = true;
    TC_LOG_INFO("araxia.mcp", "[MCPPlayerManager] Logout queued for session {}", sessionId);
}

void MCPPlayerManager::LogoutInternal(MCPPlayerSession* session)
{
    if (!session || !session->player)
        return;
    
    TC_LOG_INFO("araxia.mcp", "[MCPPlayerManager] Logging out '{}' (session {})",
                session->player->GetName(), session->sessionId);
    
    // For bot players, we need to manually clean up without going through the full
    // RemovePlayerFromMap path which calls RemoveFromWorld -> DoLootReleaseAll
    // and can cause hangs due to uninitialized player state
    
    Player* player = session->player;
    session->player = nullptr;  // Clear reference first
    
    TC_LOG_DEBUG("araxia.mcp", "[MCPPlayerManager] Step 1: Clearing session player reference");
    
    // Unlink from WorldSession
    if (session->worldSession)
    {
        session->worldSession->SetPlayer(nullptr);
    }
    
    TC_LOG_DEBUG("araxia.mcp", "[MCPPlayerManager] Step 2: Unlinked from WorldSession");
    
    // Remove from ObjectAccessor
    ObjectAccessor::RemoveObject(player);
    
    TC_LOG_DEBUG("araxia.mcp", "[MCPPlayerManager] Step 3: Removed from ObjectAccessor");
    
    // Remove from map grid without full cleanup (avoid RemoveFromWorld)
    if (player->IsInWorld())
    {
        TC_LOG_DEBUG("araxia.mcp", "[MCPPlayerManager] Step 4: Player is in world, removing from grid");
        
        // Manually remove from grid if in grid
        if (player->IsInGrid())
        {
            player->RemoveFromGrid();
            TC_LOG_DEBUG("araxia.mcp", "[MCPPlayerManager] Step 4a: Removed from grid");
        }
        
        // Clear the in-world flag manually
        player->Object::RemoveFromWorld();
        TC_LOG_DEBUG("araxia.mcp", "[MCPPlayerManager] Step 4b: Cleared in-world flag");
    }
    
    TC_LOG_DEBUG("araxia.mcp", "[MCPPlayerManager] Step 5: Deleting player object");
    
    // Delete the player
    delete player;
    
    TC_LOG_DEBUG("araxia.mcp", "[MCPPlayerManager] Step 6: Player deleted");
    
    // Mark offline
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_ONLINE);
    stmt->setUInt64(0, session->playerGuid.GetCounter());
    CharacterDatabase.Execute(stmt);
    
    TC_LOG_INFO("araxia.mcp", "[MCPPlayerManager] Logout complete for session {}", session->sessionId);
    
    if (session->worldSession)
    {
        delete session->worldSession;
        session->worldSession = nullptr;
    }
    
    session->playerGuid.Clear();
}

bool MCPPlayerManager::IsOnline(uint32 sessionId) const
{
    std::lock_guard<std::mutex> lock(_sessionMutex);
    auto it = _sessions.find(sessionId);
    return (it != _sessions.end()) && it->second->isOnline();
}

Player* MCPPlayerManager::GetPlayer(uint32 sessionId) const
{
    std::lock_guard<std::mutex> lock(_sessionMutex);
    auto it = _sessions.find(sessionId);
    return (it != _sessions.end() && it->second->isOnline()) ? it->second->player : nullptr;
}

// ============================================================================
// Movement
// ============================================================================

bool MCPPlayerManager::TeleportTo(uint32 sessionId, uint32 mapId, float x, float y, float z, float o)
{
    Player* player = GetPlayer(sessionId);
    if (!player)
        return false;
    
    TC_LOG_INFO("araxia.mcp", "[MCPPlayerManager] Session {}: Teleporting to ({:.1f}, {:.1f}, {:.1f}) map {}",
                sessionId, x, y, z, mapId);
    
    bool isCrossMap = (player->GetMapId() != mapId);
    
    if (!isCrossMap)
    {
        // Same map teleport - direct relocate for headless sessions
        // NearTeleportTo expects client ACK, so we manually relocate
        player->Relocate(x, y, z, o);
        player->UpdatePositionData();
        player->UpdateObjectVisibility();
        return true;
    }
    
    // Cross-map teleport - manual implementation for headless sessions
    // We can't use Player::TeleportTo() + HandleMoveWorldportAck() because
    // it expects client packets and has state assumptions that don't work headless
    
    TC_LOG_DEBUG("araxia.mcp", "[MCPPlayerManager] Session {}: Cross-map teleport from {} to {}", 
                 sessionId, player->GetMapId(), mapId);
    
    // Validate destination
    if (!MapManager::IsValidMapCoord(mapId, x, y, z))
    {
        TC_LOG_ERROR("araxia.mcp", "[MCPPlayerManager] Invalid coordinates for teleport");
        return false;
    }
    
    // Get/create the destination map
    Map* newMap = sMapMgr->CreateMap(mapId, player);
    if (!newMap)
    {
        TC_LOG_ERROR("araxia.mcp", "[MCPPlayerManager] Failed to create map {}", mapId);
        return false;
    }
    
    // Check if we can enter
    if (newMap->CannotEnter(player))
    {
        TC_LOG_ERROR("araxia.mcp", "[MCPPlayerManager] Cannot enter map {}", mapId);
        return false;
    }
    
    // Remove from current map
    Map* oldMap = player->FindMap();
    if (oldMap && player->IsInWorld())
    {
        oldMap->RemovePlayerFromMap(player, false);
    }
    
    // Relocate and set new map
    player->Relocate(x, y, z, o);
    player->ResetMap();
    player->SetMap(newMap);
    player->UpdatePositionData();
    
    // Add to new map
    if (!newMap->AddPlayerToMap(player))
    {
        TC_LOG_ERROR("araxia.mcp", "[MCPPlayerManager] Failed to add player to map {}", mapId);
        // Try to recover by going back to old map
        if (oldMap)
        {
            player->ResetMap();
            player->SetMap(oldMap);
            oldMap->AddPlayerToMap(player);
        }
        return false;
    }
    
    TC_LOG_INFO("araxia.mcp", "[MCPPlayerManager] Session {}: Cross-map teleport complete", sessionId);
    return true;
}

bool MCPPlayerManager::MoveTo(uint32 sessionId, float x, float y, float z)
{
    Player* player = GetPlayer(sessionId);
    if (!player)
        return false;
    
    TC_LOG_DEBUG("araxia.mcp", "[MCPPlayerManager] Session {}: Moving to ({:.1f}, {:.1f}, {:.1f})",
                sessionId, x, y, z);
    
    // Use walking speed for natural movement
    float walkSpeed = player->GetSpeed(MOVE_WALK);
    player->GetMotionMaster()->MovePoint(0, x, y, z, true, {}, walkSpeed);
    return true;
}

void MCPPlayerManager::StopMovement(uint32 sessionId)
{
    Player* player = GetPlayer(sessionId);
    if (!player)
        return;
    
    player->StopMoving();
    player->GetMotionMaster()->Clear();
}

Position MCPPlayerManager::GetPosition(uint32 sessionId) const
{
    Player* player = GetPlayer(sessionId);
    return player ? player->GetPosition() : Position();
}

uint32 MCPPlayerManager::GetMapId(uint32 sessionId) const
{
    Player* player = GetPlayer(sessionId);
    return player ? player->GetMapId() : 0;
}

// ============================================================================
// Actions
// ============================================================================

bool MCPPlayerManager::SetTarget(uint32 sessionId, ObjectGuid targetGuid)
{
    Player* player = GetPlayer(sessionId);
    if (!player)
        return false;
    
    player->SetTarget(targetGuid);
    return true;
}

bool MCPPlayerManager::CastSpell(uint32 sessionId, uint32 spellId, ObjectGuid targetGuid)
{
    Player* player = GetPlayer(sessionId);
    if (!player)
        return false;
    
    Unit* target = nullptr;
    if (!targetGuid.IsEmpty())
        target = ObjectAccessor::GetUnit(*player, targetGuid);
    else
        target = player->GetSelectedUnit();
    
    if (!target)
        target = player;
    
    return player->CastSpell(target, spellId, false) == SPELL_CAST_OK;
}

bool MCPPlayerManager::InteractWith(uint32 sessionId, ObjectGuid /*targetGuid*/)
{
    Player* player = GetPlayer(sessionId);
    if (!player)
        return false;
    
    // TODO: Implement interaction logic based on target type
    TC_LOG_WARN("araxia.mcp", "[MCPPlayerManager] InteractWith not fully implemented");
    return false;
}

bool MCPPlayerManager::Attack(uint32 sessionId, ObjectGuid targetGuid)
{
    Player* player = GetPlayer(sessionId);
    if (!player)
        return false;
    
    Unit* target = ObjectAccessor::GetUnit(*player, targetGuid);
    if (!target)
        return false;
    
    player->Attack(target, true);
    return true;
}

void MCPPlayerManager::StopAttack(uint32 sessionId)
{
    Player* player = GetPlayer(sessionId);
    if (player)
        player->AttackStop();
}

// ============================================================================
// GM Commands
// ============================================================================

std::string MCPPlayerManager::ExecuteCommand(uint32 sessionId, const std::string& command)
{
    Player* player = GetPlayer(sessionId);
    if (!player)
        return "Error: Player not online";
    
    TC_LOG_INFO("araxia.mcp", "[MCPPlayerManager] Session {} executing command: {}", 
                sessionId, command);
    
    // Execute command via ChatHandler
    ChatHandler handler(player->GetSession());
    
    // Commands need the dot prefix
    std::string fullCommand = "." + command;
    
    if (handler.ParseCommands(fullCommand.c_str()))
        return "Command executed successfully";
    else
        return "Command failed or not found";
}

// ============================================================================
// Perception
// ============================================================================

std::vector<ObjectGuid> MCPPlayerManager::GetNearbyEntities(uint32 sessionId, float /*range*/, uint32 /*typeMask*/) const
{
    std::vector<ObjectGuid> result;
    
    Player* player = GetPlayer(sessionId);
    if (!player || !player->IsInWorld())
        return result;
    
    // TODO: Implement entity scanning based on typeMask
    
    return result;
}

// ============================================================================
// Packet Capture
// ============================================================================

void MCPPlayerManager::CapturePacket(uint32 sessionId, WorldPacket const& packet)
{
    std::lock_guard<std::mutex> lock(_sessionMutex);
    
    auto it = _sessions.find(sessionId);
    if (it == _sessions.end())
        return;
    
    MCPPlayerSession* session = it->second.get();
    
    // Store packet data: opcode + raw data
    std::lock_guard<std::mutex> pktLock(session->packetMutex);
    
    std::vector<uint8> data;
    data.resize(4 + packet.size());  // 4 bytes for opcode + payload
    
    // Store opcode (4 bytes, little endian)
    uint32 opcode = packet.GetOpcode();
    data[0] = opcode & 0xFF;
    data[1] = (opcode >> 8) & 0xFF;
    data[2] = (opcode >> 16) & 0xFF;
    data[3] = (opcode >> 24) & 0xFF;
    
    // Copy packet data
    if (packet.size() > 0)
        memcpy(&data[4], packet.data(), packet.size());
    
    session->outboundPackets.push(std::move(data));
    
    // Limit queue size to prevent memory bloat
    while (session->outboundPackets.size() > 1000)
        session->outboundPackets.pop();
}

void MCPPlayerManager::QueueInboundPacket(uint32 /*sessionId*/, uint16 /*opcode*/, const std::vector<uint8>& /*data*/)
{
    // TODO: Implement when we need to inject packets into the session
    TC_LOG_DEBUG("araxia.mcp", "[MCPPlayerManager] QueueInboundPacket not yet implemented");
}

std::vector<std::pair<uint16, std::vector<uint8>>> MCPPlayerManager::GetOutboundPackets(uint32 sessionId)
{
    std::lock_guard<std::mutex> lock(_sessionMutex);
    
    auto it = _sessions.find(sessionId);
    if (it == _sessions.end())
        return {};
    
    MCPPlayerSession* session = it->second.get();
    std::lock_guard<std::mutex> pktLock(session->packetMutex);
    
    std::vector<std::pair<uint16, std::vector<uint8>>> result;
    
    while (!session->outboundPackets.empty())
    {
        auto& pkt = session->outboundPackets.front();
        if (pkt.size() >= 4)
        {
            uint16 opcode = pkt[0] | (pkt[1] << 8);
            std::vector<uint8> payload(pkt.begin() + 4, pkt.end());
            result.emplace_back(opcode, std::move(payload));
        }
        session->outboundPackets.pop();
    }
    
    return result;
}

} // namespace Araxia
