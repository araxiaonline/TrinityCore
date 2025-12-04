# Waypoint Visualization Feature

**Date:** November 29, 2025  
**Status:** ✅ Complete and Working  
**Session Duration:** ~30 minutes

---

## The Prompt

> "Ok, i want to do something really crazy now. I will be REALLY impressed if you can figure this one out. I want to have some sort of spell cast at each waypoint in 3d space that represents one of the waypoints. This might require server side and client side code to accomplish. Everything is on the table and changeable to make this possible. Pull out all the stops."

---

## The Plan

### Initial Investigation
1. Research how TrinityCore handles waypoint visualization internally
2. Determine if we can expose existing functionality vs building new
3. Identify the best approach (spells, creatures, visual effects)

### Discovery
Found that TrinityCore already has a built-in waypoint visualization system used by the `.wp show on` GM command:
- `WaypointMgr::VisualizePath()` - Spawns marker creatures at each waypoint
- `WaypointMgr::DevisualizePath()` - Removes the marker creatures
- Uses `VISUAL_WAYPOINT` (creature entry 1) as the marker
- Markers are only visible to GMs

### Implementation Strategy
Instead of reinventing the wheel with spells or custom effects, we decided to:
1. Expose the existing C++ visualization methods to Eluna
2. Create Lua handlers to trigger visualization via AMS
3. Add a UI button to toggle waypoint markers on/off

---

## What Was Built

### 1. C++ Eluna Methods (CreatureMethods.h)

**File:** `src/server/game/LuaEngine/methods/TrinityCore/CreatureMethods.h`

```cpp
/**
 * Visualizes the creature's waypoint path by spawning marker creatures.
 * 
 * Uses TrinityCore's built-in visualization system (same as .wp show on).
 * Markers are only visible to GMs.
 *
 * @param [displayId] : optional display ID for custom marker appearance
 * @return bool : true if visualization was created
 */
int VisualizeWaypointPath(Eluna* E, Creature* creature)
{
    uint32 pathId = creature->GetWaypointPathId();
    if (pathId == 0)
    {
        E->Push(false);
        return 1;
    }

    WaypointPath const* path = sWaypointMgr->GetPath(pathId);
    if (!path || path->Nodes.empty())
    {
        E->Push(false);
        return 1;
    }

    Optional<uint32> displayId;
    if (!lua_isnoneornil(E->L, 2))
        displayId = E->CHECKVAL<uint32>(2);

    sWaypointMgr->VisualizePath(creature, path, displayId);
    E->Push(true);
    return 1;
}

/**
 * Removes waypoint visualization markers for the creature's path.
 *
 * @return bool : true if markers were removed
 */
int DevisualizeWaypointPath(Eluna* E, Creature* creature)
{
    uint32 pathId = creature->GetWaypointPathId();
    if (pathId == 0)
    {
        E->Push(false);
        return 1;
    }

    WaypointPath const* path = sWaypointMgr->GetPath(pathId);
    if (!path)
    {
        E->Push(false);
        return 1;
    }

    sWaypointMgr->DevisualizePath(creature, path);
    E->Push(true);
    return 1;
}
```

**Required Include:** Added `#include "WaypointManager.h"` to `ElunaIncludes.h`

**Method Registration:**
```cpp
{ "VisualizeWaypointPath", &LuaCreature::VisualizeWaypointPath },
{ "DevisualizeWaypointPath", &LuaCreature::DevisualizeWaypointPath },
```

### 2. Server Lua Handlers (admin_handlers.lua)

**File:** `lua_scripts/admin_handlers.lua`

