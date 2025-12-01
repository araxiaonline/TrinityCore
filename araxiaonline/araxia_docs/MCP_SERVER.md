# Araxia MCP Server

## Overview

The Araxia MCP Server embeds a Model Context Protocol server directly into the worldserver, enabling AI assistants (like Claude/Cascade) to interact with the game server in real-time.

**Status:** ✅ Phase 1, 2 & World Scan Complete (Nov 30, 2025)

**Codename:** "Scarlet Vision" 👁️ - The AI can now SEE inside the game world!

## 🎉 What This Enables

With this integration, the AI assistant can:
- **Query databases directly** - No more asking you to run SQL commands
- **See online players** - Know who's logged in and where they are
- **Read client messages** - Receive data from the WoW client via AMS bridge
- **Write to client** - Send messages that display in the WoW client
- **Debug in real-time** - Direct access to server state while you're playing
- **SEE your surroundings** - LIDAR-style spatial awareness using VMAP data!

## Features

- **Database Access**: ✅ Direct SQL queries to world, characters, and auth databases
- **Server Status**: ✅ Real-time server info, player lists, uptime
- **Shared Data Bridge**: ✅ Read/write ElunaSharedData (client ↔ MCP communication)
- **World Scan (LIDAR)**: ✅ Spatial awareness - see walls, creatures, room layout!
- **GM Commands**: ✅ Stub (needs ChatHandler integration)
- **Eluna Integration**: ⏳ (Phase 3) Execute Lua, inspect state, hot-reload
- **World Object Tools**: ⏳ (Phase 4) Creature/GO manipulation

## World Scan (LIDAR Vision)

The `world_scan` tool gives the AI spatial awareness by casting rays using vmaps:

```
+---------------------+
|   #######           |
|   #     ###         |
|   # S       #       |
|   #    @>   #       |  ← You facing East
|   #         #       |
|   #  D      #       |
|   ###########       |
+---------------------+
@ = You, # = Wall, S = Sentry, D = Disciple
```

**How it works:**
1. Casts 72 rays (every 5°) from player position
2. Uses VMAP data to detect walls/obstacles
3. Detects all creatures in range
4. Generates ASCII art visualization

**Usage:** AI calls `world_scan` tool directly - no player action needed!

## Configuration

Add to `worldserver.conf`:

```ini
###################################################################################################
# ARAXIA MCP SERVER
#
#    Araxia.MCP.Enable
#        Enable the embedded MCP server for AI assistant integration.
#        Default: 0 (disabled)
#
#    Araxia.MCP.Port
#        Port for the MCP HTTP server.
#        Default: 8765
#
#    Araxia.MCP.AuthToken
#        Bearer token for authentication. Leave empty for no auth (development only!).
#        Generate a secure token for production use.
#        Default: "" (no auth)
#
#    Araxia.MCP.AllowRemote
#        Allow connections from non-localhost addresses.
#        WARNING: Only enable if you understand the security implications!
#        Default: 0 (localhost only)
#
###################################################################################################

Araxia.MCP.Enable = 1
Araxia.MCP.Port = 8765
Araxia.MCP.AuthToken = "your-secret-token-here"
Araxia.MCP.AllowRemote = 0
```

## Security

1. **Localhost Only**: By default, only accepts connections from 127.0.0.1
2. **Bearer Token**: Requires `Authorization: Bearer <token>` header
3. **Query Safety**: SELECT-only for `db_query`, DDL blocked for `db_execute`
4. **Audit Logging**: All database modifications are logged

## MCP Tools

### Database Tools

| Tool | Description |
|------|-------------|
| `db_query` | Execute SELECT queries |
| `db_execute` | Execute INSERT/UPDATE/DELETE with audit log |
| `db_tables` | List all tables in a database |
| `db_describe` | Get table schema |

### Server Tools

| Tool | Description |
|------|-------------|
| `server_info` | Get server status, uptime, player count |
| `player_list` | List all online players |
| `gm_command` | Execute GM commands |
| `reload_scripts` | Reload Eluna scripts |

## Usage Examples

### Windsurf/Cursor MCP Config

Add to your MCP configuration:

```json
{
  "mcpServers": {
    "araxia-worldserver": {
      "url": "http://localhost:8765/mcp",
      "headers": {
        "Authorization": "Bearer your-secret-token-here"
      }
    }
  }
}
```

