# Eluna Lua Scripts

This directory contains Lua scripts for the Eluna scripting engine in TrinityCore.

## Directory Structure

```
lua_scripts/
├── init.lua                     # Main initialization script (runs at startup)
├── integration_tests/           # Integration test suite
│   ├── test_runner.lua         # Test framework
│   ├── test_core_functionality.lua
│   ├── test_events.lua
│   ├── test_data_types.lua
│   ├── test_bindings.lua
│   └── README.md
└── README.md                    # This file
```

## Quick Start

1. **Server Startup**: The `init.lua` script runs automatically when the server starts
2. **Test Execution**: Integration tests run automatically and output results to server logs
3. **Custom Scripts**: Add your own Lua scripts to this directory

## Integration Tests

The `integration_tests/` directory contains a comprehensive test suite that validates:

- **Core Lua Functionality** (15 tests)
- **Event System** (15 tests)
- **Data Types** (20 tests)
- **C++ Bindings** (25 tests)

**Total: 75 tests**

See `integration_tests/README.md` for detailed information.

## Adding Custom Scripts

To add your own Lua scripts:

1. Create a `.lua` file in this directory
2. The script will be automatically loaded by Eluna
3. Use the Eluna API to interact with the server

Example:

```lua
-- my_script.lua
print("Hello from Eluna!")

-- Register a world event
RegisterServerEvent(1, function()
    print("World update event!")
end)
```

## Eluna API

Common Eluna functions available:

- `GetWorldElapsedTime()` - Get elapsed time since server start
- `RegisterServerEvent(eventId, callback)` - Register server events
- `RegisterPlayerEvent(eventId, callback)` - Register player events
- `RegisterCreatureEvent(eventId, callback)` - Register creature events

## Debugging

To debug scripts:

1. Add `print()` statements
2. Check server logs for output
3. Use `pcall()` for error handling

Example:

```lua
local status, result = pcall(function()
    -- Your code here
    return "success"
end)

if not status then
    print("Error: " .. result)
else
    print("Result: " .. result)
end
```

## Performance Tips

- Keep scripts efficient to avoid server lag
- Use local variables instead of global when possible
- Cache frequently accessed values
- Avoid infinite loops

## Testing

Run the integration test suite:

```lua
local runner = require("integration_tests/test_runner")
runner:LoadTests()
runner:RunAll()
```

## Documentation

- Eluna Documentation: https://elunaluaengine.github.io/
- TrinityCore: https://www.trinitycore.org/
- Lua 5.1 Reference: https://www.lua.org/manual/5.1/

## Support

For issues or questions:

1. Check the integration test output
2. Review Eluna documentation
3. Check TrinityCore forums

## License

These scripts are part of the TrinityCore project and follow the same license.
