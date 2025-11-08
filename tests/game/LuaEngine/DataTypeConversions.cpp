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

extern "C"
{
#include "lua.h"
#include "lauxlib.h"
}

TEST_CASE("Data Type Conversions - Lua to C++ Integer", "[LuaEngine][DataTypes]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Positive integer conversion")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = 42"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 42);
    }

    SECTION("Negative integer conversion")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = -100"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == -100);
    }

    SECTION("Zero conversion")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = 0"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 0);
    }

    SECTION("Large integer conversion")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = 999999"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 999999);
    }
}

TEST_CASE("Data Type Conversions - Lua to C++ String", "[LuaEngine][DataTypes]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Simple string conversion")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "s = 'hello'"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "s") == "hello");
    }

    SECTION("String with spaces")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "s = 'hello world'"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "s") == "hello world");
    }

    SECTION("Empty string")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "s = ''"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "s") == "");
    }

    SECTION("String with numbers")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "s = 'test123'"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "s") == "test123");
    }
}

TEST_CASE("Data Type Conversions - Lua to C++ Boolean", "[LuaEngine][DataTypes]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("True conversion")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "b = true"));
        REQUIRE(fixture.GetGlobalAsBoolean(eluna, "b") == true);
    }

    SECTION("False conversion")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "b = false"));
        REQUIRE(fixture.GetGlobalAsBoolean(eluna, "b") == false);
    }

    SECTION("Nil as false")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "b = nil"));
        REQUIRE(fixture.GetGlobalAsBoolean(eluna, "b") == false);
    }
}

TEST_CASE("Data Type Conversions - Lua to C++ Nil", "[LuaEngine][DataTypes]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Nil value detection")
    {
        // In Lua, assigning nil to a variable removes it from the table
        // So x = nil actually makes x undefined
        REQUIRE(fixture.ExecuteScript(eluna, "x = nil"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "x") == "");  // Returns empty string (undefined)
    }

    SECTION("Unset variable is nil")
    {
        REQUIRE(fixture.GetGlobalAsString(eluna, "undefined_var") == "");
    }
}

TEST_CASE("Data Type Conversions - Type Coercion", "[LuaEngine][DataTypes]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Number to string coercion")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = 42 .. ''"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "x") == "42");
    }

    SECTION("String to number coercion in arithmetic")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = '10' + 5"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 15);
    }

    SECTION("Boolean truthiness")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = true and 1 or 0"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 1);
    }
}

TEST_CASE("Data Type Conversions - Table Array Handling", "[LuaEngine][DataTypes]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Simple array size")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "arr = {1, 2, 3}"));
        REQUIRE(fixture.GetTableSize(eluna, "arr") == 3);
    }

    SECTION("Empty table size")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "t = {}"));
        REQUIRE(fixture.GetTableSize(eluna, "t") == 0);
    }

    SECTION("Large array size")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "arr = {}; for i=1,100 do arr[i] = i end"));
        REQUIRE(fixture.GetTableSize(eluna, "arr") == 100);
    }
}

TEST_CASE("Data Type Conversions - Nested Tables", "[LuaEngine][DataTypes]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Nested table creation")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "nested = { {1, 2}, {3, 4} }"));
        REQUIRE(fixture.GetTableSize(eluna, "nested") == 2);
    }

    SECTION("Table with mixed types")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "mixed = {1, 'text', true, nil, {}}"));
        REQUIRE(fixture.GetTableSize(eluna, "mixed") == 5);
    }
}

TEST_CASE("Data Type Conversions - Type Checking", "[LuaEngine][DataTypes]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Type function for numbers")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = type(42)"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "x") == "number");
    }

    SECTION("Type function for strings")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = type('hello')"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "x") == "string");
    }

    SECTION("Type function for tables")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = type({})"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "x") == "table");
    }

    SECTION("Type function for booleans")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = type(true)"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "x") == "boolean");
    }
}

TEST_CASE("Data Type Conversions - Float Precision", "[LuaEngine][DataTypes]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Float to int truncation")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = 3.7"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 3);
    }

    SECTION("Small float to int")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = 0.5"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 0);
    }
}
