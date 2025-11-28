--[[
    Araxia Messaging System (AMS) - Client Side
    
    A lightweight, modern client-server messaging library for WoW 11.2.5
    Inspired by Rochet2's AIO but built for modern WoW and simplified for our needs.
    
    Features:
    - Handler registration system
    - Smallfolk serialization
    - Message splitting for long messages
    - Request/response pattern
    - Error isolation via pcall
    
    Usage:
        -- Register a handler
        AMS.RegisterHandler("NPC_SEARCH_RESULT", function(data)
            DisplayResults(data)
        end)
        
        -- Send a message (fluent API)
        AMS.Msg():Add("NPC_SEARCH", {searchTerm = "Ragnaros"}):Send()
        
        -- Request/response pattern
        AMS.Request("NPC_SEARCH", {searchTerm = "Ragnaros"}, function(results)
            DisplayResults(results)
        end)
]]

-- ============================================================================
-- Configuration
-- ============================================================================

local AMS_VERSION = "1.0.0-alpha"
local AMS_PREFIX = "AMS"
local AMS_MAX_MSG_LENGTH = 240  -- WoW client limit is 255, leave some buffer
local AMS_DEBUG = false  -- Set to true for debugging

-- Client can only send 255 byte messages
-- local AMS_MAX_MSG_LENGTH = 240  -- Leave room for overhead (removed duplicate line)

-- Message ID tracking
local AMS_MSG_MIN_ID = 1
local AMS_MSG_MAX_ID = 65535  -- 16-bit ID

-- ============================================================================
-- Dependencies
-- ============================================================================

-- Smallfolk for serialization
-- Loaded via .toc file (Dep_Smallfolk\smallfolk.lua)
-- Available as global: Smallfolk

-- Verify Smallfolk is loaded
if not Smallfolk then
    error("AMS_Client requires Smallfolk library! Check .toc file loading order.")
end

-- ============================================================================
-- Core AMS Table
-- ============================================================================

AMS = {
    version = AMS_VERSION,
    handlers = {},
    pendingMessages = {},  -- For message reassembly
    nextMessageID = AMS_MSG_MIN_ID,
    pendingRequests = {},  -- For request/response pattern
    nextRequestID = 1,
}

-- Make globally accessible
_G.AMS = AMS

-- ============================================================================
-- Utility Functions
-- ============================================================================

-- Debug logging
local function Debug(...)
    if AMS_DEBUG then
        print("|cFF00FF00[AMS Client]|r", ...)
    end
end

-- Encode number as 4-character hex string (text-safe)
local function NumberToHex(num)
    return string.format("%04X", num)
end

-- Decode hex string to number
local function HexToNumber(str)
    if #str < 4 then return 0 end
    return tonumber(str:sub(1, 4), 16) or 0
end

-- Get next message ID
local function GetNextMessageID()
    local msgID = AMS.nextMessageID
    
    -- Increment and wrap around if needed
    if msgID >= AMS_MSG_MAX_ID then
        AMS.nextMessageID = AMS_MSG_MIN_ID
    else
        AMS.nextMessageID = msgID + 1
    end
    
    return msgID
end

-- ============================================================================
-- Message Sending
-- ============================================================================

