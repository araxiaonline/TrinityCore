# Eluna Shared Data Registry - Implementation Complete

**Date:** November 29, 2025  
**Status:** ✅ COMPLETE - All 8 AMS tests passing

## Problem Solved

Eluna runs in multiple isolated Lua states (global state + per-map states). Each state has completely isolated `_G` tables, meaning:
- Multi-part addon messages could be handled by different states
- Message reassembly data stored in Lua tables was lost between parts
- `_G.SharedTable` approaches failed because `_G` is NOT shared

This caused NESTED_DATA and LARGE_PAYLOAD tests to timeout because message parts couldn't be reassembled.

## Solution: C++ Shared Data Registry

Added a C++-backed global key-value store accessible from ALL Eluna states:

### New C++ Files
- `src/server/game/LuaEngine/ElunaSharedData.h` - Singleton header
- `src/server/game/LuaEngine/ElunaSharedData.cpp` - Thread-safe implementation

### New Lua API Functions (GlobalMethods.h)
```lua
SetSharedData(key, value)    -- Store string value
GetSharedData(key)           -- Retrieve string value (or nil)
HasSharedData(key)           -- Check if key exists
ClearSharedData(key)         -- Remove specific key
ClearAllSharedData()         -- Clear everything
GetSharedDataKeys()          -- List all keys
```

### Key Design Decisions
1. **Strings only in C++** - C++ stores raw strings, Lua handles serialization with Smallfolk
2. **Thread-safe** - Uses `std::shared_mutex` for concurrent access
3. **Simple API** - Avoids complex lmarshal stack manipulation that caused crashes

## Usage Pattern (AMS_Server.lua)

```lua
local function HandleIncomingMessage(player, rawMessage)
    local playerGUID = tostring(player:GetGUIDLow())
    local dataKey = "AMS_PLAYER_" .. playerGUID
    
    -- Retrieve and deserialize
    local serializedData = GetSharedData(dataKey)
    local playerData
    if serializedData then
        local success, decoded = pcall(Smallfolk.loads, serializedData)
        if success and type(decoded) == 'table' then
            playerData = decoded
        else
            playerData = { pendingMessages = {} }
        end
    else
        playerData = { pendingMessages = {} }
    end
    
    -- ... process message parts ...
    
    -- Serialize and store
    SetSharedData(dataKey, Smallfolk.dumps(playerData))
end
```

## Key Learnings

### 1. Eluna State Isolation is Deep
- `_G` tables are completely isolated per state
- `require()` caching doesn't help - each state has its own cache
- Module-level variables are NOT shared

### 2. lmarshal Stack Semantics
- `mar_encode(L)` reads from index 1, pushes result to TOP
- `mar_decode(L)` reads from index 1, pushes result to TOP
- Neither replaces values - they PUSH new values
- Initial implementation caused "bad magic", "bad header" errors and crashes

### 3. Simpler is Better
- Original design: C++ serializes Lua values with lmarshal
- Final design: C++ stores strings, Lua serializes with Smallfolk
- Avoided complex Lua stack manipulation that was error-prone

### 4. GetGUIDLow() Returns uint64
- Can't concatenate directly with strings in Lua
- Must use `tostring(player:GetGUIDLow())`

### 5. Message Part Indexing
- Client sends parts 1-indexed (partID = 1, 2, 3...)
- Server reassembly loop must match: `for i = 1, totalParts do`

## Test Results

| Test | Status | Description |
|------|--------|-------------|
| RAPID_FIRE | ✅ PASSED | 50 messages rapidly |
| ECHO | ✅ PASSED | Basic round-trip |
| TYPES | ✅ PASSED | All Lua data types |
| SERVER_PUSH | ✅ PASSED | Server-initiated messages |
| PERFORMANCE | ✅ PASSED | 25 round-trip iterations |
| ERROR_GRACEFUL | ✅ PASSED | Graceful error handling |
| NESTED_DATA | ✅ PASSED | 10-level deep nested tables |
| LARGE_PAYLOAD | ✅ PASSED | 3KB multi-part message |

## Files Modified

### C++ (requires rebuild)
- `src/server/game/LuaEngine/ElunaSharedData.h` (new)
- `src/server/game/LuaEngine/ElunaSharedData.cpp` (new)
- `src/server/game/LuaEngine/methods/TrinityCore/GlobalMethods.h` (added functions)

### Lua (hot-reloadable)
- `lua_scripts/AMS_Server/AMS_Server.lua` (uses SetSharedData/GetSharedData)
- `lua_scripts/ams_test_handlers.lua` (fixed countDepth off-by-one)

## Future Opportunities

This shared data infrastructure enables:
- Cross-state event coordination
- Shared configuration between map instances
- Player session data accessible from any state
- Server-wide broadcast systems
- More complex multi-part message protocols

## Build Notes

WSL build (faster iteration than Docker):
```bash
mkdir -p ~/trinitycore-build
cd ~/trinitycore-build
cmake /mnt/q/github.com/araxiaonline/TrinityCore -DWITH_ELUNA=1 -DCMAKE_BUILD_TYPE=RelWithDebInfo
make -j4
```

Requires `libluajit-5.1-dev` package installed in WSL.
