/*
 * Araxia MCP Player Manager
 * 
 * Multi-session manager for AI-controlled players.
 * Allows multiple LLMs to each control their own player character via MCP.
 * 
 * Architecture:
 *   LLM (Cascade/Claude/etc.)
 *        ↓ MCP HTTP Request
 *   MCPPlayerManager
 *        ↓ session_id lookup
 *   MCPPlayerSession
 *        ↓
 *   WorldSession + Player
 */

#ifndef ARAXIA_MCP_PLAYER_MANAGER_H
#define ARAXIA_MCP_PLAYER_MANAGER_H

#include "Define.h"
#include "ObjectGuid.h"
#include "Position.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <queue>
#include <mutex>

class Player;
class WorldSession;
class WorldPacket;
class SQLQueryHolderBase;

namespace Araxia
{

/**
 * Represents a single AI-controlled player session.
 * Each LLM that connects gets their own session.
 */
struct MCPPlayerSession
{
    uint32 sessionId{0};            // Unique session identifier
    std::string sessionToken;       // Auth token for this session
    std::string ownerName;          // "Cascade", "Claude", "GPT-4", etc.
    
    WorldSession* worldSession{nullptr};  // The WoW session (with virtual socket)
    Player* player{nullptr};              // The player entity in world
    ObjectGuid playerGuid;                // Character GUID
    std::string characterName;            // Character name (for display)
    uint32 accountId{0};                  // WoW account ID
    
    // Cached character info (from DB query, before full load)
    uint8 level{0};
    uint8 race{0};
    uint8 playerClass{0};
    uint32 mapId{0};
    float posX{0}, posY{0}, posZ{0}, orientation{0};
    
    // Timestamps
    time_t createdAt{0};            // When session was created
    time_t lastActivity{0};         // Last MCP activity
    
    // Packet queues for advanced packet-level control
    std::queue<std::vector<uint8>> outboundPackets;  // Packets FROM server TO LLM
    std::mutex packetMutex;
    
    // State
    bool loginPending{false};
    bool logoutPending{false};
    
    // Note: isOnline() is defined in .cpp since Player must be fully defined
    bool isOnline() const;
};

/**
 * MCPPlayerManager - Singleton that manages all AI player sessions.
 * 
 * Workflow:
 *   1. LLM calls mcp_session_create → gets session_id and token
 *   2. LLM calls mcp_player_login with session_id → player loads into world
 *   3. LLM calls various mcp_player_* tools with session_id
 *   4. LLM calls mcp_session_destroy when done
 */
class MCPPlayerManager
{
public:
    static MCPPlayerManager* Instance();
    
    // ========== Initialization ==========
    
    bool Initialize();
    void Shutdown();
    void Update(uint32 diff);  // Called from world thread
    
    // ========== Session Management ==========
    
    /**
     * Create a new AI player session.
     * @param ownerName Identifier for the LLM (e.g., "Cascade", "Claude")
     * @return Session ID (0 if failed)
     */
    uint32 CreateSession(const std::string& ownerName);
    
    /**
     * Destroy a session, logging out the player if online.
     * @param sessionId The session to destroy
     * @return true if session was found and destroyed
     */
    bool DestroySession(uint32 sessionId);
    
    /**
     * Get a session by ID.
     */
    MCPPlayerSession* GetSession(uint32 sessionId);
    
    /**
     * Get a session by its token.
     */
    MCPPlayerSession* GetSessionByToken(const std::string& token);
    
    /**
     * Get all active session IDs.
     */
    std::vector<uint32> GetActiveSessions() const;
    
    /**
     * Get count of active sessions.
     */
    size_t GetSessionCount() const;
    
    // ========== Player Lifecycle ==========
    
    /**
     * Log in a character for a session.
     * @param sessionId The session
     * @param playerGuid Character GUID to log in
     * @return true if login initiated (async)
     */
    bool Login(uint32 sessionId, ObjectGuid playerGuid);
    
    /**
     * Log in by character name.
     */
    bool Login(uint32 sessionId, const std::string& characterName);
    
    /**
     * Log out the player for a session (keeps session alive).
     */
    void Logout(uint32 sessionId);
    
    /**
     * Check if a session's player is online.
     */
    bool IsOnline(uint32 sessionId) const;
    
    /**
     * Get the Player object for a session.
     */
    Player* GetPlayer(uint32 sessionId) const;
    
    // ========== Movement ==========
    
    bool TeleportTo(uint32 sessionId, uint32 mapId, float x, float y, float z, float o = 0.0f);
    bool MoveTo(uint32 sessionId, float x, float y, float z);
    void StopMovement(uint32 sessionId);
    Position GetPosition(uint32 sessionId) const;
    uint32 GetMapId(uint32 sessionId) const;
    
    // ========== Actions ==========
    
    bool SetTarget(uint32 sessionId, ObjectGuid targetGuid);
    bool CastSpell(uint32 sessionId, uint32 spellId, ObjectGuid target = ObjectGuid::Empty);
    bool InteractWith(uint32 sessionId, ObjectGuid targetGuid);
    bool Attack(uint32 sessionId, ObjectGuid targetGuid);
    void StopAttack(uint32 sessionId);
    
    // ========== GM Commands ==========
    
    std::string ExecuteCommand(uint32 sessionId, const std::string& command);
    
    // ========== Perception ==========
    
    std::vector<ObjectGuid> GetNearbyEntities(uint32 sessionId, float range, uint32 typeMask) const;
    
    // ========== Packet Access (Advanced) ==========
    
    /**
     * Queue a packet to be sent to the session's player.
     * This is for advanced LLMs that want raw packet control.
     */
    void QueueInboundPacket(uint32 sessionId, uint16 opcode, const std::vector<uint8>& data);
    
    /**
     * Get packets that the server has sent to the player.
     * Returns and clears the outbound queue.
     */
    std::vector<std::pair<uint16, std::vector<uint8>>> GetOutboundPackets(uint32 sessionId);
    
    // ========== Configuration ==========
    
    uint32 GetMaxSessions() const { return _maxSessions; }
    void SetMaxSessions(uint32 max) { _maxSessions = max; }
    
private:
    MCPPlayerManager();
    ~MCPPlayerManager();
    
    // Non-copyable
    MCPPlayerManager(const MCPPlayerManager&) = delete;
    MCPPlayerManager& operator=(const MCPPlayerManager&) = delete;
    
    // Internal helpers
    std::string GenerateToken();
    WorldSession* CreateBotSession(uint32 sessionId, uint32 accountId);
    void LogoutInternal(MCPPlayerSession* session);
    void HandleLoginCallback(SQLQueryHolderBase const& holder);
    void CapturePacket(uint32 sessionId, WorldPacket const& packet);
    
    // Members
    std::unordered_map<uint32, std::unique_ptr<MCPPlayerSession>> _sessions;
    std::unordered_map<std::string, uint32> _tokenToSession;  // token → session_id
    
    mutable std::mutex _sessionMutex;
    uint32 _nextSessionId{1};
    uint32 _maxSessions{10};  // Configurable limit
    bool _initialized{false};
    
    // Configuration
    uint32 _defaultAccountId{0};
    uint32 _sessionTimeoutSeconds{3600};  // 1 hour default
};

#define sMCPPlayerMgr Araxia::MCPPlayerManager::Instance()

// Tool registration
void RegisterMCPPlayerTools();

} // namespace Araxia

#endif // ARAXIA_MCP_PLAYER_MANAGER_H
