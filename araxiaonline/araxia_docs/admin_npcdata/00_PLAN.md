# NPC Server Data Integration Plan

**Feature:** Fetch detailed NPC combat stats from server via AMS  
**Target:** AraxiaTrinityAdmin NPC Info panel  
**Created:** November 28, 2025

---

## Goal

Replace the note *"Detailed combat stats (armor, damage, etc) are only available via server commands"* with actual server-fetched data displayed in the NPC Info panel.

## Current State

**Client-Side Data Available:**
- Name, NPC ID, GUID (from targeting)
- Level, Health, Classification, Reaction (from UnitX API)
- Creature Type, Faction (from UnitX API)

**Server-Side Data Needed:**
- ⚔️ **Combat Stats:** Armor, Attack Power, Damage Min/Max, Attack Speed
- 🛡️ **Resistances:** Holy, Fire, Nature, Frost, Shadow, Arcane
- 📊 **Advanced Stats:** Spell Power, Crit %, Hit %, Dodge %, Parry %
- 🎯 **Creature Template Data:** Scale, Speed (walk/run), Rank
- 💰 **Loot/Gold:** Min/Max gold, Loot table ID
- 🏷️ **Flags:** Unit flags, NPC flags, Type flags
- 📝 **Scripts:** AI name, Script name
- 🔧 **Mechanics:** Immunity mask, Mechanic immunity

## UI Design

### Display Location
Add new section below "Additional Info" in the left scroll pane:

```
=== Server Data ===
[Loading...] or [Refresh Server Data]

Combat Stats:
  Armor: 1234
  Attack Power: 567
  Damage: 100-150 (1.5s)
  
Resistances:
  Holy: 0    Fire: 10
  Nature: 0  Frost: 10
  Shadow: 0  Arcane: 10

Advanced:
  Rank: Elite
  Scale: 1.0
  Speed: 1.0 (walk) / 1.14286 (run)
  
Loot:
  Gold: 1-5 copper
  Loot Table: 219014
```

### Loading States
1. **Initial:** Show "Click 'Refresh Server Data' button"
2. **Loading:** Show spinner + "Fetching from server..."
3. **Success:** Display data with timestamp
4. **Error:** Show error message with retry button
5. **No Target:** Hide section or show "Target an NPC"

## Technical Implementation

### Phase 1: Eluna Investigation ✅
- [x] Check available Creature methods in Eluna
- [ ] Identify missing methods (may need C++ additions)
- [ ] Document what's available vs what we need

### Phase 2: Server Handler
- [ ] Create `lua_scripts/admin_handlers.lua`
- [ ] Implement `GET_NPC_DATA` handler
- [ ] Fetch creature from world by GUID
- [ ] Extract all available stats
- [ ] Handle edge cases (creature not found, invalid GUID)

### Phase 3: Client Integration
- [ ] Add AMS to AraxiaTrinityAdmin dependencies
- [ ] Create NPC data display module
- [ ] Implement loading states
- [ ] Update NPC Info panel UI
- [ ] Wire up to Refresh button

### Phase 4: Testing
- [ ] Test with normal NPCs
- [ ] Test with elite/rare NPCs
- [ ] Test with bosses
- [ ] Test error cases (invalid target, server timeout)
- [ ] Test caching (future)

### Phase 5: Polish
- [ ] Add formatting helpers (gold display, percentage display)
- [ ] Add tooltips for complex stats
- [ ] Add "Copy to clipboard" for server data
- [ ] Performance optimization

## Data Structure

### Request (Client → Server)
```lua
{
    npcGUID = "Creature-0-3-2552-0-219014-0000000995",
    requestedFields = {
        "combat_stats",
        "resistances",
        "advanced",
        "loot",
        "flags",
        "scripts"
    }
}
```

### Response (Server → Client)
```lua
{
    success = true,
    guid = "Creature-0-3-2552-0-219014-0000000995",
    timestamp = 1764335934,
    
    basic = {
        entry = 219014,
        name = "Oathsworn Peacekeeper",
        level = 81,
        -- ... basic info
    },
    
    combat_stats = {
        armor = 1234,
        attackPower = 567,
        damageMin = 100,
        damageMax = 150,
        attackSpeed = 1.5,
        -- ...
    },
    
    resistances = {
        holy = 0, fire = 10, nature = 0,
        frost = 10, shadow = 0, arcane = 10
    },
    
    advanced = {
        rank = 1, -- 0=normal, 1=elite, 2=rare elite, 3=boss, 4=rare
        scale = 1.0,
        walkSpeed = 1.0,
        runSpeed = 1.14286,
        -- ...
    },
    
    loot = {
        minGold = 1,
        maxGold = 5,
        lootId = 219014
    },
    
    flags = {
        unitFlags = 0x00000000,
        npcFlags = 0x00000000,
        typeFlags = 0x00000000
    },
    
    scripts = {
        aiName = "SmartAI",
        scriptName = ""
    }
}
```

### Error Response
```lua
{
    success = false,
    error = "Creature not found in world",
    guid = "Creature-0-3-2552-0-219014-0000000995"
}
```

## Eluna Methods to Investigate

**Creature Object Methods:**
- `GetEntry()` - Creature template ID
- `GetGUIDLow()` - Low GUID
- `GetName()` - Creature name
- `GetLevel()` - Level
- `GetHealth()` - Current HP
- `GetMaxHealth()` - Max HP
- Need to check for: Armor, Attack Power, Damage, etc.

**CreatureTemplate Access:**
- May need to access via C++ if Eluna doesn't expose template
- Check `GetCreatureTemplate()` or similar
- Might need custom Eluna method additions

## Dependencies

**Client:**
- AMS_Client (already available)
- AraxiaTrinityAdmin (existing)

**Server:**
- AMS_Server (already available)
- Eluna Creature API
- Potentially: Custom C++ methods for template access

## Timeline

- **Phase 1:** 30 minutes (investigation)
- **Phase 2:** 1 hour (server handler)
- **Phase 3:** 2 hours (client integration)
- **Phase 4:** 1 hour (testing)
- **Phase 5:** 1 hour (polish)

**Total Estimate:** 5-6 hours

## Success Criteria

✅ Targeting an NPC shows "Refresh Server Data" button  
✅ Clicking button fetches data from server via AMS  
✅ Loading state displays while fetching  
✅ Combat stats display correctly formatted  
✅ Error handling works (NPC not found, timeout)  
✅ Data refreshes when targeting different NPCs  
✅ No performance impact on client or server  

## Future Enhancements

- Client-side caching (reduce server requests)
- Batch requests (fetch multiple NPCs at once)
- Real-time updates (when NPC stats change)
- Compare NPC stats side-by-side
- Export NPC data to file
- Integration with NPC editing features

---

**Status:** Planning Phase  
**Next Step:** Investigate Eluna Creature API
