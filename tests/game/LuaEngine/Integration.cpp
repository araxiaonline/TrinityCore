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

TEST_CASE("Integration - Script Lifecycle", "[LuaEngine][Integration]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Load -> Execute -> Verify pattern")
    {
        // Load
        REQUIRE(fixture.ExecuteScript(eluna, "value = 42"));
        // Execute (implicit in ExecuteScript)
        // Verify
        REQUIRE(fixture.GetGlobalAsInt(eluna, "value") == 42);
    }

    SECTION("Complex script lifecycle")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "function initialize() "
            "  state = {count = 0, name = 'test'} "
            "end "
            "initialize()"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "state.count") == 0);
        REQUIRE(fixture.GetGlobalAsString(eluna, "state.name") == "test");
    }
}

TEST_CASE("Integration - Cross-Script Variable Access", "[LuaEngine][Integration]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Variables accessible across script loads")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "shared = 100"));
        REQUIRE(fixture.ExecuteScript(eluna, "result = shared + 50"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "result") == 150);
    }

    SECTION("Function calls across scripts")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "function helper(x) return x * 2 end"));
        REQUIRE(fixture.ExecuteScript(eluna, "result = helper(21)"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "result") == 42);
    }
}

TEST_CASE("Integration - Module Pattern Implementation", "[LuaEngine][Integration]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Simple module pattern")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "MyModule = {} "
            "MyModule.value = 10 "
            "function MyModule.getValue() return MyModule.value end"));
        REQUIRE(fixture.FunctionExists(eluna, "MyModule.getValue"));
    }

    SECTION("Module with private state")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "local private = 42 "
            "PublicModule = {} "
            "function PublicModule.getPrivate() return private end"));
        REQUIRE(fixture.FunctionExists(eluna, "PublicModule.getPrivate"));
    }
}

TEST_CASE("Integration - Event Manager Pattern", "[LuaEngine][Integration]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Event manager with handlers")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "EventManager = {} "
            "EventManager.handlers = {} "
            "function EventManager.register(name, handler) "
            "  EventManager.handlers[name] = handler "
            "end "
            "function EventManager.fire(name) "
            "  if EventManager.handlers[name] then "
            "    EventManager.handlers[name]() "
            "  end "
            "end"));
        REQUIRE(fixture.FunctionExists(eluna, "EventManager.register"));
        REQUIRE(fixture.FunctionExists(eluna, "EventManager.fire"));
    }
}

TEST_CASE("Integration - Error Recovery and Resilience", "[LuaEngine][Integration]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Graceful error handling")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "function safeOperation() "
            "  local ok, result = pcall(function() return 1/0 end) "
            "  if ok then return result else return 0 end "
            "end"));
        REQUIRE(fixture.FunctionExists(eluna, "safeOperation"));
    }

    SECTION("Script continues after error")
    {
        REQUIRE(!fixture.ExecuteScript(eluna, "invalid syntax here"));
        // But we can still execute new scripts
        REQUIRE(fixture.ExecuteScript(eluna, "x = 123"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 123);
    }
}

TEST_CASE("Integration - Complex Game Script Scenarios", "[LuaEngine][Integration]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Player login event simulation")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "players = {} "
            "function OnPlayerLogin(playerId, playerName) "
            "  players[playerId] = {name = playerName, level = 1} "
            "end"));
        REQUIRE(fixture.FunctionExists(eluna, "OnPlayerLogin"));
    }

    SECTION("Creature spawn event simulation")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "creatures = {} "
            "function OnCreatureSpawn(creatureId, x, y, z) "
            "  creatures[creatureId] = {x = x, y = y, z = z} "
            "end"));
        REQUIRE(fixture.FunctionExists(eluna, "OnCreatureSpawn"));
    }
}

TEST_CASE("Integration - Lua Standard Library Functions", "[LuaEngine][Integration]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Table library functions")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "t = {1, 2, 3} "
            "table.insert(t, 4) "
            "result = #t"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "result") == 4);
    }

    SECTION("String library functions")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "s = 'hello' "
            "result = string.upper(s)"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "result") == "HELLO");
    }

    SECTION("Math library functions")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "result = math.floor(3.7)"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "result") == 3);
    }
}
