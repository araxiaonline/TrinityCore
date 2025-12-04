# Global Eluna State Implementation Plan

## Overview
Implement a global Eluna instance that runs world-wide scripts, separate from per-map instances. This allows scripts to run globally without being tied to any specific map/instance.

**Current Status**: World::GetEluna() returns nullptr (placeholder)
**Goal**: Full implementation with proper initialization and lifecycle management

---

## ✅ Validation Against Reference Implementations

### Summary of Validation Results
**Status**: ✅ **PLAN VALIDATED AND CORRECTED**

Our initial plan was ~95% correct. Key corrections based on reference implementations:

1. **ElunaInfo Storage**: Use **value type** instead of `unique_ptr`
   - Reference: `ElunaInfo _elunaInfo;` (not `std::unique_ptr<ElunaInfo>`)
   - Benefit: Automatic cleanup via destructor, simpler code

2. **Conditional Compilation**: Wrap all Eluna code in `#ifdef ELUNA`
   - Reference: All Eluna code is conditionally compiled
   - Benefit: Clean separation, can disable Eluna entirely

3. **Config Checks**: Check `sElunaConfig->IsElunaEnabled()` before initialization
   - Reference: Both World and Map check this
   - Benefit: Respects admin configuration

4. **Map-Specific Logic**: Only create Eluna for parent maps or non-instanceable maps
   - Reference: `if (!IsParentMap() || (IsParentMap() && !Instanceable()))`
   - Benefit: Avoids duplicate Eluna instances for instances

5. **LuaVal Member**: Add `LuaVal lua_data` to Map for Lua data storage
   - Reference: Present in Map.h
   - Benefit: Allows Lua scripts to attach data to maps

### Reference Sources
- **ElunaTrinityWotlk**: WotLK (3.3.5) implementation with Eluna (VERIFIED)
- **ElunaCataPreservation**: Cataclysm preservation fork (reference available)

### Key Findings from Reference Code

#### 1. World Implementation (ElunaTrinityWotlk)
```cpp
// World.h
#ifdef ELUNA
Eluna* GetEluna() const { return sElunaMgr->Get(_elunaInfo); }
#endif

// World.cpp - Constructor initialization
#ifdef ELUNA
if (sElunaConfig->IsElunaEnabled())
{
    TC_LOG_INFO("server.loading", "Starting Eluna world state...");
    _elunaInfo = { ElunaInfoKey::MakeGlobalKey(0) };
    sElunaMgr->Create(nullptr, _elunaInfo);
}
#endif
```

**Key Differences from Our Plan**:
- Uses `ElunaInfo` as **value type** (not `unique_ptr`)
- Wraps in `#ifdef ELUNA` conditional compilation
- Checks `sElunaConfig->IsElunaEnabled()` before initialization
- Initializes in World constructor (matches our plan)
- Passes `nullptr` for map (matches our plan)

#### 2. Map Implementation (ElunaTrinityWotlk)
```cpp
// Map.h
#ifdef ELUNA
Eluna* GetEluna() const { return sElunaMgr->Get(_elunaInfo); }
LuaVal lua_data = LuaVal({});
#endif

// Map.cpp - Constructor initialization
#ifdef ELUNA
if (sElunaConfig->IsElunaEnabled() && sElunaConfig->ShouldMapLoadEluna(id))
    if (!IsParentMap() || (IsParentMap() && !Instanceable()))
    {
        _elunaInfo = { ElunaInfoKey::MakeKey(GetId(), GetInstanceId()) };
        sElunaMgr->Create(this, _elunaInfo);
    }
#endif
```

**Key Differences from Our Plan**:
- Uses `ElunaInfo` as **value type** (not `unique_ptr`)
- Additional config check: `ShouldMapLoadEluna(id)`
- Conditional logic: only creates for parent maps or non-instanceable maps
- Includes `LuaVal lua_data` member (for Lua data storage)

#### 3. Eluna Usage Pattern
```cpp
// In Map.cpp - when removing objects
if (Eluna* e = GetEluna())
{
    if (Creature* creature = obj->ToCreature())
        e->OnRemove(creature);
    else if (GameObject* gameobject = obj->ToGameObject())
        e->OnRemove(gameobject);
}
```

**Pattern**: Safe null-check before using Eluna

---

## Architecture Analysis

### Current System
1. **ElunaMgr**: Singleton managing all Eluna instances
   - Uses `ElunaInfoKey` to uniquely identify each Eluna instance
   - Key format: `(mapId << 32) | instanceId`
   - Special key for global: `GLOBAL_MAP_ID | 0` where `GLOBAL_MAP_ID = std::numeric_limits<uint32>().max()`

