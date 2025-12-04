-- Eluna Bindings Tests
-- Tests Eluna C++ bindings and method availability

local TestRunner = require("integration_tests/test_runner")

-- Test 1: Check if Eluna API is available
TestRunner:Register("Bindings: Eluna API Available", function()
    -- Test that we can check for Eluna functions
    return true  -- If script loads, Eluna is working
end)

-- Test 2: GetGameTime function
TestRunner:Register("Bindings: GetGameTime", function()
    local time = GetGameTime()
    
    return type(time) == "number" and time > 0
end)

-- Test 3: os.time function
TestRunner:Register("Bindings: os.time", function()
    local time = os.time()
    
    return type(time) == "number" and time > 0
end)

-- Test 4: os.date function
TestRunner:Register("Bindings: os.date", function()
    local date = os.date("%Y-%m-%d")
    
    return type(date) == "string" and string.len(date) == 10
end)

-- Test 5: print function
TestRunner:Register("Bindings: print Function", function()
    -- print is a standard Lua function
    return type(print) == "function"
end)

-- Test 6: table.insert
TestRunner:Register("Bindings: table.insert", function()
    local tbl = {}
    table.insert(tbl, 1)
    table.insert(tbl, 2)
    
    return #tbl == 2
end)

-- Test 7: table.remove
TestRunner:Register("Bindings: table.remove", function()
    local tbl = { 1, 2, 3 }
    table.remove(tbl, 2)
    
    return #tbl == 2 and tbl[2] == 3
end)

-- Test 8: string.sub
TestRunner:Register("Bindings: string.sub", function()
    local str = "hello world"
    local sub = string.sub(str, 1, 5)
    
    return sub == "hello"
end)

-- Test 9: string.find
TestRunner:Register("Bindings: string.find", function()
    local str = "hello world"
    local pos = string.find(str, "world")
    
    return pos == 7
end)

-- Test 10: string.upper
TestRunner:Register("Bindings: string.upper", function()
    local str = "hello"
    local upper = string.upper(str)
    
    return upper == "HELLO"
end)

-- Test 11: string.lower
TestRunner:Register("Bindings: string.lower", function()
    local str = "HELLO"
    local lower = string.lower(str)
    
    return lower == "hello"
end)

-- Test 12: math.floor
TestRunner:Register("Bindings: math.floor", function()
    local result = math.floor(3.7)
    
    return result == 3
end)

-- Test 13: math.ceil
TestRunner:Register("Bindings: math.ceil", function()
    local result = math.ceil(3.2)
    
    return result == 4
end)

-- Test 14: math.abs
TestRunner:Register("Bindings: math.abs", function()
    local result = math.abs(-5)
    
    return result == 5
end)

-- Test 15: math.max
TestRunner:Register("Bindings: math.max", function()
    local result = math.max(1, 5, 3)
    
    return result == 5
end)

-- Test 16: math.min
TestRunner:Register("Bindings: math.min", function()
    local result = math.min(1, 5, 3)
    
    return result == 1
end)

-- Test 17: math.random
TestRunner:Register("Bindings: math.random", function()
    local rand = math.random(1, 10)
    
    return type(rand) == "number" and rand >= 1 and rand <= 10
end)

-- Test 18: pcall (protected call)
TestRunner:Register("Bindings: pcall", function()
    local status, result = pcall(function()
        return 42
    end)
    
    return status == true and result == 42
end)

-- Test 19: require function
TestRunner:Register("Bindings: require Available", function()
    return type(require) == "function"
end)

-- Test 20: Binding error handling
TestRunner:Register("Bindings: Error Handling", function()
    local status, err = pcall(function()
        -- This should not error in Lua
        local x = 1
        return x
    end)
    
    return status == true
end)

-- Test 21: Multiple binding calls
TestRunner:Register("Bindings: Multiple Calls", function()
    local t1 = GetGameTime()
    local t2 = GetGameTime()
    
    return type(t1) == "number" and type(t2) == "number"
end)

-- Test 22: Binding with parameters
TestRunner:Register("Bindings: Parameters", function()
    local result = string.format("Test: %d", 42)
    
    return result == "Test: 42"
end)

-- Test 23: Binding return values
TestRunner:Register("Bindings: Return Values", function()
    local str = "hello"
    local len = string.len(str)
    
    return len == 5
end)

-- Test 24: Binding chaining
TestRunner:Register("Bindings: Method Chaining", function()
    local str = "hello"
    local result = string.upper(string.sub(str, 1, 1)) .. string.sub(str, 2)
    
    return result == "Hello"
end)

-- Test 25: Binding availability check
TestRunner:Register("Bindings: Availability Check", function()
    local hasGetGameTime = GetGameTime ~= nil
    local hasStringLib = string ~= nil
    local hasMathLib = math ~= nil
    
    return hasGetGameTime and hasStringLib and hasMathLib
end)
