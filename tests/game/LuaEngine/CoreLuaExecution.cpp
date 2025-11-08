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

TEST_CASE("Core Lua Execution - Simple Variable Assignment", "[LuaEngine][CoreExecution]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Integer assignment")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = 42"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 42);
    }

    SECTION("String assignment")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "name = 'hello'"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "name") == "hello");
    }

    SECTION("Boolean assignment")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "flag = true"));
        REQUIRE(fixture.GetGlobalAsBoolean(eluna, "flag") == true);
    }

    SECTION("Float assignment")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "pi = 3.14"));
        int value = fixture.GetGlobalAsInt(eluna, "pi");
        REQUIRE(value == 3);
    }
}

TEST_CASE("Core Lua Execution - Function Definition and Calling", "[LuaEngine][CoreExecution]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Simple function definition")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "function greet() return 'hello' end"));
        REQUIRE(fixture.FunctionExists(eluna, "greet"));
    }

    SECTION("Function with side effects")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "function setGlobal() result = 123 end"));
        REQUIRE(fixture.CallFunction(eluna, "setGlobal"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "result") == 123);
    }

    SECTION("Function not found")
    {
        REQUIRE(!fixture.FunctionExists(eluna, "nonexistent"));
    }
}

TEST_CASE("Core Lua Execution - Table Operations", "[LuaEngine][CoreExecution]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Table creation")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "t = {}"));
        REQUIRE(fixture.GetTableSize(eluna, "t") == 0);
    }

    SECTION("Array table")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "arr = {1, 2, 3, 4, 5}"));
        REQUIRE(fixture.GetTableSize(eluna, "arr") == 5);
    }

    SECTION("Table with mixed content")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "mixed = {10, 'text', true}"));
        REQUIRE(fixture.GetTableSize(eluna, "mixed") == 3);
    }
}

TEST_CASE("Core Lua Execution - Arithmetic Operations", "[LuaEngine][CoreExecution]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Addition")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "result = 10 + 5"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "result") == 15);
    }

    SECTION("Subtraction")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "result = 20 - 8"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "result") == 12);
    }

    SECTION("Multiplication")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "result = 6 * 7"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "result") == 42);
    }

    SECTION("Division")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "result = 100 / 5"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "result") == 20);
    }
}

TEST_CASE("Core Lua Execution - String Operations", "[LuaEngine][CoreExecution]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("String concatenation")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "result = 'hello' .. ' ' .. 'world'"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "result") == "hello world");
    }

    SECTION("String length")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "result = #'test'"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "result") == 4);
    }
}

TEST_CASE("Core Lua Execution - Conditional Logic", "[LuaEngine][CoreExecution]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("If statement - true branch")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "if true then x = 1 else x = 2 end"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 1);
    }

    SECTION("If statement - false branch")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "if false then x = 1 else x = 2 end"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 2);
    }

    SECTION("Comparison operators")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "if 5 > 3 then result = true else result = false end"));
        REQUIRE(fixture.GetGlobalAsBoolean(eluna, "result") == true);
    }
}

TEST_CASE("Core Lua Execution - Loops", "[LuaEngine][CoreExecution]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("For loop")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "sum = 0 for i = 1, 5 do sum = sum + i end"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "sum") == 15);
    }

    SECTION("While loop")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "count = 0 while count < 5 do count = count + 1 end"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "count") == 5);
    }
}

TEST_CASE("Core Lua Execution - Error Handling", "[LuaEngine][CoreExecution]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Syntax error detection")
    {
        REQUIRE(!fixture.ExecuteScript(eluna, "this is not valid lua"));
    }

    SECTION("Runtime error handling")
    {
        // This should fail gracefully
        REQUIRE(!fixture.ExecuteScript(eluna, "x = nil x.foo()"));
    }
}

TEST_CASE("Core Lua Execution - Lua State Isolation", "[LuaEngine][CoreExecution]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna1 = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna1 != nullptr);

    SECTION("Separate Eluna instances have separate state")
    {
        REQUIRE(fixture.ExecuteScript(eluna1, "globalVar = 100"));
        REQUIRE(fixture.GetGlobalAsInt(eluna1, "globalVar") == 100);

        // Create another instance
        Eluna* eluna2 = fixture.CreateGlobalElunaInstance();
        REQUIRE(eluna2 != nullptr);

        // The new instance should not have the variable from the first
        REQUIRE(fixture.GetGlobalAsInt(eluna2, "globalVar") == 0);
    }
}
