# LuaEngine Test Coverage Analysis

**Date**: November 2, 2025
**Status**: Comprehensive coverage assessment

## Current Test Coverage (98 tests)

### ✅ Well Covered

1. **Core Lua Execution** (9 tests)
   - Basic variable assignment, functions, tables
   - Arithmetic, strings, conditionals, loops
   - Error handling, state isolation

2. **Data Type Conversions** (13 tests)
   - Lua ↔ C++ type conversions
   - Type coercion, table handling

3. **Event System** (8 tests)
   - Event registration, callbacks, persistence
   - Unregistration, priority ordering

4. **Integration** (8 tests)
   - Script lifecycle, cross-script access
   - Module patterns, event managers

5. **Edge Cases** (13 tests)
   - Nil/false handling, large numbers
   - Recursion, closures, metamethods

6. **Method Bindings** (11 tests)
   - Binding availability, signatures
   - Return/argument conversion

7. **Global Eluna Tests** (7 tests)
   - World singleton, GetEluna consistency
   - Configuration awareness

8. **Server Integration** (6 tests)
   - Map::GetEluna(), ElunaInfo management
   - ElunaMgr singleton, configuration

---

## ❌ NOT Covered - Priority Areas

### 1. **Eluna Core Functionality** (HIGH PRIORITY)

#### A. Lua State Management
- `OpenLua()` - Lua state initialization
- `CloseLua()` - Lua state cleanup
- `HasLuaState()` - State existence check
- Lua library initialization (luaL_openlibs)
- Require path configuration
- Precompiled script loader

**Why Important**: Core to Eluna functionality
**Test Approach**: Can test through ExecuteScript validation

#### B. Script Execution & Reloading
- `RunScripts()` - Script loading and execution
- `_ReloadEluna()` - Eluna reload mechanism
- `ReloadEluna()` - Reload flag setting
- Script cache integration
- Reload state management

**Why Important**: Scripts are the core of Eluna
**Test Approach**: Test script execution and reload behavior

#### C. Binding Management
- `CreateBindStores()` - Binding store creation
- `DestroyBindStores()` - Binding store cleanup
- `GetBinding<T>()` - Retrieve binding maps
- Binding map types (Server, Player, Guild, Group, etc.)

**Why Important**: Bindings connect Lua to C++ objects
**Test Approach**: Verify binding store creation/destruction

---

### 2. **Event Management** (HIGH PRIORITY)

#### A. EventMgr Class
- `EventMgr::UpdateProcessors()` - Process timed events
- `EventMgr::SetStates()` - Set event states
- `EventMgr::SetState()` - Set specific event state
- Event processor management
- Timed event execution

**Why Important**: Events are how Lua scripts respond to game events
**Test Approach**: Test event registration and execution

#### B. Timed Events
- `OnTimedEvent()` - Register timed events
- Event delay handling
- Event call count limiting
- Event cancellation

**Why Important**: Timed events are critical for scripting
**Test Approach**: Test event scheduling and execution

---

### 3. **Lua Stack Operations** (MEDIUM PRIORITY)

#### A. Push Operations
- `Push()` - Push nil
- `Push(int)`, `Push(float)`, `Push(bool)`, `Push(string)`
- `Push(Object*)`, `Push(Unit*)`, `Push(Pet*)`, etc.
- `Push(ObjectGuid)`
- `HookPush()` variants

**Why Important**: Core mechanism for passing data to Lua
**Test Approach**: Test type conversions through script execution

#### B. Check Operations
- `CHECKVAL<T>()` - Check and convert values
- `CHECKOBJ<T>()` - Check object types
- `CHECKTYPE()` - Check Lua types

**Why Important**: Validates data from Lua scripts
**Test Approach**: Test through method binding validation

---

### 4. **Instance Data Management** (MEDIUM PRIORITY)

#### A. Instance Data Lifecycle
- `HasInstanceData()` - Check instance data existence
- `CreateInstanceData()` - Create instance data table
- `PushInstanceData()` - Push instance data to stack
- Instance data persistence
- Instance data reloading

**Why Important**: Instance data persists across reloads
**Test Approach**: Test data creation and retrieval

#### B. Continent Data
- Continent-specific data storage
- Continent data references

**Why Important**: Persistent data for continents
**Test Approach**: Test data storage and retrieval

---

### 5. **Hook System** (MEDIUM PRIORITY)

#### A. Hook Registration
- `Register()` - Register event handlers
- Hook entry/guid/instance management
- Function reference management
- Shot counting (limited executions)

**Why Important**: Hooks connect game events to Lua handlers
**Test Approach**: Test hook registration and execution

#### B. Hook Globals
- `RegisterHookGlobals()` - Register event ID table
- Event ID lookup
- Hook category organization