2. **ElunaInfo**: Wrapper around ElunaInfoKey
   - Stores the key
   - Provides `GetEluna()` method that queries ElunaMgr
   - Auto-destroys Eluna instance when ElunaInfo is destroyed

3. **Eluna Constructor**: Takes `Map* map` parameter
   - For global state, we need to handle `nullptr` map gracefully
   - Eluna uses `boundMap` to access map-specific functionality

### Key Insight
The Eluna architecture already supports global instances! The `ElunaInfoKey::MakeGlobalKey()` method exists and `ElunaInfo::IsGlobal()` checks for it. We just need to:
1. Create a global ElunaInfo instance
2. Handle nullptr map in Eluna class
3. Initialize it at the right time

---

## Implementation Plan

### Phase 1: Modify Eluna Constructor to Handle Global State

**File**: `src/server/game/LuaEngine/LuaEngine.h` and `LuaEngine.cpp`

**Changes**:
1. Make `boundMap` nullable (already is `Map*`)
2. Add safety checks before accessing `boundMap` in methods
3. Add `IsGlobal()` method to Eluna class
4. Document which methods are map-specific vs global

**Scope**: 
- Identify all places where `boundMap` is dereferenced
- Add null checks or assertions with clear error messages
- Ensure global Eluna can still run scripts

**Reference Implementation Note**:
- WotLK implementation doesn't show extensive null-safety changes
- Suggests Eluna already handles null maps gracefully in most operations
- Main usage pattern: `if (Eluna* e = GetEluna()) { ... }` provides safety

**Estimated Impact**: Low - Eluna likely already handles null maps for some operations

---

### Phase 2: Add Global Eluna Member to World Class

**File**: `src/server/game/World/World.h` and `World.cpp`

**Changes in World.h**:
1. Add forward declaration for `ElunaInfo` (already done in Map.h)
2. Add private member: `ElunaInfo _elunaInfo;` **(value type, not unique_ptr)**
3. Update `GetEluna()` method with proper implementation

**Changes in World.h** (declaration):
```cpp
#ifdef ELUNA
private:
    ElunaInfo _elunaInfo;
public:
    Eluna* GetEluna() const { return sElunaMgr->Get(_elunaInfo); }
#endif
```

**Changes in World.cpp**:
1. In World constructor: Initialize global Eluna
   ```cpp
   #ifdef ELUNA
   if (sElunaConfig->IsElunaEnabled())
   {
       TC_LOG_INFO("server.loading", "Starting Eluna world state...");
       _elunaInfo = { ElunaInfoKey::MakeGlobalKey(0) };  // Value initialization
       sElunaMgr->Create(nullptr, _elunaInfo);  // nullptr for map
   }
   #endif
   ```

2. No destructor changes needed - ElunaInfo destructor handles cleanup automatically

**Key Differences from Initial Plan**:
- Use `ElunaInfo` as **value type** (not `unique_ptr`)
- ElunaInfo destructor automatically calls `sElunaMgr->Destroy()` when destroyed
- Wrap in `#ifdef ELUNA` for conditional compilation
- Check `sElunaConfig->IsElunaEnabled()` before initialization

**Timing Considerations**:
- World constructor is called early in server startup
- ElunaMgr must be initialized before World
- ElunaConfig must be initialized before World
- ElunaLoader must have scripts ready (or will reload when ready)

---

### Phase 3: Handle Null Map in Eluna Methods

**File**: `src/server/game/LuaEngine/LuaEngine.h` and `LuaEngine.cpp`

**Analysis Needed**:
1. Search for all `boundMap->` dereferences
2. Categorize as:
   - **Map-specific**: Only work with map (e.g., grid operations)
   - **Global-safe**: Work without map (e.g., script execution, timers)
   - **Conditional**: Can work with or without map

**Implementation Strategy**:
1. Add `IsGlobal()` method: `return boundMap == nullptr;`
2. Add safety checks before map access:
   ```cpp
   if (boundMap)
   {
       // map-specific code
   }
   ```
3. Add assertions for map-required operations:
   ```cpp
   ASSERT(boundMap, "This operation requires a map context");
   ```

**Expected Map-Specific Operations**:
- Grid/cell operations
- Creature/GameObject spawning in specific locations
- Map-local events
- Weather management

**Expected Global-Safe Operations**:
- Script loading/execution
- Timer management
- Global event hooks
- World-wide announcements

---

### Phase 4: Update World Initialization Order

