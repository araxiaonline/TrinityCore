# LuaEngine Server Testing Guide

This guide explains how to test LuaEngine functionality once the TrinityCore server is running.

## Prerequisites

1. Server compiled with `-DELUNA=1`
2. Server running and accessible
3. Database populated with test data
4. Admin account created for testing

## Quick Start: Unit Tests (Before Server Start)

Run the comprehensive unit test suite to verify core functionality:

```bash
cd /opt/github.com/araxiaonline/TrinityCore/build
./bin/RelWithDebInfo/bin/tests "[LuaEngine]"
```

Expected output:
```
All tests passed (933 assertions in 135 test cases)
```

## Server-Side Testing (After Server Start)

### 1. Verify Eluna is Enabled

**In-Game Command:**
```
.lua print("Eluna is working!")
```

**Expected Result:**
- No error message
- Server logs show the print output
- Console shows: `Eluna is working!`

### 2. Test Basic Lua Execution

**In-Game Command:**
```
.lua local x = 10; local y = 20; print("Result: " .. (x + y))
```

**Expected Result:**
- Console shows: `Result: 30`

### 3. Test Lua State Persistence

**First Command:**
```
.lua test_var = 42
```

**Second Command:**
```
.lua print("Stored value: " .. test_var)
```

**Expected Result:**
- Console shows: `Stored value: 42`
- Demonstrates that Lua state persists across commands

### 4. Test Table Operations

**In-Game Command:**
```
.lua 
local data = {name = "Test", value = 100, items = {1, 2, 3}}
print("Name: " .. data.name)
print("Value: " .. data.value)
print("Items count: " .. #data.items)
```

**Expected Result:**
- Console shows:
  ```
  Name: Test
  Value: 100
  Items count: 3
  ```

### 5. Test Function Definition and Calling

**First Command:**
```
.lua
function add_numbers(a, b)
    return a + b
end
result = add_numbers(15, 25)
print("Sum: " .. result)
```

**Expected Result:**
- Console shows: `Sum: 40`

### 6. Test Lua Standard Libraries

**Math Library:**
```
.lua print("Floor of 3.7: " .. math.floor(3.7))
```

**String Library:**
```
.lua print("Uppercase: " .. string.upper("hello"))
```

**Table Library:**
```
.lua local t = {3, 1, 2}; table.sort(t); print("Sorted: " .. t[1] .. "," .. t[2] .. "," .. t[3])
```

**Expected Results:**
- Math: `Floor of 3.7: 3`
- String: `Uppercase: HELLO`
- Table: `Sorted: 1,2,3`

## Testing Game Integration

### 7. Test World Access

**In-Game Command:**
```
.lua
local world = GetWorld()
if world then
    print("World object accessible")
else
    print("World object not accessible")
end
```

**Expected Result:**
- Console shows: `World object accessible`

### 8. Test Player Access

**In-Game Command:**
```
.lua
local player = GetPlayer()
if player then
    print("Player name: " .. player:GetName())
    print("Player level: " .. player:GetLevel())
else
    print("Player object not accessible")
end
```

**Expected Result:**
- Console shows player name and level

### 9. Test Creature Access

**In-Game Command (requires creature nearby):**
```
.lua
local creature = GetCreature()
if creature then
    print("Creature name: " .. creature:GetName())
    print("Creature health: " .. creature:GetHealth())
else
    print("No creature targeted")
end
```

**Expected Result:**
- If creature targeted: Shows creature info
- If no creature: Shows "No creature targeted"

### 10. Test Event System

**Register an Event Handler:**
```
.lua
function on_test_event(data)
    print("Event triggered with data: " .. tostring(data))
end
```

**Trigger the Event:**
```
.lua
if on_test_event then
    on_test_event("test_data")
    print("Event handler executed")
end
```

**Expected Result:**
- Console shows:
  ```
  Event triggered with data: test_data
  Event handler executed
  ```

## Testing Error Handling

### 11. Test Error Recovery

**First Command (valid):**
```
.lua valid_var = 100
```

**Second Command (invalid):**
```
.lua this_will_cause_an_error()
```

**Third Command (valid):**
```
.lua print("Still working: " .. valid_var)
```

**Expected Result:**
- First command succeeds
- Second command shows error but doesn't crash server
- Third command works, showing `Still working: 100`
- Server continues running

### 12. Test Type Safety

**In-Game Command:**
```
.lua
local x = 42
local y = "hello"
local z = true
print("Number: " .. type(x))
print("String: " .. type(y))
print("Boolean: " .. type(z))
```

**Expected Result:**
- Console shows:
  ```
  Number: number
  String: string
  Boolean: boolean
  ```

## Advanced Testing

### 13. Test Complex Game Scenario

**Create a test scenario:**
```
.lua
-- Simulate a game scenario
local scenario = {
    players = {},
    creatures = {},
    events = {}
}

function add_player(id, name)
    table.insert(scenario.players, {id = id, name = name})
end

function spawn_creature(id, name)
    table.insert(scenario.creatures, {id = id, name = name})
end

function log_event(event_type)
    table.insert(scenario.events, event_type)
end

-- Execute scenario
add_player(1, "Hero")
spawn_creature(100, "Goblin")
log_event("combat")

print("Players: " .. #scenario.players)
print("Creatures: " .. #scenario.creatures)
print("Events: " .. #scenario.events)
```

