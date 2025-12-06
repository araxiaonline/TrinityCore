/*
 * Araxia MCP Server Implementation
 */

#include "AraxiaMCPServer.h"
#include "Config.h"
#include "Log.h"
#include "World.h"

// cpp-httplib - header only HTTP server
#define CPPHTTPLIB_OPENSSL_SUPPORT 0
#include "httplib.h"

namespace Araxia
{

// Pimpl implementation
class MCPServer::Impl
{
public:
    httplib::Server server;
};

MCPServer::MCPServer() : _impl(std::make_unique<Impl>())
{
}

MCPServer::~MCPServer()
{
    // Note: During static destruction, other singletons may already be destroyed.
    // We need to be careful not to access them.
    try
    {
        Shutdown();
    }
    catch (...)
    {
        // Swallow any exceptions during destruction
    }
}

MCPServer* MCPServer::Instance()
{
    static MCPServer instance;
    return &instance;
}

bool MCPServer::Initialize()
{
    if (_running)
        return true;
    
    // Load configuration
    _port = static_cast<uint16>(sConfigMgr->GetIntDefault("Araxia.MCP.Port", 8765));
    _authToken = sConfigMgr->GetStringDefault("Araxia.MCP.AuthToken", "");
    _allowRemote = sConfigMgr->GetBoolDefault("Araxia.MCP.AllowRemote", false);
    
    if (!sConfigMgr->GetBoolDefault("Araxia.MCP.Enable", false))
    {
        TC_LOG_INFO("araxia.mcp", "[MCP] Araxia MCP Server is disabled in config");
        return true;
    }
    
    if (_authToken.empty())
    {
        TC_LOG_WARN("araxia.mcp", "[MCP] No auth token configured! Set Araxia.MCP.AuthToken for security.");
    }
    
    // Register built-in tools
    RegisterServerTools();
    RegisterDatabaseTools();
    RegisterWorldScanTools();  // LIDAR-style spatial awareness
    RegisterSpawnTools();      // Headless spawn management
    // RegisterElunaTools();  // Phase 3
    // RegisterWorldTools();  // Phase 4
    
    // Setup HTTP routes
    auto& svr = _impl->server;
    
    // Health check endpoint
    svr.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"status":"ok"})", "application/json");
    });
    
    // MCP endpoint (JSON-RPC)
    svr.Post("/mcp", [this](const httplib::Request& req, httplib::Response& res) {
        // Check auth
        std::string authHeader = req.get_header_value("Authorization");
        if (!ValidateAuth(authHeader))
        {
            res.status = 401;
            res.set_content(R"({"error":"Unauthorized"})", "application/json");
            return;
        }
        
        std::string response;
        HandleMCPRequest(req.body, response);
        res.set_content(response, "application/json");
    });
    
    // SSE endpoint for streaming (simple implementation)
    svr.Get("/mcp/stream", [this](const httplib::Request& req, httplib::Response& res) {
        std::string authHeader = req.get_header_value("Authorization");
        if (!ValidateAuth(authHeader))
        {
            res.status = 401;
            res.set_content(R"({"error":"Unauthorized"})", "application/json");
            return;
        }
        
        // For now just return the server-sent events headers
        // Full streaming implementation in Phase 5
        res.set_header("Content-Type", "text/event-stream");
        res.set_header("Cache-Control", "no-cache");
        res.set_content("data: {\"type\":\"connected\"}\n\n", "text/event-stream");
    });
    
    // Start server thread
    _serverThread = std::make_unique<std::thread>(&MCPServer::ServerThread, this);
    
    TC_LOG_INFO("araxia.mcp", "[MCP] Araxia MCP Server starting on port %u (remote: %s)", 
                _port, _allowRemote ? "allowed" : "localhost only");
    
    return true;
}

void MCPServer::Shutdown()
{
    // Only shutdown if we actually started
    if (!_serverThread && !_running)
        return;
    
    // Prevent double shutdown
    if (_shutdownRequested.exchange(true))
        return;
    
    // Stop the HTTP server first (this will cause listen() to return)
    // httplib::Server::stop() is thread-safe
    if (_impl)
        _impl->server.stop();
    
    // Wait for server thread to finish
    if (_serverThread)
    {
        if (_serverThread->joinable())
            _serverThread->join();
        _serverThread.reset();
    }
    
    _running = false;
    // Note: Don't reset _shutdownRequested - keep it true to prevent re-init during destruction
}

void MCPServer::ServerThread()
{
    std::string bindAddr = _allowRemote ? "0.0.0.0" : "127.0.0.1";
    
    _running = true;
    
    TC_LOG_INFO("araxia.mcp", "[MCP] Server listening on %s:%u", bindAddr.c_str(), _port);
    
    if (!_impl->server.listen(bindAddr.c_str(), _port))
    {
        if (!_shutdownRequested)
            TC_LOG_ERROR("araxia.mcp", "[MCP] Failed to start server on %s:%u", bindAddr.c_str(), _port);
    }
    
    _running = false;
}

