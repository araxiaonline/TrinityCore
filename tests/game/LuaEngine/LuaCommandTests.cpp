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
#include <string>
#include <vector>

extern "C"
{
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

/**
 * @brief Mock ChatHandler for testing console commands
 * 
 * Captures all messages sent through the handler for verification
 */
class MockChatHandler
{
private:
    std::vector<std::string> messages;
    bool lastCommandResult = false;

public:
    MockChatHandler() = default;
    ~MockChatHandler() = default;

    /**
     * @brief Send a system message (mimics ChatHandler::SendSysMessage)
     */
    void SendSysMessage(const char* msg)
    {
        if (msg)
            messages.push_back(msg);
    }

    /**
     * @brief Send a formatted message (mimics ChatHandler::PSendSysMessage)
     */
    void PSendSysMessage(const char* format, ...)
    {
        char buffer[1024];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        messages.push_back(buffer);
    }

    /**
     * @brief Get all captured messages
     */
    const std::vector<std::string>& GetMessages() const { return messages; }

    /**
     * @brief Get the last message
     */
    std::string GetLastMessage() const
    {
        return messages.empty() ? "" : messages.back();
    }

    /**
     * @brief Clear all captured messages
     */
    void ClearMessages() { messages.clear(); }

    /**
     * @brief Check if a specific message was sent
     */
    bool HasMessage(const std::string& msg) const
    {
        for (const auto& m : messages)
        {
            if (m.find(msg) != std::string::npos)
                return true;
        }
        return false;
    }

    /**
     * @brief Get message count
     */
    size_t GetMessageCount() const { return messages.size(); }
};

/**
 * @brief Helper function to simulate the Lua command handler
 * 
 * This mimics the HandleLuaCommand function from cs_lua.cpp
 * Works with lua_State pointers cast as Eluna* for testing
 */
bool SimulateLuaCommand(MockChatHandler& handler, Eluna* eluna, const char* args)
{
    if (!args || !*args)
    {
        handler.SendSysMessage("Usage: .lua <code>");
        return false;
    }

    if (!eluna)
    {
        handler.SendSysMessage("Eluna is not initialized.");
        return false;
    }

    // In tests, eluna is actually a lua_State* cast to Eluna*
    lua_State* L = reinterpret_cast<lua_State*>(eluna);
    if (!L)
    {
        handler.SendSysMessage("Eluna is not initialized.");
        return false;
    }

    int err = luaL_dostring(L, args);

    if (err)
    {
        const char* errMsg = lua_tostring(L, -1);
        handler.SendSysMessage(errMsg ? errMsg : "Unknown error");
        lua_pop(L, 1);
        return false;
    }

    // Check if there's a return value
    if (lua_gettop(L) > 0)
    {
        if (lua_isstring(L, -1))
        {
            handler.SendSysMessage(lua_tostring(L, -1));
        }
        else if (lua_isnumber(L, -1))
        {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "%g", lua_tonumber(L, -1));
            handler.SendSysMessage(buffer);
        }
        else if (lua_isboolean(L, -1))
        {
            handler.SendSysMessage(lua_toboolean(L, -1) ? "true" : "false");
        }
        else if (lua_isnil(L, -1))
        {
            handler.SendSysMessage("nil");
        }
        else
        {
            handler.SendSysMessage("Executed successfully");
        }
        lua_pop(L, 1);
    }
    else
    {
        handler.SendSysMessage("Executed successfully");
    }

    return true;
}

// ============================================================================
// TEST CASES
// ============================================================================

TEST_CASE("Lua Command - Basic String Output", "[LuaEngine][LuaCommand]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Simple print statement")
    {
        MockChatHandler handler;
        // Note: print() writes to stdout, not captured by handler
        // Instead test with return value
        REQUIRE(SimulateLuaCommand(handler, eluna, "return 'Hello, Eluna!'"));
        REQUIRE(handler.HasMessage("Hello, Eluna!"));
    }

    SECTION("Return string value")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "return 'test string'"));
        REQUIRE(handler.HasMessage("test string"));
    }

    SECTION("Concatenated string")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "return 'Hello' .. ' ' .. 'World'"));
        REQUIRE(handler.HasMessage("Hello World"));
    }
}