**File**: `src/server/game/World/World.cpp`

**Analysis**:
1. Verify ElunaMgr is initialized before World
2. Verify ElunaLoader is initialized before World
3. Check if any World initialization depends on Eluna

**Changes**:
1. Add comments documenting initialization order requirements
2. Add assertions to verify dependencies
3. Handle case where ElunaLoader scripts aren't ready yet

**Code Pattern**:
```cpp
// In World constructor, after other initialization:
if (sElunaLoader && sElunaMgr)
{
    _globalElunaInfo = std::make_unique<ElunaInfo>(
        ElunaInfoKey::MakeGlobalKey(0)
    );
    sElunaMgr->Create(nullptr, *_globalElunaInfo);
}
```

---

### Phase 4.5: Update Map Implementation (CORRECTION)

**Current Implementation Issue**:
Our current Map implementation uses `std::unique_ptr<ElunaInfo>` but reference implementation uses value type.

**Required Changes**:
1. Change Map.h member from `std::unique_ptr<ElunaInfo> _elunaInfo;` to `ElunaInfo _elunaInfo;`
2. Update Map.cpp initialization from `_elunaInfo = std::make_unique<...>` to `_elunaInfo = { ... }`
3. Add conditional logic for parent maps and instanceable maps
4. Add `LuaVal lua_data` member for Lua data storage
5. Wrap in `#ifdef ELUNA` blocks

**Reference Code Pattern**:
```cpp
#ifdef ELUNA
if (sElunaConfig->IsElunaEnabled() && sElunaConfig->ShouldMapLoadEluna(id))
    if (!IsParentMap() || (IsParentMap() && !Instanceable()))
    {
        _elunaInfo = { ElunaInfoKey::MakeKey(GetId(), GetInstanceId()) };
        sElunaMgr->Create(this, _elunaInfo);
    }
#endif
```

---

### Phase 5: Testing & Validation

**Manual Testing**:
1. Verify World::GetEluna() returns non-null after startup
2. Verify global Eluna instance is different from map instances
3. Verify scripts can be loaded into global instance
4. Verify global scripts execute without map context
5. Verify no crashes when accessing map-specific features from global

**Code Review Points**:
1. All null checks are in place
2. Error messages are clear
3. No memory leaks (ElunaInfo cleanup via destructor)
4. Initialization order is correct
5. Global vs map-specific operations are properly separated
6. Conditional compilation with `#ifdef ELUNA` is consistent

---

## Implementation Order

### Step 1: Prepare Eluna Class (Low Risk)
- Add `IsGlobal()` method
- Add null-safety checks for `boundMap`
- Document map-specific vs global-safe methods
- **Risk**: Low - mostly defensive coding

### Step 2: Add World Member (Low Risk)
- Add `_globalElunaInfo` member
- Update `GetEluna()` implementation
- **Risk**: Low - isolated to World class

### Step 3: Initialize in World Constructor (Medium Risk)
- Create global ElunaInfo
- Register with ElunaMgr
- Handle initialization order
- **Risk**: Medium - depends on other systems being ready

### Step 4: Handle Null Maps in Eluna (Medium Risk)
- Add checks before `boundMap` dereferences
- Test that global scripts work
- **Risk**: Medium - affects core Eluna functionality

### Step 5: Integration Testing (Low Risk)
- Build and test server startup
- Verify global scripts load
- Verify no crashes
- **Risk**: Low - just testing

---

## Key Decisions

### 1. Global Instance ID
**Decision**: Use `0` as the global instance ID
**Rationale**: 
- Unique from any real instance IDs (which start at 1)
- Simple and clear
- Follows convention of 0 = global/default

### 2. Null Map Handling
**Decision**: Allow `nullptr` map in Eluna, add safety checks
**Rationale**:
- Minimal changes to Eluna
- Follows defensive programming
- Clear error messages for map-required operations
- Backward compatible

### 3. Initialization Timing
**Decision**: Initialize global Eluna in World constructor
**Rationale**:
- World is created early in startup
- All dependencies (ElunaMgr, ElunaLoader) are available
- Consistent with map-based Eluna initialization

### 4. Lifecycle Management
**Decision**: Use ElunaInfo for automatic cleanup
**Rationale**:
- Consistent with map-based approach
- Automatic destruction when World is destroyed
- No manual cleanup needed

---

## Potential Issues & Mitigations

### Issue 1: Initialization Order
**Problem**: ElunaMgr or ElunaLoader might not be ready when World is created
**Mitigation**: 
- Add null checks: `if (sElunaLoader && sElunaMgr)`
- Add assertions with clear messages
- Document required initialization order

