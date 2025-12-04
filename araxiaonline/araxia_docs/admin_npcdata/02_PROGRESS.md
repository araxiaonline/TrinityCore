# NPC Server Data - Progress Tracker

**Feature:** Server-side NPC data integration for AraxiaTrinityAdmin  
**Started:** November 28, 2025  
**Status:** ✅ Phase 3 Complete (Nov 29, 2025) - 3D Waypoint Visualization

---

## Current Sprint: Phase 1 - Basic Integration

### Completed ✅
- [x] Created documentation structure
- [x] Created planning document (`00_PLAN.md`)
- [x] Investigated Eluna API (`01_ELUNA_API_INVESTIGATION.md`)
- [x] Identified available vs missing methods
- [x] Decided on phased approach

### In Progress 🔵
- [x] Create server handler with available data
- [x] Create ServerDataModule.lua
- [x] Update AraxiaTrinityAdmin UI
- [x] Test basic AMS integration ✅

### Todo 📋
- [x] Add loading state to UI
- [x] Format display for server data
- [x] Handle errors gracefully
- [x] Test with different NPC types
- [x] Verify full round-trip works ✅
- [x] Document any bugs found (see Session 2 notes)

---

## Phase 1: Basic Integration (Current)

**Goal:** Get server data flowing to client using only available Eluna methods

**Data to Display:**
- Basic Stats: STR, AGI, STA, INT, SPI
- Level, Health, Power
- Movement Speed (walk/run/fly)
- Spell Power (per school)
- AI Name, Script Name
- Faction, Creature Type, Rank
- Respawn Delay, Wander Radius

**Files to Create/Modify:**
1. Server:
   - `lua_scripts/admin_handlers.lua` - New NPC data handler
   - `lua_scripts/init.lua` - Load admin handlers
   
2. Client:
   - `AraxiaTrinityAdmin/ServerDataModule.lua` - New module for server data
   - `AraxiaTrinityAdmin/NPCInfoPanel.lua` - Update to display server data
   - `AraxiaTrinityAdmin/AraxiaTrinityAdmin.toc` - Add new file

**Estimated Time:** 2-3 hours

---

## Phase 2: Custom Eluna Methods

**Goal:** Add C++ methods for combat stats

**Methods to Add:**
- `GetArmor()` - Armor value
- `GetResistance(school)` - Resistance per school
- `GetDamage(weaponType)` - Min/Max damage
- `GetAttackTime(weaponType)` - Attack speed
- `GetAttackPower()` - Attack power

**Files to Modify:**
- `src/server/game/LuaEngine/methods/TrinityCore/CreatureMethods.h`

**Estimated Time:** 1-2 hours

---

## Phase 3: Template Data Access

**Goal:** Add comprehensive creature template data

**Method to Add:**
- `GetCreatureTemplate()` - Returns table with all template fields

**Data Included:**
- Gold drop (min/max)
- Loot table ID
- Unit flags, NPC flags, Type flags
- Family, Class
- Immunity masks
- And more...

**Estimated Time:** 2-3 hours

---

## Phase 4: Polish & Optimization

**Goal:** Make it production-ready

**Tasks:**
- Client-side caching
- Better error messages
- Tooltips for stats
- Copy to clipboard
- Performance testing
- Visual improvements

**Estimated Time:** 2-3 hours

---

## Total Estimated Time

- Phase 1: 2-3 hours ⏱️
- Phase 2: 1-2 hours
- Phase 3: 2-3 hours
- Phase 4: 2-3 hours

**Total:** 7-11 hours

---

## Session Log

### Session 1 - November 28, 2025 (Morning)

**Time:** 9:45am - 

**Goals:**
- Plan the feature
- Investigate Eluna API
- Start Phase 1 implementation

**Accomplished:**
- ✅ Created documentation structure
- ✅ Wrote comprehensive plan
- ✅ Investigated available Eluna methods
- ✅ Documented what's available vs what needs C++
- ✅ Updated docs to reflect Araxia C++ philosophy
- ✅ Created `admin_handlers.lua` with GET_NPC_DATA handler
- ✅ Integrated into server init.lua
- ✅ Created `ServerDataModule.lua` for client
- ✅ Updated AraxiaTrinityAdmin.toc with AMS_Client dependency
- ✅ Updated NPCInfoPanel.lua to request/display server data
- 🔵 Ready for testing!

**Next:**
- Restart worldserver
- `/reload` client addons
- Test with real NPC
- Debug any issues

**Notes:**
- Decided on phased approach (start with available data, add C++ later)
- Aligns with Araxia philosophy (C++ changes welcomed)
- AMS infrastructure already in place from previous work

---

### Session 2 - November 29, 2025

**Time:** 12:45pm - 1:00pm

**Goals:**
- Test NPC data round-trip with working AMS

