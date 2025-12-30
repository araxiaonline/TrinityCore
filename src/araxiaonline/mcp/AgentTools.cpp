/*
 * Araxia MCP Server - Agent Tools
 * 
 * Tools for AI agent registration and bidirectional chat with players.
 * Agents register with friendly names (e.g., "Scarlet") and can receive
 * messages from players and send responses.
 * 
 * Message flow:
 *   Player (WoW) -> AMS -> ElunaSharedData -> MCP poll -> AI Agent
 *   AI Agent -> MCP send -> ElunaSharedData -> AMS push -> Player (WoW)
 * 
 * Data stored in ElunaSharedData:
 *   - agent_registry: JSON object mapping agent names to info
 *   - agent_inbox_<name>: JSON array of pending messages for agent
 *   - player_inbox_<guid>: JSON array of pending responses for player
 */

#include "AraxiaMCPServer.h"
#include "Log.h"
#include "GameTime.h"
#include "LuaEngine/ElunaSharedData.h"
#include <sstream>
#include <random>
#include <iomanip>

namespace Araxia
{

// Helper: Generate a unique message ID
static std::string GenerateMessageId()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;
    
    std::stringstream ss;
    ss << "msg_" << std::hex << dis(gen);
    return ss.str();
}

// Helper: Get current timestamp
static uint64_t GetTimestamp()
{
    return static_cast<uint64_t>(GameTime::GetGameTime());
}

// Helper: Get agent registry from shared data
static json GetAgentRegistry()
{
    std::string registryStr;
    if (!sElunaSharedData->Get("agent_registry", registryStr) || registryStr.empty())
        return json::object();
    
    try {
        return json::parse(registryStr);
    } catch (...) {
        return json::object();
    }
}

// Helper: Save agent registry to shared data
static void SaveAgentRegistry(const json& registry)
{
    sElunaSharedData->Set("agent_registry", registry.dump());
}

// Helper: Get agent inbox
static json GetAgentInbox(const std::string& agentName)
{
    std::string key = "agent_inbox_" + agentName;
    std::string inboxStr;
    if (!sElunaSharedData->Get(key, inboxStr) || inboxStr.empty())
        return json::array();
    
    try {
        return json::parse(inboxStr);
    } catch (...) {
        return json::array();
    }
}

// Helper: Save agent inbox
static void SaveAgentInbox(const std::string& agentName, const json& inbox)
{
    std::string key = "agent_inbox_" + agentName;
    sElunaSharedData->Set(key, inbox.dump());
}

// Helper: Get player inbox
static json GetPlayerInbox(uint64_t playerGuid)
{
    std::string key = "player_inbox_" + std::to_string(playerGuid);
    std::string inboxStr;
    if (!sElunaSharedData->Get(key, inboxStr) || inboxStr.empty())
        return json::array();
    
    try {
        return json::parse(inboxStr);
    } catch (...) {
        return json::array();
    }
}

// Helper: Save player inbox
static void SavePlayerInbox(uint64_t playerGuid, const json& inbox)
{
    std::string key = "player_inbox_" + std::to_string(playerGuid);
    sElunaSharedData->Set(key, inbox.dump());
}

// Helper: Normalize agent name (lowercase for comparison)
static std::string NormalizeAgentName(const std::string& name)
{
    std::string normalized = name;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    return normalized;
}

