# Eluna Quick Start Guide

## TL;DR - Minimal Configuration

Add this to your `worldserver.conf`:

```ini
###################################################################################################
# ELUNA CONFIGURATION
###################################################################################################
Eluna.Enabled = 1
Eluna.ScriptPath = lua_scripts
Eluna.UseUnsafeMethods = 1
Eluna.UseDeprecatedMethods = 1
Eluna.ReloadCommand = 1
Eluna.ReloadSecurityLevel = 3
Eluna.ScriptReloader = 0
Eluna.TraceBack = 0
Eluna.OnlyOnMaps =
Eluna.RequirePaths =
Eluna.RequireCPaths =
```

## Setup Steps

1. **Compile with Eluna**
   ```bash
   cd /opt/github.com/araxiaonline/TrinityCore/build
   cmake .. -DELUNA=1
   make worldserver -j$(nproc)
   ```

2. **Add Config to worldserver.conf**
   - Copy the configuration above
   - Paste into your `worldserver.conf`

3. **Create Scripts Directory**
   ```bash
   mkdir -p /path/to/server/lua_scripts
   ```

4. **Restart Server**
   ```bash
   ./worldserver
   ```

5. **Test in Console**
   ```
   .lua return "Eluna works!"
   ```

## Essential Commands

| Command | Purpose |
|---------|---------|
| `.lua return 2 + 2` | Execute Lua code |
| `.lua return "test"` | Return string |
| `.reload lua` | Reload scripts |

## Configuration Quick Reference

| Setting | Value | Purpose |
|---------|-------|---------|
| `Eluna.Enabled` | 1 | Enable Eluna |
| `Eluna.ScriptPath` | lua_scripts | Script directory |
| `Eluna.UseUnsafeMethods` | 1 | Allow unsafe functions |
| `Eluna.ReloadCommand` | 1 | Enable reload command |
| `Eluna.ReloadSecurityLevel` | 3 | Admin-only reload |

## Verify Installation

```bash
# Check if Eluna is compiled in
./worldserver --version | grep -i eluna

# Test in console
.lua return "Eluna works!"
```

## Common Issues

| Issue | Solution |
|-------|----------|
| `.lua` command not found | Recompile with `-DELUNA=1`, restart server |
| Scripts not loading | Check `Eluna.ScriptPath` directory exists |
| Permission denied | Verify account level for `.reload lua` |

## File Locations

- **Config**: `worldserver.conf`
- **Scripts**: `lua_scripts/` (relative to server root)
- **Command**: `.lua <code>`

## Next Steps

1. ✅ Add configuration to worldserver.conf
2. ✅ Create lua_scripts directory
3. ✅ Add Lua scripts
4. ✅ Restart server
5. ✅ Test with `.lua` command

---

**For detailed configuration**: See `ELUNA_CONFIG_GUIDE.md`