```lua
-- Show waypoint markers in 3D space
AMS.RegisterHandler("SHOW_WAYPOINTS", function(player, data)
    print("[Admin Handlers] SHOW_WAYPOINTS request received")
    
    -- Get creature from player's current selection
    local creature = player:GetSelection()
    if not creature then
        AMS.Send(player, "WAYPOINTS_RESPONSE", { success = false, error = "No target selected" })
        return
    end
    
    creature = creature:ToCreature()
    if not creature then
        AMS.Send(player, "WAYPOINTS_RESPONSE", { success = false, error = "Target is not a creature" })
        return
    end
    
    -- Check if creature has waypoints
    local pathId = creature:GetWaypointPath()
    if not pathId or pathId == 0 then
        AMS.Send(player, "WAYPOINTS_RESPONSE", { success = false, error = "No waypoint path" })
        return
    end
    
    -- Visualize the path
    local success, err = pcall(function() return creature:VisualizeWaypointPath() end)
    
    if success then
        AMS.Send(player, "WAYPOINTS_RESPONSE", { 
            success = true, 
            pathId = pathId,
            message = "Waypoint markers spawned" 
        })
    else
        AMS.Send(player, "WAYPOINTS_RESPONSE", { success = false, error = "Failed to visualize" })
    end
end)

-- Hide waypoint markers
AMS.RegisterHandler("HIDE_WAYPOINTS", function(player, data)
    -- Similar structure, calls creature:DevisualizeWaypointPath()
end)
```

### 3. Client UI (NPCInfoPanel.lua)

**File:** `Interface/AddOns/AraxiaTrinityAdmin/UI/Panels/NPCInfoPanel.lua`

```lua
-- State tracking
local waypointsVisible = false

-- Button creation
local waypointButton = CreateFrame("Button", nil, npcPanel, "UIPanelButtonTemplate")
waypointButton:SetSize(110, 22)
waypointButton:SetPoint("LEFT", deleteButton, "RIGHT", 5, 0)
waypointButton:SetText("Show Waypoints")
waypointButton:Disable()  -- Enabled when creature has waypoint path

-- Button handler
waypointButton:SetScript("OnClick", function()
    if not AMS then return end
    
    local npcData = ATA:GetTargetNPCInfo()
    if not npcData or not npcData.guid then return end
    
    waypointButton:SetText("Working...")
    waypointButton:Disable()
    
    if waypointsVisible then
        AMS.Send("HIDE_WAYPOINTS", { guid = npcData.guid })
    else
        AMS.Send("SHOW_WAYPOINTS", { guid = npcData.guid })
    end
end)

-- Response handler
AMS.RegisterHandler("WAYPOINTS_RESPONSE", function(data)
    if data.success then
        waypointsVisible = not waypointsVisible
        waypointButton:SetText(waypointsVisible and "Hide Waypoints" or "Show Waypoints")
        waypointButton:Enable()
    else
        waypointButton:SetText(waypointsVisible and "Hide Waypoints" or "Show Waypoints")
        waypointButton:Enable()
        print("|cFFFF0000[ATA]|r " .. (data.error or "Failed"))
    end
end)

-- Enable button when server data shows waypoint path exists
if data and data.movement and data.movement.waypointPath then
    waypointButton:Enable()
end
```

---

## What Went Right ✅

1. **Leveraged existing TrinityCore code** - Instead of building a custom visualization system, we exposed the built-in one. This saved significant development time and ensured compatibility.

2. **Clean Eluna integration** - The C++ methods follow established patterns and integrate seamlessly with existing Eluna methods.

3. **Proper error handling** - Used `pcall` to gracefully handle cases where the C++ method might not be available (pre-rebuild).

4. **State management** - Client properly tracks visualization state and resets on target change.

5. **AMS messaging** - Reused the established AMS pattern for client-server communication.

---

## What We Missed & Fixed Later ❌ → ✅

### Issue 1: Wrong AMS API
**Problem:** Used `ATA.AMS:Send()` instead of `AMS.Send()` (global)  
**Symptom:** Client said "Working..." but server never received the message  
**Fix:** Changed to `AMS.Send()` and `AMS.RegisterHandler()` patterns

### Issue 2: Non-existent Helper Function
**Problem:** Used `GetCreatureFromGUID()` which doesn't exist  
**Symptom:** Server error: `attempt to call global 'GetCreatureFromGUID' (a nil value)`  
**Fix:** Used `player:GetSelection():ToCreature()` like GET_NPC_DATA handler

### Issue 3: Scope-limited Helper Function
**Problem:** Used `SafeGet()` which was defined inside GET_NPC_DATA handler scope  
**Symptom:** Server error: `attempt to call global 'SafeGet' (a nil value)`  
**Fix:** Used `pcall()` directly for error-safe method calls

