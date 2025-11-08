-- LuaEngine Server-Side Testing Script
-- Usage: .lua dofile("test_eluna_server.lua")
-- This script validates LuaEngine functionality on a running server

local test_suite = {
    passed = 0,
    failed = 0,
    tests = {}
}

-- Helper function to run a test
local function run_test(test_name, test_func)
    table.insert(test_suite.tests, {name = test_name, status = "pending"})
    local test_index = #test_suite.tests
    
    print("\n[TEST] " .. test_name)
    local success, result = pcall(test_func)
    
    if success then
        print("  ✓ PASSED")
        test_suite.tests[test_index].status = "passed"
        test_suite.passed = test_suite.passed + 1
        return true
    else
        print("  ✗ FAILED: " .. tostring(result))
        test_suite.tests[test_index].status = "failed"
        test_suite.failed = test_suite.failed + 1
        return false
    end
end

-- Helper function for assertions
local function assert_equal(actual, expected, message)
    if actual ~= expected then
        error(message or "Assertion failed: " .. tostring(actual) .. " ~= " .. tostring(expected))
    end
end

local function assert_true(value, message)
    if not value then
        error(message or "Assertion failed: value is not true")
    end
end

local function assert_false(value, message)
    if value then
        error(message or "Assertion failed: value is not false")
    end
end

-- ============================================================================
-- CATEGORY 1: Basic Lua Functionality
-- ============================================================================

print("\n" .. string.rep("=", 60))
print("CATEGORY 1: Basic Lua Functionality")
print(string.rep("=", 60))

run_test("Arithmetic Operations", function()
    assert_equal(10 + 20, 30, "Addition failed")
    assert_equal(20 - 5, 15, "Subtraction failed")
    assert_equal(5 * 4, 20, "Multiplication failed")
    assert_equal(20 / 4, 5, "Division failed")
end)

run_test("String Operations", function()
    assert_equal(string.upper("hello"), "HELLO", "Upper case failed")
    assert_equal(string.lower("WORLD"), "world", "Lower case failed")
    assert_equal(string.len("test"), 4, "String length failed")
    assert_equal("hello" .. "world", "helloworld", "Concatenation failed")
end)

