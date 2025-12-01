/*
 * Araxia MCP Server - World Scan (LIDAR-style room visualization)
 * 
 * Provides spatial awareness by casting rays and detecting walls/creatures.
 */

#include "AraxiaMCPServer.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "Map.h"
#include "World.h"
#include "Creature.h"
#include "GameObject.h"
#include "Log.h"
#include "VMapFactory.h"
#include "VMapManager2.h"
#include <cmath>

namespace Araxia
{

// Constants
constexpr float PI = 3.14159265358979323846f;
constexpr float DEG_TO_RAD = PI / 180.0f;

// Ray cast result
struct RayCastResult
{
    float angle;        // Angle in degrees from North
    float distance;     // Distance to hit (max range if no hit)
    bool hit;           // Did we hit something?
};

// Scan a single ray from player position
RayCastResult CastRay(Map* map, float startX, float startY, float startZ, 
                      float angle, float maxRange, float heightOffset = 1.0f)
{
    RayCastResult result;
    result.angle = angle;
    result.hit = false;
    result.distance = maxRange;
    
    // Convert angle to radians (WoW: 0 = North, clockwise)
    float radians = angle * DEG_TO_RAD;
    
    // Calculate end point
    float endX = startX + std::cos(radians) * maxRange;
    float endY = startY + std::sin(radians) * maxRange;
    float endZ = startZ + heightOffset;  // Slightly above ground
    
    // Use VMAP for line of sight check
    // Note: isInLineOfSight returns true if CAN see (no obstacle)
    VMAP::VMapManager2* vmgr = VMAP::VMapFactory::createOrGetVMapManager();
    if (vmgr)
    {
        // Binary search to find collision distance
        float lo = 0.0f;
        float hi = maxRange;
        float checkX, checkY;
        
        // Quick check - can we see the max range?
        if (!vmgr->isInLineOfSight(map->GetId(), startX, startY, startZ + heightOffset,
                                    endX, endY, endZ, VMAP::ModelIgnoreFlags::Nothing))
        {
            // There's an obstacle, find it with binary search
            result.hit = true;
            
            for (int i = 0; i < 10; ++i)  // 10 iterations = ~0.1 yard precision
            {
                float mid = (lo + hi) / 2.0f;
                checkX = startX + std::cos(radians) * mid;
                checkY = startY + std::sin(radians) * mid;
                
                if (vmgr->isInLineOfSight(map->GetId(), startX, startY, startZ + heightOffset,
                                          checkX, checkY, startZ + heightOffset, 
                                          VMAP::ModelIgnoreFlags::Nothing))
                {
                    lo = mid;  // Can see this far, obstacle is further
                }
                else
                {
                    hi = mid;  // Can't see this far, obstacle is closer
                }
            }
            
            result.distance = (lo + hi) / 2.0f;
        }
    }
    
    return result;
}

// Perform full 360 degree scan
json PerformWorldScan(Player* player, float range, int rayCount)
{
    if (!player || !player->GetMap())
    {
        return {{"success", false}, {"error", "Invalid player or map"}};
    }
    
    Map* map = player->GetMap();
    float playerX = player->GetPositionX();
    float playerY = player->GetPositionY();
    float playerZ = player->GetPositionZ();
    float playerO = player->GetOrientation();  // Facing direction in radians
    
    // Convert orientation to degrees (WoW uses radians, 0 = North, counter-clockwise)
    float facingDegrees = playerO * (180.0f / PI);
    
    json rays = json::array();
    float angleStep = 360.0f / rayCount;
    
    for (int i = 0; i < rayCount; ++i)
    {
        float angle = i * angleStep;
        RayCastResult result = CastRay(map, playerX, playerY, playerZ, angle, range);
        
        rays.push_back({
            {"angle", result.angle},
            {"distance", result.distance},
            {"hit", result.hit}
        });
    }
    
    // Get nearby creatures
    json creatures = json::array();
    std::list<Creature*> creatureList;
    player->GetCreatureListWithEntryInGrid(creatureList, 0, range);  // 0 = any entry
    
    for (Creature* creature : creatureList)
    {
        if (!creature || !creature->IsAlive())
            continue;
            
        float dx = creature->GetPositionX() - playerX;
        float dy = creature->GetPositionY() - playerY;
        float distance = std::sqrt(dx*dx + dy*dy);
        float angleToCreature = std::atan2(dy, dx) * (180.0f / PI);
        
        // Normalize angle to 0-360
        if (angleToCreature < 0) angleToCreature += 360.0f;
        
        // Calculate relative angle from player facing
        float relativeAngle = angleToCreature - facingDegrees;
        if (relativeAngle < -180) relativeAngle += 360;
        if (relativeAngle > 180) relativeAngle -= 360;
        
        creatures.push_back({
            {"name", creature->GetName()},
            {"entry", creature->GetEntry()},
            {"guid", creature->GetGUID().GetCounter()},
            {"distance", distance},
            {"angle", angleToCreature},
            {"relativeAngle", relativeAngle},  // Positive = right, negative = left
            {"level", creature->GetLevel()},
            {"health", creature->GetHealth()},
            {"maxHealth", creature->GetMaxHealth()},
            {"x", creature->GetPositionX()},
            {"y", creature->GetPositionY()},
            {"z", creature->GetPositionZ()}
        });
    }
    
    return {
        {"success", true},
        {"player", {
            {"x", playerX},
            {"y", playerY},
            {"z", playerZ},
            {"facing", facingDegrees},
            {"facingRad", playerO},
            {"mapId", map->GetId()},
            {"zone", player->GetZoneId()},
            {"area", player->GetAreaId()}
        }},
        {"scan", {
            {"range", range},
            {"rayCount", rayCount},
            {"rays", rays}
        }},
        {"creatures", creatures},
        {"creatureCount", creatures.size()}
    };
}

// Generate ASCII art visualization
std::string GenerateAsciiMap(const json& scanData, int size = 21)
{
    if (!scanData.contains("success") || !scanData["success"].get<bool>())
        return "Scan failed";
    
    // Create empty grid
    std::vector<std::string> grid(size, std::string(size, ' '));
    int center = size / 2;
    
    // Draw walls based on ray hits
    float range = scanData["scan"]["range"].get<float>();
    auto& rays = scanData["scan"]["rays"];
    
    for (const auto& ray : rays)
    {
        if (ray["hit"].get<bool>())
        {
            float angle = ray["angle"].get<float>() * DEG_TO_RAD;
            float dist = ray["distance"].get<float>();
            float scale = (size / 2.0f - 1) / range;
            
            int x = center + static_cast<int>(std::cos(angle) * dist * scale);
            int y = center - static_cast<int>(std::sin(angle) * dist * scale);  // Invert Y for display
            
            if (x >= 0 && x < size && y >= 0 && y < size)
                grid[y][x] = '#';
        }
    }
    
    // Draw creatures
    auto& creatures = scanData["creatures"];
    float facingDeg = scanData["player"]["facing"].get<float>();
    
    for (const auto& creature : creatures)
    {
        float angle = creature["angle"].get<float>() * DEG_TO_RAD;
        float dist = creature["distance"].get<float>();
        float scale = (size / 2.0f - 1) / range;
        
        int x = center + static_cast<int>(std::cos(angle) * dist * scale);
        int y = center - static_cast<int>(std::sin(angle) * dist * scale);
        
        if (x >= 0 && x < size && y >= 0 && y < size && grid[y][x] == ' ')
        {
            std::string name = creature["name"].get<std::string>();
            grid[y][x] = name.empty() ? '?' : std::toupper(name[0]);
        }
    }
    
    // Draw player at center with facing direction
    grid[center][center] = '@';
    
    // Draw facing indicator
    float facingRad = facingDeg * DEG_TO_RAD;
    int fx = center + static_cast<int>(std::cos(facingRad) * 1.5f);
    int fy = center - static_cast<int>(std::sin(facingRad) * 1.5f);
    if (fx >= 0 && fx < size && fy >= 0 && fy < size && grid[fy][fx] == ' ')
        grid[fy][fx] = '>';
    
    // Add border and compile
    std::string result = "+" + std::string(size, '-') + "+\n";
    for (const auto& row : grid)
    {
        result += "|" + row + "|\n";
    }
    result += "+" + std::string(size, '-') + "+\n";
    result += "@ = You, # = Wall, Letters = Creatures\n";
    result += "Facing: " + std::to_string(static_cast<int>(facingDeg)) + " degrees";
    
    return result;
}

void RegisterWorldScanTools()
{
    // world_scan - LIDAR-style room scan
    sMCPServer->RegisterTool(
        "world_scan",
        "Perform a LIDAR-style scan of surroundings. Returns wall distances and creature positions.",
        {
            {"type", "object"},
            {"properties", {
                {"player", {
                    {"type", "string"},
                    {"description", "Player name to scan from (default: first online player)"}
                }},
                {"range", {
                    {"type", "number"},
                    {"description", "Scan range in yards (default: 40)"}
                }},
                {"rayCount", {
                    {"type", "integer"},
                    {"description", "Number of rays to cast (default: 72 = every 5 degrees)"}
                }},
                {"ascii", {
                    {"type", "boolean"},
                    {"description", "Include ASCII art visualization (default: true)"}
                }}
            }}
        },
        [](const json& params) -> json {
            try
            {
                std::string playerName = params.value("player", "");
                float range = params.value("range", 40.0f);
                int rayCount = params.value("rayCount", 72);
                bool includeAscii = params.value("ascii", true);
                
                // Clamp values
                range = std::min(std::max(range, 5.0f), 100.0f);
                rayCount = std::min(std::max(rayCount, 8), 360);
                
                // Find player
                Player* player = nullptr;
                if (!playerName.empty())
                {
                    player = ObjectAccessor::FindPlayerByName(playerName);
                }
                else
                {
                    // Get first online player
                    SessionMap const& sessions = sWorld->GetAllSessions();
                    for (auto const& [id, session] : sessions)
                    {
                        if (session && session->GetPlayer())
                        {
                            player = session->GetPlayer();
                            break;
                        }
                    }
                }
                
                if (!player)
                {
                    return {
                        {"success", false},
                        {"error", "No player found"}
                    };
                }
                
                TC_LOG_INFO("araxia.mcp", "[MCP] World scan for %s (range: %.0f, rays: %d)",
                            player->GetName().c_str(), range, rayCount);
                
                json result = PerformWorldScan(player, range, rayCount);
                
                if (includeAscii && result["success"].get<bool>())
                {
                    result["asciiMap"] = GenerateAsciiMap(result);
                }
                
                return result;
            }
            catch (const std::exception& e)
            {
                return {{"success", false}, {"error", std::string("Scan exception: ") + e.what()}};
            }
            catch (...)
            {
                return {{"success", false}, {"error", "Unknown scan exception"}};
            }
        }
    );
    
    TC_LOG_INFO("araxia.mcp", "[MCP] World scan tools registered");
}

} // namespace Araxia
