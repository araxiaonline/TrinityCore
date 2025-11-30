# Eluna Reload Issue - Root Cause Analysis & Fix

**Date:** November 30, 2025  
**Issue:** `.reload eluna` command does not properly reload scripts  
**Status:** ✅ FIXED

---

## Root Cause Identified

**`Eluna::UpdateEluna()` is NEVER called!**

The reload mechanism works as follows:

1. `.reload eluna` command calls `sElunaLoader->ReloadElunaForMap(RELOAD_GLOBAL_STATE)`
2. This reloads the bytecode cache (works correctly)
3. Then calls `e->ReloadEluna()` which just sets `reload = true`
4. The actual reload happens in `Eluna::UpdateEluna()`:

```cpp
void Eluna::UpdateEluna(uint32 diff)
{
    if (reload && sElunaLoader->GetCacheState() == SCRIPT_CACHE_READY)
#if defined ELUNA_TRINITY
        if (GetQueryProcessor().Empty())
#endif
            _ReloadEluna();  // This is where scripts actually reload!

    eventMgr->UpdateProcessors(diff);
#if defined ELUNA_TRINITY
    GetQueryProcessor().ProcessReadyCallbacks();
#endif
}
```

**Problem:** `UpdateEluna()` is defined but never called from the main game loops!

---

## What Should Be Happening

### For Global (World) Eluna:
```cpp
// In World::Update(uint32 diff) - should exist but doesn't
if (Eluna* e = GetEluna())
    e->UpdateEluna(diff);
```

### For Per-Map Eluna:
```cpp
// In Map::Update(uint32 diff) - should exist but doesn't
if (Eluna* e = GetEluna())
    e->UpdateEluna(diff);
```

---

## Current Flow (Broken)

```
.reload eluna
    │
    ▼
ReloadElunaForMap(RELOAD_GLOBAL_STATE)
    │
    ├─► ReloadScriptCache() → Thread recompiles bytecode → WORKS ✓
    │
    └─► e->ReloadEluna() → Sets reload = true
                │
                ▼
            (DEAD END - nothing checks this flag!)
```

---

## What OnWorldUpdate Does (NOT the same!)

`Eluna::OnWorldUpdate()` is a HOOK that fires Lua callbacks:
```cpp
void Eluna::OnWorldUpdate(uint32 diff)
{
    START_HOOK(WORLD_EVENT_ON_UPDATE);
    HookPush(diff);
    CallAllFunctions(binding, key);  // Calls Lua handlers
}
```

This is called from TrinityCore but it does NOT call `UpdateEluna()`!

---

## Fix Options

### Option 1: Call UpdateEluna from hooks (Minimal Change)

Add to `OnWorldUpdate` and `OnMapUpdate`:

```cpp
void Eluna::OnWorldUpdate(uint32 diff)
{
    UpdateEluna(diff);  // ADD THIS LINE
    
    START_HOOK(WORLD_EVENT_ON_UPDATE);
    HookPush(diff);
    CallAllFunctions(binding, key);
}
```

**Pros:** Single-line fix, minimal code change  
**Cons:** Reload only happens when OnWorldUpdate is called (which requires scripts to register for it)

### Option 2: Explicit calls in World.cpp and Map.cpp (Proper Fix)

**World.cpp - Add to Update():**
```cpp
void World::Update(uint32 diff)
{
    // ... existing code ...
    
    // Update Eluna (process reload, events, async queries)
    if (Eluna* e = GetEluna())
        e->UpdateEluna(diff);
    
    // ... rest of update ...
}
```

**Map.cpp - Add to Update():**
```cpp
void Map::Update(uint32 diff)
{
    // ... existing code ...
    
    // Update Eluna for this map
    if (Eluna* e = GetEluna())
        e->UpdateEluna(diff);
    
    // ... rest of update ...
}
```

**Pros:** Proper integration, reload always works  
**Cons:** More code changes, need to find right location in Update functions

### Option 3: Make reload synchronous (Quick Fix)

Change `ReloadEluna()` to call `_ReloadEluna()` directly:

```cpp
void Eluna::ReloadEluna() 
{ 
    // Instead of: reload = true;
    _ReloadEluna();  // Do it now!
}
```

**Pros:** Immediate reload, simplest fix  
**Cons:** May have threading issues, doesn't update event processors properly

---

## Recommended Fix: Option 2

Add explicit `UpdateEluna()` calls to both `World::Update()` and `Map::Update()`.

This is the proper fix because:
1. `UpdateEluna` does more than just reload - it processes events and queries
2. It should be called every update tick regardless of reload
3. It matches how other subsystems are updated

---

## Files Modified ✅

### 1. `src/server/game/World/World.cpp`

Added to `World::Update()` after sWorldUpdateTime.UpdateWithDiff:

```cpp
///- Update Eluna (process reload flag, timed events, async queries)
if (Eluna* e = GetEluna())
    e->UpdateEluna(diff);
```

### 2. `src/server/game/Maps/Map.cpp`

Added to `Map::Update()` at the beginning of the function:

```cpp
///- Update Eluna for this map (process reload flag, timed events, async queries)
if (Eluna* e = GetEluna())
    e->UpdateEluna(t_diff);
```

### 3. `src/server/game/LuaEngine/LuaEngine.cpp`

Added INFO-level logging to show each script as it loads:

```cpp
ELUNA_LOG_INFO("[Eluna]: Loaded script: %s", it->filepath.c_str());
```

---

## Testing the Fix

After implementing:

1. Start server, verify scripts load normally
2. Modify a Lua script (add a print statement)
3. Run `.reload eluna`
4. Verify:
   - "Reloading Eluna scripts..." message appears
   - Script init messages appear (print statements from init.lua)
   - New script changes take effect

---

## Why This Wasn't Caught Before

The Eluna codebase has hooks (`OnWorldUpdate`, `OnMapUpdate`) that sound like they should handle this, but they're actually Lua event dispatchers, not the internal update function.

The naming is confusing:
- `OnWorldUpdate()` = Fire Lua event WORLD_EVENT_ON_UPDATE
- `UpdateEluna()` = Internal maintenance (reload, events, queries)

These are completely different functions!
