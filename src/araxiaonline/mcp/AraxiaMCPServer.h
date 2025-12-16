/*
 * Araxia MCP Server
 * 
 * Embedded Model Context Protocol server for AI assistant integration.
 * Provides direct access to server state, database, Eluna, and AMS.
 */

#ifndef ARAXIA_MCP_SERVER_H
#define ARAXIA_MCP_SERVER_H

#include "Define.h"
#include <string>
#include <functional>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <thread>
#include "json.hpp"

using json = nlohmann::json;

namespace Araxia
{

// Tool function signature: takes params JSON, returns result JSON
using MCPToolFunc = std::function<json(const json& params)>;

struct MCPToolInfo
{
    std::string name;
    std::string description;
    json inputSchema;
    MCPToolFunc handler;
};

class MCPServer
{
public:
    static MCPServer* Instance();
    
    // Lifecycle
    bool Initialize();
    void Shutdown();
    bool IsRunning() const { return _running; }
    
    // Tool registration
    void RegisterTool(const std::string& name, const std::string& description, 
                      const json& inputSchema, MCPToolFunc handler);
    void UnregisterTool(const std::string& name);
    
    // Get registered tools (for tools/list)
    std::vector<MCPToolInfo> GetTools() const;
    
    // Configuration
    void SetPort(uint16 port) { _port = port; }
    void SetAuthToken(const std::string& token) { _authToken = token; }
    void SetAllowRemote(bool allow) { _allowRemote = allow; }
    
private:
    MCPServer();
    ~MCPServer();
    
    // Non-copyable
    MCPServer(const MCPServer&) = delete;
    MCPServer& operator=(const MCPServer&) = delete;
    
    // HTTP request handlers
    void HandleMCPRequest(const std::string& body, std::string& response);
    json ProcessJsonRpc(const json& request);
    
    // MCP protocol methods
    json HandleInitialize(const json& params);
    json HandleToolsList(const json& params);
    json HandleToolsCall(const json& params);
    json HandleResourcesList(const json& params);
    json HandleResourcesRead(const json& params);
    
    // Auth validation
    bool ValidateAuth(const std::string& authHeader) const;
    
    // Server thread
    void ServerThread();
    
    // Members
    std::unique_ptr<std::thread> _serverThread;
    std::atomic<bool> _running{false};
    std::atomic<bool> _shutdownRequested{false};
    
    uint16 _port{8765};
    std::string _authToken;
    bool _allowRemote{false};
    
    mutable std::mutex _toolsMutex;
    std::unordered_map<std::string, MCPToolInfo> _tools;
    
    // Server implementation (pimpl to hide httplib)
    class Impl;
    std::unique_ptr<Impl> _impl;
};

#define sMCPServer Araxia::MCPServer::Instance()

// Built-in tool registration (called during Initialize)
void RegisterDatabaseTools();
void RegisterServerTools();
void RegisterElunaTools();
void RegisterWorldTools();
void RegisterWorldScanTools();  // LIDAR-style spatial awareness
void RegisterSpawnTools();      // Headless spawn management (no player required)
void RegisterMCPPlayerTools();  // AI player session management

} // namespace Araxia

#endif // ARAXIA_MCP_SERVER_H
