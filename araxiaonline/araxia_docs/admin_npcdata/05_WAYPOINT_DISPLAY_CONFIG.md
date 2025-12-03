# Waypoint Display Configuration

## Overview

Waypoint markers are visual indicators spawned at each node of a creature's patrol path. They use the `VISUAL_WAYPOINT` creature entry (ID 1) and are only visible to GMs.

## Display IDs for 11.2.5 Client

**Working Display IDs:**
| ID | Name | Description |
|----|------|-------------|
| 1824 | Elven Wisp | Blue floating wisp - **recommended for waypoints** |
| 169 | Rock Elemental | Small rock creature |
| 31366 | Green Circle | Targeting circle - used for spawn markers |

**Non-Working Display IDs:**
| ID | Name | Issue |
|----|------|-------|
| 17188 | Blue Skull | Shows as human model in 11.2.5 |
| 17519 | Red Orb | Shows as human model in 11.2.5 |
| 10045 | Wisp | Shows as human model in 11.2.5 |

## Runtime Configuration via Lua Shared Data

Display IDs are configurable at runtime without recompiling:

```lua
-- In admin_handlers.lua
local DEFAULT_DISPLAYS = {
    waypoint_marker = 1824,       -- Elven Wisp
    waypoint_highlight = 1824,    -- Same, differentiated by scale
    spawn_marker = 31366,         -- Green targeting circle
}
```

### Changing via MCP

```
-- Set waypoint marker to rock elemental
mcp0_shared_data_write(key="config_display_waypoint_marker", value="169")

-- Then reload Lua and hide/show waypoints
.reload eluna
```

### Pattern for Configurable Values

```lua
local function GetDisplayId(key)
    local sharedKey = "config_display_" .. key
    local value = GetSharedData(sharedKey)
    if value and value ~= "" then
        local num = tonumber(value)
        if num then return num end
    end
    return DEFAULT_DISPLAYS[key] or 1824
end
```

## C++ Client Refresh Pattern

**Critical:** `SetDisplayId()` alone does NOT update creatures already visible to the client. You must force a refresh:

```cpp
summon->SetDisplayId(displayId, true);
summon->SetObjectScale(0.5f);

// Force client to see the change
summon->DestroyForNearbyPlayers();
summon->UpdateObjectVisibilityOnCreate();
```

Without these calls, the client shows cached appearance from when the creature spawned.

## Files Modified

### C++ Changes
- `src/server/game/Movement/Waypoints/WaypointManager.cpp`
  - Added client refresh after SetDisplayId
  - Default display: 1824 (Elven Wisp)
  
- `src/server/game/LuaEngine/methods/TrinityCore/CreatureMethods.h`
  - Fixed displayId arg parsing for `VisualizeWaypointPath(player, displayId)`

- `src/server/database/Database/MySQLConnection.cpp`
  - Graceful handling of `ER_BAD_FIELD_ERROR`, `ER_NO_SUCH_TABLE`, `ER_PARSE_ERROR`
  - No longer crashes server on bad MCP queries

### Lua Changes
- `lua_scripts/admin_handlers.lua`
  - Added `DEFAULT_DISPLAYS` table
  - Added `GetDisplayId()`, `SetDisplayIdConfig()`, `InitDisplayConfigs()`
  - AMS handlers: `SET_DISPLAY_CONFIG`, `GET_DISPLAY_CONFIGS`

### Database Changes
- `creature_template_model` for `CreatureID = 1` set to `CreatureDisplayID = 1824`

## Lessons Learned

1. **Client version matters** - Display IDs from older WoW versions may not render correctly in modern clients

2. **Test displays in-game** - Use `.npc add <entry>` to spawn creatures and verify their appearance before using displayIds

3. **Client caches creature appearance** - Must force refresh with `DestroyForNearbyPlayers()` + `UpdateObjectVisibilityOnCreate()`

4. **Prefer Lua for config values** - Allows testing without 5+ minute recompiles. Use MCP to change values at runtime.

5. **Shared data clears on restart** - `InitDisplayConfigs()` re-initializes defaults on server start
