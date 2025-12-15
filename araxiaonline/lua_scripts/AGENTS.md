# Eluna Lua Scripts - Agent Guidelines

## ⚠️ IMPORTANT: Commit Workflow

**This directory (`/opt/trinitycore/server/lua_scripts/`) is NOT a git repo.**

To commit changes to lua_scripts:
1. Copy modified files to: `/opt/trinitycore/TrinityCore/araxiaonline/lua_scripts/`
2. Commit from the TrinityCore repo

```bash
# Example sync command
cp -r /opt/trinitycore/server/lua_scripts/* /opt/trinitycore/TrinityCore/araxiaonline/lua_scripts/
```

**AI assistants: Do this automatically when committing session work.**

---

## Overview

This directory contains Eluna Lua scripts for TrinityCore. These scripts handle boss encounters, instance mechanics, and custom functionality.

## Directory Structure

- `instances/` - Instance scripts organized by expansion/dungeon
- `AMS_Server/` - Araxia Message System server-side scripts
- `integration_tests/` - Test scripts for validating Eluna functionality

---

## Code Commenting Guidelines

**ALWAYS add comments when modifying Lua scripts in this project.**

### Why?
- Context is lost between sessions
- Future AI assistants (and humans) need to understand design decisions
- Prevents reimplementing the same solutions repeatedly
- Code comments stay close to the implementation

### What to Comment:
1. **File headers** - Document the purpose, creature entries, spell IDs
2. **Non-obvious API choices** - Why did you use this Eluna function?
3. **Workarounds** - What Eluna limitations did you encounter?
4. **Event handlers** - What triggers this code?

### Eluna API Gotchas (Document These!)

When you discover that an Eluna function doesn't work as expected, **add a comment**:

```lua
-- NOTE: GetPlayersInWorld() does NOT work in map-specific Eluna states (e.g., state 870)
-- NOTE: GetPlayersInMap() does NOT exist in Eluna at all!
-- For proximity detection, use creature:GetPlayersInRange() on the creature itself
-- or iterate players via OnLogin/OnLogout events and track them manually
local players = boss:GetPlayersInRange(30)
```

```lua
-- NOTE: GetCreaturesInWorld() does not exist in Eluna
-- Use player:GetCreaturesInRange(range, entry) to find creatures near a player
local creatures = player:GetCreaturesInRange(30, BOSS_ENTRY)
```

### Example Boss Script Header:

```lua
--[[
    Boss: Ook-Ook
    Instance: Stormstout Brewery (Map 961)
    Entry: 56637
    
    Abilities:
    - Ground Pound (106807) - Frontal cone, knocks back
    - Going Bananas (106651) - Enrage at health thresholds
    
    Mechanics:
    - Rolling barrels (56682) spawn from walls
    - Kick barrels into boss for damage/stun
    
    Known Issues:
    - Retail requires killing 40 Hozen first; using proximity trigger for testing
]]--
```

### Stale Timer Issue After `.reload eluna`

**Problem:** `CreateLuaEvent` timers continue running after reload with OLD code.  
**Solution:** Use script version tracking or accept errors until server restart.

```lua
-- Add at top of script to track version
local SCRIPT_VERSION = 2
_G.MY_SCRIPT_VERSION = SCRIPT_VERSION

-- In timer callbacks, check version (optional)
if _G.MY_SCRIPT_VERSION ~= SCRIPT_VERSION then return end
```

### Key Scripts with Important Comments:
- `instances/mists_of_pandaria/stormstout_brewery/boss_ook_ook.lua` - Eluna API workarounds, stale timer handling
