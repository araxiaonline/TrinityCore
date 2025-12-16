# Araxia Online Trinity Core Fork

## Agent

This project is a fork of Trinity Core, a World of Warcraft server emulator. 

## Components

- [Trinity Core](https://github.com/TrinityCore/TrinityCore)
- [Trinity Core Database](https://github.com/TrinityCore/TrinityCore-Database)
- [Eluna](https://github.com/ElunaLua/Eluna)

## Status

We are currently in the process of getting the Eluna addon working with the master branch.

Documentation with status can be found in [ELUNA_INTEGRATION_COMPLETE.md](ELUNA_INTEGRATION_COMPLETE.md), [GLOBAL_ELUNA_PLAN.md](GLOBAL_ELUNA_PLAN.md), and [VALIDATION_SUMMARY.md](VALIDATION_SUMMARY.md). Please consult them before tacking any new tasks. All tasks should be tracked in these files.

---

## Code Commenting Guidelines

**ALWAYS add comments when modifying C++ code in this project.**

### Why?
- Context is lost between sessions
- Future AI assistants (and humans) need to understand design decisions
- Prevents reimplementing the same solutions repeatedly
- Code comments stay close to the implementation

### What to Comment:
1. **File headers** - Document the purpose and any important patterns used
2. **Non-obvious design decisions** - Why did you choose this approach?
3. **Integration patterns** - How does this code interact with TrinityCore systems?
4. **Gotchas and warnings** - What mistakes should be avoided?

### Example Pattern (MCP GM Commands):
```cpp
// PREFER ChatHandler::ParseCommands() over reimplementing GM command logic
// This allows ANY existing command to work without custom code
ChatHandler handler(player->GetSession());
handler.ParseCommands(command);
```

### Key Files with Important Comments:
- `src/araxiaonline/mcp/ServerTools.cpp` - GM command implementation pattern
- `src/araxiaonline/mcp/AraxiaMCPServer.cpp` - MCP server architecture
- `src/araxiaonline/eventbus/AraxiaEvents.h` - Event interface and concrete event classes

---

## Araxia Event Bus

The Event Bus is a ZeroMQ-based real-time event system that publishes game events to external consumers.

- **Documentation**: `/opt/trinitycore/araxia-content-tools/docs/game_engine/araxia_event_bus.md`
- **Source**: `src/araxiaonline/eventbus/`
- **Port**: `tcp://*:5555` (configurable)

### Supported Events
| Category | Events |
|----------|--------|
| Player | login, logout, death |
| Quest | accept, complete, abandon |
| Combat | enter, leave |
| Loot | item |
| Spawn | create, delete |
| Encounter | start, wipe, end |

### Quick Test
```bash
cd /opt/trinitycore/TrinityCore/src/araxiaonline/tools
source .venv/bin/activate
python zmq_subscriber.py
```

**Keep the wiki page updated** when adding new event types or changing the event bus architecture.

---

## AI Testing with Scarletseer

**Scarletseer** is a dedicated test character for AI assistants to use for automated testing via MCP.

- **Character**: Scarletseer (GUID 7, Level 80 Tauren Shaman)
- **Location**: Pandaria (Map 870) - Halfhill Farm area
- **Documentation**: See `/opt/trinitycore/araxia-content-tools/docs/game_engine/scarletseer.md`

### Quick Start
```
mcp_session_create(owner_name="Cascade")
mcp_player_login(session_id=1, character_name="Scarletseer")
mcp_player_status(session_id=1)  # Verify login
# ... run tests ...
mcp_player_logout(session_id=1)
```

### Event Bus Testing
Scarletseer can trigger events for the ZeroMQ event bus:
- **Player events**: login, logout, death
- **Quest events**: accept, complete, abandon
- **Combat events**: enter, leave
- **Loot events**: item looted
- **Encounter events**: start, wipe, end (requires dungeon/raid)

Monitor events with: `python /opt/trinitycore/TrinityCore/src/araxiaonline/tools/zmq_subscriber.py`

---

### Building the server
- Always use the max number of threads when building the server
- Always use @araxiaonline/cmake_setup.sh to setup the build environment. Modify it if needed.
- Please fix all compile warnings before marking a task as complete.

### Pushing changes
- Our working branch is `araxia-main` and all branches and PRs should be based on this branch.
