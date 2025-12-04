# Eluna Reload Command

## Overview

Added `.lua reload` command to reload Eluna Lua scripts without restarting the server, similar to AzerothCore's mod-eluna behavior.

## Commands

### `.lua <code>`
Execute Lua code directly (existing functionality)
```
.lua print("Hello from Lua!")
.lua return GetPlayersInWorld()
```

### `.lua reload` ✨ NEW!
Reload all Eluna Lua scripts without server restart
```
.lua reload
```

**Output:**
```
Reloading Eluna scripts...
Eluna scripts reloaded successfully!
```

## Configuration

In `worldserver.conf`:

```conf
# Enable .lua reload command
Eluna.ReloadCommand = 1

# Minimum account level for .lua reload command (0-3)
# 0 = Player, 1 = Moderator, 2 = GameMaster, 3 = Administrator
Eluna.ReloadSecurityLevel = 3
```

## Security

- **Requires RBAC permission:** `RBAC_PERM_COMMAND_RELOAD`
- **Configurable security level:** Default = 3 (Administrator only)
- **Config check:** Command can be disabled via `Eluna.ReloadCommand = 0`

## What Gets Reloaded

The command calls `sElunaLoader->ReloadElunaForMap(RELOAD_GLOBAL_STATE)` which:

1. ✅ Reloads all Lua scripts from `lua_scripts/` directory
2. ✅ Re-executes `init.lua` and all loaded scripts
3. ✅ Refreshes all registered event handlers
4. ✅ Reloads AMS (Araxia Messaging System) handlers
5. ✅ Applies any code changes made to Lua files

**Note:** This reloads the **global Eluna state** only. Per-map states remain unchanged.

## Usage Workflow

### Development Workflow (Fast Iteration)

1. **Edit your Lua script** in `lua_scripts/`
   - Modify `ams_test_handlers.lua`
   - Add new handlers
   - Fix bugs

2. **In-game, type:** `.lua reload`

3. **Test immediately** - Changes are live!

No server restart needed! 🎉

### Example: Adding a New AMS Handler

**Before:**
```lua
-- ams_test_handlers.lua (missing handler)
```

**Edit the file:**
```lua
-- ams_test_handlers.lua
AMS.RegisterHandler("MY_NEW_HANDLER", function(player, data)
    print("New handler called!")
    AMS.Send(player, "MY_RESPONSE", {success = true})
end)
```

**Reload:**
```
.lua reload
```

**Test:**
Client sends message → Handler fires immediately!

## Comparison: AzerothCore vs TrinityCore

| Feature | AzerothCore (mod-eluna) | TrinityCore (Araxia) |
|---------|-------------------------|----------------------|
| Command | `.reload eluna` | `.lua reload` |
| Config | mod-eluna config | `Eluna.ReloadCommand` |
| Scope | Global + Maps | Global state |
| Security | GM Level | RBAC + Config |

**Why `.lua reload` instead of `.reload eluna`?**
- Follows TrinityCore's command structure
- Groups all Lua commands under `.lua` namespace
- Maintains consistency with `.lua <code>` execution

## Implementation Details

**File:** `src/server/scripts/Commands/cs_lua.cpp`

**Added:**
1. `HandleLuaReloadCommand()` function
2. Command registration in command table
3. Security checks (config, RBAC, account level)
4. Integration with `ElunaLoader::ReloadElunaForMap()`

**Headers:**
- `ElunaLoader.h` - For reload functionality
- `ElunaConfig.h` - For config checks

## Troubleshooting

### "Eluna is not enabled on this server"
**Fix:** Set `Eluna.Enabled = 1` in worldserver.conf

### "Eluna reload command is disabled in configuration"
**Fix:** Set `Eluna.ReloadCommand = 1` in worldserver.conf

### "You don't have permission to reload Eluna scripts"
**Fix:** 
- Check `Eluna.ReloadSecurityLevel` (default 3 = Admin only)
- Ensure your account has sufficient security level
- Console always has permission

### Scripts don't reload / errors persist
**Check:**
1. Lua syntax errors in your scripts
2. `@logs` Eluna.log for error messages
3. Ensure you're editing the correct file path
4. Try `/reload` in-game to refresh client-side if needed

## Benefits

✅ **Fast Development Iteration**
- No server restart for Lua changes
- Test changes in seconds, not minutes

✅ **Production Hot-Fixes**
- Fix bugs without downtime
- Update handlers on the fly

✅ **Safe & Secure**
- Configurable security levels
- RBAC integration
- Can be disabled in config

✅ **AzerothCore Parity**
- Same workflow as mod-eluna users expect
- Easier migration for developers

## Next Steps

After rebuilding the server with this change:

1. **Rebuild:** `cmake --build . -j$(nproc)` in Docker
2. **Restart worldserver** (one time only)
3. **Test:** `.lua reload` in-game
4. **Develop:** Edit Lua scripts → reload → test → iterate!

---

**Now you can iterate on AMS handlers and Lua scripts without server restarts!** 🚀
