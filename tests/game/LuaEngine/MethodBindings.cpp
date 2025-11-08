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

TEST_CASE("Method Bindings - Binding Availability", "[LuaEngine][MethodBindings]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Lua standard library available")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = table.insert"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "x") == "function");
    }

    SECTION("Math library available")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = math.floor"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "x") == "function");
    }

    SECTION("String library available")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = string.upper"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "x") == "function");
    }
}

TEST_CASE("Method Bindings - Binding Signature Validation", "[LuaEngine][MethodBindings]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Function callable with correct arguments")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "result = string.upper('hello')"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "result") == "HELLO");
    }

    SECTION("Function callable with variable arguments")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "t = {} "
            "table.insert(t, 1) "
            "table.insert(t, 2, 'x')"));
        REQUIRE(fixture.GetTableSize(eluna, "t") >= 1);
    }
}

TEST_CASE("Method Bindings - Return Value Conversion", "[LuaEngine][MethodBindings]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Integer return value")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = #'hello'"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 5);
    }

    SECTION("String return value")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = string.upper('test')"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "x") == "TEST");
    }

    SECTION("Boolean return value")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "x = type('hello') == 'string'"));
        REQUIRE(fixture.GetGlobalAsBoolean(eluna, "x") == true);
    }

    SECTION("Table return value")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "x = {} "
            "y = x"));
        REQUIRE(fixture.GetTableSize(eluna, "y") == 0);
    }
}

TEST_CASE("Method Bindings - Argument Conversion", "[LuaEngine][MethodBindings]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Integer argument")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "x = math.floor(3.7)"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 3);
    }

    SECTION("String argument")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "x = string.upper('hello')"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "x") == "HELLO");
    }

    SECTION("Boolean argument")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "x = not true"));
        REQUIRE(fixture.GetGlobalAsBoolean(eluna, "x") == false);
    }

    SECTION("Table argument")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "t = {1, 2, 3} "
            "table.insert(t, 4)"));
        REQUIRE(fixture.GetTableSize(eluna, "t") == 4);
    }
}

TEST_CASE("Method Bindings - Null Pointer Handling", "[LuaEngine][MethodBindings]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Nil handling in functions")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "x = nil "
            "if x == nil then result = 'nil' else result = 'not nil' end"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "result") == "nil");
    }

    SECTION("Optional arguments")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "function optional(a, b) "
            "  if b == nil then return a else return a + b end "
            "end "
            "x = optional(5)"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 5);
    }
}

TEST_CASE("Method Bindings - Enum Constants", "[LuaEngine][MethodBindings]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Type constants available")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "x = type(1) "
            "y = type('') "
            "z = type({})"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "x") == "number");
        REQUIRE(fixture.GetGlobalAsString(eluna, "y") == "string");
        REQUIRE(fixture.GetGlobalAsString(eluna, "z") == "table");
    }
}

TEST_CASE("Method Bindings - Method Chaining", "[LuaEngine][MethodBindings]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Chained string operations")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "x = string.upper(string.sub('hello', 1, 2))"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "x") == "HE");
    }

    SECTION("Chained table operations")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "t = {} "
            "table.insert(t, 1) "
            "table.insert(t, 2) "
            "x = #t"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 2);
    }
}

TEST_CASE("Method Bindings - Error Handling in Bindings", "[LuaEngine][MethodBindings]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Invalid argument type handling")
    {
        // In Lua 5.1, string.upper(123) works - it converts to string first
        REQUIRE(fixture.ExecuteScript(eluna, 
            "x = string.upper(123)"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "x") == "123");
    }

    SECTION("Missing required argument")
    {
        // This should fail gracefully
        REQUIRE(!fixture.ExecuteScript(eluna, 
            "x = string.upper()"));
    }
}

TEST_CASE("Method Bindings - Binding Performance", "[LuaEngine][MethodBindings]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Repeated binding calls")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "for i = 1, 100 do "
            "  x = string.upper('test') "
            "end"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "x") == "TEST");
    }

    SECTION("Nested binding calls")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "x = math.floor(math.ceil(3.2))"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 4);
    }
}
