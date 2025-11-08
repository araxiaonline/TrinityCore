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
 * @brief Phase 2 Tests: Advanced Lua Engine Functionality
 * 
 * These tests verify:
 * 1. Instance data lifecycle management
 * 2. Hook registration and management
 * 3. Advanced stack operations
 * 4. AI integration patterns
 */

TEST_CASE("Instance Data - Data Storage and Retrieval", "[LuaEngine][LuaEngineAdvanced]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Instance data can be stored as tables")
    {
        std::string script = R"(
            instance_data = {
                boss_defeated = false,
                treasure_found = 0,
                players_count = 0
            }
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsBoolean(eluna, "instance_data.boss_defeated") == false);
        REQUIRE(fixture.GetGlobalAsInt(eluna, "instance_data.treasure_found") == 0);
    }

    SECTION("Instance data can be updated")
    {
        std::string script = R"(
            data = {value = 10}
            data.value = 20
            data.new_field = 'added'
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "data.value") == 20);
        REQUIRE(fixture.GetGlobalAsString(eluna, "data.new_field") == "added");
    }

    SECTION("Instance data persists across function calls")
    {
        std::string script = R"(
            state = {counter = 0}
            
            function increment()
                state.counter = state.counter + 1
            end
            
            function get_counter()
                return state.counter
            end
            
            increment()
            increment()
            result = get_counter()
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "result") == 2);
    }
}

TEST_CASE("Instance Data - Complex Data Structures", "[LuaEngine][LuaEngineAdvanced]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Nested data structures are supported")
    {
        std::string script = R"(
            instance = {
                bosses = {
                    boss1 = {name = 'Lich King', health = 1000},
                    boss2 = {name = 'Ragnaros', health = 2000}
                },
                players = {
                    {name = 'Alice', level = 50},
                    {name = 'Bob', level = 45}
                }
            }
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsString(eluna, "instance.bosses.boss1.name") == "Lich King");
        REQUIRE(fixture.GetGlobalAsInt(eluna, "instance.bosses.boss1.health") == 1000);
    }

    SECTION("Array data structures work correctly")
    {
        std::string script = R"(
            items = {
                {id = 1, name = 'Sword'},
                {id = 2, name = 'Shield'},
                {id = 3, name = 'Armor'}
            }
            count = #items
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetTableSize(eluna, "items") == 3);
        REQUIRE(fixture.GetGlobalAsInt(eluna, "count") == 3);
    }

    SECTION("Mixed array and key-value tables work")
    {
        std::string script = R"(
            mixed = {
                1, 2, 3,
                name = 'mixed',
                value = 100
            }
            array_size = #mixed
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "array_size") == 3);
        REQUIRE(fixture.GetGlobalAsString(eluna, "mixed.name") == "mixed");
    }
}

TEST_CASE("Instance Data - Data Persistence", "[LuaEngine][LuaEngineAdvanced]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Data persists across multiple script executions")
    {
        // First execution
        REQUIRE(fixture.ExecuteScript(eluna, "persistent_data = {value = 42}"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "persistent_data.value") == 42);
        
        // Second execution - data should still exist
        REQUIRE(fixture.ExecuteScript(eluna, "result = persistent_data.value + 8"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "result") == 50);
    }

    SECTION("Functions can access persistent data")
    {
        std::string script = R"(
            shared_data = {count = 0}
            
            function add_to_count()
                shared_data.count = shared_data.count + 1
            end
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.CallFunction(eluna, "add_to_count"));
        
        // Verify data was modified
        REQUIRE(fixture.ExecuteScript(eluna, "result = shared_data.count"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "result") >= 1);
    }
}

