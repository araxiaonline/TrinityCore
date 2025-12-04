# Waypoint Detail Panel Implementation

**Date:** November 30, 2025  
**Status:** Complete - Ready for Testing  
**Requires:** Server rebuild (C++ changes)

## Overview

Implemented comprehensive waypoint visualization and inspection system allowing admins to:
- View all waypoints for a creature path in a clickable list
- See detailed information about each waypoint (position, orientation, delay, etc.)
- Click waypoints in the list to highlight them in the world
- Target waypoint markers in the world to auto-select them in the UI
- Keep NPC panel focused while inspecting waypoints

## Architecture

### Client-Server Communication

**New AMS Handlers (Server → Client):**
- `GET_WAYPOINT_DETAILS` - Request all nodes for a path
- `SELECT_WAYPOINT` - Highlight a specific waypoint
- `GET_WAYPOINT_FOR_GUID` - Lookup waypoint from creature GUID

**New AMS Responses (Client ← Server):**
- `WAYPOINT_DETAILS_RESPONSE` - Array of waypoint nodes with full data
- `WAYPOINT_SELECTED_RESPONSE` - Confirmation of waypoint selection
- `WAYPOINT_FOR_GUID_RESPONSE` - Path/node IDs for a visual waypoint GUID

### C++ Bindings

**New WaypointManager Methods:**
```cpp
bool GetPathAndNodeByVisualGUID(ObjectGuid guid, uint32& outPathId, uint32& outNodeId) const;
```

**New Global Lua Function:**
```lua
pathId, nodeId = GetWaypointNodeForVisualGUID(guidLow)
```

## UI Components

### Waypoints Tab Layout

```
┌─────────────────────────────────────┐
│ Waypoint List (Top 50%)             │
├─────────────────────────────────────┤
│ Waypoint Details (Bottom 50%)       │
│ [Deselect]                          │
└─────────────────────────────────────┘
```

**List Features:**
- Clickable node entries (Node 1, Node 2, etc.)
- Highlighted selected node (green background)
- Scrollable for paths with many nodes
- Auto-populated when "Show Waypoints" is clicked

**Detail Panel Features:**
- Node ID and detailed information
- Position (X, Y, Z coordinates)
- Orientation angle
- Delay in milliseconds
- Move type and action IDs
- Deselect button to close detail view

## Implementation Details

### Panel Locking

When waypoints are shown:
1. `panelLockedToNPC` flag is set to `true`
2. NPC panel data remains loaded even when targeting other creatures
3. Targeting waypoint markers doesn't unload NPC info
4. Flag is cleared when "Hide Waypoints" is clicked

### Waypoint Target Detection

When panel is locked and player targets a creature:
1. Extract GUID from target
2. Send `GET_WAYPOINT_FOR_GUID` to server
3. Server looks up path/node IDs using `GetPathAndNodeByVisualGUID`
4. Client receives response and auto-selects waypoint in list
5. Waypoints tab automatically switches to show detail

### Data Flow

```
User clicks "Show Waypoints"
    ↓
SHOW_WAYPOINTS sent to server
    ↓
Server visualizes path (spawns markers)
    ↓
WAYPOINTS_RESPONSE received
    ↓
GET_WAYPOINT_DETAILS sent to server
    ↓
WAYPOINT_DETAILS_RESPONSE received with all nodes
    ↓
Waypoints tab populated with clickable list
    ↓
User clicks waypoint in list
    ↓
Detail panel shows (split view)
    ↓
User targets waypoint marker in world
    ↓
GET_WAYPOINT_FOR_GUID sent to server
    ↓
WAYPOINT_FOR_GUID_RESPONSE received
    ↓
Waypoint auto-selected in list, detail shown
```

## Files Modified

### C++ (Requires Rebuild)

**`WaypointManager.h`**
- Added `GetPathAndNodeByVisualGUID()` declaration

**`WaypointManager.cpp`**
- Implemented `GetPathAndNodeByVisualGUID()` to lookup path/node from visual creature GUID

**`GlobalMethods.h`**
- Added `GetWaypointNodeForVisualGUID()` Lua binding
- Registered in method table

### Server Lua

**`admin_handlers.lua`**
- Added `GET_WAYPOINT_DETAILS` handler - returns array of waypoint nodes
- Added `SELECT_WAYPOINT` handler - highlights waypoint in world
- Added `GET_WAYPOINT_FOR_GUID` handler - looks up path/node from creature GUID

### Client Lua

**`NPCInfoPanel.lua`**
- Implemented waypoint list UI with scrollable frame
- Implemented waypoint detail panel with split view
- Added `UpdateWaypointList()` and `UpdateWaypointDetail()` functions
- Added `RequestWaypointDetails()` to fetch data from server
- Added `WAYPOINT_DETAILS_RESPONSE` handler
- Added `WAYPOINT_FOR_GUID_RESPONSE` handler
- Integrated panel locking when showing waypoints
- Enhanced `PLAYER_TARGET_CHANGED` to detect waypoint markers
- Added waypoint detail request when showing waypoints

## Testing Checklist

- [ ] Server compiles without errors
- [ ] Server loads admin_handlers.lua successfully
- [ ] Target NPC with waypoints
- [ ] Click "Show Waypoints" button
- [ ] Waypoints tab populates with node list
- [ ] Click a waypoint in the list
- [ ] Detail panel shows with waypoint information
- [ ] Target a waypoint marker in the world
- [ ] Waypoint auto-selects in the list
- [ ] NPC panel data remains loaded during waypoint inspection
- [ ] Click "Hide Waypoints" to deactivate
- [ ] Panel lock is cleared

## Known Limitations

- Visual spell effects for waypoint highlighting not yet implemented (placeholder)
- Waypoint modification/creation not yet implemented
- No persistence of waypoint data between sessions (by design)

## Future Enhancements

1. Add visual spell effects to highlight selected waypoints
2. Implement waypoint creation/editing UI
3. Add waypoint deletion functionality
4. Persist waypoint inspection state in SavedVariables
5. Add waypoint search/filter functionality
6. Implement waypoint path visualization (lines between nodes)

## Notes for Developer

- The `panelLockedToNPC` flag is crucial for keeping NPC data loaded
- GUID extraction from `UnitGUID()` uses pattern matching - ensure it matches creature GUID format
- Waypoint nodes are 1-indexed in the UI but use their actual IDs from the database
- The split view resizes dynamically - ensure scroll frames handle height changes
- Always call `UpdateWaypointList()` after `UpdateWaypointDetail()` to refresh highlighting
