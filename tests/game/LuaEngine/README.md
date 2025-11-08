# Eluna/LuaEngine Unit Test Suite

## Overview

This directory contains a comprehensive unit test suite for the Eluna Lua scripting engine integration with TrinityCore. The tests validate functionality across multiple layers of the Eluna system.

## Test Organization

### Test Files

1. **CoreLuaExecution.cpp** (11 tests)
   - Basic script loading and execution
   - Variable assignment (integers, strings, booleans)
   - Function definition and calling
   - Table operations
   - Error handling and recovery
   - Lua state isolation
   - Arithmetic operations
   - String operations
   - Conditional logic
   - Loops (for, while)

2. **DataTypeConversions.cpp** (13 tests)
   - LuaValue creation and type checking
   - Lua to C++ conversions (integers, floats, strings, booleans, nil)
   - Type coercion
   - Table handling (arrays, dictionaries, nested)

3. **EventSystem.cpp** (8 tests)
   - Event registration and handler patterns
   - Event callback execution
   - Event handler state persistence
   - Event handler unregistration
   - Event handler priority ordering
   - Conditional event handling
   - Error handling in event handlers

4. **Integration.cpp** (8 tests)
   - Global Eluna instance creation
   - Script lifecycle (load → execute → verify)
   - Cross-script variable access
   - Module pattern implementation
   - Event manager pattern
   - Error recovery and resilience
   - Complex game script scenarios
   - Lua standard library functions (table, string, math)

5. **EdgeCases.cpp** (13 tests)
   - Empty and whitespace-only scripts
   - Nil and false value handling
   - Large numbers
   - Special characters in strings
   - Recursive functions (simple and mutual)
   - Circular references
   - Variable shadowing
   - Operator precedence
   - Table iteration edge cases
   - Function return values (multiple, none)
   - Metamethods
   - Upvalues and closures

6. **MethodBindings.cpp** (11 tests - Placeholders)
   - Binding availability checks
   - Binding signature validation
   - Return value conversion (int, string, bool, object)
   - Argument conversion (int, string, bool, object)
   - Null pointer handling
   - Enum constants
   - Method chaining

### Test Fixture

**LuaEngineTestFixture.h** provides common utilities:
- `CreateGlobalElunaInstance()` - Create a test Eluna instance
- `ExecuteScript()` - Execute Lua script string
- `GetGlobalAsString()` - Retrieve global variable as string
- `FunctionExists()` - Check if function is defined
- `CallFunction()` - Call a Lua function
- `GetLuaState()` - Get raw Lua state pointer

## Running Tests

### Build Tests

```bash
cd /opt/github.com/araxiaonline/TrinityCore
mkdir build && cd build
cmake .. -DELUNA=1
make tests
```

### Run All Tests

```bash
./bin/tests
```

### Run Specific Test Category

```bash
# Run only core execution tests
./bin/tests "[LuaEngine][CoreExecution]"

# Run only data type tests
./bin/tests "[LuaEngine][DataTypes]"

# Run only event system tests
./bin/tests "[LuaEngine][Events]"

# Run only integration tests
./bin/tests "[LuaEngine][Integration]"

# Run only edge case tests
./bin/tests "[LuaEngine][EdgeCases]"

# Run only method binding tests
./bin/tests "[LuaEngine][MethodBindings]"
```

### Run Specific Test

```bash
./bin/tests "Core Lua Execution - Simple Variable Assignment"
```

### Verbose Output

```bash
./bin/tests -v
```

## Test Statistics

- **Total Tests**: 64
- **Core Execution**: 11 tests
- **Data Types**: 13 tests
- **Event System**: 8 tests
- **Integration**: 8 tests
- **Edge Cases**: 13 tests
- **Method Bindings**: 11 tests (placeholders)

## Test Coverage

### Implemented Coverage

✅ Core Lua execution and state management
✅ Data type conversions and coercion
✅ Event handler patterns and callbacks
✅ Script lifecycle and error recovery
✅ Lua standard library functions
✅ Edge cases and error conditions

### Placeholder Coverage (Requires Server Context)

⏳ C++ method bindings (requires World/Map/Creature singletons)
⏳ Server event integration (requires full server initialization)
⏳ Player/Creature/GameObject method bindings
⏳ GUID and object reference handling

## Test Fixture Requirements

The test fixture requires:
- `sElunaMgr` singleton to be initialized
- `sElunaConfig` singleton to be initialized
- Eluna to be compiled with `ELUNA` define

## Known Limitations

1. **Server Context**: Some tests are placeholders because they require full server initialization (World, Map, Creature objects)
2. **Method Bindings**: Full validation of C++ method bindings requires server singletons
3. **Event Firing**: Actual event firing requires server event system integration

## Future Enhancements

1. **Mock Objects**: Create mock World/Map/Creature for testing without full server
2. **Method Binding Tests**: Implement actual binding validation with mocks
3. **Performance Tests**: Add benchmarks for script execution
4. **Stress Tests**: Test with large scripts and many concurrent handlers
5. **Memory Tests**: Validate garbage collection and memory management

## Contributing

When adding new tests:

1. Follow the existing test structure and naming conventions
2. Use descriptive test names and sections
3. Add comments explaining complex test scenarios
4. Update this README with new test categories
5. Ensure tests are isolated and don't depend on execution order
6. Use the LuaEngineTestFixture for common operations

## Debugging Tests

### Enable Verbose Output

```bash
./bin/tests -v "[LuaEngine][CoreExecution]"
```

### Run Single Test with Debugging

```bash
gdb ./bin/tests
(gdb) run "Core Lua Execution - Simple Variable Assignment"
```

### Check Lua Stack State

The fixture provides `GetLuaState()` to access the raw Lua state for debugging:

```cpp
lua_State* L = fixture.GetLuaState(eluna);
// Use Lua API directly for debugging
```

## References

- [Catch2 Documentation](https://github.com/catchorg/Catch2)
- [Lua 5.1 Reference Manual](https://www.lua.org/manual/5.1/)
- [Eluna Documentation](https://github.com/ElunaLuaEngine/Eluna)
- [TrinityCore Testing](https://github.com/TrinityCore/TrinityCore/tree/master/tests)
