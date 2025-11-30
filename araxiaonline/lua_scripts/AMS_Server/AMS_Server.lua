--[[
    Araxia Messaging System (AMS) - Server Side
    
    A lightweight, modern client-server messaging library for TrinityCore 11.2.5
    Inspired by Rochet2's AIO but built for modern WoW and simplified for our needs.
    
    Features:
    - Handler registration system
    - Smallfolk serialization
    - Message splitting for long messages
    - Request/response pattern
    - Error isolation via pcall
    
    Usage:
        -- Register a handler
        AMS.RegisterHandler("NPC_SEARCH", function(player, data)
            local results = QueryDatabase(data.searchTerm)
            AMS.Send(player, "NPC_SEARCH_RESULT", results)
        end)
        
        -- Send a message (fluent API)
        AMS.Msg():Add("UPDATE_NPC", {npcID = 1234, hp = 5000}):Send(player)
]]

-- ============================================================================
-- Configuration
-- ============================================================================

local AMS_VERSION = "1.0.0-alpha"
local AMS_PREFIX = "AMS"
local AMS_DEBUG = true  -- Set to true for debugging

-- Message limits (server can send larger messages than client)
-- Client: 255 bytes max, Server: ~2560 bytes safe on most patches
local AMS_MAX_MSG_LENGTH = 2500 - #AMS_PREFIX - 10  -- Reserve space for overhead

-- Message ID tracking
local AMS_MSG_MIN_ID = 1
local AMS_MSG_MAX_ID = 65535  -- 16-bit ID

-- ============================================================================
-- Dependencies
-- ============================================================================

-- Smallfolk for serialization
local Smallfolk = require("AMS_Server.smallfolk")

-- ============================================================================
-- Core AMS Table
-- ============================================================================

-- NOTE: Cross-state data persistence is now handled by C++ ElunaSharedData
-- using SetSharedData()/GetSharedData() functions. The AMS table here is
-- only for local state within this Eluna instance.

AMS = {
    version = AMS_VERSION,
    handlers = {},
    playerData = {},  -- Local cache (C++ shared data is authoritative)
    nextMessageID = {},  -- Per-player message ID counter
}

-- ============================================================================
-- Utility Functions
-- ============================================================================

-- Debug logging (verbose)
local function Debug(...)
    if AMS_DEBUG then
        print("[AMS Server]", ...)
    end
end

-- Info logging (always shown)
local function Info(...)
    print("[AMS Server]", ...)
end

-- Error logging (always shown)
local function Error(...)
    print("[AMS Server] ERROR:", ...)
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

-- Get next message ID for a player
local function GetNextMessageID(playerGUID)
    if not AMS.nextMessageID[playerGUID] then
        AMS.nextMessageID[playerGUID] = AMS_MSG_MIN_ID
    end
    
    local msgID = AMS.nextMessageID[playerGUID]
    
    -- Increment and wrap around if needed
    if msgID >= AMS_MSG_MAX_ID then
        AMS.nextMessageID[playerGUID] = AMS_MSG_MIN_ID
    else
        AMS.nextMessageID[playerGUID] = msgID + 1
    end
    
    return msgID
end

-- ============================================================================
-- Message Sending
-- ============================================================================