### Direct API Calls

```bash
# Health check
curl http://localhost:8765/health

# List tools
curl -X POST http://localhost:8765/mcp \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer your-token" \
  -d '{"jsonrpc":"2.0","method":"tools/list","id":1}'

# Query database
curl -X POST http://localhost:8765/mcp \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer your-token" \
  -d '{
    "jsonrpc":"2.0",
    "method":"tools/call",
    "params":{
      "name":"db_query",
      "arguments":{
        "database":"world",
        "query":"SELECT guid, wander_distance FROM creature WHERE id = 4283 LIMIT 5"
      }
    },
    "id":2
  }'
```

## Troubleshooting

### Server won't start
- Check `Araxia.MCP.Enable = 1` in config
- Check port isn't already in use
- Look for `[MCP]` messages in Server.log

### Authentication failing
- Ensure `Authorization: Bearer <token>` header is set
- Token must match `Araxia.MCP.AuthToken` exactly

### Can't connect remotely
- Set `Araxia.MCP.AllowRemote = 1`
- Ensure firewall allows port 8765

## Implementation Notes

### Dependencies (Header-Only)
- **cpp-httplib** (`httplib.h`) - Embedded HTTP server
- **nlohmann/json** (`json.hpp`) - JSON parsing and serialization

### TrinityCore API Learnings
- Use `sConfigMgr->GetBoolDefault()`, `GetIntDefault()`, `GetStringDefault()` (not `GetOption<T>`)
- `Field::GetString()` works for all field types (no `GetType()` method)
- `QueryResultFieldMetadata` has `Alias` and `Name` (may be null)
- JSON-RPC `id` field: use `request.contains("id") ? request["id"] : json(nullptr)`

### Files Created
```
src/araxiaonline/mcp/
├── AraxiaMCPServer.cpp   # Core server, JSON-RPC handling
├── AraxiaMCPServer.h     # Public interface, tool registration
├── DatabaseTools.cpp     # db_query, db_execute, db_tables, db_describe
├── ServerTools.cpp       # server_info, player_list, gm_command
├── httplib.h             # cpp-httplib (external)
└── json.hpp              # nlohmann/json (external)
```

### Integration Points
- `World.cpp` - `sMCPServer->Initialize()` in `SetInitialWorldSettings()`
- `World.cpp` - `sMCPServer->Shutdown()` in destructor
- `CMakeLists.txt` - Auto-collected via `CollectSourceFiles`

## Windsurf MCP Configuration

To connect Cascade/Claude directly to the worldserver, add to `~/.codeium/windsurf/mcp_config.json`:

```json
{
  "mcpServers": {
    "araxia-worldserver": {
      "url": "http://localhost:8765/mcp",
      "transport": "http"
    }
  }
}
```

After adding, restart Windsurf. The AI will have direct access to all MCP tools.

## AMS Bridge (Client ↔ MCP)

The AMS bridge enables bidirectional communication between the WoW client and the MCP server.

### Data Flow
```
Client Addon → AMS.Send() → Server Lua → ElunaSharedData → MCP Tool
MCP Tool → ElunaSharedData → Server Lua → AMS.Send() → Client Addon
```

### Client Side (`AraxiaTrinityAdmin/MCPBridge.lua`)
- Captures chat messages and sends to server
- Provides `/mcpbridge` commands for control
- Polls for messages from MCP (disabled by default)

### Server Side (`lua_scripts/mcp_bridge.lua`)
- Receives client messages via AMS handlers
- Stores in ElunaSharedData for MCP to read
- Reads MCP messages and sends to client

### Shared Data Keys
| Key | Purpose |
|-----|---------|
| `mcp_client_chat` | Chat messages from client |
| `mcp_client_logs` | Log/error messages from client |
| `mcp_to_client` | Messages from MCP to display on client |
| `mcp_ui_state` | Full UI state (semantic screenshot) |
| `mcp_current_target` | Current target info (updated on /mcpbridge ui) |

### Client Commands
```
/mcpbridge status   - Show bridge status
/mcpbridge test     - Send test message to MCP
/mcpbridge on/off   - Enable/disable bridge
/mcpbridge poll     - Start polling for MCP messages
/mcpbridge ui       - Send UI state to MCP (semantic screenshot)
```

