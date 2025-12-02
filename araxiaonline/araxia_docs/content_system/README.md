# Araxia Content System

## Overview

A YAML-based content definition system for creating, versioning, and sharing game content across WoW private server engines.

**Status:** 🚧 Phase 2 - In Development

## Goals

1. **Human-Readable Content** - Define creatures, encounters, dungeons in YAML, not SQL
2. **Version Control Friendly** - Git-trackable content that merges cleanly
3. **Import/Export** - Two-way sync between YAML and database
4. **Diff/Patch** - Compare content versions, generate migrations
5. **Cross-Engine Portability** - Content that works on TrinityCore AND AzerothCore
6. **AI-Assisted Creation** - Scarlet Seer can generate and modify content

## Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        CONTENT SYSTEM ARCHITECTURE                       │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌──────────────┐     ┌──────────────┐     ┌──────────────────────────┐ │
│  │ External     │     │ YAML Content │     │ Content Processor        │ │
│  │ Sources      │────▶│ Definitions  │────▶│ (Import/Export/Diff)     │ │
│  │ (Wowhead)    │     │ (Git VCS)    │     │                          │ │
│  └──────────────┘     └──────────────┘     └────────────┬─────────────┘ │
│                                                          │               │
│  ┌──────────────┐     ┌──────────────┐     ┌────────────▼─────────────┐ │
│  │ AI Assistant │     │ Engine       │     │ Database                 │ │
│  │ (Scarlet     │◀───▶│ Adapters     │◀───▶│ (TrinityCore/AzerothCore)│ │
│  │  Seer)       │     │              │     │                          │ │
│  └──────────────┘     └──────────────┘     └──────────────────────────┘ │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

## Components

| Component | Description | Status |
|-----------|-------------|--------|
| [YAML Schemas](./schemas/) | Content type definitions | 🚧 In Progress |
| [Importer](./importer/) | YAML → Database | ⏳ Planned |
| [Exporter](./exporter/) | Database → YAML | ⏳ Planned |
| [Differ](./differ/) | Compare & generate patches | ⏳ Planned |
| [Engine Adapters](./adapters/) | TrinityCore & AzerothCore support | ⏳ Planned |
| [CLI Tool](./cli/) | Command-line interface | ⏳ Planned |

## Content Types

### Core Types
- **Creature** - NPCs, mobs, bosses
- **Creature Template** - Base creature definitions
- **Spawn** - World spawn points
- **Loot Table** - Item drops
- **Ability/Spell** - Custom abilities

### Encounter Types
- **Encounter** - Boss fight definitions
- **Phase** - Encounter phases
- **Script** - Scripted events

### Instance Types
- **Instance** - Dungeon/Raid definitions
- **Trash Pack** - Grouped spawns
- **Patrol** - Waypoint paths

### World Types
- **Zone** - Area definitions
- **Quest** - Quest chains
- **Event** - World events

## Cross-Engine Compatibility

A key goal is content portability between TrinityCore and AzerothCore.

### Strategy

1. **Abstract YAML Layer** - Engine-agnostic content definitions
2. **Engine Adapters** - Translate to engine-specific SQL/schemas
3. **Map Compatibility Matrix** - Track which maps match between engines
4. **Translation Tables** - Map IDs, entry numbers, faction IDs, etc.

### Compatibility Levels

| Level | Description |
|-------|-------------|
| **Full** | Maps, IDs, and schemas match exactly |
| **Translatable** | Can convert with ID mapping |
| **Partial** | Some features not supported |
| **Incompatible** | Cannot be ported |

See [CROSS_ENGINE.md](./CROSS_ENGINE.md) for detailed compatibility notes.

## Quick Start

```bash
# Import content to database
araxia-content import creatures/scarlet_champion.yml

# Export existing creature to YAML
araxia-content export creature 3977 > whitemane.yml

# Diff two versions
araxia-content diff v1/dungeon.yml v2/dungeon.yml

# Generate migration SQL
araxia-content migrate old/ new/ > migration.sql
```

## Directory Structure

```
araxiaonline/content/
├── creatures/           # Creature definitions
│   ├── templates/       # Base templates
│   └── spawns/          # Spawn configurations
├── encounters/          # Boss encounters
├── instances/           # Dungeon/raid definitions
├── loot/                # Loot tables
├── quests/              # Quest definitions
└── world/               # World content
```

## Documentation

- [Schema Reference](./schemas/SCHEMA_REFERENCE.md)
- [Importer Guide](./importer/IMPORTER_GUIDE.md)
- [Cross-Engine Compatibility](./CROSS_ENGINE.md)
- [AI Integration](./AI_INTEGRATION.md)
