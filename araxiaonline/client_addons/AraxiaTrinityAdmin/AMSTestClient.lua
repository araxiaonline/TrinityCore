-- AraxiaTrinityAdmin AMS Test Client
-- Client-side test framework for validating AMS messaging

local addonName = "AraxiaTrinityAdmin"

-- Test Client Module
local AMSTestClient = {}

-- Test state
AMSTestClient.tests = {}
AMSTestClient.results = {}
AMSTestClient.running = false
AMSTestClient.currentTest = nil
AMSTestClient.startTime = 0

-- Test status constants
local STATUS = {
    PENDING = "PENDING",
    RUNNING = "RUNNING",
    PASSED = "PASSED",
    FAILED = "FAILED",
    TIMEOUT = "TIMEOUT"
}

-- ============================================================================
-- Helper Functions
-- ============================================================================

local function Log(...)
    print("|cFF00FF00[AMS]|r", ...)
end

local function LogError(...)
    print("|cFFFF0000[AMS ERROR]|r", ...)
end

local function LogWarn(...)
    print("|cFFFFAA00[AMS WARN]|r", ...)
end

-- Generate large payload
local function GenerateLargePayload(size)
    local chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
    local payload = {}
    for i = 1, size do
        local idx = math.random(1, #chars)
        payload[i] = chars:sub(idx, idx)
    end
    return table.concat(payload)
end

-- Calculate simple hash
local function SimpleHash(str)
    local hash = 0
    for i = 1, #str do
        hash = (hash * 31 + string.byte(str, i)) % 0xFFFFFFFF
    end
    return hash
end

-- Create nested table structure
local function CreateNestedTable(depth)
    if depth <= 0 then
        return "leaf"
    end
    return {
        level = depth,
        nested = CreateNestedTable(depth - 1)
    }
end

-- ============================================================================
-- Test Definitions
-- ============================================================================

function AMSTestClient:DefineTests()
    -- TEST 1: Echo Test
    self.tests["ECHO"] = {
        name = "Echo Test",
        description = "Basic round-trip message test",
        category = "Basic",
        run = function()
            Log("Running ECHO test...")
            local testData = {
                message = "Hello from client!",
                timestamp = time()
            }
            AMS.Send("TEST_ECHO", testData)
        end,
        timeout = 5
    }
    
    -- TEST 2: Data Types Test
    self.tests["TYPES"] = {
        name = "Data Types Test",
        description = "Test all Lua data types",
        category = "Basic",
        run = function()
            Log("Running TYPES test...")
            local testData = {
                string = "Hello World",
                number = 42,
                float = 3.14159,
                boolean = true,
                nilValue = nil,
                table = {nested = "data", value = 123},
                array = {1, 2, 3, 4, 5}
            }
            AMS.Send("TEST_TYPES", testData)
        end,
        timeout = 5
    }
    
    -- TEST 3: Large Payload Test
    self.tests["LARGE_PAYLOAD"] = {
        name = "Large Payload Test",
        description = "Test message splitting (3KB payload)",
        category = "Performance",
        run = function()
            Log("Running LARGE_PAYLOAD test...")
            local payload = GenerateLargePayload(3000)
            local hash = SimpleHash(payload)
            local testData = {
                payload = payload,
                expectedHash = hash,
                responseSize = 3000
            }
            AMS.Send("TEST_LARGE_PAYLOAD", testData)
        end,
        timeout = 10
    }
    
    -- TEST 4: Rapid Fire Test
    self.tests["RAPID_FIRE"] = {
        name = "Rapid Fire Test",
        description = "Send 50 messages rapidly",
        category = "Performance",
        run = function()
            Log("Running RAPID_FIRE test...")
            local totalMessages = 50
            for i = 1, totalMessages do
                AMS.Send("TEST_RAPID_FIRE", {
                    messageId = i,
                    totalMessages = totalMessages,
                    timestamp = time()
                })
            end
        end,
        timeout = 15
    }
    
    -- TEST 5: Server Push Test
    self.tests["SERVER_PUSH"] = {
        name = "Server Push Test",
        description = "Request server to push 5 messages",
        category = "Advanced",
        run = function()
            Log("Running SERVER_PUSH test...")
            AMS.Send("TEST_REQUEST_PUSH", {
                pushCount = 5
            })
        end,
        timeout = 10
    }
    
    -- TEST 6: Error Handling Test (Graceful)
    self.tests["ERROR_GRACEFUL"] = {
        name = "Error Handling (Graceful)",
        description = "Test graceful error handling",
        category = "Error Handling",
        run = function()
            Log("Running ERROR_GRACEFUL test...")
            AMS.Send("TEST_ERROR_HANDLING", {
                errorType = "unknown"
            })
        end,
        timeout = 5
    }
    
    -- TEST 7: Performance Test
    self.tests["PERFORMANCE"] = {
        name = "Performance Test",
        description = "Measure round-trip latency (25 iterations)",
        category = "Performance",
        run = function()
            Log("Running PERFORMANCE test...")
            local totalIterations = 25
            for i = 1, totalIterations do
                AMS.Send("TEST_PERFORMANCE", {
                    startTime = GetTime(),
                    iteration = i,
                    totalIterations = totalIterations
                })
            end
        end,
        timeout = 15
    }
    
    -- TEST 8: Nested Data Test
    self.tests["NESTED_DATA"] = {
        name = "Nested Data Test",
        description = "Test 10-level deep nested tables",
        category = "Advanced",
        run = function()
            Log("Running NESTED_DATA test...")
            local nestedData = CreateNestedTable(10)
            AMS.Send("TEST_NESTED_DATA", {
                depth = 10,
                nestedData = nestedData
            })
        end,
        timeout = 5
    }
end

-- ============================================================================
-- Test Execution
-- ============================================================================

function AMSTestClient:RunTest(testId)
    if not AMS then
        LogError("AMS not loaded!")
        return false
    end
    
    local test = self.tests[testId]
    if not test then
        LogError("Test not found:", testId)
        return false
    end
    
    -- Initialize result
    self.results[testId] = {
        status = STATUS.RUNNING,
        startTime = GetTime(),
        endTime = nil,
        duration = nil,
        error = nil,
        data = {}
    }
    
    self.currentTest = testId
    
    -- Run the test
    local success, err = pcall(test.run)
    if not success then
        self.results[testId].status = STATUS.FAILED
        self.results[testId].error = err
        self.results[testId].endTime = GetTime()
        self.results[testId].duration = self.results[testId].endTime - self.results[testId].startTime
        LogError("Test", testId, "failed to run:", err)
        return false
    end
    
    -- Set timeout timer
    C_Timer.After(test.timeout, function()
        if self.results[testId] and self.results[testId].status == STATUS.RUNNING then
            self.results[testId].status = STATUS.TIMEOUT
            self.results[testId].endTime = GetTime()
            self.results[testId].duration = self.results[testId].endTime - self.results[testId].startTime
            self.results[testId].error = "Test timed out after " .. test.timeout .. " seconds"
            LogWarn("Test", testId, "timed out")
        end
    end)
    
    return true
end

function AMSTestClient:RunAll()
    Log("Running all tests...")
    self.running = true
    self.startTime = GetTime()
    
    for testId, test in pairs(self.tests) do
        self:RunTest(testId)
        -- Small delay between tests
        C_Timer.After(0.1, function() end)
    end
end

function AMSTestClient:CompleteTest(testId, success, data)
    if not self.results[testId] then
        LogWarn("Received response for unknown test:", testId)
        return
    end
    
    if self.results[testId].status ~= STATUS.RUNNING then
        -- Test already completed or timed out
        return
    end
    
    self.results[testId].status = success and STATUS.PASSED or STATUS.FAILED
    self.results[testId].endTime = GetTime()
    self.results[testId].duration = self.results[testId].endTime - self.results[testId].startTime
    self.results[testId].data = data
    
    if success then
        Log("Test", testId, "PASSED in", string.format("%.3f", self.results[testId].duration), "seconds")
    else
        LogError("Test", testId, "FAILED:", data.error or "Unknown error")
    end
end

function AMSTestClient:GetResults()
    return self.results
end

function AMSTestClient:GetSummary()
    local total = 0
    local passed = 0
    local failed = 0
    local timeout = 0
    local running = 0
    
    for testId, result in pairs(self.results) do
        total = total + 1
        if result.status == STATUS.PASSED then
            passed = passed + 1
        elseif result.status == STATUS.FAILED then
            failed = failed + 1
        elseif result.status == STATUS.TIMEOUT then
            timeout = timeout + 1
        elseif result.status == STATUS.RUNNING then
            running = running + 1
        end
    end
    
    return {
        total = total,
        passed = passed,
        failed = failed,
        timeout = timeout,
        running = running
    }
end

-- ============================================================================
-- Response Handlers
-- ============================================================================

-- Initialize response handlers when AMS is available
local function InitResponseHandlers()
    if not AMS then
        C_Timer.After(0.5, InitResponseHandlers)
        return
    end
    
    -- TEST_ECHO_RESPONSE
    AMS.RegisterHandler("TEST_ECHO_RESPONSE", function(data)
        Log("ECHO response received:", data.message)
        AMSTestClient:CompleteTest("ECHO", true, data)
    end)
    
    -- TEST_TYPES_RESPONSE
    AMS.RegisterHandler("TEST_TYPES_RESPONSE", function(data)
        local success = data.validation and data.validation.success
        AMSTestClient:CompleteTest("TYPES", success, data)
    end)
    
    -- TEST_LARGE_PAYLOAD_RESPONSE
    AMS.RegisterHandler("TEST_LARGE_PAYLOAD_RESPONSE", function(data)
        local success = data.hashMatch
        Log("LARGE_PAYLOAD response - Size:", data.receivedSize, "Hash match:", success)
        AMSTestClient:CompleteTest("LARGE_PAYLOAD", success, data)
    end)
    
    -- TEST_RAPID_FIRE_COMPLETE
    AMS.RegisterHandler("TEST_RAPID_FIRE_COMPLETE", function(data)
        Log("RAPID_FIRE complete:", data.totalMessages, "messages")
        AMSTestClient:CompleteTest("RAPID_FIRE", true, data)
    end)
    
    -- TEST_REQUEST_PUSH_COMPLETE
    AMS.RegisterHandler("TEST_REQUEST_PUSH_COMPLETE", function(data)
        Log("SERVER_PUSH complete:", data.pushesSent, "pushes sent")
        AMSTestClient:CompleteTest("SERVER_PUSH", true, data)
    end)
    
    -- TEST_SERVER_PUSH (actual server push)
    AMS.RegisterHandler("TEST_SERVER_PUSH", function(data)
        Log("Received server push", data.pushNumber, "of", data.totalPushes)
    end)
    
    -- TEST_ERROR_HANDLING_RESPONSE
    AMS.RegisterHandler("TEST_ERROR_HANDLING_RESPONSE", function(data)
        AMSTestClient:CompleteTest("ERROR_GRACEFUL", true, data)
    end)
    
    -- TEST_PERFORMANCE_RESPONSE
    AMS.RegisterHandler("TEST_PERFORMANCE_RESPONSE", function(data)
        -- Track latencies
        if not AMSTestClient.results["PERFORMANCE"].latencies then
            AMSTestClient.results["PERFORMANCE"].latencies = {}
        end
        
        local roundTrip = GetTime() - data.clientStartTime
        table.insert(AMSTestClient.results["PERFORMANCE"].latencies, roundTrip)
        
        -- If this was the last iteration, complete the test
        if data.testIteration == data.totalIterations then
            local sum = 0
            for _, latency in ipairs(AMSTestClient.results["PERFORMANCE"].latencies) do
                sum = sum + latency
            end
            local avg = sum / #AMSTestClient.results["PERFORMANCE"].latencies
            
            AMSTestClient:CompleteTest("PERFORMANCE", true, {
                averageLatency = avg,
                iterations = #AMSTestClient.results["PERFORMANCE"].latencies
            })
        end
    end)
    
    -- TEST_NESTED_DATA_RESPONSE
    AMS.RegisterHandler("TEST_NESTED_DATA_RESPONSE", function(data)
        local success = data.depthMatch
        AMSTestClient:CompleteTest("NESTED_DATA", success, data)
    end)
    
    Log("Response handlers registered")
end

-- ============================================================================
-- Initialization
-- ============================================================================

function AMSTestClient:Initialize()
    self:DefineTests()
    InitResponseHandlers()
    
    -- Count tests properly (hash table, not array)
    local testCount = 0
    for _ in pairs(self.tests) do
        testCount = testCount + 1
    end
    
    Log("Test Client initialized with", testCount, "tests")
end

-- Auto-initialize
AMSTestClient:Initialize()

-- ============================================================================
-- Export
-- ============================================================================

_G.AraxiaTrinityAdmin = _G.AraxiaTrinityAdmin or {}
_G.AraxiaTrinityAdmin.Tests = AMSTestClient

-- Quick access commands
print("|cFF00FF00[AMS]|r Registering test commands...")

SLASH_AMS1 = "/ams"
SlashCmdList["AMS"] = function(msg)
    local args = {}
    for word in msg:gmatch("%S+") do
        table.insert(args, word:lower())
    end
    
    local cmd = args[1] or ""
    local subcmd = args[2] or ""
    
    if cmd == "test" then
        if subcmd == "run" or subcmd == "" then
            print("|cFF00FF00[AMS]|r Starting all tests...")
            AMSTestClient:RunAll()
        elseif subcmd == "results" then
            local summary = AMSTestClient:GetSummary()
            print("|cFF00FF00[AMS Test Results]|r")
            print("  Total:", summary.total)
            print("  Passed:", summary.passed)
            print("  Failed:", summary.failed)
            print("  Timeout:", summary.timeout)
            print("  Running:", summary.running)
        else
            print("|cFF00FF00[AMS Test Commands]|r")
            print("  /ams test run - Run all tests")
            print("  /ams test results - Show test summary")
        end
    else
        print("|cFF00FF00[AMS Commands]|r")
        print("  /ams test run - Run AMS test suite")
        print("  /ams test results - View test results")
        print("  /ams info - Show AMS status")
    end
end

-- Count tests
local testCount = 0
for _ in pairs(AMSTestClient.tests) do
    testCount = testCount + 1
end

print("|cFF00FF00[AMS]|r Test suite loaded with " .. testCount .. " tests")
print("|cFF00FF00[AMS]|r Use '/ams test run' to start testing")