**Accomplished:**
- ✅ Fixed AMS multi-part message reassembly (C++ ElunaSharedData)
- ✅ All 8 AMS tests passing
- ✅ Tested GET_NPC_DATA handler
- ✅ Fixed server crash from GetStat/GetBaseSpellPower calls
- ✅ Fixed userdata serialization issue
- ✅ NPC data displaying in client UI!

**Bugs Found & Fixed:**
1. **Segfault on GetStat/GetBaseSpellPower** - These Eluna methods crash on some creatures (null pointer). Disabled for now, will add safe C++ methods in Phase 2.
2. **Userdata serialization error** - Some Eluna methods return userdata instead of primitives. Added `tostring()` conversion in SafeGet helper.

**Phase 1 Status:** ✅ COMPLETE

---

### Session 3 - November 29, 2025 (Evening)

**Time:** 7:00pm - 7:30pm

**Goals:**
- Implement Phase 2 C++ methods for combat stats and template data

**Accomplished:**
- ✅ Added `creature:GetArmor()` C++ method
- ✅ Added `creature:GetResistance(school)` C++ method
- ✅ Added `creature:GetBaseAttackTime(type)` C++ method
- ✅ Added `creature:GetStat(statType)` C++ method
- ✅ Added `creature:GetCreatureTemplateData()` - returns full Lua table with template fields
- ✅ Updated admin_handlers.lua to use new C++ methods
- ✅ Updated client ServerDataModule.lua to display new data
- ✅ Auto-fetch server data on target change

**Key Learnings:**
1. **Creatures don't use base stats (STR/AGI/STA/INT/SPI)** - The `Creature::UpdateStats()` function is empty! Creatures get health/damage/armor directly from `creature_template` tables, not calculated from stats like players.
2. **GetBaseSpellPower crashes on creatures** - Still disabled, needs investigation.
3. **Eluna class doesn't have SetField** - Had to use raw Lua C API with macros for table population.

**New C++ Methods Added to CreatureMethods.h:**
```cpp
GetArmor()                    // Returns armor value
GetResistance(school)         // Returns resistance (0-6)
GetBaseAttackTime(type)       // Returns attack time in ms (0-2)
GetStat(statType)             // Returns stat value (0-4) - always 0 for creatures!
GetCreatureTemplateData()     // Returns Lua table with all template fields
```

**Template Data Fields Exposed:**
- entry, name, subName, iconName
- npcFlags, unitFlags, unitFlags2, unitFlags3, extraFlags
- type, family, unitClass, faction
- baseAttackTime, rangeAttackTime, baseVariance, rangeVariance, dmgSchool
- speedWalk, speedRun, scale, movementType
- aiName, scriptId
- vehicleId, regenHealth, racialLeader, modExperience, requiredExpansion
- resistances[0-6], spells[]

**Phase 2 Status:** ✅ COMPLETE

---

### Session 4 - November 29, 2025 (Evening)

**Time:** 7:45pm - 8:05pm

**Goals:**
- Redesign NPC Info panel with tabbed interface
- Add flag decoding for human-readable display
- Investigate why creatures move when wander radius is 0
- Add waypoint path data retrieval

**Accomplished:**
- ✅ **Tabbed UI Interface** - Redesigned NPCInfoPanel.lua with 3 tabs:
  - **Basic** - Client-side info (name, ID, GUID, level, classification, type, faction, GM commands)
  - **Stats** - Vitals, combat stats (armor, attack times), movement speeds, resistances
  - **AI** - Scripts, movement info, behavior, template with decoded flags
  
- ✅ **Flag Decoders** - Added human-readable flag interpretation:
  - `NPC_FLAGS` → "Services" (Vendor, Quest Giver, Trainer, etc.)
  - `UNIT_FLAGS` → "Behaviors" (Immune to PC/NPC, Skinnable, etc.)
  - `EXTRA_FLAGS` → "Properties" (Guard, No XP, Dungeon Boss, etc.)
  
- ✅ **Movement Type Detection** - Added `movement` section to server response:
  - `defaultType` - 0=Idle, 1=Random, 2=Waypoint (from creature spawn)
  - `currentType` - Current MotionMaster type (Idle, Random, Waypoint, Chasing, Fleeing, etc.)
  - `currentWaypointId` - Which waypoint the creature is currently at
  
- ✅ **Waypoint Path Data** - New C++ method `GetWaypointPathData()`:
  - Returns path ID, node count, move type
  - Returns array of all waypoint nodes (id, x, y, z, delay)
  - Current node highlighted in display

**Key Learnings:**

1. **Wander Radius vs Movement Type** - A creature can have `wander_radius=0` but still move if its `MovementType=2` (Waypoint). The wander radius only applies to Random movement type.

