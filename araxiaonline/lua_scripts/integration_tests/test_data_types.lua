-- Eluna Data Type Tests
-- Tests Lua data types and type conversions

local TestRunner = require("integration_tests/test_runner")

-- Test 1: Number type
TestRunner:Register("DataTypes: Number Type", function()
    local num = 42
    local float = 3.14
    
    return type(num) == "number" and type(float) == "number"
end)

-- Test 2: String type
TestRunner:Register("DataTypes: String Type", function()
    local str = "hello"
    local empty = ""
    
    return type(str) == "string" and type(empty) == "string"
end)

-- Test 3: Boolean type
TestRunner:Register("DataTypes: Boolean Type", function()
    local t = true
    local f = false
    
    return type(t) == "boolean" and type(f) == "boolean"
end)

-- Test 4: Table type
TestRunner:Register("DataTypes: Table Type", function()
    local tbl = {}
    local tbl2 = { a = 1, b = 2 }
    
    return type(tbl) == "table" and type(tbl2) == "table"
end)

-- Test 5: Nil type
TestRunner:Register("DataTypes: Nil Type", function()
    local x = nil
    local y
    
    return type(x) == "nil" and type(y) == "nil"
end)

-- Test 6: Function type
TestRunner:Register("DataTypes: Function Type", function()
    local func = function() end
    
    return type(func) == "function"
end)

-- Test 7: Number to string conversion
TestRunner:Register("DataTypes: Number to String", function()
    local num = 42
    local str = tostring(num)
    
    return str == "42" and type(str) == "string"
end)

-- Test 8: String to number conversion
TestRunner:Register("DataTypes: String to Number", function()
    local str = "42"
    local num = tonumber(str)
    
    return num == 42 and type(num) == "number"
end)

-- Test 9: Boolean to string conversion
TestRunner:Register("DataTypes: Boolean to String", function()
    local t = true
    local f = false
    
    return tostring(t) == "true" and tostring(f) == "false"
end)

-- Test 10: Table to string representation
TestRunner:Register("DataTypes: Table Representation", function()
    local tbl = { 1, 2, 3 }
    local str = tostring(tbl)
    
    return string.find(str, "table") ~= nil
end)

-- Test 11: Mixed type operations
TestRunner:Register("DataTypes: Mixed Type Operations", function()
    local num = 10
    local str = "5"
    local result = num + tonumber(str)
    
    return result == 15
end)

-- Test 12: Type coercion in strings
TestRunner:Register("DataTypes: String Coercion", function()
    local num = 42
    local str = "The answer is " .. num
    
    return str == "The answer is 42"
end)

-- Test 13: Nested table types
TestRunner:Register("DataTypes: Nested Tables", function()
    local data = {
        numbers = { 1, 2, 3 },
        strings = { "a", "b", "c" },
        mixed = { 1, "two", 3.0 }
    }
    
    return type(data.numbers) == "table" and 
           type(data.numbers[1]) == "number" and
           type(data.strings[1]) == "string"
end)

-- Test 14: Array vs dictionary tables
TestRunner:Register("DataTypes: Array vs Dictionary", function()
    local array = { 1, 2, 3 }
    local dict = { a = 1, b = 2, c = 3 }
    
    return #array == 3 and dict.a == 1
end)

-- Test 15: Type checking with conditionals
TestRunner:Register("DataTypes: Type Checking", function()
    local function processValue(val)
        if type(val) == "number" then
            return val * 2
        elseif type(val) == "string" then
            return val .. val
        elseif type(val) == "table" then
            return #val
        else
            return 0
        end
    end
    
    return processValue(5) == 10 and 
           processValue("x") == "xx" and 
           processValue({1,2,3}) == 3
end)

-- Test 16: Nil handling
TestRunner:Register("DataTypes: Nil Handling", function()
    local x = nil
    local y = x or "default"
    
    return y == "default"
end)

-- Test 17: Truthy/Falsy values
TestRunner:Register("DataTypes: Truthy Falsy", function()
    local t1 = true
    local t2 = 1
    local t3 = "string"
    local f1 = false
    local f2 = nil
    
    return (t1 and t2 and t3) and not (f1 or f2)
end)

-- Test 18: Type identity
TestRunner:Register("DataTypes: Type Identity", function()
    local a = 42
    local b = 42
    local c = a
    
    return a == b and c == a
end)

-- Test 19: Reference vs value
TestRunner:Register("DataTypes: Reference vs Value", function()
    local tbl1 = { x = 1 }
    local tbl2 = tbl1
    tbl2.x = 2
    
    return tbl1.x == 2  -- Tables are references
end)

-- Test 20: Immutable strings
TestRunner:Register("DataTypes: String Immutability", function()
    local str1 = "hello"
    local str2 = str1
    
    return str1 == str2 and str1 == "hello"
end)
