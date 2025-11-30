# Eluna Shared Data Registry - Design Document

## Overview

This document describes the implementation of a C++-backed shared data registry for Eluna that enables cross-state data sharing. This solves the fundamental limitation where each Eluna state (global + per-map) has isolated Lua environments.

## Problem Statement

In Eluna's multi-state architecture:
- Each map instance has its own Eluna Lua state
- The "World" state is a separate global state
- `_G` tables are **completely isolated** between states
- Event handlers may fire in different states for the same logical operation
- Multi-part addon messages can be received by different isolated environments

This makes it impossible to:
- Reassemble multi-part messages reliably
- Share state between map instances
- Maintain persistent data across event boundaries

## Solution: C++ Shared Data Registry

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    C++ Layer (Thread-Safe)                   │
│  ┌─────────────────────────────────────────────────────┐   │
│  │           ElunaSharedData Singleton                  │   │
│  │  ┌─────────────────────────────────────────────┐    │   │
│  │  │  std::unordered_map<string, string> data_   │    │   │
│  │  │  (serialized Lua values)                     │    │   │
│  │  └─────────────────────────────────────────────┘    │   │
│  │  + std::shared_mutex mutex_                          │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
        ▲           ▲           ▲           ▲
        │           │           │           │
   SetShared   GetShared   SetShared   GetShared
        │           │           │           │
┌───────┴───┐ ┌─────┴─────┐ ┌───┴───┐ ┌─────┴─────┐
│ World     │ │ Map 1     │ │ Map 2 │ │ Map N     │
│ Eluna     │ │ Eluna     │ │ Eluna │ │ Eluna     │
│ State     │ │ State     │ │ State │ │ State     │
└───────────┘ └───────────┘ └───────┘ └───────────┘
```

### New Lua API

```lua
-- Set a shared value (any serializable Lua value)
SetSharedData(key, value)

-- Get a shared value (returns nil if not found)
local value = GetSharedData(key)

-- Delete a shared value
ClearSharedData(key)

-- Check if key exists
local exists = HasSharedData(key)

-- Get all keys (for debugging)
local keys = GetSharedDataKeys()
```

### Implementation Files

#### 1. `ElunaSharedData.h`
```cpp
#ifndef ELUNA_SHARED_DATA_H
#define ELUNA_SHARED_DATA_H

#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <vector>

class ElunaSharedData
{
public:
    static ElunaSharedData* instance();
    
    // Thread-safe operations
    void Set(const std::string& key, const std::string& serializedValue);
    bool Get(const std::string& key, std::string& outValue) const;
    bool Has(const std::string& key) const;
    void Clear(const std::string& key);
    void ClearAll();
    std::vector<std::string> GetKeys() const;
    
private:
    ElunaSharedData() = default;
    
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, std::string> data_;
};

#define sElunaSharedData ElunaSharedData::instance()

#endif
```

#### 2. `ElunaSharedData.cpp`
```cpp
#include "ElunaSharedData.h"

ElunaSharedData* ElunaSharedData::instance()
{
    static ElunaSharedData instance;
    return &instance;
}

void ElunaSharedData::Set(const std::string& key, const std::string& serializedValue)
{
    std::unique_lock lock(mutex_);
    data_[key] = serializedValue;
}

bool ElunaSharedData::Get(const std::string& key, std::string& outValue) const
{
    std::shared_lock lock(mutex_);
    auto it = data_.find(key);
    if (it == data_.end())
        return false;
    outValue = it->second;
    return true;
}

bool ElunaSharedData::Has(const std::string& key) const
{
    std::shared_lock lock(mutex_);
    return data_.find(key) != data_.end();
}

void ElunaSharedData::Clear(const std::string& key)
{
    std::unique_lock lock(mutex_);
    data_.erase(key);
}

void ElunaSharedData::ClearAll()
{
    std::unique_lock lock(mutex_);
    data_.clear();
}

std::vector<std::string> ElunaSharedData::GetKeys() const
{
    std::shared_lock lock(mutex_);
    std::vector<std::string> keys;
    keys.reserve(data_.size());
    for (const auto& pair : data_)
        keys.push_back(pair.first);
    return keys;
}
```

#### 3. GlobalMethods.h additions
```cpp
/**
 * Sets a shared data value accessible from all Eluna states.
 * Uses lmarshal to serialize Lua values.
 *
 * @param string key : unique identifier for the data
 * @param any value : Lua value to store (table, string, number, boolean, nil)
 */
