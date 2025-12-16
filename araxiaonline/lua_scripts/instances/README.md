# Instance Boss Encounter Scripts

This directory contains Eluna Lua scripts for instance boss encounters.

## Directory Structure

```
instances/
├── README.md
├── cataclysm/
│   ├── vortex_pinnacle/
│   │   ├── instance.lua         # Instance-wide handlers
│   │   ├── boss_ertan.lua       # Grand Vizier Ertan
│   │   ├── boss_altairus.lua    # Altairus
│   │   └── boss_asaad.lua       # Asaad
│   ├── blackrock_caverns/
│   ├── throne_of_tides/
│   └── ...
├── wrath/
│   ├── utgarde_keep/
│   └── ...
├── burning_crusade/
└── ...
```

## Purpose

These scripts compensate for C++ AI scripts that are not available from repack database exports. They provide:

1. **Boss encounter completion** - Trigger instance state changes when bosses die
2. **Basic mechanics** - Simple spell timers and phase transitions via SmartAI-like logic
3. **Event triggers** - Spawn groups, doors, cinematics

## Creating Boss Scripts

### Template

```lua
-- boss_example.lua
local BOSS_ENTRY = 12345
local MAP_ID = 657

-- CREATURE_EVENT_ON_JUST_DIED = 4
RegisterCreatureEvent(BOSS_ENTRY, 4, function(event, creature, killer)
    print("[Instance] Boss killed")
    -- Add encounter completion logic here
end)

-- CREATURE_EVENT_ON_ENTER_COMBAT = 1
RegisterCreatureEvent(BOSS_ENTRY, 1, function(event, creature, target)
    print("[Instance] Boss engaged")
    -- Add combat start logic here
end)

print("[Eluna] Boss script loaded: " .. BOSS_ENTRY)
```

### Creature Events

| Event ID | Name | Description |
|----------|------|-------------|
| 1 | ON_ENTER_COMBAT | Creature enters combat |
| 2 | ON_LEAVE_COMBAT | Creature leaves combat |
| 3 | ON_TARGET_DIED | Creature kills a target |
| 4 | ON_DIED | Creature dies |
| 5 | ON_SPAWN | Creature spawns |
| 6 | ON_REACH_WP | Creature reaches waypoint |
| 7 | ON_AI_UPDATE | Called every ~1 second |

## Integration with Import Pipeline

When importing instance content:

1. Check if TC has C++ scripts for the bosses
2. If not, create placeholder Eluna scripts
3. Add to `instances/<expansion>/<instance>/` directory
4. Test and iterate on mechanics

## Related Documentation

- `/opt/trinitycore/araxia-trinity-content/docs/repack_cpp_ai_gap.md`
- Eluna API: https://elunaluaengine.github.io/