void RegisterAgentTools()
{
    TC_LOG_INFO("araxia.mcp", "[MCP] Registering Agent Chat tools...");

    // ========================================================================
    // mcp_agent_register - Register an AI agent with a friendly name
    // ========================================================================
    sMCPServer->RegisterTool(
        "mcp_agent_register",
        "Register an AI agent with a friendly name. Players can then send messages to this agent. "
        "Agent names are case-insensitive and must be unique.",
        {
            {"type", "object"},
            {"properties", {
                {"name", {
                    {"type", "string"},
                    {"description", "Friendly name for the agent (e.g., 'Scarlet', 'Helper')"}
                }},
                {"owner", {
                    {"type", "string"},
                    {"description", "Identifier for the LLM/system (e.g., 'Cascade', 'Claude')"}
                }},
                {"description", {
                    {"type", "string"},
                    {"description", "Optional description of what this agent does"}
                }}
            }},
            {"required", {"name", "owner"}}
        },
        [](const json& params) -> json {
            std::string name = params["name"];
            std::string owner = params["owner"];
            std::string description = params.value("description", "");
            
            if (name.empty()) {
                return {{"success", false}, {"error", "Agent name cannot be empty"}};
            }
            
            std::string normalizedName = NormalizeAgentName(name);
            
            // Check if name already taken
            json registry = GetAgentRegistry();
            for (auto& [key, value] : registry.items()) {
                if (NormalizeAgentName(key) == normalizedName) {
                    return {
                        {"success", false}, 
                        {"error", "Agent name already registered"},
                        {"existing_owner", value.value("owner", "")}
                    };
                }
            }
            
            // Register the agent
            registry[name] = {
                {"owner", owner},
                {"description", description},
                {"registered_at", GetTimestamp()},
                {"last_poll", GetTimestamp()},
                {"status", "online"}
            };
            
            SaveAgentRegistry(registry);
            
            TC_LOG_INFO("araxia.mcp", "[MCP] Agent '{}' registered by {}", name, owner);
            
            return {
                {"success", true},
                {"agent_name", name},
                {"owner", owner},
                {"message", "Agent registered successfully. Use mcp_agent_poll_messages to receive player messages."}
            };
        }
    );

    // ========================================================================
    // mcp_agent_unregister - Unregister an agent
    // ========================================================================
    sMCPServer->RegisterTool(
        "mcp_agent_unregister",
        "Unregister an AI agent. Pending messages will be discarded.",
        {
            {"type", "object"},
            {"properties", {
                {"name", {
                    {"type", "string"},
                    {"description", "Name of the agent to unregister"}
                }}
            }},
            {"required", {"name"}}
        },
        [](const json& params) -> json {
            std::string name = params["name"];
            std::string normalizedName = NormalizeAgentName(name);
            
            json registry = GetAgentRegistry();
            std::string foundKey;
            
            for (auto& [key, value] : registry.items()) {
                if (NormalizeAgentName(key) == normalizedName) {
                    foundKey = key;
                    break;
                }
            }
            
            if (foundKey.empty()) {
                return {{"success", false}, {"error", "Agent not found"}};
            }
            
            // Remove from registry
            registry.erase(foundKey);
            SaveAgentRegistry(registry);
            
            // Clear inbox
            std::string inboxKey = "agent_inbox_" + foundKey;
            sElunaSharedData->Clear(inboxKey);
            
            TC_LOG_INFO("araxia.mcp", "[MCP] Agent '{}' unregistered", foundKey);
            
            return {
                {"success", true},
                {"agent_name", foundKey},
                {"message", "Agent unregistered"}
            };
        }
    );

    // ========================================================================
    // mcp_agent_list - List all registered agents
    // ========================================================================
    sMCPServer->RegisterTool(
        "mcp_agent_list",
        "List all registered AI agents with their status.",
        {
            {"type", "object"},
            {"properties", json::object()}
        },
        [](const json& /*params*/) -> json {
            json registry = GetAgentRegistry();
            json agents = json::array();
            
            uint64_t now = GetTimestamp();
            
            for (auto& [name, info] : registry.items()) {
                // Mark as offline if no poll in 60 seconds
                uint64_t lastPoll = info.value("last_poll", 0ULL);
                bool isOnline = (now - lastPoll) < 60;
                
                // Get pending message count
                json inbox = GetAgentInbox(name);
                
                agents.push_back({
                    {"name", name},
                    {"owner", info.value("owner", "")},
                    {"description", info.value("description", "")},
                    {"online", isOnline},
                    {"last_poll", lastPoll},
                    {"pending_messages", inbox.size()}
                });
            }
            
            return {
                {"success", true},
                {"agent_count", agents.size()},
                {"agents", agents}
            };
        }
    );

    // ========================================================================
    // mcp_agent_poll_messages - Get pending messages for an agent
    // ========================================================================
    sMCPServer->RegisterTool(
        "mcp_agent_poll_messages",
        "Poll for pending messages sent to this agent by players. "
        "By default, messages are acknowledged (removed from queue) after retrieval.",
        {
            {"type", "object"},
            {"properties", {
                {"name", {
                    {"type", "string"},
                    {"description", "Agent name to poll messages for"}
                }},
                {"limit", {
                    {"type", "integer"},
                    {"description", "Maximum messages to return (default: 10)"}
                }},
                {"acknowledge", {
                    {"type", "boolean"},
                    {"description", "Remove messages from queue after returning (default: true)"}
                }}
            }},
            {"required", {"name"}}
        },
        [](const json& params) -> json {
            std::string name = params["name"];
            int limit = params.value("limit", 10);
            bool acknowledge = params.value("acknowledge", true);
            
            std::string normalizedName = NormalizeAgentName(name);
            
            // Find agent in registry
            json registry = GetAgentRegistry();
            std::string foundKey;
            
            for (auto& [key, value] : registry.items()) {
                if (NormalizeAgentName(key) == normalizedName) {
                    foundKey = key;
                    break;
                }
            }
            
            if (foundKey.empty()) {
                return {{"success", false}, {"error", "Agent not registered"}};
            }
            
            // Update last poll time
            registry[foundKey]["last_poll"] = GetTimestamp();
            registry[foundKey]["status"] = "online";
            SaveAgentRegistry(registry);
            
            // Get messages from inbox
            json inbox = GetAgentInbox(foundKey);
            json messages = json::array();
            
            int count = 0;
            for (auto& msg : inbox) {
                if (count >= limit) break;
                messages.push_back(msg);
                count++;
            }
            
            // Remove acknowledged messages
            if (acknowledge && count > 0) {
                json remainingInbox = json::array();
                for (size_t i = count; i < inbox.size(); i++) {
                    remainingInbox.push_back(inbox[i]);
                }
                SaveAgentInbox(foundKey, remainingInbox);
            }
            
            return {
                {"success", true},
                {"agent_name", foundKey},
                {"message_count", messages.size()},
                {"messages", messages},
                {"remaining", inbox.size() - count}
            };
        }
    );

    // ========================================================================
    // mcp_agent_send_message - Send a response to a player
    // ========================================================================
    sMCPServer->RegisterTool(
        "mcp_agent_send_message",
        "Send a message/response from this agent to a player. "
        "The message will be queued and delivered to the player via AMS.",
        {
            {"type", "object"},
            {"properties", {
                {"name", {
                    {"type", "string"},
                    {"description", "Agent name sending the message"}
                }},
                {"to_player_guid", {
                    {"type", "integer"},
                    {"description", "Target player GUID (from the original message)"}
                }},
                {"content", {
                    {"type", "string"},
                    {"description", "Message content to send"}
                }},
                {"reply_to_id", {
                    {"type", "string"},
                    {"description", "ID of the message being replied to (optional)"}
                }}
            }},
            {"required", {"name", "to_player_guid", "content"}}
        },
        [](const json& params) -> json {
            std::string name = params["name"];
            uint64_t toPlayerGuid = params["to_player_guid"];
            std::string content = params["content"];
            std::string replyToId = params.value("reply_to_id", "");
            
            std::string normalizedName = NormalizeAgentName(name);
            
            // Verify agent is registered
            json registry = GetAgentRegistry();
            std::string foundKey;
            
            for (auto& [key, value] : registry.items()) {
                if (NormalizeAgentName(key) == normalizedName) {
                    foundKey = key;
                    break;
                }
            }
            
            if (foundKey.empty()) {
                return {{"success", false}, {"error", "Agent not registered"}};
            }
            
            // Create the response message
            std::string messageId = GenerateMessageId();
            json message = {
                {"message_id", messageId},
                {"from_agent", foundKey},
                {"content", content},
                {"timestamp", GetTimestamp()}
            };
            
            if (!replyToId.empty()) {
                message["reply_to_id"] = replyToId;
            }
            
            // Add to player's inbox
            json playerInbox = GetPlayerInbox(toPlayerGuid);
            playerInbox.push_back(message);
            
            // Cap inbox size at 100 messages
            while (playerInbox.size() > 100) {
                playerInbox.erase(playerInbox.begin());
            }
            
            SavePlayerInbox(toPlayerGuid, playerInbox);
            
            TC_LOG_DEBUG("araxia.mcp", "[MCP] Agent '{}' sent message to player {}", foundKey, toPlayerGuid);
            
            return {
                {"success", true},
                {"message_id", messageId},
                {"to_player_guid", toPlayerGuid},
                {"queued", true},
                {"note", "Message queued. Will be delivered when player's client polls for responses."}
            };
        }
    );

    // ========================================================================
    // mcp_agent_get_player_responses - Get pending responses for a player (used by Lua)
    // ========================================================================
    sMCPServer->RegisterTool(
        "mcp_agent_get_player_responses",
        "Get pending agent responses for a specific player. Used internally by server scripts.",
        {
            {"type", "object"},
            {"properties", {
                {"player_guid", {
                    {"type", "integer"},
                    {"description", "Player GUID to get responses for"}
                }},
                {"acknowledge", {
                    {"type", "boolean"},
                    {"description", "Remove messages from queue after returning (default: true)"}
                }}
            }},
            {"required", {"player_guid"}}
        },
        [](const json& params) -> json {
            uint64_t playerGuid = params["player_guid"];
            bool acknowledge = params.value("acknowledge", true);
            
            json inbox = GetPlayerInbox(playerGuid);
            
            if (acknowledge && !inbox.empty()) {
                // Clear the inbox
                SavePlayerInbox(playerGuid, json::array());
            }
            
            return {
                {"success", true},
                {"player_guid", playerGuid},
                {"message_count", inbox.size()},
                {"messages", inbox}
            };
        }
    );

    TC_LOG_INFO("araxia.mcp", "[MCP] Registered 6 Agent Chat tools");
}

} // namespace Araxia
