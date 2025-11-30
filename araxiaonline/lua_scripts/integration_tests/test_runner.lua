-- Eluna Integration Test Runner
-- This script runs at server startup and executes all integration tests

local TestRunner = {}
TestRunner.tests = {}
TestRunner.passed = 0
TestRunner.failed = 0
TestRunner.results = {}

-- Register a test
function TestRunner:Register(name, testFunc)
    table.insert(self.tests, { name = name, func = testFunc })
end

-- Run all tests
function TestRunner:RunAll()
    print("\n" .. string.rep("=", 80))
    print("ELUNA INTEGRATION TEST SUITE")
    print(string.rep("=", 80))
    print("Starting tests at " .. os.date("%Y-%m-%d %H:%M:%S"))
    print(string.rep("=", 80) .. "\n")
    
    for i, test in ipairs(self.tests) do
        self:RunTest(test.name, test.func, i)
    end
    
    self:PrintSummary()
end

-- Run a single test
function TestRunner:RunTest(name, testFunc, index)
    local status, result = pcall(testFunc)
    
    if status then
        if result then
            self.passed = self.passed + 1
            print(string.format("[%d/%d] ✓ PASS: %s", index, #self.tests, name))
            table.insert(self.results, { name = name, status = "PASS", message = "" })
        else
            self.failed = self.failed + 1
            print(string.format("[%d/%d] ✗ FAIL: %s", index, #self.tests, name))
            table.insert(self.results, { name = name, status = "FAIL", message = "Test returned false" })
        end
    else
        self.failed = self.failed + 1
        print(string.format("[%d/%d] ✗ ERROR: %s - %s", index, #self.tests, name, result))
        table.insert(self.results, { name = name, status = "ERROR", message = result })
    end
end

-- Print test summary
function TestRunner:PrintSummary()
    print("\n" .. string.rep("=", 80))
    print("TEST SUMMARY")
    print(string.rep("=", 80))
    print(string.format("Total Tests: %d", #self.tests))
    print(string.format("Passed: %d", self.passed))
    print(string.format("Failed: %d", self.failed))
    print(string.format("Success Rate: %.1f%%", (self.passed / #self.tests) * 100))
    print(string.rep("=", 80) .. "\n")
    
    if self.failed > 0 then
        print("FAILED TESTS:")
        for _, result in ipairs(self.results) do
            if result.status ~= "PASS" then
                print(string.format("  - %s: %s", result.name, result.message))
            end
        end
        print()
    end
end

-- Load and run all test modules
function TestRunner:LoadTests()
    -- Load individual test modules
    local testModules = {
        "integration_tests/test_core_functionality",
        "integration_tests/test_events",
        "integration_tests/test_data_types",
        "integration_tests/test_bindings",
    }
    
    for _, module in ipairs(testModules) do
        local status, result = pcall(require, module)
        if not status then
            print(string.format("Warning: Failed to load test module %s: %s", module, result))
        end
    end
end

return TestRunner
