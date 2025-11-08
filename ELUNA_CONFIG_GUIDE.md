# Eluna Configuration Guide

**Date**: November 3, 2025  
**Status**: ✅ Complete Configuration Reference

## Overview

This guide explains how to configure Eluna (Lua scripting engine) in TrinityCore's `worldserver.conf` file.

## Build Requirements

First, ensure Eluna is enabled during the build:

```bash
cd /opt/github.com/araxiaonline/TrinityCore/build
cmake .. -DELUNA=1
make worldserver -j$(nproc)
```

## Configuration Options

Add the following settings to your `worldserver.conf` file:

### 1. Enable Eluna (Required)

```ini
# Enable or disable Eluna
# Default: 1 (enabled)
Eluna.Enabled = 1
```

### 2. Script Path

```ini
# Path to Lua scripts directory (relative to server root)
# Default: lua_scripts
Eluna.ScriptPath = lua_scripts
```

**Directory Structure**:
```
server_root/
├── lua_scripts/
│   ├── script1.lua
│   ├── script2.lua
│   └── modules/
│       └── helper.lua
```

### 3. Unsafe Methods (Optional)

```ini
# Enable unsafe methods (can be dangerous)
# Default: 1 (enabled)
Eluna.UseUnsafeMethods = 1
```

### 4. Deprecated Methods (Optional)

```ini
# Enable deprecated methods (for backward compatibility)
# Default: 1 (enabled)
Eluna.UseDeprecatedMethods = 1
```

### 5. Reload Command (Optional)

```ini
# Enable .reload lua command
# Default: 1 (enabled)
Eluna.ReloadCommand = 1
```

### 6. Reload Security Level (Optional)

```ini
# Minimum account level required to use .reload lua command
# 0 = Player, 1 = Moderator, 2 = GameMaster, 3 = Administrator
# Default: 3 (Administrator only)
Eluna.ReloadSecurityLevel = 3
```

### 7. Script Reloader (Optional)

```ini
# Enable automatic script reloading on file changes
# Default: 0 (disabled)
Eluna.ScriptReloader = 0
```

### 8. Traceback (Optional)

```ini
# Enable detailed error tracebacks
# Default: 0 (disabled)
Eluna.TraceBack = 0
```

### 9. Only Load on Specific Maps (Optional)

```ini
# Comma-separated list of map IDs to load Eluna on
# Leave empty to load on all maps
# Default: "" (all maps)
Eluna.OnlyOnMaps = 0,1,530
```

### 10. Require Paths (Optional)

```ini
# Extra paths for Lua require() function
# Semicolon-separated list
# Default: ""
Eluna.RequirePaths = /path/to/lua/modules;/another/path
```

### 11. Require C Paths (Optional)

```ini
# Extra C library paths for Lua require() function
# Semicolon-separated list
# Default: ""
Eluna.RequireCPaths = /path/to/c/libs;/another/c/path
```

## Complete Configuration Example

Add this section to your `worldserver.conf`:

```ini
###################################################################################################
# ELUNA CONFIGURATION
###################################################################################################

# Enable Eluna Lua scripting engine
Eluna.Enabled = 1

# Path to Lua scripts directory
Eluna.ScriptPath = lua_scripts

# Enable unsafe methods (use with caution)
Eluna.UseUnsafeMethods = 1

# Enable deprecated methods for backward compatibility
Eluna.UseDeprecatedMethods = 1

# Enable .reload lua command
Eluna.ReloadCommand = 1

# Minimum account level for .reload lua command (0-3)
# 0 = Player, 1 = Moderator, 2 = GameMaster, 3 = Administrator
Eluna.ReloadSecurityLevel = 3

# Enable automatic script reloading on file changes
Eluna.ScriptReloader = 0

# Enable detailed error tracebacks
Eluna.TraceBack = 0

# Only load Eluna on specific maps (comma-separated, empty = all maps)
Eluna.OnlyOnMaps =

# Extra paths for Lua require() function (semicolon-separated)
Eluna.RequirePaths =

# Extra C library paths for Lua require() (semicolon-separated)
Eluna.RequireCPaths =
```

## Console Commands

Once Eluna is enabled, you can use these console commands:

### Execute Lua Code

```
.lua return 2 + 2
.lua print("Hello, World!")
.lua return "Eluna works!"
```

### Reload Lua Scripts

```
.reload lua
```

(Requires `Eluna.ReloadCommand = 1` and appropriate account level)

## Verification

To verify Eluna is working:

1. Start the server with the updated config
2. In the console, run: `.lua return "Eluna works!"`
3. You should see the output: `Eluna works!`

## Troubleshooting

### Eluna Not Loading

**Problem**: Scripts not loading or `.lua` command not available

**Solutions**:
1. Verify `Eluna.Enabled = 1` in worldserver.conf
2. Check that server was compiled with `-DELUNA=1`
3. Verify `Eluna.ScriptPath` directory exists
4. Check server logs for Eluna errors

### Scripts Not Found

**Problem**: Lua scripts in the script path are not being loaded

**Solutions**:
1. Verify script files exist in `Eluna.ScriptPath` directory
2. Check file permissions (readable by server process)
3. Verify script syntax is valid Lua
4. Check server logs for script loading errors

### Command Not Available

**Problem**: `.lua` command returns "Command does not exist"

**Solutions**:
1. Verify server was recompiled with updated code
2. Check that `Eluna.Enabled = 1`
3. Verify account has sufficient permissions
4. Restart server with new binary

## Default Values Reference

| Setting | Default | Type |
|---------|---------|------|
| Eluna.Enabled | 1 | Boolean |
| Eluna.ScriptPath | lua_scripts | String |
| Eluna.UseUnsafeMethods | 1 | Boolean |
| Eluna.UseDeprecatedMethods | 1 | Boolean |
| Eluna.ReloadCommand | 1 | Boolean |
| Eluna.ReloadSecurityLevel | 3 | Integer (0-3) |
| Eluna.ScriptReloader | 0 | Boolean |
| Eluna.TraceBack | 0 | Boolean |
| Eluna.OnlyOnMaps | "" | String (comma-separated) |
| Eluna.RequirePaths | "" | String (semicolon-separated) |
| Eluna.RequireCPaths | "" | String (semicolon-separated) |

## Security Considerations

### Unsafe Methods
- Enabling `Eluna.UseUnsafeMethods` allows Lua scripts to access potentially dangerous functions
- Only enable if you trust all Lua scripts on your server
- Default: Enabled (1)

### Reload Security Level
- Controls who can reload Lua scripts via `.reload lua` command
- Recommended: 3 (Administrator only)
- Options:
  - 0 = Any player
  - 1 = Moderators and above
  - 2 = GameMasters and above
  - 3 = Administrators only

### Script Reloader
- Automatically reloads scripts when files change
- Can cause performance issues on large servers
- Recommended: Disabled (0) for production

## Performance Tips

1. **Disable Script Reloader** in production (`Eluna.ScriptReloader = 0`)
2. **Limit Maps** if Eluna is not needed on all maps (`Eluna.OnlyOnMaps`)
3. **Disable Traceback** in production (`Eluna.TraceBack = 0`)
4. **Optimize Scripts** - Avoid heavy computations in frequently-called hooks

## Related Files

- **Configuration Source**: `/opt/github.com/araxiaonline/TrinityCore/src/server/game/LuaEngine/ElunaConfig.cpp`
- **Configuration Header**: `/opt/github.com/araxiaonline/TrinityCore/src/server/game/LuaEngine/ElunaConfig.h`
- **Command Implementation**: `/opt/github.com/araxiaonline/TrinityCore/src/server/scripts/Commands/cs_lua.cpp`

## Next Steps

1. Add Eluna configuration to `worldserver.conf`
2. Create `lua_scripts/` directory in server root
3. Add Lua scripts to the directory
4. Restart server
5. Test with `.lua` console command

## Support

For more information about Eluna:
- **Official Website**: https://elunaluaengine.github.io/
- **GitHub**: https://github.com/ElunaLuaEngine
- **Documentation**: https://elunaluaengine.github.io/docs/

## Summary

Eluna is now fully configured and ready to use. The `.lua` console command allows you to execute Lua code directly from the server console for testing and debugging purposes.