### Issue 4: C++ Rebuild Required
**Problem:** New C++ methods aren't available until server is rebuilt  
**Symptom:** `pcall` would fail silently or return false  
**Note:** This is expected - the Lua handlers gracefully handle this case

---

## Key Technical Learnings

### TrinityCore Waypoint System
- Waypoint paths stored in `waypoint_path` and `waypoint_path_node` tables
- `WaypointMgr` singleton manages all path data
- `VISUAL_WAYPOINT` (creature entry 1) used for visualization markers
- Markers spawn as `TempSummon` creatures (auto-despawn capable)
- GM mode required to see markers (designed for content developers)

### Eluna Integration Pattern
```cpp
// Pattern for exposing Core functionality to Lua:
int MyMethod(Eluna* E, Creature* creature)
{
    // 1. Validate prerequisites
    uint32 requiredValue = creature->GetSomeValue();
    if (!requiredValue) {
        E->Push(false);
        return 1;
    }
    
    // 2. Get optional parameters
    Optional<uint32> optParam;
    if (!lua_isnoneornil(E->L, 2))
        optParam = E->CHECKVAL<uint32>(2);
    
    // 3. Call Core functionality
    sSomeMgr->DoSomething(creature, requiredValue, optParam);
    
    // 4. Return result
    E->Push(true);
    return 1;
}
```

### AMS Client Pattern
```lua
-- Correct pattern for client-side AMS:
if AMS then
    AMS.Send("HANDLER_NAME", { data = value })
end

-- Handler registration (with delayed init for load order):
local function InitHandlers()
    if not AMS then
        C_Timer.After(0.5, InitHandlers)
        return
    end
    AMS.RegisterHandler("RESPONSE_NAME", function(data)
        -- handle response
    end)
end
InitHandlers()
```

---

## Files Modified

| File | Changes |
|------|---------|
| `src/server/game/LuaEngine/methods/TrinityCore/CreatureMethods.h` | Added VisualizeWaypointPath, DevisualizeWaypointPath methods |
| `src/server/game/LuaEngine/ElunaIncludes.h` | Added WaypointManager.h include |
| `lua_scripts/admin_handlers.lua` | Added SHOW_WAYPOINTS, HIDE_WAYPOINTS handlers |
| `Interface/AddOns/AraxiaTrinityAdmin/UI/Panels/NPCInfoPanel.lua` | Added waypoint toggle button and handlers |

---

## Next Steps: Waypoint Editing

This visualization feature is foundational for the next phase: **in-game waypoint editing**.

### Planned Features
1. **Add Waypoint** - Click to add a new waypoint at player position
2. **Remove Waypoint** - Target a waypoint marker and remove it
3. **Move Waypoint** - Drag or reposition existing waypoints
4. **Reorder Waypoints** - Change the patrol sequence
5. **Set Delays** - Configure pause time at each waypoint
6. **Save to Database** - Persist changes to waypoint_path_node table

### Required C++ Methods to Expose
- `WaypointMgr::AddNode()` or direct DB insert via Eluna
- `WaypointMgr::DeleteNode()` or direct DB delete
- `WaypointMgr::ReloadPath()` to refresh after changes
- `Creature::LoadPath()` to apply new path to creature

### UI Enhancements Needed
- Waypoint list panel showing all nodes with coordinates
- Add/Remove/Edit buttons for each node
- Drag-and-drop reordering
- Visual feedback when clicking in world to place waypoints

---

## Summary

We successfully exposed TrinityCore's internal waypoint visualization system to the AraxiaTrinityAdmin addon, enabling GMs to see waypoint paths in 3D space with a single button click. This feature uses the existing `WaypointMgr::VisualizePath()` functionality, ensuring reliability and consistency with the built-in `.wp show on` command.

The implementation spans C++ (Eluna bindings), server-side Lua (AMS handlers), and client-side Lua (UI button and state management), demonstrating the full-stack integration capabilities of the Araxia Online development environment.

**Total fixes required:** 3 (AMS API, helper function, scope issue)  
**Time from concept to working feature:** ~30 minutes  
**Lines of code added:** ~150 (across all files)