### Semantic Screenshot
The `/mcpbridge ui` command captures:
- **Target info**: Name, GUID, level, health, creature type
- **Player info**: Position, zone, health, combat status
- **Mouseover**: What you're hovering over
- **Tooltip**: Current tooltip text
- **Open frames**: Which addon panels are visible

AI can read via: `shared_data_read("mcp_current_target")` or `shared_data_read("mcp_ui_state")`

## Roadmap

| Phase | Feature | Status |
|-------|---------|--------|
| 1 | Database tools, server info | ✅ Complete |
| 2 | Shared data bridge (ElunaSharedData) | ✅ Complete |
| 3 | Content Creator Commands (via AMS, not GM-only) | ⏳ Next |
| 4 | World object tools (creatures, GOs) | ⏳ Planned |
| 5 | Eluna integration (lua_eval, hot-reload) | ⏳ Planned |
| 6 | **Event Bus** (unified pub/sub for all systems) | ⏳ Planned |

### Event Bus (See EVENT_BUS_DESIGN.md)

Unified internal event system connecting C++ Core, Eluna, MCP, and AMS:
- Any component can publish/subscribe to events
- Real-time event streaming (no polling)
- Full event history for debugging
- Enables MCP to see player target changes, spawns, errors, etc.

### Phase 3: Content Creator Commands

**Goal:** Allow any player with AraxiaTrinityAdmin addon to execute server commands without GM status.

**Architecture:**
```
Client Addon → AMS → Server Lua → Eluna API (bypasses GM check)
```

**Planned Commands:**
- `target <name>` - Target creature by name/entry
- `teleport <x,y,z>` - Teleport player
- `spawn <entry>` - Spawn creature/GO
- `modify <property>` - Modify targeted creature
- `waypoint` - Waypoint management
- `respawn` - Force respawn targeted creature

**Security:**
- Server-side validation of addon presence
- Configurable permission system (whitelist accounts/players)
- All actions logged for audit
- No access to actual GM commands (ban, kick, etc.)

## Key Learnings

### nlohmann/json Gotchas
- **Never use `request.value("id", nullptr)`** - causes type errors
- Use `request.contains("id") ? request["id"] : json(nullptr)` instead

### TrinityCore ConfigMgr API
- Use `sConfigMgr->GetBoolDefault()`, `GetIntDefault()`, `GetStringDefault()`
- NOT `GetOption<T>()` which doesn't exist

### ElunaSharedData API
- Singleton pattern: `sElunaSharedData->Get()`, `Set()`, `GetKeys()`
- Lua API: `SetSharedData()`, `GetSharedData()`, `HasSharedData()`

### AMS Client/Server Pattern
- Client: `AMS.Send("HANDLER_NAME", data)` (dot notation, NOT colon!)
- Server: `AMS.Send(player, "HANDLER_NAME", data)`
- Never serialize functions - Smallfolk will error

### VMAP Usage (World Scan)
- Include `VMapFactory.h` AND `VMapManager2.h` for full class definition
- Use `VMAP::VMapManager2*` not `VMAP::IVMapManager*`
- `isInLineOfSight()` returns TRUE if CAN see (no obstacle)
- Binary search for precise wall distance detection

### Database Safety
- Always use `db_describe` before querying unfamiliar tables
- TrinityCore 11.x has different column names than older versions
- Bad SQL can crash the server - wrap in try/catch!
- All DB tools now have exception handling

### CMake Tips
- New source files require `cmake ..` re-run to be detected
- `CollectSourceFiles()` auto-discovers .cpp files in subdirectories

## Session Summary (Nov 30, 2025)

### What We Built Tonight 🚀
1. **Safe SQL Queries** - All database tools wrapped in try/catch
2. **Semantic Screenshots** - `/mcpbridge ui` captures UI state
3. **LIDAR World Scan** - 360° ray casting using VMAP data
4. **Event Bus Design** - Architecture for unified pub/sub system

### First Successful World Scan
```
Location: Scarlet Monastery (2898.6, -802.9, 160.3)
Facing: 77° East-Northeast
Creatures: Scarlet Sentry (23y), Disciple (20y), Augur (22y)
Room: Walls 4-7y behind, corridor opening East
```

The AI can now SEE your surroundings! 👁️
