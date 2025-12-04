# Eluna API Investigation for NPC Data

**Date:** November 28, 2025  
**Updated:** November 29, 2025  
**Purpose:** Determine what NPC data is available through Eluna without C++ modifications

---

## ⚠️ CRITICAL LEARNING: Creatures Don't Use Stats!

**Discovery (Nov 29):** The `Creature::UpdateStats()` function in TrinityCore is **empty**!

```cpp
bool Creature::UpdateStats(Stats /*stat*/)
{
    return true;  // Does nothing!
}
```

**This means:**
- `GetStat(statType)` will ALWAYS return 0 for creatures
- Creatures don't have STR/AGI/STA/INT/SPI like players
- Creature health/damage/armor comes directly from `creature_template` tables
- The stat system is player-only

**What creatures use instead:**
- `creature_template` - Base values
- `creature_template_scaling` - Level scaling
- `creature_classlevelstats` - Class-based stat scaling
- Direct fields: `BaseAttackTime`, `unit_class`, `resistance[]`, etc.

---

## Available Methods

### Unit Methods (Creature inherits from Unit)

⚠️ **Basic Stats (Always 0 for creatures!)**
- `GetStat(statType)` - Returns stat value (0=STR, 1=AGI, 2=STA, 3=INT, 4=SPI) - **Always 0!**
- `GetLevel()` - Level
- `GetHealth()` / `GetMaxHealth()` - HP values
- `GetPower(powerType)` / `GetMaxPower(powerType)` - Mana/Energy/etc
- `GetSpeed(speedType)` - Movement speeds

✅ **Display**
- `GetDisplayId()` - Model ID
- `GetNativeDisplayId()` - Original model ID
- `GetScale()` - Model scale
- `GetName()` - Name

✅ **Combat**
- `GetVictim()` - Current target
- `GetAttackDistance(target)` - Attack range for target
- `IsInCombat()` - Combat state

✅ **Spell Power**
- `GetBaseSpellPower(spellSchool)` - Spell power for school (0-6)

✅ **Misc**
- `GetFaction()` - Faction ID
- `GetCreatureType()` - Creature type
- `GetRank()` - Rank (normal, elite, rare, boss)

### Creature-Specific Methods

✅ **IDs & GUIDs**
- `GetEntry()` - Creature template entry
- `GetGUID()` / `GetGUIDLow()` - Instance GUID
- `GetDBTableGUIDLow()` - Database GUID

✅ **Scripts & AI**
- `GetAIName()` - AI script name
- `GetScriptName()` - Script name
- `GetScriptId()` - Script ID

✅ **Behavior**
- `GetRespawnDelay()` - Respawn time
- `GetWanderRadius()` - Wander distance
- `GetDefaultMovementType()` - Movement type
- `GetAggroRange(target)` - Aggro range

✅ **Loot**
- `GetLootRecipient()` - Who gets loot
- `HasLootRecipient()` - Has loot recipient
- `HasLootMode(mode)` - Check loot mode

---

## Data NOT Directly Available

❌ **Combat Stats (Need C++ Access)**
- Armor value
- Attack Power
- Damage Min/Max
- Attack Speed/Timer
- Resistances (Holy, Fire, Nature, Frost, Shadow, Arcane)
- Crit %, Hit %, Dodge %, Parry %

❌ **Template Data (Need CreatureTemplate Access)**
- Gold drop (min/max)
- Loot table ID
- Unit flags
- NPC flags
- Type flags
- Family
- Creature class
- Immunity masks

❌ **Advanced Stats**
- Spell crit/hit
- Block %
- Expertise
- Haste
- Spell penetration

---

## Workarounds

### Option 1: Use Available Stats Only (Phase 1)
Display only what's available through Eluna:
- Basic stats (STR, AGI, STA, INT, SPI)
- Level, Health, Power
- Speed (walk/run)
- Spell power per school
- AI name, Script name
- Respawn delay, Wander radius

**Benefits:**
- ✅ Works immediately (good for Phase 1)
- ✅ No build required for initial testing
- ✅ Still provides useful data

**Limitations:**
- ⚠️ Missing detailed combat stats
- ⚠️ Missing template data
- ⚠️ Incomplete picture of NPC

### Option 2: Add Custom Eluna Methods ⚡ RECOMMENDED
Add new methods to `CreatureMethods.h` or create `araxia` namespace extensions:
- `GetArmor()` - Access `GetArmor()`
- `GetResistance(school)` - Access `GetResistance(SpellSchools school)`
- `GetAttackPower()` - Access attack power
- `GetMinDamage()` / `GetMaxDamage()` - Damage range
- `GetAttackTime(weaponType)` - Attack speed
- `GetCreatureTemplate()` - Return table with template data

**Benefits:**
- ✅ Complete data access
- ✅ Clean, documented API
- ✅ Reusable for other addons/tools
- ✅ Follows Eluna patterns
- ✅ Full control over implementation
- ✅ Can be isolated in `araxia` namespace if desired
- ✅ Direct debugging access in C++

