# Araxia MCP Server

## Overview

The Araxia MCP Server embeds a Model Context Protocol server directly into the worldserver, enabling AI assistants (like Claude/Cascade) to interact with the game server in real-time.

**Status:** ✅ Phase 1 Complete (Nov 30, 2025)

## Features

- **Database Access**: ✅ Direct SQL queries to world, characters, and auth databases
- **Server Status**: ✅ Real-time server info, player lists, uptime
- **GM Commands**: ✅ Stub (needs ChatHandler integration)
- **Eluna Integration**: ⏳ (Phase 2) Execute Lua, inspect state, hot-reload
- **AMS Bridge**: ⏳ (Phase 4) Communicate with client addons

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

## Roadmap

| Phase | Feature | Status |
|-------|---------|--------|
| 1 | Database tools, server info | ✅ Complete |
| 2 | Eluna integration (lua_eval, shared_data) | ⏳ Planned |
| 3 | World object tools (creatures, GOs) | ⏳ Planned |
| 4 | AMS bridge (client addon communication) | ⏳ Planned |
| 5 | Event streaming (logs, world events) | ⏳ Planned |
