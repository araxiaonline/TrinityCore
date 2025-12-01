--[[
    MCP Bridge - Client Side
    
    Captures client-side events and sends them to the server via AMS
    for the MCP server to read. Also displays messages from MCP.
]]

local addonName, addon = ...

-- Create the MCP Bridge module
addon.MCPBridge = addon.MCPBridge or {}
local MCPBridge = addon.MCPBridge

-- Configuration
MCPBridge.enabled = true
MCPBridge.captureChat = true
MCPBridge.captureErrors = true
MCPBridge.pollInterval = 5 -- seconds between MCP message polls

-- Chat types to capture
MCPBridge.chatTypes = {
    ["CHAT_MSG_SAY"] = "SAY",
    ["CHAT_MSG_YELL"] = "YELL",
    ["CHAT_MSG_PARTY"] = "PARTY",
    ["CHAT_MSG_GUILD"] = "GUILD",
    ["CHAT_MSG_WHISPER"] = "WHISPER",
    ["CHAT_MSG_CHANNEL"] = "CHANNEL",
    ["CHAT_MSG_SYSTEM"] = "SYSTEM",
}

-- Send chat message to server for MCP
function MCPBridge:SendChatToMCP(message, chatType)
    if not self.enabled or not self.captureChat then return end
    if not AMS then return end
    
    AMS.Send("MCP_CHAT", {
        message = message,
        chatType = chatType or "CHAT"
    })
end

-- Send client log/error to server for MCP
function MCPBridge:SendLogToMCP(message, level, source)
    if not self.enabled or not self.captureErrors then return end
    if not AMS then return end
    
    AMS.Send("MCP_CLIENT_LOG", {
        message = message,
        level = level or "INFO",
        source = source or "AraxiaTrinityAdmin"
    })
end

-- Send structured data to server for MCP
function MCPBridge:SendDataToMCP(dataType, payload)
    if not self.enabled then return end
    if not AMS then return end
    
    AMS.Send("MCP_CLIENT_DATA", {
        dataType = dataType,
        payload = payload
    })
end

-- Request pending messages from MCP
function MCPBridge:PollMCPMessages()
    if not self.enabled then return end
    if not AMS then return end
    
    AMS.Send("MCP_GET_MESSAGES", {})
end

-- Handle messages received from MCP
function MCPBridge:OnMCPMessagesReceived(data)
    local messages = data.messages or {}
    
    for _, msg in ipairs(messages) do
        -- Display MCP messages in chat
        print("|cFF00FF00[MCP]|r " .. (msg.message or tostring(msg)))
    end
end

-- Register AMS handler for MCP messages
local function RegisterAMSHandler()
    if not AMS then
        C_Timer.After(1, RegisterAMSHandler) -- Retry
        return
    end
    
    AMS:RegisterHandler("MCP_MESSAGES_RESPONSE", function(data)
        MCPBridge:OnMCPMessagesReceived(data)
    end)
    
    print("|cFF00FF00[MCP Bridge]|r Client-side bridge ready")
end

-- Chat event handler
local chatFrame = CreateFrame("Frame")
chatFrame:RegisterEvent("CHAT_MSG_SAY")
chatFrame:RegisterEvent("CHAT_MSG_YELL")
chatFrame:RegisterEvent("CHAT_MSG_PARTY")
chatFrame:RegisterEvent("CHAT_MSG_GUILD")
chatFrame:RegisterEvent("CHAT_MSG_SYSTEM")

chatFrame:SetScript("OnEvent", function(self, event, message, sender, ...)
    local chatType = MCPBridge.chatTypes[event]
    if chatType and MCPBridge.captureChat then
        -- Only capture our own messages or system messages
        local playerName = UnitName("player")
        if sender == playerName or event == "CHAT_MSG_SYSTEM" then
            MCPBridge:SendChatToMCP(message, chatType)
        end
    end
end)

-- Error handler - capture Lua errors (disabled to prevent recursion)
-- The error handler can cause infinite loops if AMS serialization fails
-- We'll rely on explicit error logging instead
MCPBridge.captureErrors = false -- Disabled by default

-- Poll timer for MCP messages
local pollTimer
local function StartPolling()
    if pollTimer then return end
    pollTimer = C_Timer.NewTicker(MCPBridge.pollInterval, function()
        MCPBridge:PollMCPMessages()
    end)
end

-- Slash command to toggle MCP bridge
SLASH_MCPBRIDGE1 = "/mcpbridge"
SlashCmdList["MCPBRIDGE"] = function(msg)
    local cmd = msg:lower():trim()
    
    if cmd == "on" then
        MCPBridge.enabled = true
        print("|cFF00FF00[MCP Bridge]|r Enabled")
    elseif cmd == "off" then
        MCPBridge.enabled = false
        print("|cFFFF0000[MCP Bridge]|r Disabled")
    elseif cmd == "chat on" then
        MCPBridge.captureChat = true
        print("|cFF00FF00[MCP Bridge]|r Chat capture enabled")
    elseif cmd == "chat off" then
        MCPBridge.captureChat = false
        print("|cFFFF0000[MCP Bridge]|r Chat capture disabled")
    elseif cmd == "status" then
        print("|cFF00FFFF[MCP Bridge Status]|r")
        print("  Enabled: " .. (MCPBridge.enabled and "Yes" or "No"))
        print("  Chat Capture: " .. (MCPBridge.captureChat and "Yes" or "No"))
        print("  Error Capture: " .. (MCPBridge.captureErrors and "Yes" or "No"))
    elseif cmd == "test" then
        -- Send directly, bypassing captureErrors check
        if AMS then
            AMS.Send("MCP_CLIENT_LOG", {
                message = "Test message from client @ " .. date("%H:%M:%S"),
                level = "INFO",
                source = "MCPBridge Test"
            })
            print("|cFF00FF00[MCP Bridge]|r Test message sent to server")
        else
            print("|cFFFF0000[MCP Bridge]|r AMS not available")
        end
    elseif cmd == "poll" then
        StartPolling()
        print("|cFF00FF00[MCP Bridge]|r Polling started")
    else
        print("|cFF00FFFF[MCP Bridge Commands]|r")
        print("  /mcpbridge on|off - Enable/disable bridge")
        print("  /mcpbridge chat on|off - Toggle chat capture")
        print("  /mcpbridge status - Show status")
        print("  /mcpbridge test - Send test message")
    end
end

-- Initialize
C_Timer.After(2, function()
    RegisterAMSHandler()
    -- Polling disabled by default - use /mcpbridge poll to enable
    -- StartPolling()
end)