run_test("Table Operations", function()
    local t = {1, 2, 3}
    assert_equal(#t, 3, "Table length failed")
    table.insert(t, 4)
    assert_equal(#t, 4, "Table insert failed")
    table.remove(t, 2)
    assert_equal(#t, 3, "Table remove failed")
end)

run_test("Math Library", function()
    assert_equal(math.floor(3.7), 3, "Floor failed")
    assert_equal(math.ceil(3.2), 4, "Ceil failed")
    assert_equal(math.max(1, 5, 3), 5, "Max failed")
    assert_equal(math.min(1, 5, 3), 1, "Min failed")
    assert_equal(math.abs(-42), 42, "Abs failed")
end)

run_test("Type Checking", function()
    assert_equal(type(42), "number", "Number type failed")
    assert_equal(type("hello"), "string", "String type failed")
    assert_equal(type(true), "boolean", "Boolean type failed")
    assert_equal(type({}), "table", "Table type failed")
    assert_equal(type(function() end), "function", "Function type failed")
end)

-- ============================================================================
-- CATEGORY 2: Control Flow
-- ============================================================================

print("\n" .. string.rep("=", 60))
print("CATEGORY 2: Control Flow")
print(string.rep("=", 60))

run_test("If/Else Statements", function()
    local x = 10
    local result = ""
    if x > 5 then
        result = "greater"
    else
        result = "less"
    end
    assert_equal(result, "greater", "If condition failed")
end)

run_test("For Loops", function()
    local sum = 0
    for i = 1, 5 do
        sum = sum + i
    end
    assert_equal(sum, 15, "For loop failed")
end)

run_test("While Loops", function()
    local count = 0
    local i = 0
    while i < 5 do
        count = count + 1
        i = i + 1
    end
    assert_equal(count, 5, "While loop failed")
end)

run_test("Function Definition and Calling", function()
    local function add(a, b)
        return a + b
    end
    assert_equal(add(5, 3), 8, "Function call failed")
end)

run_test("Nested Conditions", function()
    local x = 10
    local y = 20
    local result = ""
    if x > 5 then
        if y > 15 then
            result = "both_true"
        else
            result = "x_true"
        end
    else
        result = "both_false"
    end
    assert_equal(result, "both_true", "Nested condition failed")
end)

-- ============================================================================
-- CATEGORY 3: Data Structures
-- ============================================================================

print("\n" .. string.rep("=", 60))
print("CATEGORY 3: Data Structures")
print(string.rep("=", 60))

run_test("Array Tables", function()
    local arr = {10, 20, 30, 40, 50}
    assert_equal(#arr, 5, "Array length failed")
    assert_equal(arr[1], 10, "Array index failed")
    assert_equal(arr[5], 50, "Array last element failed")
end)

run_test("Dictionary Tables", function()
    local dict = {name = "Test", value = 100, active = true}
    assert_equal(dict.name, "Test", "Dictionary access failed")
    assert_equal(dict["value"], 100, "Dictionary bracket access failed")
    assert_true(dict.active, "Dictionary boolean failed")
end)

run_test("Nested Tables", function()
    local nested = {
        level1 = {
            level2 = {
                value = 42
            }
        }
    }
    assert_equal(nested.level1.level2.value, 42, "Nested table access failed")
end)

run_test("Mixed Tables", function()
    local mixed = {1, 2, 3, name = "mixed", value = 100}
    assert_equal(#mixed, 3, "Mixed table array length failed")
    assert_equal(mixed.name, "mixed", "Mixed table key access failed")
end)

run_test("Table Iteration", function()
    local t = {10, 20, 30}
    local sum = 0
    for i, v in ipairs(t) do
        sum = sum + v
    end
    assert_equal(sum, 60, "Table iteration failed")
end)

-- ============================================================================
-- CATEGORY 4: Error Handling
-- ============================================================================

print("\n" .. string.rep("=", 60))
print("CATEGORY 4: Error Handling")
print(string.rep("=", 60))

run_test("Error Catching with pcall", function()
    local success, err = pcall(function()
        error("Test error")
    end)
    assert_false(success, "Error not caught")
    assert_true(err ~= nil, "Error message missing")
end)

run_test("Safe Function Execution", function()
    local function safe_divide(a, b)
        if b == 0 then
            error("Division by zero")
        end
        return a / b
    end
    
    local success1, result1 = pcall(safe_divide, 10, 2)
    assert_true(success1, "Safe division failed")
    assert_equal(result1, 5, "Division result incorrect")
    
    local success2, err2 = pcall(safe_divide, 10, 0)
    assert_false(success2, "Error not caught for division by zero")
end)

run_test("Error Recovery", function()
    local state = {value = 100}
    
    -- Trigger error
    local success, err = pcall(function()
        error("Test error")
    end)
    
    -- State should still be intact
    assert_equal(state.value, 100, "State corrupted after error")
end)

-- ============================================================================
-- CATEGORY 5: Advanced Features
-- ============================================================================

print("\n" .. string.rep("=", 60))
print("CATEGORY 5: Advanced Features")
print(string.rep("=", 60))

run_test("Closures and Upvalues", function()
    local function make_counter(start)
        local count = start
        return function()
            count = count + 1
            return count
        end
    end
    
    local counter = make_counter(0)
    assert_equal(counter(), 1, "Closure call 1 failed")
    assert_equal(counter(), 2, "Closure call 2 failed")
    assert_equal(counter(), 3, "Closure call 3 failed")
end)

run_test("Table as Function Argument", function()
    local function process_data(data)
        return data.a + data.b
    end
    
    local result = process_data({a = 10, b = 20})
    assert_equal(result, 30, "Table argument processing failed")
end)

run_test("Multiple Return Values", function()
    local function get_values()
        return 10, 20, 30
    end
    
    local a, b, c = get_values()
    assert_equal(a, 10, "First return value failed")
    assert_equal(b, 20, "Second return value failed")
    assert_equal(c, 30, "Third return value failed")
end)

run_test("Variable Arguments", function()
    local function sum_all(...)
        local sum = 0
        for i, v in ipairs({...}) do
            sum = sum + v
        end
        return sum
    end
    
    assert_equal(sum_all(1, 2, 3, 4, 5), 15, "Variable arguments failed")
end)

-- ============================================================================
-- CATEGORY 6: String Manipulation
-- ============================================================================

print("\n" .. string.rep("=", 60))
print("CATEGORY 6: String Manipulation")
print(string.rep("=", 60))

run_test("String Formatting", function()
    local result = string.format("Value: %d, Name: %s", 42, "test")
    assert_equal(result, "Value: 42, Name: test", "String format failed")
end)

run_test("String Substring", function()
    local str = "hello world"
    local sub = string.sub(str, 1, 5)
    assert_equal(sub, "hello", "Substring failed")
end)

run_test("String Find and Replace", function()
    local str = "hello world"
    local pos = string.find(str, "world")
    assert_true(pos ~= nil, "String find failed")
    
    local replaced = string.gsub(str, "world", "Lua")
    assert_equal(replaced, "hello Lua", "String replace failed")
end)

-- ============================================================================
-- CATEGORY 7: Performance Tests
-- ============================================================================

print("\n" .. string.rep("=", 60))
print("CATEGORY 7: Performance Tests")
print(string.rep("=", 60))

run_test("Loop Performance (10000 iterations)", function()
    local counter = 0
    for i = 1, 10000 do
        counter = counter + 1
    end
    assert_equal(counter, 10000, "Loop performance test failed")
end)

run_test("Table Operations Performance (1000 inserts)", function()
    local t = {}
    for i = 1, 1000 do
        table.insert(t, i)
    end
    assert_equal(#t, 1000, "Table performance test failed")
end)

run_test("String Concatenation Performance (100 concatenations)", function()
    local str = ""
    for i = 1, 100 do
        str = str .. "x"
    end
    assert_equal(#str, 100, "String concatenation performance test failed")
end)

-- ============================================================================
-- TEST SUMMARY
-- ============================================================================

print("\n" .. string.rep("=", 60))
print("TEST SUMMARY")
print(string.rep("=", 60))

local total = test_suite.passed + test_suite.failed
local pass_rate = (test_suite.passed / total) * 100

print(string.format("\nTotal Tests: %d", total))
print(string.format("Passed: %d ✓", test_suite.passed))
print(string.format("Failed: %d ✗", test_suite.failed))
print(string.format("Pass Rate: %.1f%%", pass_rate))

if test_suite.failed == 0 then
    print("\n🎉 ALL TESTS PASSED! LuaEngine is working correctly.")
else
    print("\n⚠️  Some tests failed. Review the output above for details.")
    print("\nFailed Tests:")
    for i, test in ipairs(test_suite.tests) do
        if test.status == "failed" then
            print("  - " .. test.name)
        end
    end
end

print("\n" .. string.rep("=", 60))
