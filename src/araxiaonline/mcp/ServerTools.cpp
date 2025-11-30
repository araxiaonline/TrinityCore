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
            
            TC_LOG_INFO("araxia.mcp", "[MCP] Executing GM command: .%s (player context: %s)", 
                        command.c_str(), playerName.empty() ? "none" : playerName.c_str());
            
            // Find player session if specified (for future ChatHandler integration)
            // TODO: Implement proper command execution via ChatHandler
            if (!playerName.empty())
            {
                Player* player = ObjectAccessor::FindPlayerByName(playerName);
                (void)player; // Suppress unused warning until ChatHandler is implemented
            }
            
            return {
                {"success", true},
                {"command", command},
                {"player", playerName},
                {"message", "Command logged. Note: Full command execution requires integration with ChatHandler."},
                {"note", "Use 'reload eluna' for Lua script reloading."}
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
            int maxLines = params.value("lines", 50);
            std::string logType = params.value("logType", "server");
            
            // For now, return info about where logs are
            return {
                {"success", true},
                {"message", "Log search requires file access. Logs are typically in /opt/trinitycore/logs/"},
                {"logFiles", {
                    {"server", "Server.log"},
                    {"eluna", "Eluna.log"},
                    {"dberrors", "DBErrors.log"},
                    {"gm", "GM.log"}
                }},
                {"pattern", pattern},
                {"maxLines", maxLines},
                {"note", "Use db_query on characters.gm_command_log for GM command history"}
            };
        }
    );
    
    TC_LOG_INFO("araxia.mcp", "[MCP] Server tools registered (server_info, player_list, gm_command, reload_scripts, log_search)");
}

} // namespace Araxia
