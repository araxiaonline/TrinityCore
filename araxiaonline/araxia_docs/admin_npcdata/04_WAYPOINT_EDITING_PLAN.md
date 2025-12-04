# Phase 4: In-Game Waypoint Editing

**Date:** November 29, 2025  
**Status:** 📋 Planning  
**Depends On:** Phase 3 (Waypoint Visualization) ✅

---

## Goal

Create an intuitive in-game workflow for spawning creatures and defining their patrol paths without using GM commands or database editing.

---

## User Workflow

### Step 1: Spawn Creature
1. Player opens AraxiaTrinityAdmin panel
2. Player selects "Add NPC" tab
3. Player searches for creature template (e.g., "Tawny Grisette")
4. Player clicks "Spawn Here"
5. **Creature spawns at player position, facing player's direction**

### Step 2: Define Waypoints
1. Player walks to first waypoint location
2. Player clicks "Add Waypoint" button
3. Waypoint marker appears at that location
4. Repeat: walk → click → marker appears
5. Chain as many waypoints as needed

### Step 3: Complete Path
1. Player clicks "Finish Path" button
2. Path is saved to database
3. Creature begins patrolling the path
4. **Patrol Mode: Ping-pong (back and forth)**

---

## UI Requirements

### Add NPC Panel Enhancements
- [ ] "Spawn Here" spawns creature facing player's orientation
- [ ] After spawn, auto-target the new creature

### New Waypoint Editing Mode
- [ ] "Start Path" button - begins waypoint recording
- [ ] "Add Waypoint" button - adds current player position as waypoint
- [ ] "Undo Last" button - removes last waypoint
- [ ] "Cancel" button - discards all waypoints
- [ ] "Finish Path" button - saves and activates

### Visual Feedback
- [ ] Show waypoint markers as they're added (reuse visualization)
- [ ] Show numbered labels on markers (1, 2, 3...)
- [ ] Highlight current waypoint being added
- [ ] Show line connecting waypoints (if possible)

---

## Server Requirements

### New Eluna Methods Needed

```lua
-- Spawn creature with specific orientation
creature = SpawnCreature(entry, x, y, z, orientation)

-- Create new waypoint path
pathId = CreateWaypointPath(creatureGUID)

-- Add waypoint to path
AddWaypointNode(pathId, nodeId, x, y, z, delay, moveType)

-- Set path on creature (DB + runtime)
SetCreatureWaypointPath(creatureGUID, pathId)

-- Reload creature's movement
ReloadCreatureMovement(creatureGUID)
```

### Database Operations
- Insert into `waypoint_path` table
- Insert into `waypoint_path_node` table
- Update `creature` table with path_id
- Update `creature_template` movement type if needed

### Path Configuration
- **Move Type:** Walk (0) or Run (1)
- **Delay:** Time to pause at each waypoint (ms)
- **Flags:** Ping-pong / Cyclic / etc.

---

## Technical Investigation ✅ COMPLETE

### Answers Found

**1. How does TrinityCore assign new path IDs?**
```sql
-- Get max path ID from waypoint_path_node table
SELECT MAX(PathId) FROM waypoint_path_node;
-- New path ID = max + 1
```
Prepared statement: `WORLD_SEL_WAYPOINT_PATH_NODE_MAX_PATHID`

**2. Can we insert via Eluna DB queries?**
Yes! Use `WorldDBQuery()` and `WorldDBExecute()` for selects and inserts.

**3. How do we reload a creature's path without restart?**
```cpp
// Reload path data from DB into memory
sWaypointMgr->ReloadPath(pathId);

// Load path onto creature and start movement
creature->LoadPath(pathId);
creature->SetDefaultMovementType(WAYPOINT_MOTION_TYPE);
creature->GetMotionMaster()->Initialize();
```

**4. What's the flag for ping-pong patrol?**
```cpp
WaypointPathFlags::FollowPathBackwardsFromEndToStart = 0x01
```
This is exactly what we need!

**5. Can we modify paths at runtime?**
Yes - insert to DB, then call `sWaypointMgr->ReloadPath(pathId)`.

---

## Implementation Details Found

### Creating Permanent Creature (from cs_npc.cpp)
```cpp
// Create creature at position
Creature* creature = Creature::CreateCreature(entry, map, position);

// Inherit player's phase
PhasingHandler::InheritPhaseShift(creature, player);

// Save to database
creature->SaveToDB(map->GetId(), { map->GetDifficultyID() });

// Get the DB GUID
ObjectGuid::LowType db_guid = creature->GetSpawnId();

// Cleanup and recreate from DB (loads vendor data, quests, etc.)
creature->CleanupsBeforeDelete();
delete creature;
creature = Creature::CreateCreatureFromDB(db_guid, map, true, true);

// Add to grid
sObjectMgr->AddCreatureToGrid(sObjectMgr->GetCreatureData(db_guid));
```

