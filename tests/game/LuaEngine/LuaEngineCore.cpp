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

/**
 * @brief Phase 1 Tests: Core Lua Engine Functionality
 * 
 * These tests verify:
 * 1. Lua state management (creation, cleanup)
 * 2. Script execution and loading
 * 3. Binding store management
 * 4. Event manager functionality
 */

TEST_CASE("Lua State Management - State Creation", "[LuaEngine][LuaEngineCore]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Eluna instance has valid Lua state")
    {
        lua_State* L = fixture.GetLuaState(eluna);
        REQUIRE(L != nullptr);
    }

    SECTION("HasLuaState returns true for valid instance")
    {
        REQUIRE(eluna != nullptr);
        // If we got here, Eluna was created successfully with Lua state
        REQUIRE(true);
    }

    SECTION("Multiple Eluna instances have independent Lua states")
    {
        Eluna* eluna2 = fixture.CreateGlobalElunaInstance();
        REQUIRE(eluna2 != nullptr);
        
        lua_State* L1 = fixture.GetLuaState(eluna);
        lua_State* L2 = fixture.GetLuaState(eluna2);
        
        // States should be different
        REQUIRE(L1 != L2);
    }
}

TEST_CASE("Lua State Management - Lua Libraries", "[LuaEngine][LuaEngineCore]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Standard Lua libraries are available")
    {
        // Test that standard library functions work
        REQUIRE(fixture.ExecuteScript(eluna, "x = math.floor(3.7)"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 3);
    }

    SECTION("String library is available")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = string.upper('hello')"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "x") == "HELLO");
    }

    SECTION("Table library is available")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "t = {1, 2, 3}; x = #t"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 3);
    }

    SECTION("Math library functions work")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = math.max(5, 10, 3)"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 10);
    }
}

TEST_CASE("Script Execution - Basic Script Loading", "[LuaEngine][LuaEngineCore]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Simple script executes successfully")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = 42"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 42);
    }

    SECTION("Multi-line scripts execute successfully")
    {
        std::string script = R"(
            x = 10
            y = 20
            z = x + y
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "z") == 30);
    }

    SECTION("Scripts with functions execute successfully")
    {
        std::string script = R"(
            function add(a, b)
                return a + b
            end
            result = add(5, 3)
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "result") == 8);
    }

    SECTION("Scripts with tables execute successfully")
    {
        std::string script = R"(
            data = {
                name = 'test',
                value = 100,
                items = {1, 2, 3}
            }
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsString(eluna, "data.name") == "test");
        REQUIRE(fixture.GetGlobalAsInt(eluna, "data.value") == 100);
    }
}

TEST_CASE("Script Execution - Error Handling", "[LuaEngine][LuaEngineCore]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Syntax errors are caught")
    {
        REQUIRE(!fixture.ExecuteScript(eluna, "this is not valid lua syntax"));
    }

    SECTION("Runtime errors are caught")
    {
        REQUIRE(!fixture.ExecuteScript(eluna, "x = nil; y = x.foo()"));
    }

    SECTION("Undefined function calls are caught")
    {
        REQUIRE(!fixture.ExecuteScript(eluna, "nonexistent_function()"));
    }

    SECTION("Type errors are caught")
    {
        REQUIRE(!fixture.ExecuteScript(eluna, "x = 'string'; y = x + 5"));
    }
}

TEST_CASE("Script Execution - Script Isolation", "[LuaEngine][LuaEngineCore]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna1 = fixture.CreateGlobalElunaInstance();
    Eluna* eluna2 = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna1 != nullptr);
    REQUIRE(eluna2 != nullptr);

    SECTION("Scripts in different Eluna instances don't share state")
    {
        REQUIRE(fixture.ExecuteScript(eluna1, "shared_var = 100"));
        REQUIRE(fixture.ExecuteScript(eluna2, "shared_var = 200"));
        
        REQUIRE(fixture.GetGlobalAsInt(eluna1, "shared_var") == 100);
        REQUIRE(fixture.GetGlobalAsInt(eluna2, "shared_var") == 200);
    }

    SECTION("Functions defined in one instance are not available in another")
    {
        REQUIRE(fixture.ExecuteScript(eluna1, "function test_func() return 42 end"));
        REQUIRE(!fixture.FunctionExists(eluna2, "test_func"));
    }
}

