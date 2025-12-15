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