TEST_CASE("Lua Command - Numeric Output", "[LuaEngine][LuaCommand]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Simple integer return")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "return 42"));
        REQUIRE(handler.HasMessage("42"));
    }

    SECTION("Arithmetic operation")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "return 2 + 2"));
        REQUIRE(handler.HasMessage("4"));
    }

    SECTION("Float calculation")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "return 3.14 * 2"));
        REQUIRE(handler.HasMessage("6.28"));
    }

    SECTION("Complex arithmetic")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "return (10 + 5) * 2 - 3"));
        REQUIRE(handler.HasMessage("27"));
    }
}

TEST_CASE("Lua Command - Boolean Output", "[LuaEngine][LuaCommand]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Return true")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "return true"));
        REQUIRE(handler.HasMessage("true"));
    }

    SECTION("Return false")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "return false"));
        REQUIRE(handler.HasMessage("false"));
    }

    SECTION("Boolean comparison")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "return 5 > 3"));
        REQUIRE(handler.HasMessage("true"));
    }

    SECTION("Boolean comparison false")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "return 5 < 3"));
        REQUIRE(handler.HasMessage("false"));
    }
}

TEST_CASE("Lua Command - Nil Output", "[LuaEngine][LuaCommand]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Return nil")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "return nil"));
        REQUIRE(handler.HasMessage("nil"));
    }

    SECTION("No return value")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "x = 5"));
        REQUIRE(handler.HasMessage("Executed successfully"));
    }
}

TEST_CASE("Lua Command - Variable Assignment", "[LuaEngine][LuaCommand]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Assign and retrieve integer")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "myvar = 100"));
        REQUIRE(SimulateLuaCommand(handler, eluna, "return myvar"));
        REQUIRE(handler.HasMessage("100"));
    }

    SECTION("Assign and retrieve string")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "mystring = 'stored value'"));
        REQUIRE(SimulateLuaCommand(handler, eluna, "return mystring"));
        REQUIRE(handler.HasMessage("stored value"));
    }

    SECTION("Multiple assignments")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "a = 10; b = 20; return a + b"));
        REQUIRE(handler.HasMessage("30"));
    }
}

TEST_CASE("Lua Command - Function Definition and Calling", "[LuaEngine][LuaCommand]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Define and call simple function")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "function add(a, b) return a + b end"));
        REQUIRE(SimulateLuaCommand(handler, eluna, "return add(5, 3)"));
        REQUIRE(handler.HasMessage("8"));
    }

    SECTION("Define function with string return")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "function greet(name) return 'Hello, ' .. name end"));
        REQUIRE(SimulateLuaCommand(handler, eluna, "return greet('World')"));
        REQUIRE(handler.HasMessage("Hello, World"));
    }

    SECTION("Recursive function")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "function factorial(n) if n <= 1 then return 1 else return n * factorial(n-1) end end"));
        REQUIRE(SimulateLuaCommand(handler, eluna, "return factorial(5)"));
        REQUIRE(handler.HasMessage("120"));
    }
}

TEST_CASE("Lua Command - Table Operations", "[LuaEngine][LuaCommand]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Create and access table")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "t = {1, 2, 3}; return t[1]"));
        REQUIRE(handler.HasMessage("1"));
    }

    SECTION("Table with named fields")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "t = {name='test', value=42}; return t.name"));
        REQUIRE(handler.HasMessage("test"));
    }

    SECTION("Table length")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "t = {10, 20, 30}; return #t"));
        REQUIRE(handler.HasMessage("3"));
    }
}

TEST_CASE("Lua Command - Control Flow", "[LuaEngine][LuaCommand]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("If statement true branch")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "if 5 > 3 then return 'yes' else return 'no' end"));
        REQUIRE(handler.HasMessage("yes"));
    }

    SECTION("If statement false branch")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "if 5 < 3 then return 'yes' else return 'no' end"));
        REQUIRE(handler.HasMessage("no"));
    }

    SECTION("For loop")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "sum = 0; for i=1,5 do sum = sum + i end; return sum"));
        REQUIRE(handler.HasMessage("15"));
    }

    SECTION("While loop")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "i = 0; while i < 5 do i = i + 1 end; return i"));
        REQUIRE(handler.HasMessage("5"));
    }
}

