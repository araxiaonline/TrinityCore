# Cross-Engine Compatibility

## Vision

Create a universal content format that allows sharing content between TrinityCore and AzerothCore (and potentially other emulators).

**This could be game-changing for the private server community** - imagine a content marketplace where creators share encounters, dungeons, and world content that works across engines!

## Engine Comparison

### TrinityCore vs AzerothCore

| Feature | TrinityCore | AzerothCore | Compatibility |
|---------|-------------|-------------|---------------|
| Core Version | 11.x (Midnight) | 3.3.5 (WotLK) | ⚠️ Different |
| creature_template | Yes | Yes | ✅ Similar |
| smart_scripts | Yes | Yes | ✅ Compatible |
| creature (spawns) | Yes | Yes | ✅ Similar |
| instance_template | Yes | Yes | ✅ Similar |
| Maps 0-1 | Eastern Kingdoms, Kalimdor | Same | ✅ Match |
| Maps 530+ | Outland, Northrend | Same | ✅ Match |
| Maps 2000+ | Modern content | N/A | ❌ TC Only |

### Schema Differences

```yaml
# TrinityCore creature_template
creature_template:
  - entry
  - name
  - subname
  - minlevel / maxlevel  # Different in 11.x!
  - HealthModifier
  - DamageModifier
  - BaseAttackTime
  - unit_class
  - unit_flags
  
# AzerothCore creature_template  
creature_template:
  - entry
  - name
  - subname
  - minlevel / maxlevel
  - HealthModifier
  - DamageModifier
  - BaseAttackTime
  - unit_class
  - unit_flags
  # Mostly compatible!
```

## Translation Strategy

### 1. Abstract Content Layer (YAML)

Our YAML format is engine-agnostic. Engine-specific details are handled by adapters.

```yaml
# Universal YAML
creature_template:
  name: "Guard Captain"
  level:
    min: 30
    max: 32
  health:
    base: 5000
  
# Adapter converts to engine-specific SQL
```

### 2. Engine Adapters

```python
class EngineAdapter:
    def export_creature(yaml_data) -> sql
    def import_creature(db_row) -> yaml_data

class TrinityAdapter(EngineAdapter):
    def export_creature(yaml_data):
        # Handle TC-specific columns
        # minlevel/maxlevel handling for 11.x
        
class AzerothAdapter(EngineAdapter):
    def export_creature(yaml_data):
        # Handle AC-specific columns
```

### 3. ID Translation Tables

Since engines may use different IDs for the same content:

```yaml
# translation/faction_ids.yml
factions:
  stormwind:
    trinitycore: 11
    azerothcore: 11
    
  scarlet_crusade:
    trinitycore: 67
    azerothcore: 67
    
  # Custom factions need mapping
  araxia_faction_1:
    trinitycore: 100001
    azerothcore: 100001  # Reserve same range
```

### 4. Map Compatibility Matrix

```yaml
# translation/maps.yml
maps:
  eastern_kingdoms:
    id: 0
    compatible: [trinitycore, azerothcore]
    
  kalimdor:
    id: 1
    compatible: [trinitycore, azerothcore]
    
  outland:
    id: 530
    compatible: [trinitycore, azerothcore]
    
  northrend:
    id: 571
    compatible: [trinitycore, azerothcore]
    
  # Modern content - TC only
  broken_isles:
    id: 1220
    compatible: [trinitycore]
    note: "Legion content, not available in AzerothCore"
```

## Import/Export Flow

### Export from AzerothCore → YAML

```bash
# On AzerothCore server
araxia-content export --engine azerothcore \
    --type instance \
    --id 189 \
    --output scarlet_monastery.yml
```

### Import YAML → TrinityCore

```bash
# On TrinityCore server
araxia-content import --engine trinitycore \
    --translate-ids \
    scarlet_monastery.yml
```

### Diff Between Engines

```bash
# Compare same content on different engines
araxia-content diff \
    --source-engine azerothcore \
    --target-engine trinitycore \
    content/
```

## Compatibility Levels

### Level 1: Full Compatibility ✅
- Maps 0, 1, 530, 571 (classic through WotLK)
- Basic creature_template fields
- Basic spawn data
- SmartAI scripts

### Level 2: Translatable ⚠️
- Faction IDs (need mapping)
- Display IDs (might differ)
- Spell IDs (core spells match, custom differ)

### Level 3: Engine-Specific ❌
- Modern maps (TC 4.x+)
- Engine-specific features
- Custom C++ scripts

## Content Sharing Vision

### Araxia Content Repository

```
araxia-content-repo/
├── dungeons/
│   ├── scarlet_monastery/
│   │   ├── cathedral.yml
│   │   ├── library.yml
│   │   └── graveyard.yml
│   └── deadmines/
│       └── deadmines.yml
├── raids/
│   └── molten_core/
│       └── molten_core.yml
├── world/
│   └── elwynn_forest/
│       └── quests.yml
└── compatibility.yml          # Engine support matrix
```

### Package Format

```yaml
# package.yml
name: "Scarlet Monastery Remastered"
version: "2.0.0"
author: "Araxia Team"
license: "MIT"

compatibility:
  trinitycore: ">=11.0.0"
  azerothcore: ">=3.3.5"

contents:
  - instances/scarlet_monastery/*
  - creatures/scarlet/*
  - loot/scarlet_loot.yml

dependencies:
  - name: "araxia-base-content"
    version: ">=1.0.0"
```

## Implementation Roadmap

### Phase 1: Single Engine (Current)
- YAML schemas for TrinityCore
- Import/export for TC only
- Establish format standards

### Phase 2: AzerothCore Support
- Create AC adapter
- Build translation tables
- Test bidirectional import/export

### Phase 3: Content Marketplace
- Git-based content repository
- Version control for content
- Dependency management

### Phase 4: Community Sharing
- Public content packages
- Rating/review system
- Automated compatibility testing

## Technical Challenges

### 1. ID Conflicts
Custom content might use same IDs on different engines.

**Solution:** Reserved ID ranges + translation tables

### 2. Spell/Ability Differences
Same spell might have different effects or not exist.

**Solution:** Abstract ability definitions, engine-specific implementations

### 3. Script Compatibility
Lua/SmartAI scripts might use engine-specific functions.

**Solution:** Abstract script layer with engine adapters

### 4. Database Schema Changes
Engines evolve, schemas change.

**Solution:** Versioned adapters per engine version

## Example: Exporting from AzerothCore

```sql
-- AzerothCore creature_template
SELECT * FROM creature_template WHERE entry = 3977;

-- Result:
-- entry: 3977
-- name: "High Inquisitor Whitemane"
-- minlevel: 42
-- maxlevel: 42
-- ...
```

```yaml
# Exported YAML (universal format)
creature_template:
  entry: 3977
  name: "High Inquisitor Whitemane"
  level:
    min: 42
    max: 42
  _source:
    engine: azerothcore
    version: 3.3.5
    exported: 2025-12-01
```

```sql
-- Imported to TrinityCore (11.x format)
INSERT INTO creature_template ...
-- Adapter handles schema differences
```

## Contributing

We welcome contributions from both TrinityCore and AzerothCore communities!

- **Schema proposals:** Discuss in GitHub issues
- **Adapter implementations:** PRs welcome
- **Translation tables:** Help us map IDs
- **Content packages:** Share your creations!