-- Send a raw addon message (handles splitting if needed)
local function SendAddonMessage(player, message)
    local playerGUID = player:GetGUIDLow()
    
    Debug("Sending message to", player:GetName(), "length:", #message)
    
    -- Short message - send directly
    if #message <= AMS_MAX_MSG_LENGTH then
        -- Prefix with marker for short message (ID = 0000, parts = 0000, partID = 0000)
        local packet = NumberToHex(0) .. NumberToHex(0) .. NumberToHex(0) .. message
        player:SendAddonMessage(AMS_PREFIX, packet, 7, player)
        return
    end
    
    -- Long message - split into chunks
    local msgID = GetNextMessageID(playerGUID)
    local chunkSize = AMS_MAX_MSG_LENGTH - 12  -- Reserve 12 bytes for hex header (3 * 4 chars)
    local totalParts = math.ceil(#message / chunkSize)
    
    Debug("Splitting message ID", msgID, "into", totalParts, "parts")
    
    for partID = 1, totalParts do
        local startPos = (partID - 1) * chunkSize + 1
        local endPos = math.min(partID * chunkSize, #message)
        local chunk = message:sub(startPos, endPos)
        
        -- Header: msgID (4 hex) + totalParts (4 hex) + partID (4 hex) = 12 chars
        local header = NumberToHex(msgID) .. 
                      NumberToHex(totalParts) .. 
                      NumberToHex(partID)
        
        local packet = header .. chunk
        player:SendAddonMessage(AMS_PREFIX, packet, 7, player)
    end
end

-- Serialize and send data
function AMS.Send(player, handlerName, data)
    if type(player) ~= 'userdata' then
        Debug("ERROR: Send requires a player object")
        return
    end
    
    -- Create message block
    local messageBlock = {handlerName, data}
    
    -- Serialize using Smallfolk
    local serialized = Smallfolk.dumps({messageBlock})
    
    if not serialized then
        Debug("ERROR: Failed to serialize message for", handlerName)
        return
    end
    
    SendAddonMessage(player, serialized)
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

-- Send the message to player(s)
function MessageMT:Send(player, ...)
    if #self.blocks == 0 then
        Error("Attempted to send empty message")
        return
    end
    
    -- Serialize all blocks
    local serialized = Smallfolk.dumps(self.blocks)
    
    if not serialized then
        Error("Failed to serialize message")
        return
    end
    
    -- Send to primary player
    SendAddonMessage(player, serialized)
    
    -- Send to additional players if provided
    for i = 1, select('#', ...) do
        local additionalPlayer = select(i, ...)
        SendAddonMessage(additionalPlayer, serialized)
    end
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
-- Message Receiving & Reassembly
-- ============================================================================

-- Handle incoming message part (reassemble if split)
local function HandleIncomingMessage(player, rawMessage)
    local playerGUID = tostring(player:GetGUIDLow())
    local dataKey = "AMS_PLAYER_" .. playerGUID
    
    -- Use C++ shared data for cross-state persistence
    -- C++ stores strings, so we serialize with Smallfolk
    local serializedData = GetSharedData(dataKey)
    local playerData
    if serializedData then
        local success, decoded = pcall(Smallfolk.loads, serializedData)
        if success and type(decoded) == 'table' then
            playerData = decoded
        else
            playerData = { pendingMessages = {} }
        end
    else
        playerData = { pendingMessages = {} }
    end
    
    -- Parse header (12 chars hex: msgID + totalParts + partID)
    if #rawMessage < 12 then
        Error("Message too short for header, length:", #rawMessage)
        return nil
    end
    
    local hexMsgID = rawMessage:sub(1, 4)
    local hexTotalParts = rawMessage:sub(5, 8)
    local hexPartID = rawMessage:sub(9, 12)
    
    local msgID = HexToNumber(hexMsgID)
    local totalParts = HexToNumber(hexTotalParts)
    local partID = HexToNumber(hexPartID)
    local payload = rawMessage:sub(13)
    
    Debug(string.format("Parsed: msgID=%d, totalParts=%d, partID=%d", msgID, totalParts, partID))
    Debug("Received part", partID, "of", totalParts, "for message ID", msgID)
    
    -- Short message (msgID = 0, totalParts = 0, partID = 0)
    if msgID == 0 and totalParts == 0 and partID == 0 then
        Debug("Received short message, length:", #payload)
        return payload  -- Return payload for processing
    end
    
    -- Long message part
    Debug("Received part", partID, "of", totalParts, "for message ID", msgID)
    
    -- Initialize message tracking
    if not playerData.pendingMessages[msgID] then
        Debug("Creating new pendingMessages entry for msgID", msgID)
        playerData.pendingMessages[msgID] = {
            parts = {},
            totalParts = totalParts,
            receivedParts = 0
        }
    end
    
    local msgData = playerData.pendingMessages[msgID]
    
    -- Store the part
    if not msgData.parts[partID] then
        msgData.parts[partID] = payload
        msgData.receivedParts = msgData.receivedParts + 1
        Debug(string.format("Stored part %d, now have %d of %d parts", partID, msgData.receivedParts, msgData.totalParts))
    else
        Debug(string.format("Duplicate part %d received, ignoring", partID))
    end
    
    -- Save updated data back to C++ shared storage (serialize first)
    SetSharedData(dataKey, Smallfolk.dumps(playerData))
    
    -- Check if we have all parts
    if msgData.receivedParts == msgData.totalParts then
        Debug("Message ID", msgID, "complete, reassembling...")
        
        -- Reassemble message (parts are 1-indexed from client)
        local completeParts = {}
        for i = 1, msgData.totalParts do
            if msgData.parts[i] then
                table.insert(completeParts, msgData.parts[i])
            else
                Error("Missing part", i, "for message", msgID)
                playerData.pendingMessages[msgID] = nil
                SetSharedData(dataKey, Smallfolk.dumps(playerData))
                return nil
            end
        end
        local completeMessage = table.concat(completeParts)
        
        Debug("Reassembled message length:", #completeMessage)
        
        -- Clean up this message from pending
        playerData.pendingMessages[msgID] = nil
        SetSharedData(dataKey, Smallfolk.dumps(playerData))
        
        return completeMessage  -- Return complete message for processing
    end
    
    -- Message incomplete, wait for more parts
    return nil
end

-- Process a complete message (deserialize and dispatch handlers)
local function ProcessMessage(player, serializedMessage)
    -- Deserialize using Smallfolk
    local success, blocks = pcall(Smallfolk.loads, serializedMessage)
    
    if not success or type(blocks) ~= 'table' then
        Error("Failed to deserialize message:", blocks)
        return
    end
    
    Debug("Processing", #blocks, "message block(s)")
    
    -- Process each block
    for i, block in ipairs(blocks) do
        if type(block) == 'table' and #block >= 1 then
            local handlerName = block[1]
            local data = block[2]
            
            -- Find and call handler
            local handler = AMS.handlers[handlerName]
            if handler then
                Debug("Calling handler:", handlerName)
                
                -- Use pcall to isolate errors
                local success, err = pcall(handler, player, data)
                if not success then
                    Error("Handler", handlerName, "failed:", err)
                end
            else
                Error("No handler registered for", handlerName)
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
        Error("Handler name must be a string")
        return
    end
    
    if type(callback) ~= 'function' then
        Error("Handler callback must be a function")
        return
    end
    
    if AMS.handlers[handlerName] then
        Info("Overwriting handler:", handlerName)
    end
    
    AMS.handlers[handlerName] = callback
    Info("Registered handler:", handlerName)
end

-- ============================================================================
-- Event Hooks (using shared global data across all states)
-- ============================================================================

local stateMapId = GetStateMapId()
print(string.format("[AMS Server] Registering event handlers in state %d", stateMapId))

-- Handle incoming addon messages from clients
RegisterServerEvent(30, function(event, player, msgType, prefix, message, target)
    if prefix ~= AMS_PREFIX then
        return  -- Not our message
    end
    
    Debug(string.format("Received addon message from %s, length: %d", player:GetName(), #message))
    
    -- Handle message reassembly (uses C++ ElunaSharedData)
    local completeMessage = HandleIncomingMessage(player, message)
    
    if completeMessage then
        Debug("Message reassembled, length:", #completeMessage)
        -- Process the complete message
        ProcessMessage(player, completeMessage)
    else
        Debug("Waiting for more message parts...")
    end
end)

-- Clean up player data on logout
RegisterPlayerEvent(4, function(event, player)
    local playerGUID = tostring(player:GetGUIDLow())
    local dataKey = "AMS_PLAYER_" .. playerGUID
    
    -- Clean up C++ shared data
    if HasSharedData(dataKey) then
        Debug("Cleaning up shared data for", player:GetName())
        ClearSharedData(dataKey)
    end
    
    -- Also clean up local references (legacy)
    if AMS.playerData[playerGUID] then
        AMS.playerData[playerGUID] = nil
        AMS.nextMessageID[playerGUID] = nil
    end
end)

print(string.format("[AMS Server] Event handlers registered successfully in state %d", stateMapId))

-- ============================================================================
-- Initialization
-- ============================================================================

print("[AMS Server] Initialization complete - AMS Server v" .. AMS_VERSION)
Info("AMS Server v" .. AMS_VERSION .. " initialized")

-- Export for testing
return AMS
