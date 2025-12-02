# YAML Schema Reference

This document defines the YAML schemas for all content types in the Araxia Content System.

## Design Principles

1. **Human-Readable** - Prefer names over IDs where possible
2. **Hierarchical** - Nested structures for complex relationships
3. **Defaults** - Sensible defaults reduce boilerplate
4. **References** - Link to other content via `$ref` or named IDs
5. **Comments** - Support inline documentation

## Schema Version

```yaml
_schema: araxia-content/v1
_engine: trinitycore  # or azerothcore, universal
```

---

## Creature Template Schema

Defines the base template for a creature type.

```yaml
# creatures/templates/scarlet_champion.yml
_schema: araxia-content/v1
_type: creature_template

creature_template:
  # === IDENTITY ===
  entry: 100001                    # Unique ID (use 100000+ for custom)
  name: "Scarlet Champion"
  subname: "Crusade Elite"         # Optional subtitle
  
  # === LEVEL & STATS ===
  level:
    min: 30
    max: 32
  health:
    base: 5000
    modifier: 1.0                  # Multiplier for scaling
  mana:
    base: 2000
  
  # === CLASSIFICATION ===
  type: humanoid                   # beast, demon, dragonkin, elemental, giant, humanoid, mechanical, undead
  family: ~                        # For beasts: cat, wolf, etc.
  rank: elite                      # normal, elite, rare_elite, boss, rare
  
  # === FACTION ===
  faction: scarlet_crusade         # Named faction or ID
  # Alternative: faction_id: 67
  
  # === APPEARANCE ===
  model:
    display_id: 3241
    scale: 1.0
    # Alternative: multiple models
    # display_ids: [3241, 3242, 3243]
  
  # === COMBAT ===
  combat:
    damage:
      min: 80
      max: 120
    attack_power: 100
    ranged_attack_power: 0
    armor: 1500
    resistance:
      holy: 0
      fire: 0
      nature: 0
      frost: 0
      shadow: 0
      arcane: 0
    attack_speed:
      base: 2000                   # milliseconds
      ranged: 2000
    damage_school: physical        # physical, holy, fire, nature, frost, shadow, arcane
  
  # === MOVEMENT ===
  movement:
    speed_walk: 1.0
    speed_run: 1.14
    type: ground                   # ground, fly, swim
  
  # === AI & BEHAVIOR ===
  ai:
    script: ~                      # SmartAI script name or custom
    aggro_range: 20
    call_for_help_range: 10
    leash_range: 40
    flags:
      - civilian: false
      - immune_to_pc: false
      - immune_to_npc: false
      - can_swim: false
      - can_fly: false
      - no_combat: false
  
  # === ABILITIES ===
  abilities:
    - spell: crusader_strike       # Reference to spell definition
      cooldown: 8s
      priority: 1
      
    - spell: divine_shield
      cooldown: 60s
      trigger:
        type: health_below
        value: 20%
      
    - spell: battle_shout
      cooldown: 30s
      trigger:
        type: on_aggro
  
  # === LOOT ===
  loot:
    table: scarlet_elite_loot      # Reference to loot table
    # Or inline:
    # drops:
    #   - item: scarlet_tabard
    #     chance: 15%
    gold:
      min: 50
      max: 100
    skinnable: false
    mining: false
    herbalism: false
  
  # === VENDOR/TRAINER (Optional) ===
  vendor:
    enabled: false
    items: []
  
  trainer:
    enabled: false
    class: ~
    spells: []
  
  # === QUEST GIVER (Optional) ===
  quest_giver:
    enabled: false
    quests: []
  
  # === FLAGS ===
  flags:
    gossip: false
    quest_giver: false
    vendor: false
    trainer: false
    repair: false
    flight_master: false
    innkeeper: false
    banker: false
    auctioneer: false
  
  # === METADATA ===
  _meta:
    author: "Araxia Team"
    version: "1.0.0"
    tags: [scarlet, crusade, dungeon]
    notes: "Elite guard for Scarlet Monastery"
```

---

## Spawn Schema

Defines where creatures spawn in the world.

```yaml
# creatures/spawns/scarlet_monastery_guards.yml
_schema: araxia-content/v1
_type: spawn

spawns:
  - creature: scarlet_champion     # Reference to creature_template
    # Alternative: entry: 100001
    
    location:
      map: eastern_kingdoms        # Named map or ID
      # map_id: 0
      zone: tirisfal_glades
      area: scarlet_monastery
      position:
        x: 2898.65
        y: -802.93
        z: 160.33
        orientation: 1.35          # Facing direction (radians)
    
    spawn_config:
      spawn_time: 300              # Seconds to respawn
      wander_distance: 5           # Yards
      movement_type: random        # stay, random, waypoint
      
    # Optional: Group spawns together
    group: cathedral_entrance
    
    # Optional: Conditions
    conditions:
      - type: event_active
        event: darkmoon_faire
      - type: phase
        phase: 1
    
    # Optional: Pool spawning (random selection)
    pool:
      id: scarlet_rares
      chance: 10%

  # Multiple spawns in one file
  - creature: scarlet_wizard
    location:
      map_id: 0
      position: [2900.0, -805.0, 160.5, 0.0]  # Shorthand
    spawn_config:
      spawn_time: 300
```