### Creating Waypoint Path (from cs_wp.cpp)
```cpp
// 1. Create the path entry
stmt = WorldDatabase.GetPreparedStatement(WORLD_INS_WAYPOINT_PATH);
stmt->setUInt32(0, pathId);                                      // PathId
stmt->setUInt8(1, AsUnderlyingType(WaypointMoveType::Walk));     // MoveType (0=walk, 1=run)
stmt->setUInt8(2, AsUnderlyingType(WaypointPathFlags::FollowPathBackwardsFromEndToStart)); // Flags (0x01 = ping-pong!)
stmt->setNull(3);                                                 // Velocity
stmt->setString(4, "Created by AraxiaTrinityAdmin");             // Comment
WorldDatabase.Execute(stmt);

// 2. Add waypoint nodes
stmt = WorldDatabase.GetPreparedStatement(WORLD_INS_WAYPOINT_PATH_NODE);
stmt->setUInt32(0, pathId);
stmt->setUInt32(1, nodeId);       // 0, 1, 2, 3...
stmt->setFloat(2, x);
stmt->setFloat(3, y);
stmt->setFloat(4, z);
stmt->setFloat(5, orientation);   // Optional
WorldDatabase.Execute(stmt);
```

### Assigning Path to Creature (from cs_wp.cpp)
```cpp
// 1. Update creature_addon table
stmt = WorldDatabase.GetPreparedStatement(WORLD_UPD_CREATURE_ADDON_PATH);
stmt->setUInt32(0, pathId);
stmt->setUInt64(1, creatureGuidLow);
WorldDatabase.Execute(stmt);

// 2. Update creature movement type
stmt = WorldDatabase.GetPreparedStatement(WORLD_UPD_CREATURE_MOVEMENT_TYPE);
stmt->setUInt8(0, WAYPOINT_MOTION_TYPE);  // = 2
stmt->setUInt64(1, creatureGuidLow);
WorldDatabase.Execute(stmt);

// 3. Apply at runtime
creature->LoadPath(pathId);
creature->SetDefaultMovementType(WAYPOINT_MOTION_TYPE);
creature->GetMotionMaster()->Initialize();
```

### Key Prepared Statements
| Statement | Purpose |
|-----------|---------|
| `WORLD_SEL_WAYPOINT_PATH_NODE_MAX_PATHID` | Get highest path ID |
| `WORLD_SEL_WAYPOINT_PATH_NODE_MAX_NODEID` | Get highest node ID for a path |
| `WORLD_INS_WAYPOINT_PATH` | Create new path |
| `WORLD_INS_WAYPOINT_PATH_NODE` | Add node to path |
| `WORLD_UPD_CREATURE_ADDON_PATH` | Assign path to creature |
| `WORLD_INS_CREATURE_ADDON` | Create creature addon if missing |
| `WORLD_UPD_CREATURE_MOVEMENT_TYPE` | Set creature movement type |

### Source Files Investigated
- `cs_npc.cpp` - HandleNpcAddCommand (permanent creature spawning)
- `cs_wp.cpp` - HandleWpAddCommand, HandleWpLoadCommand (waypoint management)
- `WaypointManager.h` - ReloadPath, VisualizePath, DevisualizePath
- `WaypointDefines.h` - WaypointPathFlags (ping-pong = 0x01)

---

## Implementation Phases

### Phase 4.1: Spawn with Orientation
- Add orientation parameter to creature spawn
- Update AddNPC panel to use player facing

### Phase 4.2: Path Creation Infrastructure
- Investigate DB schema and path creation
- Create Eluna methods for path manipulation
- Test creating paths via Lua

### Phase 4.3: Waypoint Recording UI
- Add "waypoint mode" to UI
- Track waypoints in client memory
- Send waypoint list to server on "Finish"

### Phase 4.4: Path Persistence & Activation
- Save path to database
- Assign path to creature
- Reload creature movement
- Verify patrol works

### Phase 4.5: Polish
- Numbered waypoint markers
- Undo functionality
- Path editing (modify existing paths)
- Delete waypoints

---

## Path Types Reference

From TrinityCore `WaypointDefines.h`:

```cpp
enum WaypointMoveType : uint32
{
    WAYPOINT_MOVE_TYPE_WALK = 0,
    WAYPOINT_MOVE_TYPE_RUN  = 1,
    WAYPOINT_MOVE_TYPE_LAND = 2,  // Flying creatures
    WAYPOINT_MOVE_TYPE_TAKEOFF = 3
};

enum WaypointPathFlags : uint32
{
    WAYPOINT_PATH_FLAG_NONE             = 0x00,
    WAYPOINT_PATH_FLAG_FOLLOW_PATH_BACKWARDS = 0x01,  // Ping-pong!
    // Others...
};
```

