/*
 * Araxia MCP Server - Server Tools
 * 
 * Basic server information and control tools.
 * 
 * =============================================================================
 * IMPORTANT: GM Command Implementation Pattern
 * =============================================================================
 * 
 * When adding new GM commands to gm_command tool:
 * 
 * 1. PREFER using ChatHandler::ParseCommands() as a fallback rather than
 *    reimplementing command logic. This allows ANY existing GM command to
 *    work through MCP without custom code.
 * 
 * 2. Only implement custom handlers when you need:
 *    - Structured JSON response with specific data (e.g., coordinates from "gps")
 *    - Custom error handling or validation
 *    - Modified behavior from the standard command
 * 
 * 3. The fallback pattern at the end of gm_command:
 *    ```cpp
 *    ChatHandler handler(player->GetSession());
 *    bool success = handler.ParseCommands(command);
 *    ```
 *    This executes the command exactly as if typed in-game with a leading dot.
 * 
 * 4. Available GM commands are defined in src/server/scripts/Commands/cs_*.cpp
 *    Check there for command names and required permissions.
 * 
 * =============================================================================
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
#include "ObjectMgr.h"
#include "SmartScriptMgr.h"
#include "Chat.h"
#include "AraxiaEventBus.h"
#include "AuctionHouseMgr.h"
#include "Item.h"
#include <sstream>
#include <algorithm>
#include <cwctype>

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
            // Handle: reload <type> - Reload various data from database
            else if (cmd == "reload")
            {
                std::string reloadType;
                iss >> reloadType;
                
                if (reloadType.empty())
                {
                    return {
                        {"success", false},
                        {"error", "Usage: reload <type>"},
                        {"supported_types", {"smart_scripts", "creature_template", "game_tele"}}
                    };
                }
                
                TC_LOG_INFO("araxia.mcp", "[MCP] Reloading: {}", reloadType);
                
                if (reloadType == "smart_scripts" || reloadType == "smartai" || reloadType == "smart")
                {
                    sSmartScriptMgr->LoadSmartAIFromDB();
                    return {{"success", true}, {"command", "reload"}, {"type", "smart_scripts"}, {"message", "SmartAI scripts reloaded from database"}};
                }
                else if (reloadType == "creature" || reloadType == "creature_template")
                {
                    sObjectMgr->LoadCreatureTemplates();
                    return {{"success", true}, {"command", "reload"}, {"type", "creature_template"}, {"message", "Creature templates reloaded. Note: Existing spawns need server restart."}};
                }
                else if (reloadType == "game_tele" || reloadType == "tele")
                {
                    sObjectMgr->LoadGameTele();
                    return {{"success", true}, {"command", "reload"}, {"type", "game_tele"}, {"message", "Teleport locations reloaded"}};
                }
                else
                {
                    return {
                        {"success", false},
                        {"error", "Unknown reload type: " + reloadType},
                        {"supported_types", {"smart_scripts", "creature_template", "game_tele"}}
                    };
                }
            }
            // Handle: levelup [level] - Level up player to specified level (or +1 if no level given)
            else if (cmd == "levelup")
            {
                int levelInt;
                uint8 newLevel;
                if (!(iss >> levelInt))
                {
                    // No level specified, add 1
                    newLevel = player->GetLevel() + 1;
                }
                else
                {
                    newLevel = static_cast<uint8>(std::clamp(levelInt, 1, 80));
                }
                
                uint8 oldLevel = player->GetLevel();
                if (newLevel == oldLevel)
                    return {{"success", true}, {"command", "levelup"}, {"player", player->GetName()}, {"level", newLevel}, {"message", "Already at this level"}};
                
                player->GiveLevel(newLevel);
                player->InitTalentForLevel();
                player->SetXP(0);
                
                return {
                    {"success", true},
                    {"command", "levelup"},
                    {"player", player->GetName()},
                    {"oldLevel", oldLevel},
                    {"newLevel", newLevel}
                };
            }
            
            // Fallback: Pass any unhandled command to ChatHandler (existing GM command system)
            // This allows MCP to execute ANY GM command without reimplementing logic
            // NOTE: ChatHandler::ParseCommands expects commands WITH leading dot (e.g., ".npc add")
            else
            {
                ChatHandler handler(player->GetSession());
                std::string dotCommand = "." + command;  // Add leading dot for ChatHandler
                bool success = handler.ParseCommands(dotCommand);
                
                TC_LOG_INFO("araxia.mcp", "[MCP] Executed via ChatHandler: {} (success: {})", command, success);
                
                return {
                    {"success", success},
                    {"command", command},
                    {"method", "ChatHandler"},
                    {"message", success ? "Command executed via GM command system" : "Command failed or unknown"}
                };
            }
            
            // Unreachable - but satisfies compiler warning about all paths returning
            return {{"success", false}, {"error", "Unreachable code path"}};
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
    
    // publish_test_event - Publish a test event to the event bus for UI testing
    // This tool allows AI assistants to test event-driven UIs like the Auction House page
    // without needing actual in-game activity.
    sMCPServer->RegisterTool(
        "publish_test_event",
        "Publish a test event to the ZeroMQ event bus. Useful for testing event-driven UIs.",
        {
            {"type", "object"},
            {"properties", {
                {"topic", {{"description", "Event topic (e.g., 'world.auction.create', 'world.player.login')"}, {"type", "string"}}},
                {"payload", {{"description", "JSON payload object for the event"}, {"type", "object"}}}
            }},
            {"required", {"topic", "payload"}}
        },
        [](const json& params) -> json {
            std::string topic = params.value("topic", "");
            
            if (topic.empty())
                return {{"success", false}, {"error", "Topic is required"}};
            
            if (!params.contains("payload"))
                return {{"success", false}, {"error", "Payload is required"}};
            
            // Get payload as string
            std::string payloadStr = params["payload"].dump();
            
            // Publish to event bus
            sAraxiaEventBus->Publish(topic, payloadStr);
            
            TC_LOG_DEBUG("araxia.mcp", "[MCP] Published test event: {} with payload: {}", topic, payloadStr);
            
            return {
                {"success", true},
                {"topic", topic},
                {"payload", params["payload"]},
                {"message", "Event published to event bus"}
            };
        }
    );
    
    // auction_search - Search the auction house using the same bucket system the client uses.
    // The server stores auctions in "buckets" indexed by item, with pre-computed FullName
    // arrays for each locale. We search the bucket FullName to match client behavior.
    // AHBot items are stored in memory, not in item_instance table, so we must query
    // the server directly rather than the database.
    sMCPServer->RegisterTool(
        "auction_search",
        "Search the auction house using the server's native bucket search (same as game client).",
        {
            {"type", "object"},
            {"properties", {
                {"name", {{"description", "Item name to search for (partial match, case-insensitive)"}, {"type", "string"}}},
                {"quality", {{"description", "Minimum quality (0=Poor, 1=Common, 2=Uncommon, 3=Rare, 4=Epic, 5=Legendary)"}, {"type", "integer"}}},
                {"faction", {{"description", "Faction: 'alliance', 'horde', 'neutral', or 'all' (default)"}, {"type", "string"}}},
                {"limit", {{"description", "Max results to return (default 50)"}, {"type", "integer"}}}
            }}
        },
        [](const json& params) -> json {
            std::string nameFilter = params.value("name", "");
            int qualityFilter = params.value("quality", -1);
            std::string factionFilter = params.value("faction", "all");
            int limit = params.value("limit", 50);
            
            // Convert name filter to wide string for searching bucket FullName
            // The server uses wide strings for localized names
            std::wstring wideNameFilter;
            if (!nameFilter.empty())
            {
                // Convert to lowercase wide string for case-insensitive matching
                for (char c : nameFilter)
                    wideNameFilter += std::towlower(static_cast<wchar_t>(c));
            }
            
            json results = json::array();
            int total = 0;
            
            // Helper lambda to search an auction house using bucket data
            auto searchAuctionHouse = [&](AuctionHouseObject* ah, const std::string& faction) {
                if (!ah) return;
                
                for (auto itr = ah->GetAuctionsBegin(); itr != ah->GetAuctionsEnd(); ++itr)
                {
                    AuctionPosting& auction = itr->second;
                    
                    // Get bucket data for proper name searching (same as client)
                    if (!auction.Bucket) continue;
                    AuctionsBucketData* bucket = auction.Bucket;
                    
                    // Get item info from the first item
                    if (auction.Items.empty()) continue;
                    Item* item = auction.Items[0];
                    if (!item) continue;
                    
                    ItemTemplate const* itemTemplate = item->GetTemplate();
                    if (!itemTemplate) continue;
                    
                    // Quality filter - check bucket's quality mask
                    if (qualityFilter >= 0 && itemTemplate->GetQuality() < qualityFilter)
                        continue;
                    
                    // Name filter using bucket's FullName (same as server's BuildListBuckets)
                    if (!wideNameFilter.empty())
                    {
                        // Get the localized full name from the bucket
                        const std::wstring& fullName = bucket->FullName[LOCALE_enUS];
                        
                        // Convert to lowercase for case-insensitive search
                        std::wstring lowerFullName;
                        for (wchar_t wc : fullName)
                            lowerFullName += std::towlower(wc);
                        
                        // Search for substring match
                        if (lowerFullName.find(wideNameFilter) == std::wstring::npos)
                            continue;
                    }
                    
                    total++;
                    
                    if ((int)results.size() < limit)
                    {
                        // Calculate time remaining
                        auto now = GameTime::GetSystemTime();
                        auto remaining = std::chrono::duration_cast<std::chrono::seconds>(auction.EndTime - now).count();
                        std::string timeLeft = "Expired";
                        if (remaining > 0)
                        {
                            if (remaining < 1800) timeLeft = "Short";
                            else if (remaining < 7200) timeLeft = "Medium";
                            else if (remaining < 43200) timeLeft = "Long";
                            else timeLeft = "Very Long";
                        }
                        
                        // Convert wide string name back to UTF-8 for JSON
                        std::string itemName;
                        for (wchar_t wc : bucket->FullName[LOCALE_enUS])
                        {
                            if (wc < 128)
                                itemName += static_cast<char>(wc);
                            else
                                itemName += '?'; // Non-ASCII placeholder
                        }
                        
                        // Fallback to template name if bucket name is empty
                        if (itemName.empty())
                            itemName = itemTemplate->GetName(LOCALE_enUS);
                        
                        // Get item stats from bucket data
                        // RequiredLevel and ItemLevel are stored in the bucket for efficiency
                        uint8 requiredLevel = bucket->RequiredLevel;
                        uint16 itemLevel = bucket->Key.ItemLevel;
                        
                        results.push_back({
                            {"auctionId", auction.Id},
                            {"itemId", itemTemplate->GetId()},
                            {"itemName", itemName},
                            {"itemQuality", itemTemplate->GetQuality()},
                            {"itemLevel", itemLevel},
                            {"requiredLevel", requiredLevel},
                            {"count", auction.GetTotalItemCount()},
                            {"buyout", auction.BuyoutOrUnitPrice},
                            {"bid", auction.MinBid},
                            {"currentBid", auction.BidAmount},
                            {"timeLeft", timeLeft},
                            {"faction", faction}
                        });
                    }
                }
            };
            
            // Search appropriate auction houses based on faction filter
            if (factionFilter == "all" || factionFilter == "alliance")
                searchAuctionHouse(sAuctionMgr->GetAuctionsById(2), "alliance");
            if (factionFilter == "all" || factionFilter == "horde")
                searchAuctionHouse(sAuctionMgr->GetAuctionsById(6), "horde");
            if (factionFilter == "all" || factionFilter == "neutral")
                searchAuctionHouse(sAuctionMgr->GetAuctionsById(7), "neutral");
            
            return {
                {"success", true},
                {"total", total},
                {"count", results.size()},
                {"auctions", results}
            };
        }
    );
    
    TC_LOG_INFO("araxia.mcp", "[MCP] Server tools registered (server_info, player_list, gm_command, reload_scripts, log_search, shared_data_*, publish_test_event, auction_search)");
}

} // namespace Araxia