TEST_CASE("Lua Command - Error Handling", "[LuaEngine][LuaCommand]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Syntax error")
    {
        MockChatHandler handler;
        REQUIRE(!SimulateLuaCommand(handler, eluna, "return 5 +"));
        REQUIRE((handler.HasMessage("syntax error") || handler.HasMessage("unexpected")));
    }

    SECTION("Undefined variable returns nil")
    {
        MockChatHandler handler;
        // In Lua, undefined variables return nil, not an error
        REQUIRE(SimulateLuaCommand(handler, eluna, "return undefined_var"));
        REQUIRE(handler.HasMessage("nil"));
    }

    SECTION("Type mismatch in operation")
    {
        MockChatHandler handler;
        REQUIRE(!SimulateLuaCommand(handler, eluna, "return 'string' + 5"));
        REQUIRE((handler.HasMessage("attempt") || handler.HasMessage("error")));
    }
}

TEST_CASE("Lua Command - Empty and Invalid Input", "[LuaEngine][LuaCommand]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Empty command")
    {
        MockChatHandler handler;
        REQUIRE(!SimulateLuaCommand(handler, eluna, ""));
        REQUIRE(handler.HasMessage("Usage"));
    }

    SECTION("Null command")
    {
        MockChatHandler handler;
        REQUIRE(!SimulateLuaCommand(handler, eluna, nullptr));
        REQUIRE(handler.HasMessage("Usage"));
    }

    SECTION("Whitespace only")
    {
        MockChatHandler handler;
        // Whitespace-only is valid Lua (empty statement), so it should succeed
        REQUIRE(SimulateLuaCommand(handler, eluna, "   "));
        REQUIRE(handler.HasMessage("Executed successfully"));
    }
}

TEST_CASE("Lua Command - Complex Expressions", "[LuaEngine][LuaCommand]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("String manipulation")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "return string.upper('hello')"));
        REQUIRE(handler.HasMessage("HELLO"));
    }

    SECTION("Table iteration")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "t = {1,2,3}; sum = 0; for _,v in ipairs(t) do sum = sum + v end; return sum"));
        REQUIRE(handler.HasMessage("6"));
    }

    SECTION("Nested function calls")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "function f(x) return x * 2 end; function g(x) return f(x) + 1 end; return g(5)"));
        REQUIRE(handler.HasMessage("11"));
    }

    SECTION("Ternary-like operation")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "x = 10; return x > 5 and 'big' or 'small'"));
        REQUIRE(handler.HasMessage("big"));
    }
}

TEST_CASE("Lua Command - State Persistence", "[LuaEngine][LuaCommand]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Variable persists across commands")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "global_var = 'persistent'"));
        REQUIRE(SimulateLuaCommand(handler, eluna, "return global_var"));
        REQUIRE(handler.HasMessage("persistent"));
    }

    SECTION("Function persists across commands")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "function my_func() return 'stored' end"));
        REQUIRE(SimulateLuaCommand(handler, eluna, "return my_func()"));
        REQUIRE(handler.HasMessage("stored"));
    }

    SECTION("Table persists across commands")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "my_table = {a=1, b=2}"));
        REQUIRE(SimulateLuaCommand(handler, eluna, "return my_table.a + my_table.b"));
        REQUIRE(handler.HasMessage("3"));
    }
}

TEST_CASE("Lua Command - Edge Cases", "[LuaEngine][LuaCommand]")
{
    LuaEngineTestFixture fixture;
    Eluna* eluna = fixture.CreateGlobalElunaInstance();
    REQUIRE(eluna != nullptr);

    SECTION("Very large number")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "return 999999999999"));
        REQUIRE((handler.HasMessage("999999999999") || handler.HasMessage("1e+12")));
    }

    SECTION("Negative number")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "return -42"));
        REQUIRE(handler.HasMessage("-42"));
    }

    SECTION("Zero")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "return 0"));
        REQUIRE(handler.HasMessage("0"));
    }

    SECTION("Empty string")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "return ''"));
        REQUIRE(handler.GetLastMessage() == "");
    }

    SECTION("Special characters in string")
    {
        MockChatHandler handler;
        REQUIRE(SimulateLuaCommand(handler, eluna, "return 'Hello\\nWorld'"));
        REQUIRE((handler.HasMessage("Hello") || handler.HasMessage("World")));
    }
}
