# Content Importer Design

## Overview

The Content Importer reads YAML content definitions and writes them to the game database. It supports:

- **Import:** YAML → Database
- **Export:** Database → YAML
- **Diff:** Compare two YAML files or YAML vs Database
- **Migrate:** Generate SQL migration scripts

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        IMPORTER PIPELINE                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────────┐  │
│  │ YAML     │──▶│ Parser   │──▶│ Validator│──▶│ Transformer  │  │
│  │ Files    │   │          │   │          │   │              │  │
│  └──────────┘   └──────────┘   └──────────┘   └──────┬───────┘  │
│                                                       │          │
│  ┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────▼───────┐  │
│  │ Result   │◀──│ Executor │◀──│ Resolver │◀──│ Engine       │  │
│  │ Report   │   │          │   │          │   │ Adapter      │  │
│  └──────────┘   └──────────┘   └──────────┘   └──────────────┘  │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

## Components

### 1. YAML Parser
Reads YAML files and produces structured data.

```lua
-- Lua implementation using lyaml or similar
local parser = require("araxia.content.parser")

local content = parser.parse_file("creatures/scarlet_champion.yml")
-- Returns Lua table matching YAML structure
```

### 2. Validator
Ensures content matches schema and has required fields.

```lua
local validator = require("araxia.content.validator")

local errors = validator.validate(content, "creature_template")
if #errors > 0 then
    for _, err in ipairs(errors) do
        print("Validation error:", err.path, err.message)
    end
end
```

**Validations:**
- Required fields present
- Types match schema
- References exist (e.g., faction name → faction ID)
- ID ranges valid (custom content uses 100000+)

### 3. Transformer
Converts universal YAML format to engine-specific format.

```lua
local transformer = require("araxia.content.transformer")

-- Universal format
local universal = {
    level = { min = 30, max = 32 }
}

-- TrinityCore 11.x format
local tc = transformer.to_trinitycore(universal)
-- { minlevel = 30, maxlevel = 32 }

-- AzerothCore format  
local ac = transformer.to_azerothcore(universal)
-- { minlevel = 30, maxlevel = 32 }
```

### 4. Reference Resolver
Resolves named references to IDs.

```lua
local resolver = require("araxia.content.resolver")

-- Resolve faction name to ID
local faction_id = resolver.resolve("faction", "scarlet_crusade")
-- Returns: 67

-- Resolve creature reference
local entry = resolver.resolve("creature", "scarlet_champion")
-- Returns: 100001
```

**Resolution Sources:**
1. Built-in mappings (Blizzard factions, races, etc.)
2. Custom definitions in project
3. Database lookup (for existing content)

### 5. Engine Adapter
Generates engine-specific SQL.

```lua
local adapter = require("araxia.content.adapters.trinitycore")

local sql = adapter.generate_insert("creature_template", transformed_data)
-- INSERT INTO creature_template (entry, name, ...) VALUES (100001, 'Scarlet Champion', ...);
```

### 6. Executor
Runs SQL against database (or generates file).

```lua
local executor = require("araxia.content.executor")

-- Direct execution
executor.execute(sql, { database = "world" })

-- Or generate file
executor.to_file(sql, "output/migration.sql")
```

## CLI Interface

```bash
# Import single file
araxia-content import creatures/scarlet_champion.yml

# Import directory
araxia-content import creatures/

# Import with options
araxia-content import \
    --engine trinitycore \
    --database world \
    --dry-run \
    --verbose \
    content/

# Export creature to YAML
araxia-content export creature 3977 --output whitemane.yml

# Export instance
araxia-content export instance 189 --output scarlet_monastery/

# Diff two YAML files
araxia-content diff v1/creature.yml v2/creature.yml

# Diff YAML vs database
araxia-content diff creature.yml --database

# Generate migration
araxia-content migrate old/ new/ --output migration.sql
```

## Lua API