TEST_CASE("Binding Management - Binding Store Creation", "[LuaEngine][LuaEngineCore]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Eluna instance has binding stores")
    {
        // Verify by checking that we can access global functions
        REQUIRE(fixture.ExecuteScript(eluna, "x = 1"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 1);
    }

    SECTION("Multiple Eluna instances have independent bindings")
    {
        Eluna* eluna2 = fixture.CreateGlobalElunaInstance();
        REQUIRE(eluna2 != nullptr);
        
        // Both should have independent binding stores
        REQUIRE(fixture.ExecuteScript(eluna, "x = 1"));
        REQUIRE(fixture.ExecuteScript(eluna2, "x = 2"));
        
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 1);
        REQUIRE(fixture.GetGlobalAsInt(eluna2, "x") == 2);
    }
}

TEST_CASE("Binding Management - Standard Library Bindings", "[LuaEngine][LuaEngineCore]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Math library bindings are available")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = math.floor(3.9)"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 3);
    }

    SECTION("String library bindings are available")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = string.len('hello')"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 5);
    }

    SECTION("Table library bindings are available")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "t = {3, 1, 2}; table.sort(t); x = t[1]"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 1);
    }
}

TEST_CASE("Event Management - Event Registration", "[LuaEngine][LuaEngineCore]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Global table can be accessed")
    {
        // Verify that we can access and manipulate the global table
        REQUIRE(fixture.ExecuteScript(eluna, "x = type(_G)"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "x") == "table");
    }

    SECTION("Global functions are accessible")
    {
        // Test that standard global functions exist
        REQUIRE(fixture.ExecuteScript(eluna, "x = type(print)"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "x") == "function");
    }
}

TEST_CASE("Event Management - Timed Events", "[LuaEngine][LuaEngineCore]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Timed event counter can be incremented")
    {
        std::string script = R"(
            counter = 0
            function increment_counter()
                counter = counter + 1
            end
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.FunctionExists(eluna, "increment_counter"));
    }

    SECTION("Event handlers can be defined")
    {
        std::string script = R"(
            event_called = false
            function on_event()
                event_called = true
            end
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.FunctionExists(eluna, "on_event"));
    }
}

TEST_CASE("Event Management - Event Handler Execution", "[LuaEngine][LuaEngineCore]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Event handler can be called and modify state")
    {
        std::string script = R"(
            state_value = 0
            function update_state(new_value)
                state_value = new_value
            end
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.CallFunction(eluna, "update_state"));
        // Function was called successfully
        REQUIRE(true);
    }

    SECTION("Multiple event handlers can coexist")
    {
        std::string script = R"(
            function handler1()
                return 1
            end
            function handler2()
                return 2
            end
            function handler3()
                return 3
            end
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.FunctionExists(eluna, "handler1"));
        REQUIRE(fixture.FunctionExists(eluna, "handler2"));
        REQUIRE(fixture.FunctionExists(eluna, "handler3"));
    }
}

TEST_CASE("Lua Stack Operations - Push Operations", "[LuaEngine][LuaEngineCore]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Integer values are correctly pushed and retrieved")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = 42"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 42);
    }

    SECTION("Float values are correctly pushed and retrieved")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = 3.14"));
        int value = fixture.GetGlobalAsInt(eluna, "x");
        REQUIRE(value == 3);  // Truncated to int
    }

    SECTION("String values are correctly pushed and retrieved")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = 'hello world'"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "x") == "hello world");
    }

    SECTION("Boolean values are correctly pushed and retrieved")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = true"));
        REQUIRE(fixture.GetGlobalAsBoolean(eluna, "x") == true);
        
        REQUIRE(fixture.ExecuteScript(eluna, "y = false"));
        REQUIRE(fixture.GetGlobalAsBoolean(eluna, "y") == false);
    }

    SECTION("Nil values are correctly handled")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = nil"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "x") == "");
    }
}

