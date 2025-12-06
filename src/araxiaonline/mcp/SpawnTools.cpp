/*
 * Araxia MCP Server - Spawn Tools
 * 
 * Headless spawn management tools that work without a player in-game.
 * Allows MCP to:
 * - Query spawned creatures on maps
 * - Force spawn creatures at specific locations
 * - Despawn creatures
 * - Reload creature data from database
 * - Validate spawn positions
 */

#include "AraxiaMCPServer.h"
#include "World.h"
#include "MapManager.h"
#include "Map.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Log.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "DB2Stores.h"
#include <sstream>

namespace Araxia
{

// Helper: Get a map instance
static Map* GetOrCreateMap(uint32 mapId)
{
    // For continent maps, use instanceId 0
    return sMapMgr->FindMap(mapId, 0);
}


void RegisterSpawnTools()
{
    // spawn_query - Query creatures spawned on a map
    sMCPServer->RegisterTool(
        "spawn_query",
        "Query creatures currently spawned on a map. Can filter by entry ID or area.",
        {
            {"type", "object"},
            {"properties", {
                {"mapId", {
                    {"type", "integer"},
                    {"description", "Map ID to query (e.g., 870 for Pandaria)"}
                }},
                {"entry", {
                    {"type", "integer"},
                    {"description", "Optional: Filter by creature entry ID"}
                }},
                {"x", {
                    {"type", "number"},
                    {"description", "Optional: Center X coordinate for area search"}
                }},
                {"y", {
                    {"type", "number"},
                    {"description", "Optional: Center Y coordinate for area search"}
                }},
                {"radius", {
                    {"type", "number"},
                    {"description", "Optional: Search radius (default 100)"}
                }},
                {"limit", {
                    {"type", "integer"},
                    {"description", "Maximum results to return (default 50)"}
                }}
            }},
            {"required", {"mapId"}}
        },
        [](const json& params) -> json {
            uint32 mapId = params.value("mapId", 0u);
            uint32 entry = params.value("entry", 0u);
            float centerX = params.value("x", 0.0f);
            float centerY = params.value("y", 0.0f);
            float radius = params.value("radius", 100.0f);
            uint32 limit = params.value("limit", 50u);
            bool hasCenter = params.contains("x") && params.contains("y");
            
            Map* map = GetOrCreateMap(mapId);
            if (!map)
            {
                return {
                    {"success", false},
                    {"error", "Map not found or not loaded"},
                    {"mapId", mapId}
                };
            }
            
            json creatures = json::array();
            uint32 totalCount = 0;
            
            // Iterate through all creatures on the map
            // Note: This uses internal TC structures - may need adjustment based on version
            auto& creatureBySpawnId = map->GetCreatureBySpawnIdStore();
            
            for (auto const& pair : creatureBySpawnId)
            {
                Creature* creature = pair.second;
                if (!creature)
                    continue;
                
                // Filter by entry if specified
                if (entry != 0 && creature->GetEntry() != entry)
                    continue;
                
                // Filter by area if center specified
                if (hasCenter)
                {
                    float dx = creature->GetPositionX() - centerX;
                    float dy = creature->GetPositionY() - centerY;
                    float dist = std::sqrt(dx*dx + dy*dy);
                    if (dist > radius)
                        continue;
                }
                
                totalCount++;
                
                if (creatures.size() < limit)
                {
                    creatures.push_back({
                        {"guid", creature->GetSpawnId()},
                        {"entry", creature->GetEntry()},
                        {"name", creature->GetName()},
                        {"x", creature->GetPositionX()},
                        {"y", creature->GetPositionY()},
                        {"z", creature->GetPositionZ()},
                        {"orientation", creature->GetOrientation()},
                        {"isAlive", creature->IsAlive()},
                        {"level", creature->GetLevel()},
                        {"health", creature->GetHealth()},
                        {"maxHealth", creature->GetMaxHealth()}
                    });
                }
            }
            
            return {
                {"success", true},
                {"mapId", mapId},
                {"totalFound", totalCount},
                {"returned", creatures.size()},
                {"creatures", creatures}
            };
        }
    );
    
    // spawn_count - Get creature counts on a map
    sMCPServer->RegisterTool(
        "spawn_count",
        "Get total creature count on a map, optionally filtered by entry.",
        {
            {"type", "object"},
            {"properties", {
                {"mapId", {
                    {"type", "integer"},
                    {"description", "Map ID to count creatures on"}
                }},
                {"entry", {
                    {"type", "integer"},
                    {"description", "Optional: Count only this creature entry"}
                }}
            }},
            {"required", {"mapId"}}
        },
        [](const json& params) -> json {
            uint32 mapId = params.value("mapId", 0u);
            uint32 entry = params.value("entry", 0u);
            
            Map* map = GetOrCreateMap(mapId);
            if (!map)
            {
                return {
                    {"success", false},
                    {"error", "Map not found or not loaded"},
                    {"mapId", mapId}
                };
            }
            
            uint32 count = 0;
            auto& creatureBySpawnId = map->GetCreatureBySpawnIdStore();
            
            for (auto const& pair : creatureBySpawnId)
            {
                Creature* creature = pair.second;
                if (!creature)
                    continue;
                
                if (entry == 0 || creature->GetEntry() == entry)
                    count++;
            }
            
            return {
                {"success", true},
                {"mapId", mapId},
                {"entry", entry},
                {"count", count}
            };
        }
    );
    
    // spawn_creature - Force spawn a creature at a location
    sMCPServer->RegisterTool(
        "spawn_creature",
        "Spawn a creature at a specific location. Does not require a player.",
        {
            {"type", "object"},
            {"properties", {
                {"entry", {
                    {"type", "integer"},
                    {"description", "Creature template entry ID"}
                }},
                {"mapId", {
                    {"type", "integer"},
                    {"description", "Map ID to spawn on"}
                }},
                {"x", {
                    {"type", "number"},
                    {"description", "X coordinate"}
                }},
                {"y", {
                    {"type", "number"},
                    {"description", "Y coordinate"}
                }},
                {"z", {
                    {"type", "number"},
                    {"description", "Z coordinate"}
                }},
                {"orientation", {
                    {"type", "number"},
                    {"description", "Facing direction in radians (default 0)"}
                }},
                {"save", {
                    {"type", "boolean"},
                    {"description", "Save to database (default false - temporary spawn)"}
                }}
            }},
            {"required", {"entry", "mapId", "x", "y", "z"}}
        },
        [](const json& params) -> json {
            uint32 entry = params.value("entry", 0u);
            uint32 mapId = params.value("mapId", 0u);
            float x = params.value("x", 0.0f);
            float y = params.value("y", 0.0f);
            float z = params.value("z", 0.0f);
            float o = params.value("orientation", 0.0f);
            // Note: save parameter ignored - spawns are always temporary for now
            
            // Validate creature template exists
            CreatureTemplate const* cInfo = sObjectMgr->GetCreatureTemplate(entry);
            if (!cInfo)
            {
                return {
                    {"success", false},
                    {"error", "Creature template not found"},
                    {"entry", entry}
                };
            }
            
            Map* map = GetOrCreateMap(mapId);
            if (!map)
            {
                return {
                    {"success", false},
                    {"error", "Map not found or not loaded"},
                    {"mapId", mapId}
                };
            }
            
            TC_LOG_INFO("araxia.mcp", "[MCP] Spawning creature %u (%s) at (%.2f, %.2f, %.2f) on map %u",
                        entry, cInfo->Name.c_str(), x, y, z, mapId);
            
            // Create the creature
            Creature* creature = new Creature();
            
            // Create(guidlow, map, entry, pos, data, vehId, dynamic)
            if (!creature->Create(map->GenerateLowGuid<HighGuid::Creature>(), map, entry, {x, y, z, o}, nullptr, 0, false))
            {
                delete creature;
                return {
                    {"success", false},
                    {"error", "Failed to create creature instance"},
                    {"entry", entry}
                };
            }
            
            // Add to the world
            if (!map->AddToMap(creature))
            {
                delete creature;
                return {
                    {"success", false},
                    {"error", "Failed to add creature to map"},
                    {"entry", entry}
                };
            }
            
            ObjectGuid::LowType spawnId = creature->GetSpawnId();
            
            // Note: SaveToDB is complex in TC 11.x, skip for now (temporary spawns only)
            // For persistent spawns, use db_execute to insert into creature table
            
            return {
                {"success", true},
                {"entry", entry},
                {"name", cInfo->Name},
                {"spawnId", spawnId},
                {"position", {
                    {"x", x},
                    {"y", y},
                    {"z", z},
                    {"o", o},
                    {"map", mapId}
                }},
                {"temporary", true},
                {"note", "Creature is temporary. Use db_execute to persist to database."}
            };
        }
    );
    
    // reload_creatures - Reload creature spawns from database
    sMCPServer->RegisterTool(
        "reload_creatures",
        "Reload creature spawns from the database. Can reload all or specific entries.",
        {
            {"type", "object"},
            {"properties", {
                {"entry", {
                    {"type", "integer"},
                    {"description", "Optional: Reload only this creature entry. If not specified, reloads all."}
                }},
                {"mapId", {
                    {"type", "integer"},
                    {"description", "Optional: Reload only creatures on this map"}
                }}
            }}
        },
        [](const json& params) -> json {
            uint32 entry = params.value("entry", 0u);
            uint32 mapId = params.value("mapId", 0u);
            
            TC_LOG_INFO("araxia.mcp", "[MCP] Reloading creatures (entry: %u, map: %u)", entry, mapId);
            
            // TC 11.x doesn't support single-entry reload via API
            // Reload all creature templates
            (void)entry;  // Suppress unused warning
            (void)mapId;
            
            sObjectMgr->LoadCreatureTemplates();
            
            return {
                {"success", true},
                {"message", "All creature templates reloaded from database"},
                {"note", "Spawns require server restart to update"}
            };
        }
    );
    
    // console_command - Execute a server console command (no player needed)
    sMCPServer->RegisterTool(
        "console_command",
        "Execute a server console command. Does not require a player.",
        {
            {"type", "object"},
            {"properties", {
                {"command", {
                    {"type", "string"},
                    {"description", "Console command to execute (e.g., 'server info', 'reload creature')"}
                }}
            }},
            {"required", {"command"}}
        },
        [](const json& params) -> json {
            std::string command = params.value("command", "");
            
            if (command.empty())
                return {{"success", false}, {"error", "No command specified"}};
            
            TC_LOG_INFO("araxia.mcp", "[MCP] Console command: %s", command.c_str());
            
            // Parse the command
            std::istringstream iss(command);
            std::string cmd;
            iss >> cmd;
            
            // Handle specific console commands
            if (cmd == "server")
            {
                std::string subcmd;
                iss >> subcmd;
                
                if (subcmd == "info")
                {
                    return {
                        {"success", true},
                        {"command", "server info"},
                        {"result", {
                            {"uptime", GameTime::GetUptime()},
                            {"players", sWorld->GetActiveSessionCount()},
                            {"maxPlayers", sWorld->GetMaxActiveSessionCount()}
                        }}
                    };
                }
                else if (subcmd == "shutdown")
                {
                    // Don't actually shutdown via MCP for safety
                    return {
                        {"success", false},
                        {"error", "Server shutdown via MCP is disabled for safety"}
                    };
                }
            }
            else if (cmd == "reload")
            {
                std::string what;
                iss >> what;
                
                if (what == "creature")
                {
                    // Reload all creature templates
                    sObjectMgr->LoadCreatureTemplates();
                    return {
                        {"success", true},
                        {"command", "reload creature"},
                        {"message", "All creature templates reloaded from database"}
                    };
                }
                else if (what == "creature_spawns" || what == "spawns")
                {
                    // This would reload spawn data - expensive operation
                    return {
                        {"success", false},
                        {"error", "Spawn reload requires server restart for safety"},
                        {"hint", "Restart the worldserver to load new spawns from database"}
                    };
                }
            }
            
            return {
                {"success", false},
                {"error", "Unknown or unsupported console command"},
                {"command", cmd},
                {"supported", {
                    "server info",
                    "reload creature [entry]",
                    "reload creature_spawns (requires restart)"
                }}
            };
        }
    );
    
    // map_info - Get information about loaded maps
    sMCPServer->RegisterTool(
        "map_info",
        "Get information about a map including whether it's loaded and creature counts.",
        {
            {"type", "object"},
            {"properties", {
                {"mapId", {
                    {"type", "integer"},
                    {"description", "Map ID to query"}
                }}
            }},
            {"required", {"mapId"}}
        },
        [](const json& params) -> json {
            uint32 mapId = params.value("mapId", 0u);
            
            MapEntry const* mapEntry = sMapStore.LookupEntry(mapId);
            if (!mapEntry)
            {
                return {
                    {"success", false},
                    {"error", "Invalid map ID"},
                    {"mapId", mapId}
                };
            }
            
            Map* map = GetOrCreateMap(mapId);
            
            json result = {
                {"success", true},
                {"mapId", mapId},
                {"name", mapEntry->MapName[LOCALE_enUS]},  // English name
                {"isLoaded", map != nullptr},
                {"type", mapEntry->InstanceType}
            };
            
            if (map)
            {
                uint32 creatureCount = 0;
                auto& creatureBySpawnId = map->GetCreatureBySpawnIdStore();
                creatureCount = creatureBySpawnId.size();
                
                result["creatureCount"] = creatureCount;
                result["instanceId"] = map->GetInstanceId();
            }
            
            return result;
        }
    );
    
    TC_LOG_INFO("araxia.mcp", "[MCP] Spawn tools registered (spawn_query, spawn_count, spawn_creature, reload_creatures, console_command, map_info)");
}

} // namespace Araxia