**Why Important**: Scripts need access to event IDs
**Test Approach**: Test event ID availability in scripts

---

### 6. **AI Integration** (MEDIUM PRIORITY)

#### A. CreatureAI Integration
- `GetAI()` - Get creature AI
- `ElunaCreatureAI` initialization
- AI hook execution (UpdateAI, EnterCombat, etc.)

**Why Important**: Creatures use Lua for AI scripting
**Test Approach**: Test through creature event hooks

#### B. InstanceData Integration
- `GetInstanceData()` - Get instance data
- Instance AI initialization
- Instance data loading/saving

**Why Important**: Instances use Lua for scripting
**Test Approach**: Test through instance event hooks

---

### 7. **Query Processing** (LOW PRIORITY - Trinity Only)

#### A. Async Queries
- `GetQueryProcessor()` - Get query processor
- Query callback handling
- Query cancellation

**Why Important**: Async database queries from Lua
**Test Approach**: Would require database setup

---

### 8. **Utility Functions** (LOW PRIORITY)

#### A. Error Handling
- `StackTrace()` - Get Lua stack trace
- `Report()` - Report Lua errors
- Error message formatting

**Why Important**: Debugging Lua scripts
**Test Approach**: Test error reporting

#### B. Utility Methods
- `GetEluna()` - Get Eluna from Lua state
- `GetBoundMap()` - Get map Eluna is bound to
- `GetBoundMapId()` - Get map ID
- `GetBoundInstanceId()` - Get instance ID
- `GetCallstackId()` - Get callstack ID
- `UpdateEluna()` - Update Eluna
- `ExecuteCall()` - Execute Lua call

**Why Important**: Core Eluna operations
**Test Approach**: Test through script execution

---

### 9. **Game Event Hooks** (LOW PRIORITY - Requires Server Context)

These require full server initialization:
- `OnCommand()` - Chat command handling
- `OnWorldUpdate()` - World update events
- `OnLootItem()`, `OnLootMoney()` - Loot events
- `OnFirstLogin()`, `OnEquip()`, `OnRepop()`, `OnResurrect()` - Player events
- `OnQuestAbandon()`, `OnQuestStatusChanged()` - Quest events
- `OnLearnTalents()`, `OnSkillChange()`, `OnLearnSpell()` - Learning events
- `OnCanUseItem()` - Item usage validation
- `OnAddonMessage()` - Addon messaging
- `OnTradeInit()`, `OnTradeAccept()` - Trade events
- `OnSendMail()` - Mail events
- `OnDiscoverArea()` - Discovery events
- `OnLuaStateOpen()`, `OnLuaStateClose()` - State lifecycle

**Why Important**: Game integration hooks
**Test Approach**: Would require server initialization

---

## Recommended Test Implementation Order

### Phase 1 (HIGH PRIORITY - Can be done now)
1. **Lua State Management Tests** - Test OpenLua/CloseLua through script execution
2. **Script Execution Tests** - Test RunScripts and script loading
3. **Binding Management Tests** - Test binding store creation/destruction
4. **Event Management Tests** - Test EventMgr and timed events

### Phase 2 (MEDIUM PRIORITY - Requires some setup)
1. **Instance Data Tests** - Test data creation and persistence
2. **Hook Registration Tests** - Test hook registration and execution
3. **Stack Operations Tests** - Test Push/Check operations
4. **AI Integration Tests** - Test creature/instance AI

### Phase 3 (LOW PRIORITY - Requires full server)
1. **Game Event Hooks** - Requires full server initialization
2. **Query Processing** - Requires database setup
3. **Advanced Integration** - Complex multi-component scenarios

---

## Implementation Strategy

### For Phase 1 Tests
Create a new test file: `LuaEngineCore.cpp`
- Test Lua state creation/destruction
- Test script execution and loading
- Test binding store management
- Test event manager functionality

### For Phase 2 Tests
Create a new test file: `LuaEngineAdvanced.cpp`
- Test instance data lifecycle
- Test hook registration
- Test stack operations
- Test AI integration

### For Phase 3 Tests
Create a new test file: `LuaEngineIntegration.cpp`
- Test game event hooks (requires server context)
- Test query processing (requires database)
- Test complex scenarios

---

## Summary

**Currently Tested**: 98 tests covering:
- Core Lua execution (9)
- Type conversions (13)
- Event system basics (8)
- Integration patterns (8)
- Edge cases (13)
- Method bindings (11)
- Global Eluna (7)
- Server integration (6)

**Still Needs Testing**: ~50+ additional test cases for:
- Lua state management
- Script execution and reloading
- Binding management
- Event management details
- Instance data lifecycle
- Hook registration
- Stack operations
- AI integration
- Game event hooks (requires server)

**Estimated Additional Coverage**: 50-70 more tests could achieve ~95% coverage of testable code
