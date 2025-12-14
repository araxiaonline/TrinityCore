/*
 * Araxia MCP Server - Server Tools
 * 
 * Basic server information and control tools.
 */

#include "AraxiaMCPServer.h"
#include "World.h"
#include "WorldSession.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "MapManager.h"
#include "Log.h"
#include "GitRevision.h"
#include "GameTime.h"
#include "LuaEngine/ElunaSharedData.h"
#include "Cell.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Creature.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include <sstream>

namespace Araxia
{

void RegisterServerTools()
{
    // server_info - Get server status and information
    sMCPServer->RegisterTool(
        "server_info",
        "Get current server status including uptime, player count, and version info.",
        {
            {"type", "object"},
            {"properties", json::object()}
        },
        [](const json& /*params*/) -> json {
            uint32 playerCount = sWorld->GetActiveSessionCount();
            uint32 maxPlayers = sWorld->GetMaxActiveSessionCount();
            uint32 queuedCount = sWorld->GetQueuedSessionCount();
            
            auto uptime = GameTime::GetUptime();
            uint32 days = uptime / DAY;
            uint32 hours = (uptime % DAY) / HOUR;
            uint32 minutes = (uptime % HOUR) / MINUTE;
            uint32 seconds = uptime % MINUTE;
            
            std::ostringstream uptimeStr;
            if (days) uptimeStr << days << "d ";
            if (hours || days) uptimeStr << hours << "h ";
            if (minutes || hours || days) uptimeStr << minutes << "m ";
            uptimeStr << seconds << "s";
            
            return {
                {"success", true},
                {"server", {
                    {"name", "Araxia Online"},
                    {"version", GitRevision::GetFullVersion()},
                    {"branch", GitRevision::GetBranch()},
                    {"revision", GitRevision::GetHash()}
                }},
                {"players", {
                    {"online", playerCount},
                    {"max", maxPlayers},
                    {"queued", queuedCount}
                }},
                {"uptime", {
                    {"seconds", static_cast<uint64>(uptime)},
                    {"formatted", uptimeStr.str()}
                }},
                {"serverTime", GameTime::GetGameTime()}
            };
        }
    );
    
    // player_list - Get list of online players
    sMCPServer->RegisterTool(
        "player_list",
        "Get list of all online players with their details.",
        {
            {"type", "object"},
            {"properties", json::object()}
        },
        [](const json& /*params*/) -> json {
            json players = json::array();
            
            SessionMap const& sessions = sWorld->GetAllSessions();
            for (auto const& [id, session] : sessions)
            {
                if (!session)
                    continue;
                    
                Player* player = session->GetPlayer();
                if (!player)
                    continue;
                
                players.push_back({
                    {"name", player->GetName()},
                    {"guid", player->GetGUID().GetCounter()},
                    {"level", player->GetLevel()},
                    {"race", player->GetRace()},
                    {"class", player->GetClass()},
                    {"zone", player->GetZoneId()},
                    {"area", player->GetAreaId()},
                    {"map", player->GetMapId()},
                    {"position", {
                        {"x", player->GetPositionX()},
                        {"y", player->GetPositionY()},
                        {"z", player->GetPositionZ()}
                    }},
                    {"isGM", player->IsGameMaster()},
                    {"accountId", session->GetAccountId()}
                });
            }
            
            return {
                {"success", true},
                {"players", players},
                {"count", players.size()}
            };
        }
    );
    
    // gm_command - Execute a GM command
    sMCPServer->RegisterTool(
        "gm_command",
        "Execute a GM command as if typed by a GM in-game. Some commands require a player context.",
        {
            {"type", "object"},
            {"properties", {
                {"command", {
                    {"type", "string"},
                    {"description", "The GM command to execute (without leading dot)"}
                }},
                {"player", {
                    {"type", "string"},
                    {"description", "Optional: Player name to execute command as/on"}
                }}
            }},
            {"required", {"command"}}
        },
        [](const json& params) -> json {
            std::string command = params.value("command", "");
            std::string playerName = params.value("player", "");
            
            if (command.empty())
                return {{"success", false}, {"error", "No command specified"}};
            
            // Remove leading dot if present
            if (!command.empty() && command[0] == '.')
                command = command.substr(1);
            
            TC_LOG_INFO("araxia.mcp", "[MCP] GM command: .{} (player: {})", 
                        command, playerName.empty() ? "first online" : playerName);
            
            // Find the player
            Player* player = nullptr;
            if (!playerName.empty())
            {
                player = ObjectAccessor::FindPlayerByName(playerName);
            }
            else
            {
                // Use first online player
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
                return {{"success", false}, {"error", "No player found"}};
            
            // Parse and execute common commands directly
            std::istringstream iss(command);
            std::string cmd;
            iss >> cmd;
            
            // Handle: go xyz X Y Z [mapId]
            if (cmd == "go")
            {
                std::string subcmd;
                iss >> subcmd;
                
                if (subcmd == "xyz")
                {
                    float x, y, z;
                    uint32 mapId = player->GetMapId();
                    
                    if (!(iss >> x >> y >> z))
                        return {{"success", false}, {"error", "Usage: go xyz X Y Z [mapId]"}};
                    
                    iss >> mapId;  // Optional map ID
                    
                    // Teleport the player
                    if (mapId != player->GetMapId())
                    {
                        player->TeleportTo(mapId, x, y, z, player->GetOrientation());
                    }
                    else
                    {
                        player->NearTeleportTo(x, y, z, player->GetOrientation());
                    }
                    
                    TC_LOG_INFO("araxia.mcp", "[MCP] Teleported {} to ({:.2f}, {:.2f}, {:.2f}) map {}",
                                player->GetName(), x, y, z, mapId);
                    
                    return {
                        {"success", true},
                        {"command", "go xyz"},
                        {"player", player->GetName()},
                        {"teleported", {
                            {"x", x}, {"y", y}, {"z", z}, {"map", mapId}
                        }}
                    };
                }
            }
            // Handle: tele <location>
            else if (cmd == "tele")
            {
                // For now just support coordinates: tele X Y Z mapId
                float x, y, z;
                uint32 mapId;
                
                if (iss >> x >> y >> z >> mapId)
                {
                    player->TeleportTo(mapId, x, y, z, player->GetOrientation());
                    
                    return {
                        {"success", true},
                        {"command", "tele"},
                        {"player", player->GetName()},
                        {"teleported", {
                            {"x", x}, {"y", y}, {"z", z}, {"map", mapId}
                        }}
                    };
                }
                
                return {{"success", false}, {"error", "Usage: tele X Y Z mapId"}};
            }
            // Handle: gps - Get player location
            else if (cmd == "gps")
            {
                return {
                    {"success", true},
                    {"command", "gps"},
                    {"player", player->GetName()},
                    {"position", {
                        {"x", player->GetPositionX()},
                        {"y", player->GetPositionY()},
                        {"z", player->GetPositionZ()},
                        {"o", player->GetOrientation()},
                        {"map", player->GetMapId()},
                        {"zone", player->GetZoneId()},
                        {"area", player->GetAreaId()}
                    }}
                };
            }
            // Handle: additem <itemId> [count]
            else if (cmd == "additem")
            {
                uint32 itemId, count = 1;
                if (!(iss >> itemId))
                    return {{"success", false}, {"error", "Usage: additem <itemId> [count]"}};
                iss >> count;
                
                player->AddItem(itemId, count);
                
                return {
                    {"success", true},
                    {"command", "additem"},
                    {"player", player->GetName()},
                    {"item", itemId},
                    {"count", count}
                };
            }
            // Handle: die - Kill target or self
            else if (cmd == "die")
            {
                Unit* target = player->GetSelectedUnit();
                if (!target)
                    target = player;
                
                target->KillSelf();
                
                return {
                    {"success", true},
                    {"command", "die"},
                    {"killed", target == player ? player->GetName() : "target"}
                };
            }
            // Handle: revive
            else if (cmd == "revive")
            {
                player->ResurrectPlayer(1.0f);
                player->SpawnCorpseBones();
                
                return {
                    {"success", true},
                    {"command", "revive"},
                    {"player", player->GetName()}
                };
            }
            // Handle: aura <spellId> - Add aura to player
            else if (cmd == "aura")
            {
                uint32 spellId;
                if (!(iss >> spellId))
                    return {{"success", false}, {"error", "Usage: aura <spellId>"}};
                
                SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
                if (!spellInfo)
                    return {{"success", false}, {"error", "Invalid spell ID"}, {"spellId", spellId}};
                
                player->AddAura(spellId, player);
                
                return {
                    {"success", true},
                    {"command", "aura"},
                    {"player", player->GetName()},
                    {"spellId", spellId},
                    {"spellName", spellInfo->SpellName->Str[LOCALE_enUS]}
                };
            }
            // Handle: unaura <spellId> - Remove aura from player
            else if (cmd == "unaura")
            {
                uint32 spellId;
                if (!(iss >> spellId))
                    return {{"success", false}, {"error", "Usage: unaura <spellId>"}};
                
                if (!player->HasAura(spellId))
                    return {{"success", false}, {"error", "Player does not have this aura"}, {"spellId", spellId}};
                
                player->RemoveAurasDueToSpell(spellId);
                
                return {
                    {"success", true},
                    {"command", "unaura"},
                    {"player", player->GetName()},
                    {"spellId", spellId}
                };
            }
            // Handle: learn <spellId> - Teach spell to player
            else if (cmd == "learn")
            {
                uint32 spellId;
                if (!(iss >> spellId))
                    return {{"success", false}, {"error", "Usage: learn <spellId>"}};
                
                SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
                if (!spellInfo)
                    return {{"success", false}, {"error", "Invalid spell ID"}, {"spellId", spellId}};
                
                player->LearnSpell(spellId, false);
                
                return {
                    {"success", true},
                    {"command", "learn"},
                    {"player", player->GetName()},
                    {"spellId", spellId},
                    {"spellName", spellInfo->SpellName->Str[LOCALE_enUS]}
                };
            }
            // Handle: unlearn <spellId> - Remove spell from player
            else if (cmd == "unlearn")
            {
                uint32 spellId;
                if (!(iss >> spellId))
                    return {{"success", false}, {"error", "Usage: unlearn <spellId>"}};
                
                if (!player->HasSpell(spellId))
                    return {{"success", false}, {"error", "Player does not have this spell"}, {"spellId", spellId}};
                
                player->RemoveSpell(spellId, false, false);
                
                return {
                    {"success", true},
                    {"command", "unlearn"},
                    {"player", player->GetName()},
                    {"spellId", spellId}
                };
            }
            // Handle: respawn - Respawn creatures around player or targeted creature
            else if (cmd == "respawn")
            {
                Map* map = player->GetMap();
                if (!map)
                    return {{"success", false}, {"error", "Player not on valid map"}};
                
                // Check if player has a creature selected
                Unit* target = player->GetSelectedUnit();
                Creature* targetCreature = target ? target->ToCreature() : nullptr;
                
                if (targetCreature)
                {
                    // Respawn the specific targeted creature
                    if (targetCreature->isDead())
                    {
                        targetCreature->Respawn();
                        return {
                            {"success", true},
                            {"command", "respawn"},
                            {"mode", "targeted"},
                            {"creature", targetCreature->GetName()},
                            {"entry", targetCreature->GetEntry()}
                        };
                    }
                    else
                    {
                        // Force despawn and respawn for living creature
                        targetCreature->DespawnOrUnsummon(0ms, 1s);
                        return {
                            {"success", true},
                            {"command", "respawn"},
                            {"mode", "force_respawn"},
                            {"creature", targetCreature->GetName()},
                            {"entry", targetCreature->GetEntry()},
                            {"message", "Creature will despawn and respawn in 1 second"}
                        };
                    }
                }
                else
                {
                    // No target - respawn all creatures in area (like .respawn command)
                    CellCoord p(Trinity::ComputeCellCoord(player->GetPositionX(), player->GetPositionY()));
                    Cell cell(p);
                    cell.SetNoCreate();
                    
                    Trinity::RespawnDo u;
                    Trinity::WorldObjectWorker<Trinity::RespawnDo> worker(player, u);
                    Cell::VisitGridObjects(player, worker, player->GetGridActivationRange());
                    
                    return {
                        {"success", true},
                        {"command", "respawn"},
                        {"mode", "area"},
                        {"message", "Respawned all creatures in area"}
                    };
                }
            }
            
            // Unknown command
            return {
                {"success", false},
                {"error", "Unknown or unimplemented command"},
                {"command", cmd},
                {"supported", {"go xyz", "tele", "gps", "additem", "die", "revive", "aura", "unaura", "learn", "unlearn", "respawn"}}
            };
        }
    );
    
    // reload_scripts - Reload Eluna scripts
    sMCPServer->RegisterTool(
        "reload_scripts",
        "Reload Eluna Lua scripts without restarting the server.",
        {
            {"type", "object"},
            {"properties", json::object()}
        },
        [](const json& /*params*/) -> json {
            TC_LOG_INFO("araxia.mcp", "[MCP] Reloading Eluna scripts...");
            
            // This would need Eluna integration
            // For now, indicate manual reload needed
            return {
                {"success", true},
                {"message", "Script reload requested. Use '.reload eluna' in-game or implement Eluna::ReloadEluna() call."}
            };
        }
    );
    
    // log_search - Search recent log entries
    sMCPServer->RegisterTool(
        "log_search",
        "Search for log entries matching a pattern (reads from server log file).",
        {
            {"type", "object"},
            {"properties", {
                {"pattern", {
                    {"type", "string"},
                    {"description", "Text pattern to search for in logs"}
                }},
                {"lines", {
                    {"type", "integer"},
                    {"description", "Maximum number of matching lines to return (default 50)"}
                }},
                {"logType", {
                    {"type", "string"},
                    {"description", "Log type: server, eluna, dberrors, gm"},
                    {"enum", {"server", "eluna", "dberrors", "gm"}}
                }}
            }},
            {"required", {"pattern"}}
        },
        [](const json& params) -> json {
            std::string pattern = params.value("pattern", "");
            
            // This is a placeholder - actual log search would read ElunaSharedData
            // where the client/server Lua code writes log messages
            return {
                {"success", true},
                {"message", "Use shared_data tool with key 'mcp_logs' to read logs pushed by client/server"},
                {"pattern", pattern}
            };
        }
    );
    
    // shared_data_read - Read from ElunaSharedData (AMS bridge)
    sMCPServer->RegisterTool(
        "shared_data_read",
        "Read data from ElunaSharedData. This is the bridge for client/server Lua communication.",
        {
            {"type", "object"},
            {"properties", {
                {"key", {
                    {"type", "string"},
                    {"description", "The shared data key to read (e.g., 'mcp_logs', 'mcp_client_chat')"}
                }}
            }},
            {"required", {"key"}}
        },
        [](const json& params) -> json {
            std::string key = params.value("key", "");
            
            if (key.empty())
                return {{"success", false}, {"error", "Key is required"}};
            
            std::string value;
            bool exists = sElunaSharedData->Get(key, value);
            
            return {
                {"success", true},
                {"key", key},
                {"exists", exists},
                {"value", exists ? value : ""}
            };
        }
    );
    
    // shared_data_write - Write to ElunaSharedData
    sMCPServer->RegisterTool(
        "shared_data_write",
        "Write data to ElunaSharedData. Lua scripts can read this.",
        {
            {"type", "object"},
            {"properties", {
                {"key", {
                    {"type", "string"},
                    {"description", "The shared data key to write"}
                }},
                {"value", {
                    {"type", "string"},
                    {"description", "The value to store (use JSON string for complex data)"}
                }}
            }},
            {"required", {"key", "value"}}
        },
        [](const json& params) -> json {
            std::string key = params.value("key", "");
            std::string value = params.value("value", "");
            
            if (key.empty())
                return {{"success", false}, {"error", "Key is required"}};
            
            sElunaSharedData->Set(key, value);
            
            return {
                {"success", true},
                {"key", key},
                {"message", "Data written successfully"}
            };
        }
    );
    
    // shared_data_keys - List all shared data keys
    sMCPServer->RegisterTool(
        "shared_data_keys",
        "List all keys in ElunaSharedData.",
        {
            {"type", "object"},
            {"properties", json::object()}
        },
        [](const json& /*params*/) -> json {
            std::vector<std::string> keys = sElunaSharedData->GetKeys();
            
            json keysJson = json::array();
            for (const auto& k : keys)
                keysJson.push_back(k);
            
            return {
                {"success", true},
                {"keys", keysJson},
                {"count", keys.size()}
            };
        }
    );
    
    TC_LOG_INFO("araxia.mcp", "[MCP] Server tools registered (server_info, player_list, gm_command, reload_scripts, log_search, shared_data_*)");
}

} // namespace Araxia