**Expected Result:**
- Console shows:
  ```
  Players: 1
  Creatures: 1
  Events: 1
  ```

### 14. Test Data Persistence Across Reloads

**First Session:**
```
.lua persistent_data = {value = 999, timestamp = os.time()}
```

**Reload Eluna:**
```
.lua print("Data still exists: " .. (persistent_data ~= nil))
```

**Expected Result:**
- If data persists: `Data still exists: true`
- If data resets: `Data still exists: false`

### 15. Test Performance

**Stress Test:**
```
.lua
local start_time = os.time()
local counter = 0
for i = 1, 10000 do
    counter = counter + 1
end
local end_time = os.time()
print("Processed 10000 iterations in " .. (end_time - start_time) .. " seconds")
```

**Expected Result:**
- Should complete quickly (< 1 second)
- Shows iteration count and time

## Automated Testing Script

Create a Lua script file for automated testing:

**File: `test_eluna.lua`**

```lua
-- LuaEngine Automated Test Suite
local test_results = {passed = 0, failed = 0}

function run_test(test_name, test_func)
    print("Running: " .. test_name)
    local success, result = pcall(test_func)
    if success then
        print("  ✓ PASSED")
        test_results.passed = test_results.passed + 1
    else
        print("  ✗ FAILED: " .. tostring(result))
        test_results.failed = test_results.failed + 1
    end
end

-- Test 1: Basic arithmetic
run_test("Basic Arithmetic", function()
    assert(10 + 20 == 30, "Addition failed")
    assert(10 - 5 == 5, "Subtraction failed")
    assert(10 * 2 == 20, "Multiplication failed")
end)

-- Test 2: String operations
run_test("String Operations", function()
    assert(string.upper("hello") == "HELLO", "Upper case failed")
    assert(string.len("test") == 4, "String length failed")
    assert("hello" .. "world" == "helloworld", "Concatenation failed")
end)

-- Test 3: Table operations
run_test("Table Operations", function()
    local t = {1, 2, 3}
    assert(#t == 3, "Table length failed")
    table.insert(t, 4)
    assert(#t == 4, "Table insert failed")
end)

-- Test 4: Math library
run_test("Math Library", function()
    assert(math.floor(3.7) == 3, "Floor failed")
    assert(math.ceil(3.2) == 4, "Ceil failed")
    assert(math.max(1, 5, 3) == 5, "Max failed")
end)

-- Test 5: Conditional logic
run_test("Conditional Logic", function()
    local x = 10
    if x > 5 then
        assert(true, "If condition failed")
    else
        assert(false, "If condition failed")
    end
end)

-- Test 6: Loop operations
run_test("Loop Operations", function()
    local sum = 0
    for i = 1, 5 do
        sum = sum + i
    end
    assert(sum == 15, "For loop failed")
end)

-- Test 7: Function definition
run_test("Function Definition", function()
    local function multiply(a, b)
        return a * b
    end
    assert(multiply(3, 4) == 12, "Function call failed")
end)

-- Test 8: Error handling
run_test("Error Handling", function()
    local success, err = pcall(function()
        error("Test error")
    end)
    assert(not success, "Error handling failed")
end)

-- Print summary
print("\n=== TEST SUMMARY ===")
print("Passed: " .. test_results.passed)
print("Failed: " .. test_results.failed)
print("Total: " .. (test_results.passed + test_results.failed))
```

**To run this script:**
```
.lua dofile("test_eluna.lua")
```

## Troubleshooting

### Issue: "Eluna is not enabled"
**Solution:**
- Recompile with `-DELUNA=1`
- Check `ElunaConfig` in database
- Verify `Eluna.Enabled = 1` in config

### Issue: Lua commands not executing
**Solution:**
- Check server logs for errors
- Verify admin account has proper permissions
- Ensure `.lua` command is enabled

### Issue: Lua state not persisting
**Solution:**
- Check if Eluna is being reloaded
- Verify global Eluna instance is created
- Check for script reloads

### Issue: Performance issues
**Solution:**
- Check for infinite loops in scripts
- Monitor server CPU usage
- Profile Lua execution time

## Verification Checklist

- [ ] Unit tests pass (135/135)
- [ ] `.lua` command works
- [ ] Basic arithmetic works
- [ ] String operations work
- [ ] Table operations work
- [ ] Math library accessible
- [ ] Functions can be defined
- [ ] Lua state persists
- [ ] Error handling works
- [ ] Server doesn't crash on errors
- [ ] World object accessible
- [ ] Player object accessible
- [ ] Complex scenarios work
- [ ] Performance is acceptable

## Next Steps

Once all tests pass:

1. **Create Game Scripts**: Write Lua scripts for game mechanics
2. **Test Event Hooks**: Verify event system integration
3. **Performance Tuning**: Optimize Lua execution
4. **Production Deployment**: Deploy to live server

## Support

For issues or questions:
1. Check server logs: `tail -f worldserver.log`
2. Review Lua error messages
3. Consult TrinityCore documentation
4. Check Eluna documentation
