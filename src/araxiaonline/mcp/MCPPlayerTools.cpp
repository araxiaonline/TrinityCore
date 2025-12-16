/*
 * Araxia MCP Player Tools
 * 
 * Registers MCP tools for controlling AI players.
 * Supports multiple sessions - each LLM can control their own player.
 * 
 * All player-related tools take a session_id parameter.
 */

#include "MCPPlayerManager.h"
#include "AraxiaMCPServer.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Map.h"
#include "Creature.h"
#include "GameObject.h"

namespace Araxia
{

void RegisterMCPPlayerTools()
{
    TC_LOG_INFO("araxia.mcp", "[MCP] Registering MCP Player tools (multi-session)...");

    // ========================================================================
    // SESSION MANAGEMENT TOOLS
    // ========================================================================

    // mcp_session_create - Create a new AI player session
    sMCPServer->RegisterTool(
        "mcp_session_create",
        "Create a new AI player session. Returns a session_id to use with other player tools. "
        "Each LLM should create their own session to control their own player.",
        {
            {"type", "object"},
            {"properties", {
                {"owner_name", {
                    {"type", "string"},
                    {"description", "Name identifying the LLM/client (e.g., 'Cascade', 'Claude', 'GPT-4')"}
                }}
            }},
            {"required", {"owner_name"}}
        },
        [](const json& params) -> json {
            std::string ownerName = params["owner_name"];
            
            uint32 sessionId = sMCPPlayerMgr->CreateSession(ownerName);
            
            if (sessionId == 0)
            {
                return {
                    {"success", false},
                    {"error", "Failed to create session - max sessions reached?"}
                };
            }
            
            return {
                {"success", true},
                {"session_id", sessionId},
                {"owner_name", ownerName},
                {"message", "Session created. Use this session_id for all subsequent player commands."}
            };
        }
    );

    // mcp_session_destroy - Destroy a session
    sMCPServer->RegisterTool(
        "mcp_session_destroy",
        "Destroy an AI player session. Logs out the player if online.",
        {
            {"type", "object"},
            {"properties", {
                {"session_id", {
                    {"type", "integer"},
                    {"description", "Session ID to destroy"}
                }}
            }},
            {"required", {"session_id"}}
        },
        [](const json& params) -> json {
            uint32 sessionId = params["session_id"];
            
            bool result = sMCPPlayerMgr->DestroySession(sessionId);
            
            return {
                {"success", result},
                {"session_id", sessionId},
                {"message", result ? "Session destroyed" : "Session not found"}
            };
        }
    );

    // mcp_session_list - List all active sessions
    sMCPServer->RegisterTool(
        "mcp_session_list",
        "List all active AI player sessions.",
        {
            {"type", "object"},
            {"properties", json::object()}
        },
        [](const json& /*params*/) -> json {
            auto sessionIds = sMCPPlayerMgr->GetActiveSessions();
            json sessions = json::array();
            
            for (uint32 id : sessionIds)
            {
                MCPPlayerSession* session = sMCPPlayerMgr->GetSession(id);
                if (session)
                {
                    sessions.push_back({
                        {"session_id", session->sessionId},
                        {"owner_name", session->ownerName},
                        {"online", session->isOnline()},
                        {"character", session->player ? session->player->GetName() : session->characterName},
                        {"character_guid", session->playerGuid.GetCounter()}
                    });
                }
            }
            
            return {
                {"success", true},
                {"session_count", sessions.size()},
                {"sessions", sessions}
            };
        }
    );

    // ========================================================================
    // PLAYER LIFECYCLE TOOLS
    // ========================================================================

    // mcp_player_login - Log in a character for a session
    sMCPServer->RegisterTool(
        "mcp_player_login",
        "Log in a character for an AI player session. Specify by name or GUID.",
        {
            {"type", "object"},
            {"properties", {
                {"session_id", {
                    {"type", "integer"},
                    {"description", "Session ID to log in for"}
                }},
                {"character_name", {
                    {"type", "string"},
                    {"description", "Name of the character to log in"}
                }},
                {"character_guid", {
                    {"type", "integer"},
                    {"description", "GUID of the character to log in (alternative to name)"}
                }}
            }},
            {"required", {"session_id"}}
        },
        [](const json& params) -> json {
            uint32 sessionId = params["session_id"];
            std::string charName = params.value("character_name", "");
            uint64 charGuid = params.value("character_guid", 0ULL);
            
            MCPPlayerSession* session = sMCPPlayerMgr->GetSession(sessionId);
            if (!session)
            {
                return {
                    {"success", false},
                    {"error", "Session not found"},
                    {"session_id", sessionId}
                };
            }
            
            if (session->isOnline())
            {
                return {
                    {"success", false},
                    {"error", "Session already has a player logged in"},
                    {"character", session->player->GetName()}
                };
            }
            
            bool result = false;
            if (!charName.empty())
            {
                result = sMCPPlayerMgr->Login(sessionId, charName);
            }
            else if (charGuid > 0)
            {
                ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(charGuid);
                result = sMCPPlayerMgr->Login(sessionId, guid);
            }
            else
            {
                return {
                    {"success", false},
                    {"error", "Must specify character_name or character_guid"}
                };
            }
            
            return {
                {"success", result},
                {"session_id", sessionId},
                {"message", result ? "Login initiated (async)" : "Login failed"},
                {"note", "Use mcp_player_status to check if login completed"}
            };
        }
    );

    // mcp_player_logout - Log out the character (keeps session)
    sMCPServer->RegisterTool(
        "mcp_player_logout",
        "Log out the character for a session. The session remains active for re-login.",
        {
            {"type", "object"},
            {"properties", {
                {"session_id", {
                    {"type", "integer"},
                    {"description", "Session ID to log out"}
                }}
            }},
            {"required", {"session_id"}}
        },
        [](const json& params) -> json {
            uint32 sessionId = params["session_id"];
            
            MCPPlayerSession* session = sMCPPlayerMgr->GetSession(sessionId);
            if (!session)
            {
                return {{"success", false}, {"error", "Session not found"}};
            }
            
            if (!session->isOnline())
            {
                return {{"success", false}, {"error", "No player logged in for this session"}};
            }
            
            std::string charName = session->player->GetName();
            sMCPPlayerMgr->Logout(sessionId);
            
            return {
                {"success", true},
                {"session_id", sessionId},
                {"character", charName},
                {"message", "Player logged out. Session still active."}
            };
        }
    );

    // mcp_player_status - Get player status for a session
    sMCPServer->RegisterTool(
        "mcp_player_status",
        "Get the current status of the player for a session.",
        {
            {"type", "object"},
            {"properties", {
                {"session_id", {
                    {"type", "integer"},
                    {"description", "Session ID to get status for"}
                }}
            }},
            {"required", {"session_id"}}
        },
        [](const json& params) -> json {
            uint32 sessionId = params["session_id"];
            
            MCPPlayerSession* session = sMCPPlayerMgr->GetSession(sessionId);
            if (!session)
            {
                return {{"success", false}, {"error", "Session not found"}};
            }
            
            if (!session->isOnline())
            {
                // Not fully online, but may have cached character info
                if (!session->characterName.empty())
                {
                    return {
                        {"success", true},
                        {"session_id", sessionId},
                        {"online", false},
                        {"in_world", false},
                        {"message", "Character verified but not fully loaded into world"},
                        {"character", {
                            {"name", session->characterName},
                            {"guid", session->playerGuid.GetCounter()},
                            {"level", session->level},
                            {"class", session->playerClass},
                            {"race", session->race}
                        }},
                        {"position", {
                            {"map", session->mapId},
                            {"x", session->posX},
                            {"y", session->posY},
                            {"z", session->posZ}
                        }}
                    };
                }
                
                return {
                    {"success", true},
                    {"session_id", sessionId},
                    {"online", false},
                    {"message", "No player logged in for this session"}
                };
            }
            
            Player* player = session->player;
            Position pos = player->GetPosition();
            
            return {
                {"success", true},
                {"session_id", sessionId},
                {"online", true},
                {"in_world", true},
                {"character", {
                    {"name", player->GetName()},
                    {"guid", player->GetGUID().GetCounter()},
                    {"level", player->GetLevel()},
                    {"class", player->GetClass()},
                    {"race", player->GetRace()},
                    {"health", player->GetHealth()},
                    {"max_health", player->GetMaxHealth()},
                    {"alive", player->IsAlive()}
                }},
                {"position", {
                    {"map", player->GetMapId()},
                    {"zone", player->GetZoneId()},
                    {"area", player->GetAreaId()},
                    {"x", pos.GetPositionX()},
                    {"y", pos.GetPositionY()},
                    {"z", pos.GetPositionZ()},
                    {"o", pos.GetOrientation()}
                }},
                {"target", player->GetTarget().GetCounter()}
            };
        }
    );

    // ========================================================================
    // MOVEMENT TOOLS
    // ========================================================================

    // mcp_player_teleport
    sMCPServer->RegisterTool(
        "mcp_player_teleport",
        "Teleport the session's player to specified coordinates.",
        {
            {"type", "object"},
            {"properties", {
                {"session_id", {{"type", "integer"}, {"description", "Session ID"}}},
                {"map", {{"type", "integer"}, {"description", "Map ID to teleport to"}}},
                {"x", {{"type", "number"}, {"description", "X coordinate"}}},
                {"y", {{"type", "number"}, {"description", "Y coordinate"}}},
                {"z", {{"type", "number"}, {"description", "Z coordinate"}}},
                {"o", {{"type", "number"}, {"description", "Orientation (radians)"}}}
            }},
            {"required", {"session_id", "map", "x", "y", "z"}}
        },
        [](const json& params) -> json {
            uint32 sessionId = params["session_id"];
            uint32 mapId = params["map"];
            float x = params["x"];
            float y = params["y"];
            float z = params["z"];
            float o = params.value("o", 0.0f);
            
            bool result = sMCPPlayerMgr->TeleportTo(sessionId, mapId, x, y, z, o);
            
            return {
                {"success", result},
                {"session_id", sessionId},
                {"teleported_to", {{"map", mapId}, {"x", x}, {"y", y}, {"z", z}}}
            };
        }
    );

    // mcp_player_move
    sMCPServer->RegisterTool(
        "mcp_player_move",
        "Move the session's player to a location using pathfinding.",
        {
            {"type", "object"},
            {"properties", {
                {"session_id", {{"type", "integer"}, {"description", "Session ID"}}},
                {"x", {{"type", "number"}, {"description", "Target X coordinate"}}},
                {"y", {{"type", "number"}, {"description", "Target Y coordinate"}}},
                {"z", {{"type", "number"}, {"description", "Target Z coordinate"}}}
            }},
            {"required", {"session_id", "x", "y", "z"}}
        },
        [](const json& params) -> json {
            uint32 sessionId = params["session_id"];
            float x = params["x"];
            float y = params["y"];
            float z = params["z"];
            
            bool result = sMCPPlayerMgr->MoveTo(sessionId, x, y, z);
            
            return {
                {"success", result},
                {"session_id", sessionId},
                {"moving_to", {{"x", x}, {"y", y}, {"z", z}}}
            };
        }
    );

    // mcp_player_stop
    sMCPServer->RegisterTool(
        "mcp_player_stop",
        "Stop all movement for the session's player.",
        {
            {"type", "object"},
            {"properties", {
                {"session_id", {{"type", "integer"}, {"description", "Session ID"}}}
            }},
            {"required", {"session_id"}}
        },
        [](const json& params) -> json {
            uint32 sessionId = params["session_id"];
            sMCPPlayerMgr->StopMovement(sessionId);
            return {{"success", true}, {"session_id", sessionId}, {"message", "Movement stopped"}};
        }
    );

    // ========================================================================
    // ACTION TOOLS
    // ========================================================================

    // mcp_player_cast
    sMCPServer->RegisterTool(
        "mcp_player_cast",
        "Have the session's player cast a spell. For self-cast, omit target.",
        {
            {"type", "object"},
            {"properties", {
                {"session_id", {{"type", "integer"}, {"description", "Session ID"}}},
                {"spell_id", {{"type", "integer"}, {"description", "Spell ID to cast"}}},
                {"target_guid_low", {{"type", "integer"}, {"description", "Low part of target GUID (from mcp_player_look)"}}},
                {"target_is_player", {{"type", "boolean"}, {"description", "True if target is a player, false for creature"}}}
            }},
            {"required", {"session_id", "spell_id"}}
        },
        [](const json& params) -> json {
            uint32 sessionId = params["session_id"];
            uint32 spellId = params["spell_id"];
            
            // For now, cast on self or current target (GUID handling is complex)
            // TODO: Implement proper GUID creation from mcp_player_look data
            bool result = sMCPPlayerMgr->CastSpell(sessionId, spellId, ObjectGuid::Empty);
            
            return {{"success", result}, {"session_id", sessionId}, {"spell_id", spellId}};
        }
    );

    // mcp_player_target
    sMCPServer->RegisterTool(
        "mcp_player_target",
        "Set the session's player's current target. Use GUID from mcp_player_look.",
        {
            {"type", "object"},
            {"properties", {
                {"session_id", {{"type", "integer"}, {"description", "Session ID"}}},
                {"target_guid_low", {{"type", "integer"}, {"description", "Low GUID counter from mcp_player_look"}}},
                {"target_is_player", {{"type", "boolean"}, {"description", "True if target is player, false for creature"}}}
            }},
            {"required", {"session_id", "target_guid_low"}}
        },
        [](const json& params) -> json {
            uint32 sessionId = params["session_id"];
            uint64 guidLow = params["target_guid_low"];
            bool isPlayer = params.value("target_is_player", false);
            
            ObjectGuid targetGuid;
            if (isPlayer)
            {
                targetGuid = ObjectGuid::Create<HighGuid::Player>(guidLow);
            }
            else
            {
                // For creatures, we need more info. For now, try to find by searching.
                // TODO: Store full GUID info in mcp_player_look results
                return {{"success", false}, {"error", "Creature targeting requires mcp_player_look GUID - feature in progress"}};
            }
            
            bool result = sMCPPlayerMgr->SetTarget(sessionId, targetGuid);
            
            return {{"success", result}, {"session_id", sessionId}, {"target_guid_low", guidLow}};
        }
    );

    // ========================================================================
    // PERCEPTION TOOLS
    // ========================================================================

    // mcp_player_look
    sMCPServer->RegisterTool(
        "mcp_player_look",
        "Get what the session's player can see around them.",
        {
            {"type", "object"},
            {"properties", {
                {"session_id", {{"type", "integer"}, {"description", "Session ID"}}},
                {"range", {{"type", "number"}, {"description", "Range to scan (default 50)"}}},
                {"entity_type", {
                    {"type", "string"},
                    {"enum", {"all", "creatures", "players", "gameobjects"}},
                    {"description", "Type of entities to look for"}
                }}
            }},
            {"required", {"session_id"}}
        },
        [](const json& params) -> json {
            uint32 sessionId = params["session_id"];
            float range = params.value("range", 50.0f);
            std::string typeFilter = params.value("entity_type", "all");
            
            Player* player = sMCPPlayerMgr->GetPlayer(sessionId);
            if (!player)
            {
                return {{"success", false}, {"error", "Player not online for this session"}};
            }
            
            json creatures = json::array();
            json players_arr = json::array();
            json gameobjects = json::array();
            
            // Scan for creatures
            if (typeFilter == "all" || typeFilter == "creatures")
            {
                std::list<Creature*> creatureList;
                player->GetCreatureListWithEntryInGrid(creatureList, 0, range);
                
                for (Creature* creature : creatureList)
                {
                    creatures.push_back({
                        {"guid", creature->GetGUID().GetCounter()},
                        {"entry", creature->GetEntry()},
                        {"name", creature->GetName()},
                        {"level", creature->GetLevel()},
                        {"health", creature->GetHealth()},
                        {"max_health", creature->GetMaxHealth()},
                        {"alive", creature->IsAlive()},
                        {"distance", player->GetDistance(creature)},
                        {"x", creature->GetPositionX()},
                        {"y", creature->GetPositionY()},
                        {"z", creature->GetPositionZ()}
                    });
                }
            }
            
            // Scan for players
            if (typeFilter == "all" || typeFilter == "players")
            {
                std::list<Player*> playerList;
                player->GetPlayerListInGrid(playerList, range);
                
                for (Player* other : playerList)
                {
                    if (other == player) continue;
                    players_arr.push_back({
                        {"guid", other->GetGUID().GetCounter()},
                        {"name", other->GetName()},
                        {"level", other->GetLevel()},
                        {"class", other->GetClass()},
                        {"race", other->GetRace()},
                        {"distance", player->GetDistance(other)},
                        {"x", other->GetPositionX()},
                        {"y", other->GetPositionY()},
                        {"z", other->GetPositionZ()}
                    });
                }
            }
            
            // Scan for game objects
            if (typeFilter == "all" || typeFilter == "gameobjects")
            {
                std::list<GameObject*> goList;
                player->GetGameObjectListWithEntryInGrid(goList, 0, range);
                
                for (GameObject* go : goList)
                {
                    gameobjects.push_back({
                        {"guid", go->GetGUID().GetCounter()},
                        {"entry", go->GetEntry()},
                        {"name", go->GetName()},
                        {"distance", player->GetDistance(go)},
                        {"x", go->GetPositionX()},
                        {"y", go->GetPositionY()},
                        {"z", go->GetPositionZ()}
                    });
                }
            }
            
            return {
                {"success", true},
                {"session_id", sessionId},
                {"position", {
                    {"x", player->GetPositionX()},
                    {"y", player->GetPositionY()},
                    {"z", player->GetPositionZ()},
                    {"map", player->GetMapId()}
                }},
                {"creatures", creatures},
                {"players", players_arr},
                {"gameobjects", gameobjects},
                {"creature_count", creatures.size()},
                {"player_count", players_arr.size()},
                {"gameobject_count", gameobjects.size()}
            };
        }
    );

    // ========================================================================
    // GM COMMAND TOOLS
    // ========================================================================

    // mcp_player_gm_command
    sMCPServer->RegisterTool(
        "mcp_player_gm_command",
        "Execute a GM command as the session's player.",
        {
            {"type", "object"},
            {"properties", {
                {"session_id", {{"type", "integer"}, {"description", "Session ID"}}},
                {"command", {{"type", "string"}, {"description", "GM command (without leading dot)"}}}
            }},
            {"required", {"session_id", "command"}}
        },
        [](const json& params) -> json {
            uint32 sessionId = params["session_id"];
            std::string command = params["command"];
            
            std::string result = sMCPPlayerMgr->ExecuteCommand(sessionId, command);
            
            return {
                {"success", true},
                {"session_id", sessionId},
                {"command", command},
                {"result", result}
            };
        }
    );

    TC_LOG_INFO("araxia.mcp", "[MCP] Registered 14 MCP Player tools (multi-session)");
}

} // namespace Araxia