---

## Encounter Schema

Defines boss encounters with phases and mechanics.

```yaml
# encounters/scarlet_monastery/whitemane.yml
_schema: araxia-content/v1
_type: encounter

encounter:
  # === IDENTITY ===
  id: sm_whitemane
  name: "High Inquisitor Whitemane"
  instance: scarlet_monastery_cathedral
  
  # === BOSSES ===
  bosses:
    - entry: mograine              # Reference or ID
      role: primary
      spawn_position: [x, y, z, o]
      
    - entry: whitemane
      role: secondary
      spawn_position: [x, y, z, o]
      initial_state: inactive      # Praying at altar
  
  # === PHASES ===
  phases:
    - id: phase_1
      name: "Mograine's Stand"
      
      # Phase ends when:
      until:
        type: boss_health
        boss: mograine
        value: 0%
      
      # Active mechanics in this phase
      mechanics:
        - boss: mograine
          abilities:
            - crusader_strike:
                cooldown: 6s
            - divine_shield:
                trigger: health_below(30%)
            - lay_on_hands:
                trigger: health_below(10%)
                once: true
        
        - boss: whitemane
          state: inactive
          script: |
            -- Whitemane prays at the altar
            whitemane:SetReactState(REACT_PASSIVE)
    
    - id: resurrection
      name: "Resurrection"
      
      trigger:
        type: boss_dead
        boss: mograine
      
      script: |
        -- Whitemane runs to Mograine's body
        whitemane:SetReactState(REACT_AGGRESSIVE)
        whitemane:Yell("Arise, my champion!")
        whitemane:MoveTo(mograine.position)
        whitemane:CastSpell(SCARLET_RESURRECTION, mograine)
        mograine:Resurrect(100)  -- Full health
      
      duration: 5s               # Script duration before next phase
    
    - id: phase_2
      name: "Final Stand"
      
      # Both bosses active
      mechanics:
        - boss: whitemane
          abilities:
            - holy_smite:
                cooldown: 3s
                target: current
            - deep_sleep:
                cooldown: 30s
                target: random
            - heal:
                cooldown: 15s
                target: lowest_health_ally
        
        - boss: mograine
          abilities:
            - crusader_strike:
                cooldown: 4s
            - retribution_aura:
                on_phase_start: true
          modifiers:
            - type: damage_increase
              value: 25%
            - type: attack_speed
              value: 20%
  
  # === ADDS ===
  adds: []  # No adds in this encounter
  
  # === ENVIRONMENTAL ===
  environment:
    area_auras: []
    ground_effects: []
  
  # === LOOT ===
  loot:
    mode: all_bosses_dead          # When loot spawns
    chest_position: [x, y, z]      # Optional loot chest
    
    drops:
      - item: whitemane_chapeau
        chance: 15%
        
      - item: mograine_might
        chance: 10%
        boss: mograine
        
      - item: triune_amulet
        chance: 8%
        boss: whitemane
        
      - item: scarlet_leggings
        chance: 20%
  
  # === ACHIEVEMENTS ===
  achievements:
    - id: sm_speed_kill
      name: "Rapid Judgment"
      condition: time_under(5m)
      
    - id: sm_no_sleep
      name: "Wide Awake"
      condition: no_player_hit(deep_sleep)
  
  # === METADATA ===
  _meta:
    author: "Araxia Team"
    version: "1.0.0"
    difficulty: normal
    tuning:
      target_ilvl: 30
      target_group_size: 5
```

---

## Instance Schema

Defines a complete dungeon or raid.