```lua
local ContentSystem = require("araxia.content")

-- Import from YAML
local result = ContentSystem.import({
    file = "creatures/scarlet_champion.yml",
    engine = "trinitycore",
    dry_run = false
})

print("Imported:", result.created, "created,", result.updated, "updated")

-- Export to YAML
ContentSystem.export({
    type = "creature_template",
    entry = 3977,
    output = "exports/whitemane.yml"
})

-- Diff
local changes = ContentSystem.diff({
    source = "v1/creature.yml",
    target = "v2/creature.yml"
})

for _, change in ipairs(changes) do
    print(change.type, change.path, change.old, "->", change.new)
end
```

## MCP Integration

Expose content operations via MCP tools:

```lua
-- New MCP tools
sMCPServer.RegisterTool("content_import", function(params)
    return ContentSystem.import(params)
end)

sMCPServer.RegisterTool("content_export", function(params)
    return ContentSystem.export(params)
end)

sMCPServer.RegisterTool("content_diff", function(params)
    return ContentSystem.diff(params)
end)
```

**AI Usage:**
```
Me: "Let me import that creature you just defined..."
    *calls content_import with YAML data*
    "Done! Creature 100001 created in database."
```

## Diff Algorithm

### Comparing YAML Files

```yaml
# v1/creature.yml
creature_template:
  entry: 100001
  name: "Guard"
  health:
    base: 1000

# v2/creature.yml
creature_template:
  entry: 100001
  name: "Elite Guard"      # Changed
  health:
    base: 2000            # Changed
  armor: 500              # Added
```

```
Diff Result:
  MODIFIED: name: "Guard" -> "Elite Guard"
  MODIFIED: health.base: 1000 -> 2000
  ADDED: armor: 500
```

### Comparing YAML vs Database

```lua
local db_data = ContentSystem.export_to_memory({
    type = "creature_template",
    entry = 100001
})

local yaml_data = ContentSystem.parse("creature.yml")

local changes = ContentSystem.diff_objects(db_data, yaml_data)
```

## Migration Generation

Given diffs, generate SQL migrations:

```sql
-- Generated migration: creature_100001_update.sql
-- From: v1/creature.yml
-- To: v2/creature.yml
-- Generated: 2025-12-01 20:00:00

-- Update creature_template
UPDATE creature_template 
SET 
    name = 'Elite Guard',
    HealthModifier = 2.0
WHERE entry = 100001;

-- Add armor (if column exists, or via creature_template_addon)
-- Note: Some changes may require manual review
```

## Hot Reload

For development, support hot-reloading content:

```lua
-- Watch for file changes
ContentSystem.watch("content/", function(file, change_type)
    if change_type == "modified" then
        print("Reloading:", file)
        ContentSystem.import({ file = file, hot_reload = true })
        
        -- Respawn affected creatures
        ContentSystem.respawn_affected()
    end
end)
```

## Error Handling

```lua
local result = ContentSystem.import({ file = "creature.yml" })

if not result.success then
    for _, error in ipairs(result.errors) do
        print(string.format(
            "[%s] %s: %s (line %d)",
            error.severity,  -- ERROR, WARNING, INFO
            error.code,      -- MISSING_FIELD, INVALID_TYPE, etc.
            error.message,
            error.line or 0
        ))
    end
end
```

## Implementation Plan

### Phase 1: Core Parser & Validator
- [ ] YAML parser integration (lyaml or pure Lua)
- [ ] Schema definitions in code
- [ ] Validation logic
- [ ] Basic error reporting

### Phase 2: TrinityCore Adapter
- [ ] creature_template import/export
- [ ] creature (spawn) import/export
- [ ] loot_template import/export
- [ ] smart_scripts import/export

### Phase 3: Diff & Migration
- [ ] Object diff algorithm
- [ ] SQL generation
- [ ] Migration file output

### Phase 4: MCP Integration
- [ ] content_import tool
- [ ] content_export tool
- [ ] content_diff tool
- [ ] AI-assisted content creation

### Phase 5: AzerothCore Adapter
- [ ] Schema mapping
- [ ] Translation tables
- [ ] Bidirectional support

## Dependencies

- **lyaml** or **lua-yaml**: YAML parsing
- **Smallfolk**: Serialization (already have)
- **Eluna**: Lua runtime
- **MCP Server**: AI integration
