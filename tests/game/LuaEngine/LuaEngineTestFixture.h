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

#ifndef TRINITY_LUA_ENGINE_TEST_FIXTURE_H
#define TRINITY_LUA_ENGINE_TEST_FIXTURE_H

#include "tc_catch2.h"
#include "LuaEngine.h"
#include "ElunaConfig.h"
#include "ElunaMgr.h"
#include "MockServerObjects.h"

#include <cstdio>
#include <string>
#include <vector>

extern "C"
{
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

/**
 * @brief Base fixture for Eluna/LuaEngine tests
 * 
 * Provides common setup and teardown for Lua execution tests.
 * Handles Eluna initialization, state management, and cleanup.
 */
class LuaEngineTestFixture
{
private:
    std::vector<lua_State*> luaStates;  // Track all created states for cleanup

    /**
     * @brief Helper to navigate nested tables using dot notation
     * @param L Lua state
     * @param path Path like "state.value" or "MyModule.getValue"
     * @return true if path was successfully navigated, false otherwise
     */
    bool NavigateNestedPath(lua_State* L, const std::string& path)
    {
        if (path.empty())
            return false;

        // Split path by dots
        size_t pos = 0;
        std::string first;
        size_t dotPos = path.find('.');
        
        if (dotPos == std::string::npos)
        {
            // No dots, just get global
            lua_getglobal(L, path.c_str());
            return true;
        }

        // Get first part as global
        first = path.substr(0, dotPos);
        lua_getglobal(L, first.c_str());
        
        if (lua_isnil(L, -1))
            return false;

        // Navigate through remaining parts
        pos = dotPos + 1;
        while (pos < path.length())
        {
            dotPos = path.find('.', pos);
            std::string part;
            
            if (dotPos == std::string::npos)
            {
                part = path.substr(pos);
                pos = path.length();
            }
            else
            {
                part = path.substr(pos, dotPos - pos);
                pos = dotPos + 1;
            }

            if (!lua_istable(L, -1))
                return false;

            lua_getfield(L, -1, part.c_str());
            if (lua_isnil(L, -1))
            {
                lua_pop(L, 2);  // Pop nil and table
                return false;
            }
            
            lua_remove(L, -2);  // Remove the table, keep the value
        }

        return true;
    }

public:
    LuaEngineTestFixture()
    {
        // Initialize empty - states will be created on demand
    }

    ~LuaEngineTestFixture()
    {
        // Cleanup all created Lua states
        for (lua_State* L : luaStates)
        {
            if (L)
                lua_close(L);
        }
        luaStates.clear();
    }

    /**
     * @brief Create a test Eluna instance with a global key
     * @return Pointer to created Eluna instance (actually lua_State cast as Eluna)
     * 
     * Each call creates a NEW independent Lua state, allowing tests to verify
     * state isolation between instances.
     */
    Eluna* CreateGlobalElunaInstance()
    {
        // Create a new standalone Lua state for each instance
        // This allows tests to verify state isolation
        lua_State* L = luaL_newstate();
        if (L)
        {
            // Open standard libraries
            luaL_openlibs(L);
            luaStates.push_back(L);
        }
        return reinterpret_cast<Eluna*>(L);
    }

    /**
     * @brief Check if Eluna is available for testing
     * @return true if Lua states have been created
     */
    bool IsElunaAvailable() const
    {
        return !luaStates.empty();
    }

    /**
     * @brief Execute a Lua script string and return success status
     * @param eluna Eluna instance to execute in (actually lua_State cast as Eluna)
     * @param scriptContent Lua script as string
     * @return true if script executed without errors
     */
    bool ExecuteScript(Eluna* eluna, const std::string& scriptContent)
    {
        if (!eluna)
            return false;

        lua_State* L = reinterpret_cast<lua_State*>(eluna);
        if (!L)
            return false;

        // Load and execute the script
        int result = luaL_loadstring(L, scriptContent.c_str());
        if (result != LUA_OK)
        {
            // Error loading script - pop error message from stack
            if (lua_isstring(L, -1))
                lua_pop(L, 1);
            return false;
        }

        // Execute the loaded chunk
        result = lua_pcall(L, 0, 0, 0);
        if (result != LUA_OK)
        {
            // Error executing script - pop error message from stack
            if (lua_isstring(L, -1))
                lua_pop(L, 1);
            return false;
        }
        return true;
    }

    /**
     * @brief Get a global Lua value as a string
     * @param eluna Eluna instance (actually lua_State cast as Eluna)
     * @param varName Name of global variable (supports dot notation like "state.value")
     * @return String representation of the value, or empty string if not found
     */
    std::string GetGlobalAsString(Eluna* eluna, const std::string& varName)
    {
        if (!eluna)
            return "";

        lua_State* L = reinterpret_cast<lua_State*>(eluna);
        if (!L)
            return "";

        // Check if variable exists before getting it
        bool varExists = false;
        if (varName.find('.') != std::string::npos)
        {
            // For nested paths, check if we can navigate
            varExists = NavigateNestedPath(L, varName);
        }
        else
        {
            // For simple globals, check if variable is in the global table
            lua_pushvalue(L, LUA_GLOBALSINDEX);
            lua_getfield(L, -1, varName.c_str());
            
            // Check if the field exists (even if it's nil)
            int type = lua_type(L, -1);
            varExists = (type != LUA_TNIL);
            
            // If it doesn't exist, pop both the value and the globals table
            if (!varExists)
            {
                lua_pop(L, 2);  // Pop the nil value and the globals table
                return "";  // Undefined variable returns empty string
            }
            
            // Remove the globals table, keep the value on stack
            lua_remove(L, -2);
        }

        if (!varExists)
            return "";

        std::string result;

        if (lua_isstring(L, -1))
            result = lua_tostring(L, -1);
        else if (lua_isnumber(L, -1))
        {
            // Format number as string
            char buf[64];
            snprintf(buf, sizeof(buf), "%.0f", lua_tonumber(L, -1));
            result = buf;
        }
        else if (lua_isboolean(L, -1))
            result = lua_toboolean(L, -1) ? "true" : "false";
        else if (lua_isfunction(L, -1))
            result = "function";
        else if (lua_isnil(L, -1))
            result = "nil";  // Explicitly set nil returns "nil"
        else if (lua_istable(L, -1))
            result = "table";

        lua_pop(L, 1);
        return result;
    }

    /**
     * @brief Check if a global Lua function exists
     * @param eluna Eluna instance
     * @param funcName Name of function (supports dot notation like "MyModule.getValue")
     * @return true if function exists and is callable
     */
    bool FunctionExists(Eluna* eluna, const std::string& funcName)
    {
        if (!eluna)
            return false;

        lua_State* L = reinterpret_cast<lua_State*>(eluna);
        if (!L)
            return false;

        // Support nested table access with dot notation
        if (funcName.find('.') != std::string::npos)
        {
            if (!NavigateNestedPath(L, funcName))
                return false;
        }
        else
        {
            lua_getglobal(L, funcName.c_str());
        }

        bool exists = lua_isfunction(L, -1);
        lua_pop(L, 1);
        return exists;
    }

    /**
     * @brief Call a Lua function with no arguments
     * @param eluna Eluna instance
     * @param funcName Name of function to call
     * @return true if function executed successfully
     */
    bool CallFunction(Eluna* eluna, const std::string& funcName)
    {
        if (!eluna)
            return false;

        lua_State* L = reinterpret_cast<lua_State*>(eluna);
        if (!L)
            return false;

        lua_getglobal(L, funcName.c_str());
        if (!lua_isfunction(L, -1))
        {
            lua_pop(L, 1);
            return false;
        }

        int result = lua_pcall(L, 0, 0, 0);
        return result == LUA_OK;
    }

    /**
     * @brief Get Lua state from Eluna instance (actually lua_State cast as Eluna)
     * @param eluna Eluna instance
     * @return Lua state pointer, or nullptr if not available
     */
    lua_State* GetLuaState(Eluna* eluna)
    {
        if (!eluna)
            return nullptr;
        return reinterpret_cast<lua_State*>(eluna);
    }

    /**
     * @brief Get a global Lua value as an integer
     * @param eluna Eluna instance
     * @param varName Name of global variable (supports dot notation like "state.value")
     * @return Integer value, or 0 if not found or not a number
     */
    int GetGlobalAsInt(Eluna* eluna, const std::string& varName)
    {
        if (!eluna)
            return 0;

        lua_State* L = reinterpret_cast<lua_State*>(eluna);
        if (!L)
            return 0;

        // Support nested table access with dot notation
        if (varName.find('.') != std::string::npos)
        {
            if (!NavigateNestedPath(L, varName))
                return 0;
        }
        else
        {
            lua_getglobal(L, varName.c_str());
        }

        int result = 0;
        if (lua_isnumber(L, -1))
            result = static_cast<int>(lua_tonumber(L, -1));
        lua_pop(L, 1);
        return result;
    }

    /**
     * @brief Get a global Lua value as a boolean
     * @param eluna Eluna instance
     * @param varName Name of global variable
     * @return Boolean value, or false if not found or not a boolean
     */
    bool GetGlobalAsBoolean(Eluna* eluna, const std::string& varName)
    {
        if (!eluna)
            return false;

        lua_State* L = reinterpret_cast<lua_State*>(eluna);
        if (!L)
            return false;

        lua_getglobal(L, varName.c_str());
        bool result = false;
        if (lua_isboolean(L, -1))
            result = lua_toboolean(L, -1);
        lua_pop(L, 1);
        return result;
    }

    /**
     * @brief Get table size (for arrays)
     * @param eluna Eluna instance
     * @param tableName Name of global table
     * @return Size of table, or 0 if not found or not a table
     */
    int GetTableSize(Eluna* eluna, const std::string& tableName)
    {
        if (!eluna)
            return 0;

        lua_State* L = reinterpret_cast<lua_State*>(eluna);
        if (!L)
            return 0;

        lua_getglobal(L, tableName.c_str());
        if (!lua_istable(L, -1))
        {
            lua_pop(L, 1);
            return 0;
        }

        // Use lua_objlen for Lua 5.1 compatibility
        int size = lua_objlen(L, -1);
        lua_pop(L, 1);
        return size;
    }
};

#endif // TRINITY_LUA_ENGINE_TEST_FIXTURE_H