**Implementation Notes:**
- Can add directly to `CreatureMethods.h` (simple, quick)
- Or create `araxia/AraxiaCreatureExtensions.h` (isolated from upstream)
- Server rebuild required (normal part of development)

### Option 3: Direct Unit Field Access 🔧 NOT RECOMMENDED
Access Unit fields directly if Eluna exposes them:
- Check if `GetUInt32Value(field)` works for creature stats
- Use Unit field constants

**Why Not Recommended:**
- ❌ Brittle (field numbers change between versions)
- ❌ Not portable across expansions
- ❌ Hard to debug
- ❌ Unclear if all fields are accessible
- ❌ Poor developer experience

**When to Consider:**
- Only as a temporary workaround
- When prototyping to validate data exists

---

## Recommendation

**Use Phased Approach: Option 1 → Option 2**

**Phase 1:** Start with Option 1 (available data only) to get the integration working
**Phase 2:** Add custom C++ methods (Option 2) for complete data

### Why This Aligns with Araxia Philosophy

**C++ Changes Are a Strength:**
- ✅ We have full control over our TrinityCore fork
- ✅ Not concerned with upstream merge conflicts (pinned to Midnight)
- ✅ C++ provides direct access to all game data
- ✅ Better performance than workarounds
- ✅ Easier debugging and maintenance
- ✅ Creates reusable tools for future development

**Araxia Namespace Pattern:**
Consider isolating custom code in `araxia` namespace:
```cpp
// src/server/game/Araxia/AraxiaCreatureExtensions.h
namespace Araxia {
    class CreatureExtensions {
        static uint32 GetArmor(Creature* creature);
        static uint32 GetResistance(Creature* creature, SpellSchools school);
        // ... etc
    };
}
```

Then expose via Eluna with minimal changes to core files.

### Methods to Add

```cpp
// In CreatureMethods.h or new AdminMethods.h

/**
 * Returns the creature's armor value
 * @return uint32 armor
 */
int GetArmor(Eluna* E, Creature* creature)
{
    E->Push(creature->GetArmor());
    return 1;
}

/**
 * Returns resistance to a spell school
 * @param uint32 school : 0=Physical, 1=Holy, 2=Fire, 3=Nature, 4=Frost, 5=Shadow, 6=Arcane
 * @return uint32 resistance
 */
int GetResistance(Eluna* E, Creature* creature)
{
    uint32 school = E->CHECKVAL<uint32>(2);
    if (school >= MAX_SPELL_SCHOOL)
        return 1;
    E->Push(creature->GetResistance(SpellSchools(school)));
    return 1;
}

/**
 * Returns min/max damage for weapon type
 * @param uint32 weaponType : 0=BASE, 1=OFFHAND, 2=RANGED
 * @return float minDamage, float maxDamage
 */
int GetDamage(Eluna* E, Creature* creature)
{
    uint32 weaponType = E->CHECKVAL<uint32>(2, 0);
    if (weaponType >= MAX_ATTACK)
        return 1;
    
    float minDmg, maxDmg;
    creature->GetDamage((WeaponAttackType)weaponType, minDmg, maxDmg);
    E->Push(minDmg);
    E->Push(maxDmg);
    return 2;
}

/**
 * Returns attack time for weapon type
 * @param uint32 weaponType : 0=BASE, 1=OFFHAND, 2=RANGED
 * @return uint32 attackTime (milliseconds)
 */
int GetAttackTime(Eluna* E, Creature* creature)
{
    uint32 weaponType = E->CHECKVAL<uint32>(2, 0);
    if (weaponType >= MAX_ATTACK)
        return 1;
    E->Push(creature->GetAttackTime((WeaponAttackType)weaponType));
    return 1;
}

/**
 * Returns creature template data
 * @return table creatureTemplate
 */
int GetCreatureTemplate(Eluna* E, Creature* creature)
{
    CreatureTemplate const* ct = creature->GetCreatureTemplate();
    if (!ct)
        return 0;
    
    lua_newtable(E->L);
    
    // Basic info
    Eluna::Push(E->L, "entry");
    Eluna::Push(E->L, ct->Entry);
    lua_settable(E->L, -3);
    
    Eluna::Push(E->L, "name");
    Eluna::Push(E->L, ct->Name);
    lua_settable(E->L, -3);
    
    Eluna::Push(E->L, "rank");
    Eluna::Push(E->L, ct->rank);
    lua_settable(E->L, -3);
    
    Eluna::Push(E->L, "family");
    Eluna::Push(E->L, ct->family);
    lua_settable(E->L, -3);
    
    Eluna::Push(E->L, "type");
    Eluna::Push(E->L, ct->type);
    lua_settable(E->L, -3);
    
    Eluna::Push(E->L, "minGold");
    Eluna::Push(E->L, ct->mingold);
    lua_settable(E->L, -3);
    
    Eluna::Push(E->L, "maxGold");
    Eluna::Push(E->L, ct->maxgold);
    lua_settable(E->L, -3);
    
    Eluna::Push(E->L, "scale");
    Eluna::Push(E->L, ct->scale);
    lua_settable(E->L, -3);
    
    // Add more fields as needed...
    
    return 1;
}
```