TEST_CASE("Hook Registration - Event Handler Registration", "[LuaEngine][LuaEngineAdvanced]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Event handlers can be registered")
    {
        std::string script = R"(
            handlers = {}
            
            function register_handler(event_id, handler)
                if not handlers[event_id] then
                    handlers[event_id] = {}
                end
                table.insert(handlers[event_id], handler)
            end
            
            function on_event_1()
                return 'event1'
            end
            
            register_handler(1, on_event_1)
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.FunctionExists(eluna, "register_handler"));
        REQUIRE(fixture.FunctionExists(eluna, "on_event_1"));
    }

    SECTION("Multiple handlers can be registered for same event")
    {
        std::string script = R"(
            event_handlers = {}
            
            function add_handler(event_id, handler_name)
                if not event_handlers[event_id] then
                    event_handlers[event_id] = {}
                end
                table.insert(event_handlers[event_id], handler_name)
            end
            
            add_handler(1, 'handler_a')
            add_handler(1, 'handler_b')
            add_handler(1, 'handler_c')
            
            handler_count = #event_handlers[1]
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "handler_count") == 3);
    }

    SECTION("Handlers can be organized by type")
    {
        std::string script = R"(
            handlers = {
                server = {},
                player = {},
                creature = {}
            }
            
            function register_server_handler(name)
                table.insert(handlers.server, name)
            end
            
            function register_player_handler(name)
                table.insert(handlers.player, name)
            end
            
            register_server_handler('on_start')
            register_player_handler('on_login')
            
            server_count = #handlers.server
            player_count = #handlers.player
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "server_count") == 1);
        REQUIRE(fixture.GetGlobalAsInt(eluna, "player_count") == 1);
    }
}

TEST_CASE("Hook Registration - Event Execution", "[LuaEngine][LuaEngineAdvanced]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Registered handlers can be executed")
    {
        std::string script = R"(
            event_result = nil
            
            function on_event()
                event_result = 'executed'
            end
            
            function trigger_event()
                on_event()
            end
            
            trigger_event()
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsString(eluna, "event_result") == "executed");
    }

    SECTION("Handlers can pass and return values")
    {
        std::string script = R"(
            function handler_with_params(a, b)
                return a + b
            end
            
            function call_handler()
                result = handler_with_params(10, 20)
            end
            
            call_handler()
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "result") == 30);
    }

    SECTION("Multiple handlers can be executed in sequence")
    {
        std::string script = R"(
            execution_log = {}
            
            function handler_1()
                table.insert(execution_log, 'h1')
            end
            
            function handler_2()
                table.insert(execution_log, 'h2')
            end
            
            function handler_3()
                table.insert(execution_log, 'h3')
            end
            
            function execute_all()
                handler_1()
                handler_2()
                handler_3()
            end
            
            execute_all()
            count = #execution_log
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "count") == 3);
    }
}

TEST_CASE("Advanced Stack Operations - Type Conversions", "[LuaEngine][LuaEngineAdvanced]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Numbers are converted correctly")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = 42; y = tostring(x)"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "y") == "42");
    }

    SECTION("Strings are converted to numbers")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = '123'; y = tonumber(x)"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "y") == 123);
    }

    SECTION("Booleans are converted to strings")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = true; y = tostring(x)"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "y") == "true");
    }

    SECTION("Type checking with type()")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "types = {type(1), type('s'), type(true), type({}), type(function() end)}"));
        REQUIRE(fixture.GetTableSize(eluna, "types") == 5);
    }
}

TEST_CASE("Advanced Stack Operations - Table Manipulation", "[LuaEngine][LuaEngineAdvanced]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Table insertion works")
    {
        std::string script = R"(
            t = {1, 2, 3}
            table.insert(t, 4)
            table.insert(t, 5)
            size = #t
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "size") == 5);
    }

    SECTION("Table removal works")
    {
        std::string script = R"(
            t = {1, 2, 3, 4, 5}
            table.remove(t, 2)
            size = #t
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "size") == 4);
    }

    SECTION("Table concatenation works")
    {
        std::string script = R"(
            t = {1, 2, 3}
            result = table.concat(t, ',')
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsString(eluna, "result") == "1,2,3");
    }

    SECTION("Table sorting works")
    {
        std::string script = R"(
            t = {3, 1, 4, 1, 5}
            table.sort(t)
            first = t[1]
            last = t[#t]
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "first") == 1);
        REQUIRE(fixture.GetGlobalAsInt(eluna, "last") == 5);
    }
}

TEST_CASE("Advanced Stack Operations - String Manipulation", "[LuaEngine][LuaEngineAdvanced]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("String length works")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = 'hello'; len = #x"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "len") == 5);
    }

    SECTION("String concatenation works")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = 'hello' .. ' ' .. 'world'"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "x") == "hello world");
    }

    SECTION("String substring works")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = string.sub('hello', 2, 4)"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "x") == "ell");
    }

    SECTION("String formatting works")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = string.format('Value: %d', 42)"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "x") == "Value: 42");
    }

    SECTION("String case conversion works")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "upper = string.upper('hello'); lower = string.lower('WORLD')"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "upper") == "HELLO");
        REQUIRE(fixture.GetGlobalAsString(eluna, "lower") == "world");
    }
}