-- Send a raw addon message (handles splitting if needed)
local function SendAddonMessage(message)
    Debug("Sending message, length:", #message)
    
    -- Short message - send directly
    if #message <= AMS_MAX_MSG_LENGTH then
        -- Prefix with marker for short message (ID = 0000, parts = 0000, partID = 0000)
        local packet = NumberToHex(0) .. NumberToHex(0) .. NumberToHex(0) .. message
        -- Use PARTY channel for solo players, fallback to WHISPER if in party
        local channel = IsInGroup() and "PARTY" or "WHISPER"
        local target = channel == "WHISPER" and UnitName("player") or nil
        Debug("Sending via channel:", channel, "target:", target or "none", "prefix:", AMS_PREFIX)
        C_ChatInfo.SendAddonMessage(AMS_PREFIX, packet, channel, target)
        return
    end
    
    -- Long message - split into chunks
    local msgID = GetNextMessageID()
    local chunkSize = AMS_MAX_MSG_LENGTH - 12  -- Reserve 12 bytes for hex header (3 * 4 chars)
    local totalParts = math.ceil(#message / chunkSize)
    
    Debug("Splitting message ID", msgID, "into", totalParts, "parts")
    
    -- Use PARTY channel for solo players, fallback to WHISPER if in party
    local channel = IsInGroup() and "PARTY" or "WHISPER"
    local target = channel == "WHISPER" and UnitName("player") or nil
    
    for partID = 1, totalParts do
        local startPos = (partID - 1) * chunkSize + 1
        local endPos = math.min(partID * chunkSize, #message)
        local chunk = message:sub(startPos, endPos)
        
        -- Header: msgID (4 hex) + totalParts (4 hex) + partID (4 hex) = 12 chars
        local header = NumberToHex(msgID) .. 
                      NumberToHex(totalParts) .. 
                      NumberToHex(partID)
        
        local packet = header .. chunk
        C_ChatInfo.SendAddonMessage(AMS_PREFIX, packet, channel, target)
    end
end

-- Serialize and send data
function AMS.Send(handlerName, data)
    -- Create message block
    local messageBlock = {handlerName, data}
    
    -- Serialize using Smallfolk
    local serialized = Smallfolk.dumps({messageBlock})
    
    if not serialized then
        Debug("ERROR: Failed to serialize message for", handlerName)
        return
    end
    
    SendAddonMessage(serialized)
end

-- ============================================================================
-- Message Class (Fluent API)
-- ============================================================================

local MessageMT = {}
MessageMT.__index = MessageMT

-- Add a handler call to the message
function MessageMT:Add(handlerName, data)
    table.insert(self.blocks, {handlerName, data})
    return self  -- Fluent API
end

-- Send the message
function MessageMT:Send()
    if #self.blocks == 0 then
        Debug("WARNING: Sending empty message")
        return
    end
    
    -- Serialize all blocks
    local serialized = Smallfolk.dumps(self.blocks)
    
    if not serialized then
        Debug("ERROR: Failed to serialize message")
        return
    end
    
    SendAddonMessage(serialized)
end

-- Check if message has content
function MessageMT:HasContent()
    return #self.blocks > 0
end

-- Create a new message
function AMS.Msg()
    local msg = {
        blocks = {}
    }
    setmetatable(msg, MessageMT)
    return msg
end

-- ============================================================================
-- Request/Response Pattern
-- ============================================================================

-- Send a request and register a callback for the response
function AMS.Request(handlerName, data, callback)
    if type(callback) ~= 'function' then
        Debug("ERROR: Request requires a callback function")
        return
    end
    
    -- Generate request ID
    local requestID = AMS.nextRequestID
    AMS.nextRequestID = requestID + 1
    
    -- Store callback
    AMS.pendingRequests[requestID] = callback
    
    -- Send request with ID embedded
    local requestData = {
        _requestID = requestID,
        data = data
    }
    
    AMS.Send(handlerName, requestData)
    
    Debug("Sent request", requestID, "for handler", handlerName)
end

-- Internal handler for responses
AMS.RegisterHandler = function(handlerName, callback)
    -- Will be defined below after handler system is set up
end

-- ============================================================================
-- Message Receiving & Reassembly
-- ============================================================================

-- Handle incoming message part (reassemble if needed)
local function HandleIncomingMessage(rawMessage)
    -- Extract header (first 12 chars: msgID, totalParts, partID - all hex)
    if #rawMessage < 12 then
        Debug("ERROR: Message too short for header")
        return nil
    end
    
    local msgID = HexToNumber(rawMessage:sub(1, 4))
    local totalParts = HexToNumber(rawMessage:sub(5, 8))
    local partID = HexToNumber(rawMessage:sub(9, 12))
    local payload = rawMessage:sub(13)
    
    -- Short message (msgID = 0, totalParts = 0, partID = 0)
    if msgID == 0 and totalParts == 0 and partID == 0 then
        Debug("Received short message, length:", #payload)
        return payload  -- Return payload for processing
    end
    
    -- Long message part
    Debug("Received part", partID, "of", totalParts, "for message ID", msgID)
    
    -- Initialize message tracking
    if not AMS.pendingMessages[msgID] then
        AMS.pendingMessages[msgID] = {
            parts = {},
            totalParts = totalParts,
            receivedParts = 0
        }
    end
    
    local msgData = AMS.pendingMessages[msgID]
    
    -- Store the part
    if not msgData.parts[partID] then
        msgData.parts[partID] = payload
        msgData.receivedParts = msgData.receivedParts + 1
    end
    
    -- Check if we have all parts
    if msgData.receivedParts == msgData.totalParts then
        Debug("Message ID", msgID, "complete, reassembling...")
        
        -- Reassemble message
        local completeParts = {}
        for i = 1, msgData.totalParts do
            table.insert(completeParts, msgData.parts[i])
        end
        local completeMessage = table.concat(completeParts)
        
        -- Clean up
        AMS.pendingMessages[msgID] = nil
        
        return completeMessage  -- Return complete message for processing
    end
    
    -- Message incomplete, wait for more parts
    return nil
end

-- Process a complete message (deserialize and dispatch handlers)
local function ProcessMessage(serializedMessage)
    Debug("ProcessMessage called with length:", #serializedMessage)
    Debug("First 50 chars:", serializedMessage:sub(1, math.min(50, #serializedMessage)))
    
    -- Deserialize using Smallfolk
    local success, blocks = pcall(Smallfolk.loads, serializedMessage)
    
    if not success or type(blocks) ~= 'table' then
        Debug("ERROR: Failed to deserialize message:", blocks)
        Debug("Message was:", serializedMessage)
        return
    end
    
    Debug("Processing", #blocks, "message block(s)")
    
    -- Process each block
    for i, block in ipairs(blocks) do
        if type(block) == 'table' and #block >= 1 then
            local handlerName = block[1]
            local data = block[2]
            
            -- Check if this is a response to a pending request
            if type(data) == 'table' and data._responseToRequest then
                local requestID = data._responseToRequest
                local callback = AMS.pendingRequests[requestID]
                
                if callback then
                    Debug("Calling response callback for request", requestID)
                    
                    -- Use pcall to isolate errors
                    local success, err = pcall(callback, data.data)
                    if not success then
                        Debug("ERROR in response callback:", err)
                    end
                    
                    -- Clean up
                    AMS.pendingRequests[requestID] = nil
                end
            else
                -- Normal handler
                local handler = AMS.handlers[handlerName]
                if handler then
                    Debug("Calling handler:", handlerName)
                    
                    -- Use pcall to isolate errors
                    local success, err = pcall(handler, data)
                    if not success then
                        Debug("ERROR in handler", handlerName, ":", err)
                    end
                else
                    Debug("WARNING: No handler registered for", handlerName)
                end
            end
        end
    end
end

-- ============================================================================
-- Handler Registration
-- ============================================================================

-- Register a message handler
function AMS.RegisterHandler(handlerName, callback)
    if type(handlerName) ~= 'string' then
        Debug("ERROR: Handler name must be a string")
        return
    end
    
    if type(callback) ~= 'function' then
        Debug("ERROR: Handler callback must be a function")
        return
    end
    
    if AMS.handlers[handlerName] then
        Debug("WARNING: Overwriting existing handler:", handlerName)
    end
    
    AMS.handlers[handlerName] = callback
    Debug("Registered handler:", handlerName)
end

-- ============================================================================
-- Event Registration
-- ============================================================================

-- Create event frame
local eventFrame = CreateFrame("Frame")

-- Register for addon messages
eventFrame:RegisterEvent("CHAT_MSG_ADDON")

-- Register prefix with WoW
local prefixRegistered = C_ChatInfo.RegisterAddonMessagePrefix(AMS_PREFIX)
if prefixRegistered then
    Debug("Successfully registered prefix:", AMS_PREFIX)
else
    Debug("WARNING: Failed to register prefix:", AMS_PREFIX)
end

-- Event handler
eventFrame:SetScript("OnEvent", function(self, event, prefix, message, channel, sender)
    if event ~= "CHAT_MSG_ADDON" then
        return
    end
    
    if prefix ~= AMS_PREFIX then
        return  -- Not our message
    end
    
    -- Ignore our own sent messages (only process server responses)
    local playerName = UnitName("player") .. "-" .. GetRealmName()
    if sender == playerName or sender == UnitName("player") then
        Debug("Ignoring own message, sender:", sender)
        return
    end
    
    Debug("Received message from server, sender:", sender, "channel:", channel)
    
    -- Handle message reassembly
    local completeMessage = HandleIncomingMessage(message)
    
    if completeMessage then
        -- Process the complete message
        ProcessMessage(completeMessage)
    end
end)

-- ============================================================================
-- Initialization
-- ============================================================================

Debug("AMS Client v" .. AMS_VERSION .. " initialized")

-- Export
return AMS