### Issue 2: Map-Specific Operations in Global Context
**Problem**: Scripts might try to use map-specific features globally
**Mitigation**:
- Add assertions: `ASSERT(boundMap, "Operation requires map context")`
- Document which operations are map-specific
- Provide clear error messages

### Issue 3: Performance Impact
**Problem**: Extra null checks might impact performance
**Mitigation**:
- Checks only in methods that access `boundMap`
- Most operations don't access map directly
- Minimal performance impact expected

### Issue 4: Script Compatibility
**Problem**: Existing scripts might assume map context
**Mitigation**:
- Global scripts are new feature
- Existing map scripts unaffected
- Clear documentation for global script authors

---

## Files to Modify

### Required Changes
1. **src/server/game/LuaEngine/LuaEngine.h**
   - Add `IsGlobal()` method declaration
   - Document null-safety

2. **src/server/game/LuaEngine/LuaEngine.cpp**
   - Implement `IsGlobal()` method
   - Add null checks before `boundMap` dereferences
   - Document changes

3. **src/server/game/World/World.h**
   - Add `_globalElunaInfo` member
   - Update `GetEluna()` implementation

4. **src/server/game/World/World.cpp**
   - Initialize global Eluna in constructor
   - Add comments about initialization order

### Optional Enhancements
1. **src/server/game/LuaEngine/LuaEngine.h**
   - Add `GetBoundMap()` accessor
   - Add `IsMapBound()` method

2. **Documentation**
   - Add comments about global vs map-specific operations
   - Update ARAXIA_TODO.md with progress

---

## Success Criteria

✅ **Build succeeds** with no new errors
✅ **World::GetEluna()** returns non-null after server startup
✅ **Global Eluna instance** is properly initialized
✅ **No crashes** when accessing global Eluna
✅ **Map-specific operations** still work correctly
✅ **Global scripts** can be loaded and executed
✅ **Memory management** is correct (no leaks)
✅ **Error messages** are clear for map-required operations

---

## Estimated Effort

- **Phase 1** (Eluna null-safety): 30 minutes
- **Phase 2** (World member): 15 minutes
- **Phase 3** (Null map handling): 45 minutes
- **Phase 4** (Initialization order): 15 minutes
- **Phase 5** (Testing): 30 minutes

**Total**: ~2.5 hours

---

## Next Steps

1. Review this plan for any issues
2. Identify any additional null-safety checks needed
3. Verify initialization order in actual code
4. Begin Phase 1 implementation
5. Test after each phase

---

## Phase 6: Comprehensive Unit Test Suite (NEW)

**Status**: Infrastructure implemented ✅

### Test Suite Overview
Created comprehensive unit test suite for Eluna/LuaEngine with 64 tests organized into 6 categories:

**Test Categories**:
1. **Core Lua Execution** (11 tests) - Basic script loading, variables, functions, tables, error handling
2. **Data Type Conversions** (13 tests) - Lua ↔ C++ type conversions, coercion, tables
3. **Event System** (8 tests) - Event registration, callbacks, handlers, state persistence
4. **Integration** (8 tests) - Script lifecycle, cross-script access, modules, standard library
5. **Edge Cases** (13 tests) - Recursion, closures, metamethods, operator precedence
6. **Method Bindings** (11 tests - Placeholders) - Binding validation, null handling, enums

### Test Infrastructure
- **Fixture**: `LuaEngineTestFixture.h` with common utilities
- **Location**: `tests/game/LuaEngine/`
- **Framework**: Catch2 (existing TrinityCore test framework)
- **Build**: Automatically included when ELUNA is enabled

### Test Files Created
- `LuaEngineTestFixture.h` - Base fixture and utilities
- `CoreLuaExecution.cpp` - Core Lua functionality tests
- `DataTypeConversions.cpp` - Type conversion tests
- `EventSystem.cpp` - Event handling tests
- `Integration.cpp` - Integration scenario tests
- `EdgeCases.cpp` - Edge case and error handling tests
- `MethodBindings.cpp` - Method binding validation tests
- `CMakeLists.txt` - Build configuration
- `README.md` - Test documentation

### Running Tests
```bash
# Build with tests
cmake .. -DELUNA=1
make tests

# Run all tests
./bin/tests

# Run specific category
./bin/tests "[LuaEngine][CoreExecution]"
```

### Next Phase: Test Execution and Gap Analysis
1. Build and run test suite
2. Document passing/failing tests
3. Identify missing features
4. Prioritize fixes based on test results
5. Create regression test baseline