**Key Flag:** `WAYPOINT_PATH_FLAG_FOLLOW_PATH_BACKWARDS` = Ping-pong patrol

---

## Database Schema Reference

### `waypoint_path`
| Column | Type | Description |
|--------|------|-------------|
| PathId | int | Unique path identifier |
| MoveType | tinyint | Walk/Run/etc |
| Flags | int | Path behavior flags |
| Comment | varchar | Description |

### `waypoint_path_node`
| Column | Type | Description |
|--------|------|-------------|
| PathId | int | FK to waypoint_path |
| NodeId | int | Order in path (1, 2, 3...) |
| PositionX | float | World X coordinate |
| PositionY | float | World Y coordinate |
| PositionZ | float | World Z coordinate |
| Orientation | float | Optional facing |
| Delay | int | Pause time in ms |

### `creature`
| Column | Type | Description |
|--------|------|-------------|
| guid | int | Unique creature instance |
| waypointPathId | int | FK to waypoint_path |
| MovementType | tinyint | 0=Idle, 1=Random, 2=Waypoint |

---

## Success Criteria

**Phase 4 Complete When:**
- [ ] Can spawn creature facing player direction
- [ ] Can walk around and add waypoints with button clicks
- [ ] Waypoints visualize as they're added
- [ ] Can finish path and save to database
- [ ] Creature immediately starts patrolling
- [ ] Patrol goes back and forth (ping-pong)
- [ ] Path persists across server restart

---

## Performance-First Design

**Goal:** Feel native - zero perceptible lag, instant feedback.

### Design Principles
1. **Single C++ call per operation** - No Lua→C++→Lua→C++ round-trips
2. **Batch DB writes** - One transaction for entire path
3. **Memory-first, DB-second** - Update WaypointMgr cache directly
4. **Prepared statements only** - No string concatenation SQL
5. **Minimal data transfer** - Client sends coords, server does everything else

### High-Performance API

#### `player:SpawnCreatureAtPlayer(entry)` → creature
Single call: get position → create creature → save DB → return creature

#### `CreateWaypointPath(nodes, flags, moveType)` → pathId
Single call with batched transaction:
- Generate path ID
- INSERT path
- INSERT ALL nodes (batched)
- COMMIT (one DB round-trip)
- ReloadPath into memory
- Return pathId

#### `creature:AssignWaypointPath(pathId)` → bool
Single call:
- UPDATE creature_addon
- UPDATE movement type
- LoadPath (memory)
- Initialize motion
- Creature patrols immediately

### Data Flow
```
CLIENT                           SERVER
──────                           ──────
Click "Add Waypoint"
  └─► Store {x,y,z} locally      (NO server call)

Click "Add Waypoint"  
  └─► Store {x,y,z} locally      (NO server call)

Click "Finish Path"
  └─► Send ALL waypoints ───────► CreateWaypointPath()
      in single message             └─► Batched transaction
                                    └─► Return pathId
  ◄─── pathId ──────────────────────┘

  └─► Send AssignPath ──────────► creature:AssignWaypointPath()
                                    └─► Creature patrols!
```

### Performance Target
| Operation | Target | Method |
|-----------|--------|--------|
| Spawn creature | <5ms | Single C++ call |
| Create 10-node path | <15ms | Batched transaction |
| Assign + activate | <5ms | Memory operations |
| **Total workflow** | **<50ms** | **Feels instant** |

---

## C++ Methods to Implement

### 1. PlayerMethods.h - SpawnCreatureAtPlayer
```cpp
int SpawnCreatureAtPlayer(Eluna* E, Player* player)
{
    uint32 entry = E->CHECKVAL<uint32>(2);
    // Create at player pos, save to DB, return creature
}
```

### 2. GlobalMethods.h - CreateWaypointPath
```cpp
int CreateWaypointPath(Eluna* E)
{
    // Parse Lua table of nodes
    // Batched transaction insert
    // ReloadPath
    // Return pathId
}
```

### 3. CreatureMethods.h - AssignWaypointPath
```cpp
int AssignWaypointPath(Eluna* E, Creature* creature)
{
    uint32 pathId = E->CHECKVAL<uint32>(2);
    // Update DB + LoadPath + Initialize
}
```

---

## Implementation Order

1. **SpawnCreatureAtPlayer** - Quick win, immediately useful
2. **AssignWaypointPath** - Can test with existing paths
3. **CreateWaypointPath** - Batched path creation
4. **Client UI** - Waypoint recording mode

---

## Links

- [Waypoint Visualization](./03_WAYPOINT_VISUALIZATION.md) - Foundation for this work
- [Progress Tracker](./02_PROGRESS.md) - Session logs
