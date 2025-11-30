# Eluna Integration Test Suite

This directory contains integration tests for the Eluna Lua scripting engine in TrinityCore.

## Overview

The test suite runs automatically at server startup and validates core Eluna functionality including:

- **Core Lua Functionality** - Variables, functions, tables, loops, conditionals
- **Event System** - Event registration, callbacks, handlers, state persistence
- **Data Types** - Type checking, conversions, coercion
- **Bindings** - C++ function bindings, API availability

## Test Structure

```
integration_tests/
├── test_runner.lua              # Main test runner framework
├── test_core_functionality.lua  # Core Lua tests (15 tests)
├── test_events.lua              # Event system tests (15 tests)
├── test_data_types.lua          # Data type tests (20 tests)
├── test_bindings.lua            # C++ bindings tests (25 tests)
└── README.md                    # This file
```

## Running Tests

Tests run automatically when the server starts. The test output will appear in the server logs.

### Manual Test Execution

To run tests manually, load the test runner in the Lua console:

```lua
local runner = require("integration_tests/test_runner")
runner:LoadTests()
runner:RunAll()
```

## Test Categories

### Core Functionality (15 tests)
- Variable assignment and retrieval
- Arithmetic operations
- String operations
- Table creation and access
- Table iteration
- Function definition and calling
- Conditional statements
- Loop execution
- Boolean logic
- Type checking
- Nested tables
- Table length operator
- String formatting
- Math operations
- Local scope

### Event System (15 tests)
- Global Eluna table
- Event counter increment
- Multiple event tracking
- Callback simulation
- Event parameters
- Event queue simulation
- Handler registration
- Handler execution
- Event filtering
- Event priority
- State persistence
- Error handling
- Listener removal
- Event broadcasting
- Event metadata

### Data Types (20 tests)
- Number type
- String type
- Boolean type
- Table type
- Nil type
- Function type
- Number to string conversion
- String to number conversion
- Boolean to string conversion
- Table representation
- Mixed type operations
- String coercion
- Nested table types
- Array vs dictionary tables
- Type checking with conditionals
- Nil handling
- Truthy/falsy values
- Type identity
- Reference vs value
- String immutability

### Bindings (25 tests)
- Eluna API availability
- GetWorldElapsedTime function
- os.time function
- os.date function
- print function
- table.insert
- table.remove
- string.sub
- string.find
- string.upper
- string.lower
- math.floor
- math.ceil
- math.abs
- math.max
- math.min
- math.random
- pcall (protected call)
- require function
- Binding error handling
- Multiple binding calls
- Binding with parameters
- Binding return values
- Binding chaining
- Binding availability check

## Test Output

The test suite produces output in the following format:

```
================================================================================
ELUNA INTEGRATION TEST SUITE
================================================================================
Starting tests at 2025-11-09 15:30:45
================================================================================

[1/75] ✓ PASS: Core: Variable Assignment
[2/75] ✓ PASS: Core: Arithmetic Operations
...
[75/75] ✓ PASS: Bindings: Availability Check

================================================================================
TEST SUMMARY
================================================================================
Total Tests: 75
Passed: 75
Failed: 0
Success Rate: 100.0%
================================================================================
```

## Adding New Tests

To add a new test, use the `TestRunner:Register()` method:

```lua
TestRunner:Register("Category: Test Name", function()
    -- Test code here
    return true  -- Return true if test passes, false if it fails
end)
```

## Test Failure Handling

If a test fails:

1. The test name and failure reason will be printed
2. The test suite will continue running remaining tests
3. A summary will show the number of passed/failed tests
4. Check the server logs for detailed error messages

## Performance

- Total test suite: ~75 tests
- Typical execution time: < 100ms
- No performance impact on server operation

## Debugging

To debug a specific test:

1. Add `print()` statements in the test function
2. Check the server logs for output
3. Use `pcall()` to catch errors and print them

Example:

```lua
TestRunner:Register("Debug Test", function()
    local status, result = pcall(function()
        -- Your test code
        return true
    end)
    
    if not status then
        print("Error: " .. result)
        return false
    end
    
    return result
end)
```

## Continuous Integration

These tests can be integrated into CI/CD pipelines by:

1. Parsing the test output from server logs
2. Checking for "Success Rate: 100.0%"
3. Failing the build if any tests fail

## Contributing

When adding new Eluna features, please add corresponding tests to ensure:

- Feature works as expected
- No regressions in existing functionality
- Edge cases are handled properly

## License

These tests are part of the TrinityCore project and follow the same license.
