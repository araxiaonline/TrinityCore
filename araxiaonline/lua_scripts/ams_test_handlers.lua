--[[
    AMS Test Handlers - Server Side
    
    Comprehensive test suite for validating AMS client-server messaging.
    Handlers for various test scenarios including echo, data types, 
    performance, error handling, and server-push tests.
    
    See: araxiaonline/araxia_docs/admin_npcdata/AMS_TESTSUITE.md
]]

print("[AMS] Loading test handlers...")

-- Ensure AMS is loaded
if not AMS then
    print("[AMS] ERROR: AMS not loaded! Test handlers will not work.")
    return
end

-- Test statistics
local testStats = {
    echoCount = 0,
    typeTestCount = 0,
    errorTestCount = 0,
    largePayloadCount = 0,
    totalMessagesReceived = 0,
    startTime = os.time()
}

-- ============================================================================
-- Helper Functions
-- ============================================================================

-- Generate a large test payload
local function GenerateLargePayload(size)
    local chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
    local payload = {}
    for i = 1, size do
        local idx = math.random(1, #chars)
        payload[i] = chars:sub(idx, idx)
    end
    return table.concat(payload)
end

-- Calculate simple hash of string (for comparison)
local function SimpleHash(str)
    local hash = 0
    for i = 1, #str do
        hash = (hash * 31 + string.byte(str, i)) % 0xFFFFFFFF
    end
    return hash
end

-- Validate data types
local function ValidateDataTypes(data)
    local results = {
        success = true,
        validations = {}
    }
    
    if type(data.string) ~= "string" then
        results.success = false
        table.insert(results.validations, "string type failed")
    end
    
    if type(data.number) ~= "number" then
        results.success = false
        table.insert(results.validations, "number type failed")
    end
    
    if type(data.float) ~= "number" then
        results.success = false
        table.insert(results.validations, "float type failed")
    end
    
    if type(data.boolean) ~= "boolean" then
        results.success = false
        table.insert(results.validations, "boolean type failed")
    end
    
    if data.nilValue ~= nil then
        results.success = false
        table.insert(results.validations, "nil type failed")
    end
    
    if type(data.table) ~= "table" then
        results.success = false
        table.insert(results.validations, "table type failed")
    end
    
    if type(data.array) ~= "table" then
        results.success = false
        table.insert(results.validations, "array type failed")
    end
    
    if results.success then
        table.insert(results.validations, "All data types validated successfully")
    end
    
    return results
end

-- ============================================================================
-- Test Handlers
-- ============================================================================

-- TEST_ECHO: Simple echo test (round-trip)
AMS.RegisterHandler("TEST_ECHO", function(player, data)
    testStats.echoCount = testStats.echoCount + 1
    testStats.totalMessagesReceived = testStats.totalMessagesReceived + 1
    
    print("[AMS] ECHO test from", player:GetName(), "- Message:", data.message)
    
    -- Echo the data back with server timestamp
    local response = {
        message = data.message,
        clientTimestamp = data.timestamp,
        serverTimestamp = os.time(),
        echoCount = testStats.echoCount
    }
    
    AMS.Send(player, "TEST_ECHO_RESPONSE", response)
end)

-- TEST_TYPES: Test all Lua data types
AMS.RegisterHandler("TEST_TYPES", function(player, data)
    testStats.typeTestCount = testStats.typeTestCount + 1
    testStats.totalMessagesReceived = testStats.totalMessagesReceived + 1
    
    print("[AMS] TYPE test from", player:GetName())
    
    -- Validate the received data types
    local validation = ValidateDataTypes(data)
    
    -- Echo back the data with validation results
    local response = {
        receivedData = data,
        validation = validation,
        serverTimestamp = os.time()
    }
    
    AMS.Send(player, "TEST_TYPES_RESPONSE", response)
end)

-- TEST_LARGE_PAYLOAD: Test message splitting and reassembly
AMS.RegisterHandler("TEST_LARGE_PAYLOAD", function(player, data)
    testStats.largePayloadCount = testStats.largePayloadCount + 1
    testStats.totalMessagesReceived = testStats.totalMessagesReceived + 1
    
    print("[AMS] LARGE_PAYLOAD test from", player:GetName(), "- Size:", #data.payload, "bytes")
    
    -- Calculate hash of received payload
    local receivedHash = SimpleHash(data.payload)
    
    -- Generate a large response payload
    local responsePayload = GenerateLargePayload(data.responseSize or 3000)
    local responseHash = SimpleHash(responsePayload)
    
    local response = {
        receivedSize = #data.payload,
        receivedHash = receivedHash,
        expectedHash = data.expectedHash,
        hashMatch = (receivedHash == data.expectedHash),
        responsePayload = responsePayload,
        responseHash = responseHash,
        serverTimestamp = os.time()
    }
    
    AMS.Send(player, "TEST_LARGE_PAYLOAD_RESPONSE", response)
end)

-- TEST_RAPID_FIRE: Receive many messages rapidly
AMS.RegisterHandler("TEST_RAPID_FIRE", function(player, data)
    testStats.totalMessagesReceived = testStats.totalMessagesReceived + 1
    
    -- Only log every 10th message to avoid spam
    if data.messageId % 10 == 0 then
        print("[AMS] RAPID_FIRE progress:", data.messageId, "of", data.totalMessages)
    end
    
    -- Send acknowledgment back
    local response = {
        messageId = data.messageId,
        serverTimestamp = os.time()
    }
    
    AMS.Send(player, "TEST_RAPID_FIRE_ACK", response)
    
    -- If this was the last message, send completion
    if data.messageId == data.totalMessages then
        print("[AMS] RAPID_FIRE complete:", data.totalMessages, "messages received")
        AMS.Send(player, "TEST_RAPID_FIRE_COMPLETE", {
            totalMessages = data.totalMessages,
            totalReceived = testStats.totalMessagesReceived
        })
    end
end)

-- TEST_REQUEST_PUSH: Client requests server to push data
AMS.RegisterHandler("TEST_REQUEST_PUSH", function(player, data)
    testStats.totalMessagesReceived = testStats.totalMessagesReceived + 1
    
    print("[AMS] REQUEST_PUSH from", player:GetName(), "- Count:", data.pushCount)
    
    -- Send multiple server-initiated messages
    for i = 1, data.pushCount do
        local pushData = {
            pushNumber = i,
            totalPushes = data.pushCount,
            serverTimestamp = os.time(),
            randomData = math.random(1, 1000),
            message = "Server push #" .. i
        }
        
        AMS.Send(player, "TEST_SERVER_PUSH", pushData)
    end
    
    -- Send completion message
    AMS.Send(player, "TEST_REQUEST_PUSH_COMPLETE", {
        pushesSent = data.pushCount
    })
end)

-- TEST_ERROR_HANDLING: Test various error scenarios
AMS.RegisterHandler("TEST_ERROR_HANDLING", function(player, data)
    testStats.errorTestCount = testStats.errorTestCount + 1
    testStats.totalMessagesReceived = testStats.totalMessagesReceived + 1
    
    print("[AMS] ERROR_HANDLING test from", player:GetName(), "- Type:", data.errorType)
    
    if data.errorType == "throw" then
        -- This should be caught by AMS's pcall wrapper
        error("Intentional test error!")
        
    elseif data.errorType == "invalid" then
        -- Send back invalid data (nil handler name)
        AMS.Send(player, nil, {error = "invalid handler name"})
        
    elseif data.errorType == "timeout" then
        -- Simulate slow processing (don't send response immediately)
        print("[AMS] Simulating timeout (no response)")
        -- Don't send response
        
    else
        -- Unknown error type - send response
        AMS.Send(player, "TEST_ERROR_HANDLING_RESPONSE", {
            success = false,
            error = "Unknown error type: " .. tostring(data.errorType)
        })
    end
end)

-- TEST_PERFORMANCE: Measure round-trip performance
AMS.RegisterHandler("TEST_PERFORMANCE", function(player, data)
    testStats.totalMessagesReceived = testStats.totalMessagesReceived + 1
    
    local serverTime = os.time()
    
    local response = {
        clientStartTime = data.startTime,
        serverReceiveTime = serverTime,
        testIteration = data.iteration,
        totalIterations = data.totalIterations
    }
    
    AMS.Send(player, "TEST_PERFORMANCE_RESPONSE", response)
    
    -- Log every 25th iteration
    if data.iteration % 25 == 0 then
        print("[AMS] PERFORMANCE test progress:", data.iteration, "of", data.totalIterations)
    end
end)

-- TEST_NESTED_DATA: Test deeply nested table structures
AMS.RegisterHandler("TEST_NESTED_DATA", function(player, data)
    testStats.totalMessagesReceived = testStats.totalMessagesReceived + 1
    
    print("[AMS] NESTED_DATA test from", player:GetName(), "- Depth:", data.depth)
    
    -- Validate nested structure
    local function countDepth(tbl, currentDepth)
        currentDepth = currentDepth or 0
        if type(tbl) ~= "table" then
            return currentDepth
        end
        local maxDepth = currentDepth
        for k, v in pairs(tbl) do
            if type(v) == "table" then
                local depth = countDepth(v, currentDepth + 1)
                if depth > maxDepth then
                    maxDepth = depth
                end
            end
        end
        return maxDepth
    end
    
    -- countDepth counts levels below root, add 1 to include root table
    local measuredDepth = countDepth(data.nestedData) + 1
    
    local response = {
        expectedDepth = data.depth,
        measuredDepth = measuredDepth,
        depthMatch = (measuredDepth == data.depth),
        receivedData = data.nestedData
    }
    
    AMS.Send(player, "TEST_NESTED_DATA_RESPONSE", response)
end)

-- TEST_GET_STATS: Get test statistics
AMS.RegisterHandler("TEST_GET_STATS", function(player, data)
    print("[AMS] GET_STATS request from", player:GetName())
    
    local uptime = os.time() - testStats.startTime
    
    local response = {
        stats = testStats,
        uptime = uptime,
        messagesPerMinute = uptime > 0 and (testStats.totalMessagesReceived / (uptime / 60)) or 0
    }
    
    AMS.Send(player, "TEST_GET_STATS_RESPONSE", response)
end)

-- TEST_RESET_STATS: Reset test statistics
AMS.RegisterHandler("TEST_RESET_STATS", function(player, data)
    print("[AMS] RESET_STATS request from", player:GetName())
    
    testStats = {
        echoCount = 0,
        typeTestCount = 0,
        errorTestCount = 0,
        largePayloadCount = 0,
        totalMessagesReceived = 0,
        startTime = os.time()
    }
    
    AMS.Send(player, "TEST_RESET_STATS_RESPONSE", {
        success = true,
        message = "Test statistics reset"
    })
end)

-- ============================================================================
-- Initialization
-- ============================================================================

print("[AMS] Registered test handlers:")
print("  - TEST_ECHO")
print("  - TEST_TYPES")
print("  - TEST_LARGE_PAYLOAD")
print("  - TEST_RAPID_FIRE")
print("  - TEST_REQUEST_PUSH")
print("  - TEST_ERROR_HANDLING")
print("  - TEST_PERFORMANCE")
print("  - TEST_NESTED_DATA")
print("  - TEST_GET_STATS")
print("  - TEST_RESET_STATS")
print("[AMS] Test suite ready!")
