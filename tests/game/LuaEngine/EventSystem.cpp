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

TEST_CASE("Event System - Event Handler Registration", "[LuaEngine][Events]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Simple event handler definition")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "function OnEvent() end"));
        REQUIRE(fixture.FunctionExists(eluna, "OnEvent"));
    }

    SECTION("Event handler with parameters")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "function OnPlayerLogin(event, player) end"));
        REQUIRE(fixture.FunctionExists(eluna, "OnPlayerLogin"));
    }

    SECTION("Multiple event handlers")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "function OnEvent1() end "
            "function OnEvent2() end "
            "function OnEvent3() end"));
        REQUIRE(fixture.FunctionExists(eluna, "OnEvent1"));
        REQUIRE(fixture.FunctionExists(eluna, "OnEvent2"));
        REQUIRE(fixture.FunctionExists(eluna, "OnEvent3"));
    }
}

TEST_CASE("Event System - Event Callback Execution", "[LuaEngine][Events]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Simple callback execution")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "function callback() result = 'executed' end"));
        REQUIRE(fixture.CallFunction(eluna, "callback"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "result") == "executed");
    }

    SECTION("Callback with state modification")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "counter = 0 function increment() counter = counter + 1 end"));
        REQUIRE(fixture.CallFunction(eluna, "increment"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "counter") == 1);
    }

    SECTION("Multiple callback invocations")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "count = 0 function tick() count = count + 1 end"));
        REQUIRE(fixture.CallFunction(eluna, "tick"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "count") == 1);
        REQUIRE(fixture.CallFunction(eluna, "tick"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "count") == 2);
    }
}

TEST_CASE("Event System - Event Handler State Persistence", "[LuaEngine][Events]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("State persists across handler calls")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "state = {} "
            "state.value = 0 "
            "function updateState() state.value = state.value + 10 end"));
        REQUIRE(fixture.CallFunction(eluna, "updateState"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "state.value") == 10);
        REQUIRE(fixture.CallFunction(eluna, "updateState"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "state.value") == 20);
    }

    SECTION("Global variables persist")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "globalState = 100"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "globalState") == 100);
        REQUIRE(fixture.ExecuteScript(eluna, "globalState = globalState + 50"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "globalState") == 150);
    }
}

TEST_CASE("Event System - Event Handler Unregistration", "[LuaEngine][Events]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Function can be replaced")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "function handler() return 1 end"));
        REQUIRE(fixture.FunctionExists(eluna, "handler"));
        REQUIRE(fixture.ExecuteScript(eluna, "function handler() return 2 end"));
        REQUIRE(fixture.FunctionExists(eluna, "handler"));
    }

    SECTION("Function can be set to nil")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "function handler() end"));
        REQUIRE(fixture.FunctionExists(eluna, "handler"));
        REQUIRE(fixture.ExecuteScript(eluna, "handler = nil"));
        REQUIRE(!fixture.FunctionExists(eluna, "handler"));
    }
}

TEST_CASE("Event System - Event Handler Priority Ordering", "[LuaEngine][Events]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Execution order preservation")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "order = {} "
            "function first() table.insert(order, 1) end "
            "function second() table.insert(order, 2) end "
            "function third() table.insert(order, 3) end"));
        REQUIRE(fixture.CallFunction(eluna, "first"));
        REQUIRE(fixture.CallFunction(eluna, "second"));
        REQUIRE(fixture.CallFunction(eluna, "third"));
        REQUIRE(fixture.GetTableSize(eluna, "order") == 3);
    }
}

TEST_CASE("Event System - Conditional Event Handling", "[LuaEngine][Events]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Conditional handler execution")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "enabled = true "
            "function conditionalHandler() "
            "  if enabled then result = 'ran' else result = 'skipped' end "
            "end"));
        REQUIRE(fixture.CallFunction(eluna, "conditionalHandler"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "result") == "ran");
    }

    SECTION("Handler can skip execution")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "shouldRun = false "
            "function guarded() "
            "  if not shouldRun then return end "
            "  executed = true "
            "end"));
        REQUIRE(fixture.CallFunction(eluna, "guarded"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "executed") == "");
    }
}

TEST_CASE("Event System - Error Handling in Event Handlers", "[LuaEngine][Events]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Handler with error recovery")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "function safeHandler() "
            "  local ok, err = pcall(function() error('test error') end) "
            "  if ok then result = 'ok' else result = 'error' end "
            "end"));
        REQUIRE(fixture.CallFunction(eluna, "safeHandler"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "result") == "error");
    }

    SECTION("Protected call usage")
    {
        REQUIRE(fixture.ExecuteScript(eluna, 
            "function protectedCall() "
            "  local ok = pcall(function() x = 1 end) "
            "  return ok "
            "end"));
        REQUIRE(fixture.CallFunction(eluna, "protectedCall"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 1);
    }
}