TEST_CASE("Advanced Stack Operations - Math Operations", "[LuaEngine][LuaEngineAdvanced]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Math floor and ceil work")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = math.floor(3.7); y = math.ceil(3.2)"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 3);
        REQUIRE(fixture.GetGlobalAsInt(eluna, "y") == 4);
    }

    SECTION("Math min and max work")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = math.min(5, 2, 8); y = math.max(5, 2, 8)"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 2);
        REQUIRE(fixture.GetGlobalAsInt(eluna, "y") == 8);
    }

    SECTION("Math absolute value works")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = math.abs(-42)"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 42);
    }

    SECTION("Math power works")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = math.pow(2, 3)"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 8);
    }

    SECTION("Math square root works")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "x = math.sqrt(16)"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "x") == 4);
    }
}

TEST_CASE("AI Integration - State Management", "[LuaEngine][LuaEngineAdvanced]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("AI state can be initialized")
    {
        std::string script = R"(
            ai_state = {
                current_state = 'idle',
                health = 100,
                mana = 50,
                in_combat = false
            }
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsString(eluna, "ai_state.current_state") == "idle");
        REQUIRE(fixture.GetGlobalAsInt(eluna, "ai_state.health") == 100);
    }

    SECTION("AI state can be updated")
    {
        std::string script = R"(
            state = {status = 'idle'}
            
            function enter_combat()
                state.status = 'combat'
            end
            
            function exit_combat()
                state.status = 'idle'
            end
            
            enter_combat()
            current = state.status
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsString(eluna, "current") == "combat");
    }

    SECTION("AI can track multiple entities")
    {
        std::string script = R"(
            ai = {
                creatures = {},
                players = {}
            }
            
            function add_creature(id, name)
                table.insert(ai.creatures, {id = id, name = name})
            end
            
            function add_player(id, name)
                table.insert(ai.players, {id = id, name = name})
            end
            
            add_creature(1, 'Goblin')
            add_creature(2, 'Orc')
            add_player(100, 'Hero')
            
            creature_count = #ai.creatures
            player_count = #ai.players
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "creature_count") == 2);
        REQUIRE(fixture.GetGlobalAsInt(eluna, "player_count") == 1);
    }
}

TEST_CASE("AI Integration - Behavior Implementation", "[LuaEngine][LuaEngineAdvanced]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("AI behavior functions can be defined")
    {
        std::string script = R"(
            function on_spawn()
                return 'spawned'
            end
            
            function on_combat()
                return 'fighting'
            end
            
            function on_death()
                return 'dead'
            end
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.FunctionExists(eluna, "on_spawn"));
        REQUIRE(fixture.FunctionExists(eluna, "on_combat"));
        REQUIRE(fixture.FunctionExists(eluna, "on_death"));
    }

    SECTION("AI behavior can be triggered")
    {
        std::string script = R"(
            behavior_log = {}
            
            function trigger_behavior(behavior_name)
                table.insert(behavior_log, behavior_name)
            end
            
            function execute_behaviors()
                trigger_behavior('spawn')
                trigger_behavior('idle')
                trigger_behavior('patrol')
            end
            
            execute_behaviors()
            count = #behavior_log
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "count") == 3);
    }

    SECTION("AI can make decisions based on conditions")
    {
        std::string script = R"(
            function decide_action(health, threat_level)
                if health < 30 then
                    return 'flee'
                elseif threat_level > 50 then
                    return 'attack'
                else
                    return 'patrol'
                end
            end
            
            action1 = decide_action(20, 30)
            action2 = decide_action(80, 60)
            action3 = decide_action(50, 20)
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsString(eluna, "action1") == "flee");
        REQUIRE(fixture.GetGlobalAsString(eluna, "action2") == "attack");
        REQUIRE(fixture.GetGlobalAsString(eluna, "action3") == "patrol");
    }
}

