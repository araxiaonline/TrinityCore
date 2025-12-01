--[[
    MCP Bridge - Server Side
    
    Provides a communication bridge between the WoW client and the MCP server.
    Client sends messages via AMS, server stores them in ElunaSharedData,
    MCP tools can read/write to ElunaSharedData.
    
    Keys used:
    - mcp_client_chat: Last N chat messages from client
    - mcp_client_logs: Client-side addon errors/logs
    - mcp_to_client: Messages from MCP to display on client
]]

local Smallfolk = require("Smallfolk")

-- Configuration
local MAX_CHAT_MESSAGES = 50
local MAX_LOG_MESSAGES = 100

-- Initialize storage keys if not present
local function InitSharedData()
    if not HasSharedData("mcp_client_chat") then
        SetSharedData("mcp_client_chat", Smallfolk.dumps({}))
    end
    if not HasSharedData("mcp_client_logs") then
        SetSharedData("mcp_client_logs", Smallfolk.dumps({}))
    end
    if not HasSharedData("mcp_to_client") then
        SetSharedData("mcp_to_client", Smallfolk.dumps({}))
    end
end

-- Add a message to a shared data list (FIFO)
local function AddToSharedList(key, message, maxItems)
    local data = GetSharedData(key)
    local list = data and Smallfolk.loads(data) or {}
    
    table.insert(list, {
        timestamp = os.time(),
        message = message
    })
    
    -- Trim to max size
    while #list > maxItems do
        table.remove(list, 1)
    end
    
    SetSharedData(key, Smallfolk.dumps(list))
end

-- AMS Handler: Client sends chat message for MCP to see
local function HandleMCPChatMessage(player, data)
    local playerName = player:GetName()
    local message = data.message or ""
    local chatType = data.chatType or "CHAT"
    
    local formatted = string.format("[%s] %s: %s", chatType, playerName, message)
    AddToSharedList("mcp_client_chat", formatted, MAX_CHAT_MESSAGES)
    
    print("[MCP Bridge] Chat captured: " .. formatted)
end

-- AMS Handler: Client sends log/error for MCP to see
local function HandleMCPClientLog(player, data)
    local playerName = player:GetName()
    local logLevel = data.level or "INFO"
    local message = data.message or ""
    local source = data.source or "unknown"
    
    local formatted = string.format("[%s] [%s] %s: %s", logLevel, source, playerName, message)
    AddToSharedList("mcp_client_logs", formatted, MAX_LOG_MESSAGES)
    
    print("[MCP Bridge] Client log: " .. formatted)
end

-- AMS Handler: Client requests any pending MCP messages
local function HandleMCPGetMessages(player, data)
    local messagesData = GetSharedData("mcp_to_client")
    local messages = messagesData and Smallfolk.loads(messagesData) or {}
    
    -- Clear after reading
    SetSharedData("mcp_to_client", Smallfolk.dumps({}))
    
    AMS.Send(player, "MCP_MESSAGES_RESPONSE", {
        messages = messages
    })
end

-- AMS Handler: Client sends DB query result or other structured data
local function HandleMCPClientData(player, data)
    local playerName = player:GetName()
    local dataType = data.dataType or "generic"
    local payload = data.payload or {}
    
    -- Store in a type-specific key
    local key = "mcp_client_" .. dataType
    SetSharedData(key, Smallfolk.dumps({
        player = playerName,
        timestamp = os.time(),
        data = payload
    }))
    
    print("[MCP Bridge] Client data received: " .. dataType)
end

-- Register AMS handlers
local function RegisterAMSHandlers()
    -- Check if AMS is available
    if not AMS or not AMS.RegisterHandler then
        print("[MCP Bridge] Warning: AMS not available, skipping handler registration")
        return
    end
    
    AMS.RegisterHandler("MCP_CHAT", HandleMCPChatMessage)
    AMS.RegisterHandler("MCP_CLIENT_LOG", HandleMCPClientLog)
    AMS.RegisterHandler("MCP_GET_MESSAGES", HandleMCPGetMessages)
    AMS.RegisterHandler("MCP_CLIENT_DATA", HandleMCPClientData)
    
    print("[MCP Bridge] AMS handlers registered")
end

-- Initialize on load
InitSharedData()
RegisterAMSHandlers()

print("[MCP Bridge] Server-side bridge loaded")
