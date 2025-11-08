/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "tc_catch2.h"
#include "LuaEngineTestFixture.h"

TEST_CASE("Edge Cases - Empty and Whitespace Scripts", "[LuaEngine][EdgeCases]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Empty script")
    {
        REQUIRE(fixture.ExecuteScript(eluna, ""));
    }

    SECTION("Whitespace only script")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "   \n\t  "));
    }

    SECTION("Comments only")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "-- This is a comment\n-- Another comment"));
    }
}

TEST_CASE("Edge Cases - Nil and False Value Handling", "[LuaEngine][EdgeCases]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Nil value distinction")
    {
        // In Lua, assigning nil removes the variable from the table
        REQUIRE(fixture.ExecuteScript(eluna, "x = nil"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "x") == "");  // Undefined after nil assignment
    }

    SECTION("False value distinction")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = false"));
        REQUIRE(fixture.GetGlobalAsBoolean(eluna, "x") == false);
    }

    SECTION("Zero is not false")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = 0"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 0);
    }

    SECTION("Empty string is truthy")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = '' and 'truthy' or 'falsy'"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "x") == "truthy");
    }
}

TEST_CASE("Edge Cases - Large Numbers", "[LuaEngine][EdgeCases]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Large positive integer")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = 2147483647"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 2147483647);
    }

    SECTION("Large negative integer")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = -2147483648"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == -2147483648);
    }

    SECTION("Very large number")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = 999999999999"));
    }
}

TEST_CASE("Edge Cases - Special Characters in Strings", "[LuaEngine][EdgeCases]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("String with quotes")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = \"it's a test\""));
        REQUIRE(fixture.GetGlobalAsString(eluna, "x") == "it's a test");
    }

    SECTION("String with newlines")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = 'line1\\nline2'"));
    }

    SECTION("String with backslashes")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = 'path\\\\to\\\\file'"));
    }
}

TEST_CASE("Edge Cases - Recursive Functions", "[LuaEngine][EdgeCases]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Simple recursion")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "function factorial(n) "
            "  if n <= 1 then return 1 end "
            "  return n * factorial(n - 1) "
            "end "
            "result = factorial(5)"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "result") == 120);
    }

    SECTION("Mutual recursion")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "function isEven(n) "
            "  if n == 0 then return true end "
            "  return isOdd(n - 1) "
            "end "
            "function isOdd(n) "
            "  if n == 0 then return false end "
            "  return isEven(n - 1) "
            "end "
            "result = isEven(4)"));
        REQUIRE(fixture.GetGlobalAsBoolean(eluna, "result") == true);
    }
}

TEST_CASE("Edge Cases - Circular References", "[LuaEngine][EdgeCases]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Self-referencing table")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "t = {} "
            "t.self = t"));
        // lua_objlen doesn't count self-references, so size is 0
        // This is expected Lua behavior
        REQUIRE(fixture.GetTableSize(eluna, "t") == 0);
    }

    SECTION("Mutually referencing tables")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "a = {} "
            "b = {} "
            "a.ref = b "
            "b.ref = a"));
    }
}

TEST_CASE("Edge Cases - Variable Shadowing", "[LuaEngine][EdgeCases]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Local shadowing global")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "x = 'global' "
            "function test() "
            "  local x = 'local' "
            "  return x "
            "end"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "x") == "global");
    }
}

TEST_CASE("Edge Cases - Operator Precedence", "[LuaEngine][EdgeCases]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Arithmetic precedence")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = 2 + 3 * 4"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 14);
    }

    SECTION("Logical operator precedence")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = true or false and false"));
        REQUIRE(fixture.GetGlobalAsBoolean(eluna, "x") == true);
    }

    SECTION("Comparison and logical precedence")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = 5 > 3 and 2 < 4"));
        REQUIRE(fixture.GetGlobalAsBoolean(eluna, "x") == true);
    }
}

TEST_CASE("Edge Cases - Table Iteration Edge Cases", "[LuaEngine][EdgeCases]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Iteration over empty table")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "t = {} "
            "count = 0 "
            "for k, v in pairs(t) do count = count + 1 end"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "count") == 0);
    }

    SECTION("Iteration with nil values")
    {
        // In Lua, #t stops at the first nil, so {1, nil, 3} has length 1
        REQUIRE(fixture.ExecuteScript(eluna, 
            "t = {1, nil, 3} "
            "count = 0 "
            "for i = 1, #t do if t[i] then count = count + 1 end end"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "count") == 1);  // Only counts the first element
    }
}

TEST_CASE("Edge Cases - Function Return Values", "[LuaEngine][EdgeCases]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Multiple return values")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "function multi() return 1, 2, 3 end "
            "a, b, c = multi()"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "a") == 1);
        REQUIRE(fixture.GetGlobalAsInt(eluna, "b") == 2);
        REQUIRE(fixture.GetGlobalAsInt(eluna, "c") == 3);
    }

    SECTION("No return value")
    {
        // Functions without explicit return return nil, which becomes undefined
        REQUIRE(fixture.ExecuteScript(eluna, 
            "function noReturn() x = 1 end "
            "result = noReturn()"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "result") == "");  // nil becomes undefined
    }
}

TEST_CASE("Edge Cases - Metamethods and Metatables", "[LuaEngine][EdgeCases]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Table with __tostring metamethod")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "t = {} "
            "mt = {__tostring = function() return 'custom' end} "
            "setmetatable(t, mt)"));
    }
}

TEST_CASE("Edge Cases - Upvalues and Closures", "[LuaEngine][EdgeCases]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Closure capturing variable")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "function makeCounter() "
            "  local count = 0 "
            "  return function() count = count + 1 return count end "
            "end "
            "counter = makeCounter()"));
        REQUIRE(fixture.FunctionExists(eluna, "counter"));
    }

    SECTION("Multiple closures sharing upvalue")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "local shared = 0 "
            "inc = function() shared = shared + 1 end "
            "get = function() return shared end"));
        REQUIRE(fixture.FunctionExists(eluna, "inc"));
        REQUIRE(fixture.FunctionExists(eluna, "get"));
    }
}