int SetSharedData(Eluna* E)
{
    const char* key = E->CHECKVAL<const char*>(1);
    
    // Serialize the value using lmarshal
    lua_State* L = E->L;
    lua_pushvalue(L, 2);  // Push value to serialize
    mar_encode(L);        // Returns serialized string
    
    size_t len;
    const char* data = lua_tolstring(L, -1, &len);
    
    sElunaSharedData->Set(key, std::string(data, len));
    
    lua_pop(L, 1);  // Pop serialized string
    return 0;
}

/**
 * Gets a shared data value accessible from all Eluna states.
 *
 * @param string key : unique identifier for the data
 * @return any value : the stored Lua value, or nil if not found
 */
int GetSharedData(Eluna* E)
{
    const char* key = E->CHECKVAL<const char*>(1);
    
    std::string serialized;
    if (!sElunaSharedData->Get(key, serialized))
    {
        E->Push();  // Push nil
        return 1;
    }
    
    // Deserialize using lmarshal
    lua_State* L = E->L;
    lua_pushlstring(L, serialized.data(), serialized.size());
    mar_decode(L);
    
    return 1;
}

/**
 * Clears a shared data value.
 *
 * @param string key : unique identifier for the data to clear
 */
int ClearSharedData(Eluna* E)
{
    const char* key = E->CHECKVAL<const char*>(1);
    sElunaSharedData->Clear(key);
    return 0;
}

/**
 * Checks if a shared data key exists.
 *
 * @param string key : unique identifier to check
 * @return bool exists : true if the key exists
 */
int HasSharedData(Eluna* E)
{
    const char* key = E->CHECKVAL<const char*>(1);
    E->Push(sElunaSharedData->Has(key));
    return 1;
}
```

### Usage Example (AMS Message Reassembly)

```lua
-- In AMS_Server.lua

local function HandleIncomingMessage(player, rawMessage)
    local playerGUID = player:GetGUIDLow()
    local dataKey = "AMS_PLAYER_" .. playerGUID
    
    -- Get existing player data from shared storage
    local playerData = GetSharedData(dataKey) or { pendingMessages = {} }
    
    -- ... process message part ...
    
    -- Store updated data back to shared storage
    SetSharedData(dataKey, playerData)
    
    -- Check if complete
    if msgData.receivedParts == msgData.totalParts then
        -- Reassemble and process
        ClearSharedData(dataKey)  -- Clean up
        return completeMessage
    end
    
    return nil
end
```

## Implementation Steps

### Phase 1: Core Implementation
1. [ ] Create `ElunaSharedData.h` and `ElunaSharedData.cpp`
2. [ ] Update `CMakeLists.txt` to include new files
3. [ ] Add `mar_encode` and `mar_decode` declarations to header
4. [ ] Add new functions to `GlobalMethods.h`
5. [ ] Register new functions in the method table

### Phase 2: Testing
1. [ ] Create Lua test script for shared data
2. [ ] Test cross-state data persistence
3. [ ] Test thread safety with concurrent access
4. [ ] Update AMS to use shared data

### Phase 3: AMS Integration
1. [ ] Update `AMS_Server.lua` to use `SetSharedData`/`GetSharedData`
2. [ ] Remove workarounds for `_G` isolation
3. [ ] Run AMS test suite - all 8 tests should pass

## Thread Safety Considerations

- `std::shared_mutex` allows multiple readers OR single writer
- All public methods acquire appropriate locks
- Serialization/deserialization happens outside the lock when possible
- No Lua state access while holding the mutex (prevents deadlocks)

## Performance Notes

- Serialization adds overhead (~microseconds for small tables)
- For high-frequency access, consider caching in Lua with periodic sync
- Large tables should be avoided (serialize only what's needed)

## Future Enhancements

1. **TTL Support**: Auto-expire keys after timeout
2. **Pub/Sub**: Notify other states when data changes
3. **Namespaces**: Prefix-based key grouping for easier cleanup
4. **Size Limits**: Prevent memory exhaustion from large values
