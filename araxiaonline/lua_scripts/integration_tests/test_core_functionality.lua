-- Eluna Core Functionality Tests
-- Tests basic Lua execution, variables, functions, and tables

local TestRunner = require("integration_tests/test_runner")

-- Test 1: Basic variable assignment and retrieval
TestRunner:Register("Core: Variable Assignment", function()
    local x = 42
    local y = "hello"
    local z = true
    
    return x == 42 and y == "hello" and z == true
end)

-- Test 2: Arithmetic operations
TestRunner:Register("Core: Arithmetic Operations", function()
    local a = 10
    local b = 5
    
    return (a + b) == 15 and (a - b) == 5 and (a * b) == 50 and (a / b) == 2
end)

-- Test 3: String operations
TestRunner:Register("Core: String Operations", function()
    local str1 = "Hello"
    local str2 = "World"
    local combined = str1 .. " " .. str2
    
    return combined == "Hello World" and string.len(combined) == 11
end)

-- Test 4: Table creation and access
TestRunner:Register("Core: Table Creation", function()
    local tbl = { a = 1, b = 2, c = 3 }
    
    return tbl.a == 1 and tbl.b == 2 and tbl.c == 3
end)

-- Test 5: Table iteration
TestRunner:Register("Core: Table Iteration", function()
    local tbl = { 10, 20, 30, 40, 50 }
    local sum = 0
    
    for _, v in ipairs(tbl) do
        sum = sum + v
    end
    
    return sum == 150
end)

-- Test 6: Function definition and calling
TestRunner:Register("Core: Function Definition", function()
    local function add(a, b)
        return a + b
    end
    
    return add(5, 3) == 8
end)

-- Test 7: Conditional statements
TestRunner:Register("Core: Conditional Statements", function()
    local x = 10
    local result = ""
    
    if x > 5 then
        result = "greater"
    elseif x == 5 then
        result = "equal"
    else
        result = "less"
    end
    
    return result == "greater"
end)

-- Test 8: Loop execution
TestRunner:Register("Core: Loop Execution", function()
    local count = 0
    
    for i = 1, 10 do
        count = count + 1
    end
    
    return count == 10
end)

-- Test 9: Boolean logic
TestRunner:Register("Core: Boolean Logic", function()
    local a = true
    local b = false
    
    return (a and not b) == true and (a or b) == true
end)

-- Test 10: Type checking
TestRunner:Register("Core: Type Checking", function()
    local num = 42
    local str = "test"
    local tbl = {}
    local bool = true
    
    return type(num) == "number" and type(str) == "string" and 
           type(tbl) == "table" and type(bool) == "boolean"
end)

-- Test 11: Nested tables
TestRunner:Register("Core: Nested Tables", function()
    local data = {
        player = {
            name = "TestPlayer",
            level = 80,
            stats = { hp = 100, mana = 50 }
        }
    }
    
    return data.player.name == "TestPlayer" and 
           data.player.stats.hp == 100
end)

-- Test 12: Table length operator
TestRunner:Register("Core: Table Length", function()
    local tbl = { 1, 2, 3, 4, 5 }
    
    return #tbl == 5
end)

-- Test 13: String formatting
TestRunner:Register("Core: String Formatting", function()
    local formatted = string.format("Value: %d, Name: %s", 42, "Test")
    
    return formatted == "Value: 42, Name: Test"
end)

-- Test 14: Math operations
TestRunner:Register("Core: Math Operations", function()
    local result = math.floor(3.7) + math.ceil(2.3) + math.abs(-5)
    
    return result == 11  -- 3 + 3 + 5
end)

-- Test 15: Local scope
TestRunner:Register("Core: Local Scope", function()
    local x = 10
    
    do
        local x = 20
        if x ~= 20 then
            return false
        end
    end
    
    return x == 10
end)