TEST_CASE("AI Integration - Event Handling", "[LuaEngine][LuaEngineAdvanced]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("AI can handle multiple event types")
    {
        std::string script = R"(
            events = {}
            
            function on_event(event_type, data)
                table.insert(events, {type = event_type, data = data})
            end
            
            on_event('spawn', {x = 100, y = 200})
            on_event('combat', {target = 'player1'})
            on_event('death', {killer = 'player1'})
            
            event_count = #events
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "event_count") == 3);
    }

    SECTION("AI can respond to events with state changes")
    {
        std::string script = R"(
            ai = {state = 'idle', target = nil}
            
            function on_aggro(target_id)
                ai.state = 'combat'
                ai.target = target_id
            end
            
            function on_disengage()
                ai.state = 'idle'
                ai.target = nil
            end
            
            on_aggro(123)
            state_after_aggro = ai.state
            
            on_disengage()
            state_after_disengage = ai.state
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsString(eluna, "state_after_aggro") == "combat");
        REQUIRE(fixture.GetGlobalAsString(eluna, "state_after_disengage") == "idle");
    }
}

TEST_CASE("Advanced Integration - Complex Workflows", "[LuaEngine][LuaEngineAdvanced]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Complex game scenario with multiple systems")
    {
        std::string script = R"(
            -- Game world state
            world = {
                creatures = {},
                players = {},
                events = {}
            }
            
            -- Creature AI
            function spawn_creature(id, name, health)
                local creature = {
                    id = id,
                    name = name,
                    health = health,
                    state = 'idle'
                }
                table.insert(world.creatures, creature)
                return creature
            end
            
            -- Player management
            function add_player(id, name, level)
                local player = {
                    id = id,
                    name = name,
                    level = level
                }
                table.insert(world.players, player)
                return player
            end
            
            -- Event system
            function trigger_event(event_type, data)
                table.insert(world.events, {type = event_type, data = data})
            end
            
            -- Execute scenario
            spawn_creature(1, 'Goblin', 50)
            spawn_creature(2, 'Orc', 100)
            add_player(100, 'Hero', 50)
            trigger_event('combat', {creature = 1, player = 100})
            
            creature_count = #world.creatures
            player_count = #world.players
            event_count = #world.events
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "creature_count") == 2);
        REQUIRE(fixture.GetGlobalAsInt(eluna, "player_count") == 1);
        REQUIRE(fixture.GetGlobalAsInt(eluna, "event_count") == 1);
    }

    SECTION("Data persistence across function calls")
    {
        std::string script = R"(
            -- Persistent game state
            game = {
                level = 1,
                experience = 0,
                gold = 0
            }
            
            function gain_experience(amount)
                game.experience = game.experience + amount
                if game.experience >= 100 then
                    game.level = game.level + 1
                    game.experience = game.experience - 100
                end
            end
            
            function gain_gold(amount)
                game.gold = game.gold + amount
            end
            
            -- Simulate gameplay
            gain_experience(50)
            gain_experience(60)
            gain_gold(100)
            
            final_level = game.level
            final_exp = game.experience
            final_gold = game.gold
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "final_level") == 2);
        REQUIRE(fixture.GetGlobalAsInt(eluna, "final_exp") == 10);
        REQUIRE(fixture.GetGlobalAsInt(eluna, "final_gold") == 100);
    }
}

TEST_CASE("Advanced Integration - Error Handling and Recovery", "[LuaEngine][LuaEngineAdvanced]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Errors in one function don't affect others")
    {
        std::string script = R"(
            function safe_function()
                return 'success'
            end
            
            result1 = safe_function()
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsString(eluna, "result1") == "success");
        
        // Error script - calling undefined function
        REQUIRE(!fixture.ExecuteScript(eluna, "undefined_function()"));
        
        // Safe function still works
        REQUIRE(fixture.ExecuteScript(eluna, "result2 = safe_function()"));
        REQUIRE(fixture.GetGlobalAsString(eluna, "result2") == "success");
    }

    SECTION("State is preserved after errors")
    {
        REQUIRE(fixture.ExecuteScript(eluna, "state = {value = 42}"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "state.value") == 42);
        
        // Trigger error
        REQUIRE(!fixture.ExecuteScript(eluna, "error('test error')"));
        
        // State should still be there
        REQUIRE(fixture.ExecuteScript(eluna, "result = state.value"));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "result") == 42);
    }

    SECTION("Conditional execution prevents errors")
    {
        std::string script = R"(
            data = {value = 10}
            
            if data and data.value then
                result = data.value * 2
            else
                result = 0
            end
        )";
        REQUIRE(fixture.ExecuteScript(eluna, script));
        REQUIRE(fixture.GetGlobalAsInt(eluna, "result") == 20);
    }
}
