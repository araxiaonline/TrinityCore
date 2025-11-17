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

/* ScriptData
Name: lua_commandscript
%Complete: 100
Comment: All lua related commands
Category: commandscripts
EndScriptData */

#include "ScriptMgr.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "Language.h"
#include "World.h"

#ifdef ELUNA
#include "LuaEngine.h"
#include "ElunaMgr.h"
#include "ElunaConfig.h"
#endif

using namespace Trinity::ChatCommands;

class lua_commandscript : public CommandScript
{
public:
    lua_commandscript() : CommandScript("lua_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable commandTable =
        {
            { "lua",                HandleLuaCommand,               rbac::RBAC_PERM_COMMAND_SERVER,  Console::Yes },
        };
        return commandTable;
    }

private:
    static bool HandleLuaCommand(ChatHandler* handler, char const* args)
    {
#ifdef ELUNA
        // Check if Eluna is enabled at runtime
        if (!sElunaConfig->IsElunaEnabled())
        {
            handler->SendSysMessage("Eluna is not enabled on this server.");
            return false;
        }

        if (!*args)
        {
            handler->SendSysMessage("Usage: .lua <code>");
            return false;
        }

        // Get the global Eluna instance directly from ElunaMgr
        // Note: We don't use ElunaInfo here because its destructor would destroy the instance
        ElunaInfoKey key = ElunaInfoKey::MakeGlobalKey(0);
        Eluna* eluna = sElunaMgr->Get(key);

        if (!eluna || !eluna->HasLuaState())
        {
            handler->SendSysMessage("Eluna is not initialized.");
            return false;
        }

        // Execute the Lua code
        lua_State* L = eluna->L;
        int err = luaL_dostring(L, args);

        if (err)
        {
            // Get error message from stack
            const char* errMsg = lua_tostring(L, -1);
            handler->SendSysMessage(errMsg ? errMsg : "Unknown error");
            lua_pop(L, 1);
            return false;
        }

        // Check if there's a return value
        if (lua_gettop(L) > 0)
        {
            if (lua_isstring(L, -1))
            {
                handler->SendSysMessage(lua_tostring(L, -1));
            }
            else if (lua_isnumber(L, -1))
            {
                handler->PSendSysMessage("%g", lua_tonumber(L, -1));
            }
            else if (lua_isboolean(L, -1))
            {
                handler->SendSysMessage(lua_toboolean(L, -1) ? "true" : "false");
            }
            else if (lua_isnil(L, -1))
            {
                handler->SendSysMessage("nil");
            }
            else
            {
                handler->SendSysMessage("Executed successfully");
            }
            lua_pop(L, 1);
        }
        else
        {
            handler->SendSysMessage("Executed successfully");
        }

        return true;
#else
        handler->SendSysMessage("Eluna is not enabled on this server.");
        return false;
#endif
    }
};

void AddSC_lua_commandscript()
{
    new lua_commandscript();
}