```yaml
# instances/scarlet_monastery_cathedral.yml
_schema: araxia-content/v1
_type: instance

instance:
  # === IDENTITY ===
  id: sm_cathedral
  name: "Scarlet Monastery: Cathedral"
  map_id: 189
  
  # === REQUIREMENTS ===
  requirements:
    level:
      min: 28
      max: 45
      heroic_min: 70
    players:
      min: 1
      max: 5
    item_level:
      normal: ~
      heroic: 180
  
  # === ENTRANCE ===
  entrance:
    map: eastern_kingdoms
    position: [2870.0, -820.0, 160.0]
    exit_position: [2870.0, -820.0, 160.0]
  
  # === TRASH PACKS ===
  trash_packs:
    - id: entrance_guards
      spawn_on: instance_create
      creatures:
        - template: scarlet_champion
          count: 2
          positions:
            - [2898.0, -802.0, 160.3, 1.35]
            - [2895.0, -805.0, 160.3, 1.35]
        - template: scarlet_wizard
          count: 1
          position: [2893.0, -800.0, 160.3, 1.35]
      
      patrol:
        path: entrance_patrol
        speed: walk
        repeat: true
      
      # Link to next pack (pulls together)
      linked_packs: []
    
    - id: hallway_pack_1
      spawn_on: instance_create
      creatures:
        - template: scarlet_defender
          count: 3
        - template: scarlet_myrmidon
          count: 2
      
      # Random patrol from waypoint set
      patrol:
        path: hallway_patrol_set
        random: true
  
  # === BOSSES ===
  bosses:
    - encounter: sm_whitemane
      position: [altar_x, altar_y, altar_z]
      required: true               # Must kill for completion
  
  # === EVENTS ===
  events:
    - trigger: all_bosses_dead
      actions:
        - type: open_portal
          position: [exit_x, exit_y, exit_z]
          destination: entrance
        
        - type: spawn_chest
          loot_table: sm_cathedral_final
          position: [chest_x, chest_y, chest_z]
    
    - trigger: wipe
      actions:
        - type: reset_instance
          delay: 30s
  
  # === LOCKOUT ===
  lockout:
    type: weekly                   # daily, weekly, none
    shared: false                  # Share lockout with other difficulties?
  
  # === DIFFICULTIES ===
  difficulties:
    normal:
      health_modifier: 1.0
      damage_modifier: 1.0
      loot_modifier: 1.0
    
    heroic:
      health_modifier: 2.5
      damage_modifier: 1.8
      loot_modifier: 1.5
      extra_mechanics: true
    
    mythic_plus:
      enabled: true
      affixes: true
      scaling: true
  
  # === METADATA ===
  _meta:
    author: "Araxia Team"
    version: "1.0.0"
    original_source: retail_classic
```

---

## Loot Table Schema

```yaml
# loot/scarlet_elite_loot.yml
_schema: araxia-content/v1
_type: loot_table

loot_table:
  id: scarlet_elite_loot
  
  # Gold drop
  gold:
    min: 50
    max: 100
  
  # Guaranteed drops
  always:
    - item: scarlet_insignia
      count: 1
  
  # Chance drops
  drops:
    - item: scarlet_tabard
      chance: 5%
      
    - item: sword_of_omen
      chance: 2%
      
    - item: mail_gloves
      chance: 15%
      
  # Reference another table for additional drops
  includes:
    - table: generic_cloth_loot
      chance: 30%
      
    - table: generic_green_weapons
      chance: 10%
  
  # Skinning/Mining/Herbalism
  gathering:
    skinning:
      item: rugged_leather
      count: [1, 3]
```

---

## Patrol/Waypoint Schema

```yaml
# patrols/cathedral_entrance.yml
_schema: araxia-content/v1
_type: patrol

patrol:
  id: entrance_patrol
  
  waypoints:
    - position: [2898.0, -802.0, 160.3]
      wait: 5s
      action: ~
      
    - position: [2895.0, -810.0, 160.3]
      wait: 0
      
    - position: [2890.0, -815.0, 160.3]
      wait: 10s
      action: |
        creature:Emote(EMOTE_SALUTE)
      
    - position: [2895.0, -810.0, 160.3]
      wait: 0
  
  config:
    repeat: true
    speed: walk                    # walk, run
    path_type: linear              # linear, catmull_rom
```

---

## ID Ranges

To avoid conflicts with existing content:

| Range | Purpose |
|-------|---------|
| 1-99999 | Blizzard/Core content |
| 100000-199999 | Araxia custom creatures |
| 200000-299999 | Araxia custom items |
| 300000-399999 | Araxia custom spells |
| 400000-499999 | Araxia custom quests |
| 500000+ | Reserved for future use |

---

## Type Reference

### Creature Types
`beast`, `dragonkin`, `demon`, `elemental`, `giant`, `undead`, `humanoid`, `critter`, `mechanical`, `not_specified`, `totem`, `non_combat_pet`, `gas_cloud`

### Creature Ranks
`normal`, `elite`, `rare_elite`, `boss`, `rare`

### Movement Types
`stay`, `random`, `waypoint`

### Damage Schools
`physical`, `holy`, `fire`, `nature`, `frost`, `shadow`, `arcane`