bool MCPServer::ValidateAuth(const std::string& authHeader) const
{
    // No token configured = no auth required (development mode)
    if (_authToken.empty())
        return true;
    
    // Expect "Bearer <token>"
    const std::string prefix = "Bearer ";
    if (authHeader.length() <= prefix.length())
        return false;
    
    if (authHeader.substr(0, prefix.length()) != prefix)
        return false;
    
    std::string providedToken = authHeader.substr(prefix.length());
    return providedToken == _authToken;
}

void MCPServer::RegisterTool(const std::string& name, const std::string& description,
                              const json& inputSchema, MCPToolFunc handler)
{
    std::lock_guard<std::mutex> lock(_toolsMutex);
    _tools[name] = MCPToolInfo{name, description, inputSchema, handler};
    TC_LOG_DEBUG("araxia.mcp", "[MCP] Registered tool: %s", name.c_str());
}

void MCPServer::UnregisterTool(const std::string& name)
{
    std::lock_guard<std::mutex> lock(_toolsMutex);
    _tools.erase(name);
}

std::vector<MCPToolInfo> MCPServer::GetTools() const
{
    std::lock_guard<std::mutex> lock(_toolsMutex);
    std::vector<MCPToolInfo> result;
    result.reserve(_tools.size());
    for (const auto& [name, info] : _tools)
        result.push_back(info);
    return result;
}

void MCPServer::HandleMCPRequest(const std::string& body, std::string& response)
{
    try
    {
        json request = json::parse(body);
        json result = ProcessJsonRpc(request);
        response = result.dump();
    }
    catch (const json::exception& e)
    {
        json error = {
            {"jsonrpc", "2.0"},
            {"error", {
                {"code", -32700},
                {"message", "Parse error"},
                {"data", e.what()}
            }},
            {"id", nullptr}
        };
        response = error.dump();
    }
}

json MCPServer::ProcessJsonRpc(const json& request)
{
    // Extract ID first (can be number, string, or null)
    json id = request.contains("id") ? request["id"] : json(nullptr);
    
    // Validate JSON-RPC structure
    if (!request.contains("jsonrpc") || request["jsonrpc"] != "2.0")
    {
        return {
            {"jsonrpc", "2.0"},
            {"error", {{"code", -32600}, {"message", "Invalid Request"}}},
            {"id", id}
        };
    }
    
    std::string method = request.value("method", "");
    json params = request.contains("params") ? request["params"] : json::object();
    
    json result;
    
    try
    {
        if (method == "initialize")
            result = HandleInitialize(params);
        else if (method == "tools/list")
            result = HandleToolsList(params);
        else if (method == "tools/call")
            result = HandleToolsCall(params);
        else if (method == "resources/list")
            result = HandleResourcesList(params);
        else if (method == "resources/read")
            result = HandleResourcesRead(params);
        else
        {
            return {
                {"jsonrpc", "2.0"},
                {"error", {{"code", -32601}, {"message", "Method not found"}, {"data", method}}},
                {"id", id}
            };
        }
        
        return {
            {"jsonrpc", "2.0"},
            {"result", result},
            {"id", id}
        };
    }
    catch (const std::exception& e)
    {
        return {
            {"jsonrpc", "2.0"},
            {"error", {{"code", -32603}, {"message", "Internal error"}, {"data", e.what()}}},
            {"id", id}
        };
    }
}

json MCPServer::HandleInitialize(const json& /*params*/)
{
    return {
        {"protocolVersion", "2024-11-05"},
        {"capabilities", {
            {"tools", {{"listChanged", true}}},
            {"resources", {{"subscribe", false}, {"listChanged", false}}}
        }},
        {"serverInfo", {
            {"name", "Araxia MCP Server"},
            {"version", "1.0.0"}
        }}
    };
}

json MCPServer::HandleToolsList(const json& /*params*/)
{
    json tools = json::array();
    
    for (const auto& tool : GetTools())
    {
        tools.push_back({
            {"name", tool.name},
            {"description", tool.description},
            {"inputSchema", tool.inputSchema}
        });
    }
    
    return {{"tools", tools}};
}

json MCPServer::HandleToolsCall(const json& params)
{
    std::string toolName = params.value("name", "");
    json arguments = params.value("arguments", json::object());
    
    MCPToolFunc handler;
    {
        std::lock_guard<std::mutex> lock(_toolsMutex);
        auto it = _tools.find(toolName);
        if (it == _tools.end())
        {
            return {
                {"content", {{
                    {"type", "text"},
                    {"text", "Tool not found: " + toolName}
                }}},
                {"isError", true}
            };
        }
        handler = it->second.handler;
    }
    
    try
    {
        json result = handler(arguments);
        return {
            {"content", {{
                {"type", "text"},
                {"text", result.dump(2)}
            }}},
            {"isError", false}
        };
    }
    catch (const std::exception& e)
    {
        return {
            {"content", {{
                {"type", "text"},
                {"text", std::string("Tool error: ") + e.what()}
            }}},
            {"isError", true}
        };
    }
}

json MCPServer::HandleResourcesList(const json& /*params*/)
{
    // Resources are static data - we'll add these later
    return {{"resources", json::array()}};
}

json MCPServer::HandleResourcesRead(const json& params)
{
    std::string uri = params.value("uri", "");
    return {
        {"contents", {{
            {"uri", uri},
            {"mimeType", "text/plain"},
            {"text", "Resource not found: " + uri}
        }}}
    };
}

} // namespace Araxia
