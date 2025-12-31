# Eluna Lua Scripts - Agent Guidelines

## ⚠️ CRITICAL: File Editing Locations

### Server-Side Lua Scripts
- **EDIT HERE (running server):** `/opt/trinitycore/lua_scripts/`
- **COMMIT FROM:** `/opt/trinitycore/TrinityCore/araxiaonline/lua_scripts/`

### Client-Side Addons (WoW Client)
- **EDIT HERE (running client):** `/opt/trinitycore/Interface/AddOns/`
- **COMMIT FROM:** `/opt/trinitycore/TrinityCore/araxiaonline/client_addons/`

### Commit Workflow
Before committing, sync files from running locations to repo locations:

```bash
# Sync lua_scripts
cp -r /opt/trinitycore/lua_scripts/* /opt/trinitycore/TrinityCore/araxiaonline/lua_scripts/

# Sync client addons
cp -r /opt/trinitycore/Interface/AddOns/AraxiaTrinityAdmin/* /opt/trinitycore/TrinityCore/araxiaonline/client_addons/AraxiaTrinityAdmin/
cp -r /opt/trinitycore/Interface/AddOns/AMS_Client/* /opt/trinitycore/TrinityCore/araxiaonline/client_addons/AMS_Client/
```

**AI assistants: ALWAYS edit the running locations, then sync before committing.**

---

## ⚠️ NEVER Defer Documentation

**Documentation is CRITICAL and must never be skipped or deferred.**

Why:
- AI assistants have limited context windows
- Documentation is how future sessions understand past decisions
- Without docs, work gets duplicated or misunderstood
- Code comments, READMEs, and wiki pages are essential deliverables

**Every feature/change must include:**
1. Code comments explaining non-obvious decisions
2. Updated README if user-facing
3. Wiki/AGENTS.md updates for AI context
4. Inline examples where helpful

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