---

## Implementation Plan

### Phase 1: Start with Available Data ✅ COMPLETE
- Implement handler using only existing Eluna methods
- Get basic integration working
- Test AMS communication

### Phase 2: Add Custom Methods ✅ COMPLETE (Nov 29, 2025)
- Add new methods to `CreatureMethods.h`
- Rebuild server
- Update handler to use new methods

**Methods Added:**
```lua
creature:GetArmor()                  -- Returns armor value
creature:GetResistance(school)       -- Returns resistance (0-6)
creature:GetBaseAttackTime(type)     -- Returns attack time in ms (0-2)
creature:GetStat(statType)           -- Returns stat value (0-4) - Always 0 for creatures!
creature:GetCreatureTemplateData()   -- Returns Lua table with all template fields
creature:GetWaypointPathData()       -- Returns full waypoint path with all nodes (Phase 2.5)
```

### Phase 2.5: Movement & UI Enhancements ✅ COMPLETE (Nov 29, 2025)
- Tabbed UI interface (Basic, Stats, AI)
- Flag decoders for human-readable display
- Movement type detection (Idle, Random, Waypoint, Chasing, etc.)
- Waypoint path data retrieval

**Methods Added:**
```lua
creature:GetWaypointPathData()       -- Returns Lua table with:
                                     --   pathId, nodeCount, moveType, currentNodeId
                                     --   nodes[] = {id, x, y, z, delay}
creature:GetMovementType()           -- Already existed - returns current MotionMaster type
creature:GetDefaultMovementType()    -- Already existed - returns spawn movement type
creature:GetCurrentWaypointId()      -- Already existed - returns current waypoint
```

### Phase 3: Future Enhancements
- Add GetDamage() for min/max damage values
- Add GetAttackPower() 
- Add spell list with cooldowns
- Add loot table information (if accessible)

---

## Next Steps

1. ✅ Create server handler with available data
2. ✅ Test AMS integration  
3. ✅ Add custom Eluna methods for combat stats
4. ✅ Add CreatureTemplate access method
5. ✅ Update UI to display all data
6. ⏳ Add more combat stats (damage, attack power) if needed
7. ⏳ Add loot/gold information

---

## Notes

- Creature object is available in world via `ObjectAccessor::GetCreature(map, guid)`
- GUID from client may need parsing (contains map ID, instance ID, etc.)
- Consider caching template data (doesn't change per instance)
- Some stats may be modified by auras/buffs - clarify if we want base or current values
- **Creatures don't have base stats (STR/AGI/etc)** - health/damage comes from template tables

### Movement System Notes

**Movement Types (from creature spawn data):**
- 0 = Idle (stationary)
- 1 = Random (wanders within `wander_radius`)
- 2 = Waypoint (follows defined path)

**Movement Generator Types (from MotionMaster - current movement):**
```cpp
IDLE_MOTION_TYPE      = 0   // Standing still
RANDOM_MOTION_TYPE    = 1   // Wandering randomly
WAYPOINT_MOTION_TYPE  = 2   // Following waypoint path
CONFUSED_MOTION_TYPE  = 4   // Confused/feared
CHASE_MOTION_TYPE     = 5   // Chasing target
FLEEING_MOTION_TYPE   = 6   // Running away
DISTRACTED_MOTION_TYPE= 7   // Distracted
FOLLOW_MOTION_TYPE    = 8   // Following
```

**Key Insight:** `wander_radius=0` does NOT mean the creature won't move! If `MovementType=2` (Waypoint), it will follow its waypoint path. The wander radius only applies to Random movement.

### Flag System Notes

**Flag Locations:**
| Flag Type | Header File | Purpose |
|-----------|-------------|---------|
| `UnitFlags` | `UnitDefines.h` | Combat behaviors (immune, pacified, etc.) |
| `UnitFlags2` | `UnitDefines.h` | Extended combat flags |
| `UnitFlags3` | `UnitDefines.h` | More extended flags |
| `NPCFlags` | `UnitDefines.h` | NPC services (vendor, trainer, etc.) |
| `NPCFlags2` | `UnitDefines.h` | Extended NPC services |
| `CreatureFlagsExtra` | `CreatureData.h` | Custom creature properties (guard, no XP, etc.) |

**Useful Flags for Display:**
- `UNIT_NPC_FLAG_VENDOR` (0x80) → "Vendor"
- `UNIT_NPC_FLAG_QUESTGIVER` (0x02) → "Quest Giver"
- `UNIT_FLAG_IMMUNE_TO_NPC` (0x200) → "Immune to NPC"
- `CREATURE_FLAG_EXTRA_GUARD` (0x8000) → "Guard"
- `CREATURE_FLAG_EXTRA_NO_XP` (0x40) → "No XP"