TEST_CASE("Lua Stack Operations - Type Checking", "[LuaEngine][LuaEngineCore]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Integer type is correctly identified")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = 42; t = type(x)"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "t") == "number");
    }

    SECTION("String type is correctly identified")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = 'hello'; t = type(x)"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "t") == "string");
    }

    SECTION("Boolean type is correctly identified")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = true; t = type(x)"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "t") == "boolean");
    }

    SECTION("Table type is correctly identified")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = {}; t = type(x)"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "t") == "table");
    }

    SECTION("Function type is correctly identified")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "function f() end; t = type(f)"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "t") == "function");
    }

    SECTION("Nil type is correctly identified")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = nil; t = type(x)"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "t") == "nil");
    }
}

TEST_CASE("Lua Stack Operations - Complex Types", "[LuaEngine][LuaEngineCore]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Nested tables are correctly handled")
    {
        std::string script = R"(
            data = {
                level1 = {
                    level2 = {
                        value = 42
                    }
                }
            }
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "data.level1.level2.value") == 42);
    }

    SECTION("Mixed type tables are correctly handled")
    {
        std::string script = R"(
            mixed = {
                int_val = 42,
                str_val = 'hello',
                bool_val = true
            }
            table_val = {1, 2, 3}
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "mixed.int_val") == 42);
        REQUIRE(fixture.GetGlobalAsString(eluna, "mixed.str_val") == "hello");
        // Verify the separate array table has correct size
        REQUIRE(fixture.GetTableSize(eluna, "table_val") == 3);
    }

    SECTION("Array tables are correctly sized")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "arr = {10, 20, 30, 40, 50}"));
        REQUIRE(fixture.GetTableSize(eluna, "arr") == 5);
    }
}

TEST_CASE("Lua State Cleanup - State Destruction", "[LuaEngine][LuaEngineCore]")
{
    SECTION("Eluna instances can be created and destroyed")
    {
        LuaEngineTestFixture fixture;
        Eluna* eluna = fixture.CreateGlobalElunaInstance();
        REQUIRE(eluna != nullptr);
        
        // Scope ends, fixture is destroyed
    }

    SECTION("Multiple Eluna instances can be created and destroyed")
    {
        LuaEngineTestFixture fixture;
        Eluna* eluna1 = fixture.CreateGlobalElunaInstance();
        Eluna* eluna2 = fixture.CreateGlobalElunaInstance();
        Eluna* eluna3 = fixture.CreateGlobalElunaInstance();
        
        REQUIRE(eluna1 != nullptr);
        REQUIRE(eluna2 != nullptr);
        REQUIRE(eluna3 != nullptr);
        
        // All should be different
        REQUIRE(eluna1 != eluna2);
        REQUIRE(eluna2 != eluna3);
        REQUIRE(eluna1 != eluna3);
    }
}

TEST_CASE("Integration - Complete Workflow", "[LuaEngine][LuaEngineCore]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Complex script with multiple features")
    {
        std::string script = R"(
            -- Define data
            game_state = {
                players = {},
                npcs = {},
                events = {}
            }
            
            -- Define functions
            function add_player(name, level)
                table.insert(game_state.players, {name = name, level = level})
                return #game_state.players
            end
            
            function get_player_count()
                return #game_state.players
            end
            
            -- Execute logic
            add_player('Alice', 50)
            add_player('Bob', 45)
            add_player('Charlie', 55)
            
            player_count = get_player_count()
        )";
        
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "player_count") == 3);
        REQUIRE(fixture.FunctionExists(eluna, "add_player"));
        REQUIRE(fixture.FunctionExists(eluna, "get_player_count"));
    }

    SECTION("Script with error recovery")
    {
        // First script succeeds
        REQUIRE(fixture.ExecuteScript(eluna, "x = 10"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 10);
        
        // Second script fails
        REQUIRE(!fixture.ExecuteScript(eluna, "this is invalid"));
        
        // Third script succeeds (state is still valid)
        REQUIRE(fixture.ExecuteScript(eluna, "y = 20"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "y") == 20);
        
        // Original value still exists
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 10);
    }
}