2. **Movement Generator Types** (from `MovementDefines.h`):
   ```
   IDLE_MOTION_TYPE     = 0   // Stationary
   RANDOM_MOTION_TYPE   = 1   // Random wander within radius
   WAYPOINT_MOTION_TYPE = 2   // Following waypoint path
   CONFUSED_MOTION_TYPE = 4   // Confused/feared movement
   CHASE_MOTION_TYPE    = 5   // Chasing target
   FLEEING_MOTION_TYPE  = 6   // Running away
   ```

3. **Waypoint Storage** - Waypoint paths stored in `waypoint_path` and `waypoint_path_node` tables, managed by `WaypointMgr` singleton. Path ID linked from `creature` table's `path_id` field.

4. **Flag Locations** (for reference):
   - `UnitFlags`, `UnitFlags2`, `UnitFlags3` → `UnitDefines.h`
   - `NPCFlags`, `NPCFlags2` → `UnitDefines.h`  
   - `CreatureFlagsExtra` → `CreatureData.h`

**New C++ Method Added:**
```cpp
GetWaypointPathData()  // Returns Lua table with:
                       //   pathId, nodeCount, moveType, currentNodeId
                       //   nodes[] = {id, x, y, z, delay}
```

**Files Modified:**
- `ElunaIncludes.h` - Added WaypointManager.h include
- `CreatureMethods.h` - Added GetWaypointPathData method
- `admin_handlers.lua` - Added movement section with waypoint path
- `NPCInfoPanel.lua` - Complete rewrite with tabbed interface
- `ServerDataModule.lua` - Added flag decoder functions

**Phase 2.5 Status:** ✅ COMPLETE

---

## Session 5 - Nov 29, 2025 (Phase 3: 3D Waypoint Visualization)

**Goal:** Visualize creature waypoint paths in 3D space with markers

**The Prompt:**
> "I want to have some sort of spell cast at each waypoint in 3D space that represents one of the waypoints. This might require server side and client side code. Pull out all the stops."

**Accomplishments:**
1. ✅ Discovered TrinityCore's built-in `WaypointMgr::VisualizePath()` system
2. ✅ Created C++ Eluna bindings: `VisualizeWaypointPath()`, `DevisualizeWaypointPath()`
3. ✅ Created server handlers: `SHOW_WAYPOINTS`, `HIDE_WAYPOINTS`
4. ✅ Added "Show Waypoints" toggle button to NPC Info panel
5. ✅ Full end-to-end working: click button → markers spawn in world

**Bugs Fixed During Session:**
1. Wrong AMS API (`ATA.AMS:Send()` → `AMS.Send()`)
2. Non-existent function (`GetCreatureFromGUID` → `player:GetSelection():ToCreature()`)
3. Scope-limited function (`SafeGet` → `pcall`)

**Key Discovery:**
- VISUAL_WAYPOINT (creature entry 1) markers only visible in GM mode
- TrinityCore already had the perfect visualization system, just needed Eluna exposure

**New C++ Methods:**
- `creature:VisualizeWaypointPath([displayId])` - Spawn markers at waypoints
- `creature:DevisualizeWaypointPath()` - Remove waypoint markers

**Files Modified:**
- `CreatureMethods.h` - Added visualization methods
- `ElunaIncludes.h` - Added WaypointManager.h include
- `admin_handlers.lua` - Added SHOW_WAYPOINTS, HIDE_WAYPOINTS handlers
- `NPCInfoPanel.lua` - Added waypoint toggle button and handlers

**Documentation:** See `03_WAYPOINT_VISUALIZATION.md` for full implementation details

**Phase 3 Status:** ✅ COMPLETE

**Next Phase:** In-game waypoint editing (add/remove/move waypoints)

---

## Blockers

*None currently*

---

## Questions / Decisions

**Q:** Should we show base stats or current stats (with buffs)?  
**A:** TBD - probably current stats since that's what's affecting the NPC now

**Q:** Cache data client-side?  
**A:** Phase 4 enhancement - not critical for alpha

**Q:** Format for GUID passing?  
**A:** Use the full GUID string from client (e.g., "Creature-0-3-2552-0-219014-0000000995")

---

## Dependencies

**Working:**
- ✅ AMS Client/Server (from previous session)
- ✅ AraxiaTrinityAdmin basic structure
- ✅ Eluna enabled on server

**Needed:**
- Custom Eluna methods (Phase 2+)
- UI design finalized (in progress)

---

## Success Metrics

**Phase 1 Complete When:**
- ✅ Handler returns basic NPC data
- ✅ Client receives and displays data
- ✅ Loading state works
- ✅ Error handling works
- ✅ Data updates on target change

**Feature Complete When:**
- All planned data is available
- UI looks polished
- Performance is good (<100ms response)
- No errors in normal use
- Documented for future developers

---

## Links

- [Plan](./00_PLAN.md)
- [Eluna API Investigation](./01_ELUNA_API_INVESTIGATION.md)
- [Waypoint Visualization](./03_WAYPOINT_VISUALIZATION.md)
- [AMS Documentation](../aio_integration/AMS_ALPHA_RELEASE.md)
