# LuaEngine Quick Testing Reference

## Before Server Start: Unit Tests

```bash
cd /opt/github.com/araxiaonline/TrinityCore/build
./bin/RelWithDebInfo/bin/tests "[LuaEngine]"
```

**Expected:** `All tests passed (933 assertions in 135 test cases)`

## After Server Start: In-Game Testing

### Quick Verification Commands

**1. Verify Eluna is Enabled**
```
.lua print("Eluna works!")
```

**2. Test Basic Math**
```
.lua print(10 + 20)
```

**3. Test String Operations**
```
.lua print(string.upper("hello"))
```

**4. Test Table Operations**
```
.lua local t = {1,2,3}; print(#t)
```

**5. Test Function Definition**
```
.lua function test() return 42 end; print(test())
```

### Run Full Test Suite

**Copy test script to server directory:**
```bash
cp test_eluna_server.lua /path/to/server/
```

**Run in-game:**
```
.lua dofile("test_eluna_server.lua")
```

**Expected Output:**
```
TEST SUMMARY
============================================================
Total Tests: 35
Passed: 35 ✓
Failed: 0 ✗
Pass Rate: 100.0%

🎉 ALL TESTS PASSED! LuaEngine is working correctly.
```

## Test Categories

| Category | Tests | Command |
|----------|-------|---------|
| Basic Lua | 5 | `.lua print("test")` |
| Control Flow | 5 | `.lua for i=1,5 do print(i) end` |
| Data Structures | 5 | `.lua local t={1,2,3}; print(#t)` |
| Error Handling | 3 | `.lua pcall(function() error("test") end)` |
| Advanced | 4 | `.lua function f() return 42 end; print(f())` |
| String Ops | 3 | `.lua print(string.upper("hello"))` |
| Performance | 3 | `.lua for i=1,10000 do end; print("done")` |

## Troubleshooting

| Issue | Solution |
|-------|----------|
| `.lua` command not found | Recompile with `-DELUNA=1` |
| "Eluna is not enabled" | Check `ElunaConfig` in database |
| Script errors | Check server logs: `tail -f worldserver.log` |
| Performance issues | Check for infinite loops |
| State not persisting | Verify Eluna isn't being reloaded |

## Common Test Patterns

### Test Arithmetic
```lua
.lua
local x = 10 + 20
print("Result: " .. x)
```

### Test Strings
```lua
.lua
local str = "hello" .. " " .. "world"
print(str)
```

### Test Tables
```lua
.lua
local t = {a=1, b=2, c=3}
for k,v in pairs(t) do
    print(k .. "=" .. v)
end
```

### Test Functions
```lua
.lua
local function multiply(a, b)
    return a * b
end
print(multiply(6, 7))
```

### Test Error Handling
```lua
.lua
local ok, err = pcall(function()
    error("test error")
end)
print("Success: " .. tostring(ok))
```

## Performance Benchmarks

| Operation | Expected Time |
|-----------|----------------|
| 10,000 loop iterations | < 1 second |
| 1,000 table inserts | < 1 second |
| 100 string concatenations | < 1 second |
| Complex nested tables | < 1 second |

## Verification Checklist

- [ ] Unit tests pass (135/135)
- [ ] `.lua` command works
- [ ] Basic arithmetic works
- [ ] String operations work
- [ ] Table operations work
- [ ] Functions can be defined
- [ ] Error handling works
- [ ] Performance is acceptable
- [ ] Full test suite passes

## Server Integration Tests

### Test World Access
```lua
.lua
local world = GetWorld()
if world then print("World accessible") end
```

### Test Player Access
```lua
.lua
local player = GetPlayer()
if player then print("Player: " .. player:GetName()) end
```

### Test Creature Access
```lua
.lua
local creature = GetCreature()
if creature then print("Creature: " .. creature:GetName()) end
```

## Next Steps

1. ✅ Run unit tests
2. ✅ Start server
3. ✅ Run in-game tests
4. ✅ Run full test suite
5. ✅ Verify all checks pass
6. 📝 Create game scripts
7. 🚀 Deploy to production

## Support Resources

- **Unit Tests:** `/opt/github.com/araxiaonline/TrinityCore/tests/game/LuaEngine/`
- **Server Testing Guide:** `SERVER_TESTING_GUIDE.md`
- **Test Script:** `test_eluna_server.lua`
- **Server Logs:** `worldserver.log`
- **TrinityCore Docs:** https://trinitycore.atlassian.net/
- **Eluna Docs:** https://eluna.elunaluascripting.com/

## Quick Commands

```bash
# Build tests
cd /opt/github.com/araxiaonline/TrinityCore/build
cmake .. -DELUNA=1 -DBUILD_TESTING=1
make tests -j24

# Run all tests
./bin/RelWithDebInfo/bin/tests "[LuaEngine]"

# Run specific phase
./bin/RelWithDebInfo/bin/tests "[LuaEngine][LuaEngineCore]"
./bin/RelWithDebInfo/bin/tests "[LuaEngine][LuaEngineAdvanced]"
./bin/RelWithDebInfo/bin/tests "[LuaEngine][LuaEngineIntegration]"

# View server logs
tail -f worldserver.log

# Search logs for errors
grep -i "error\|lua" worldserver.log
```

## Success Indicators

✅ All 135 unit tests pass  
✅ `.lua` command executes  
✅ Lua state persists  
✅ Error handling works  
✅ Performance is acceptable  
✅ Server doesn't crash  
✅ Full test suite passes  

**If all indicators are green, LuaEngine is ready for production!** 🚀
