# Eluna Architecture Analysis & Singleton Decision

## Current Eluna Architecture in TrinityCore

### How It Works Now

**Multi-Instance Architecture:**
- Eluna uses a **manager pattern**, not a global singleton
- Each Map/Instance can have its own Eluna state
- Global state exists for world-level scripts

**Key Components:**

1. **`ElunaMgr`** - Singleton manager that holds multiple Eluna instances
   ```cpp
   #define sElunaMgr ElunaMgr::instance()
   ```
   - Manages map for `ElunaInfoKey → Eluna*`
   - Creates/destroys Eluna instances per map/instance

2. **`ElunaInfo`** - Wrapper around an Eluna instance
   ```cpp
   class ElunaInfo {
       ElunaInfoKey key;
       Eluna* GetEluna() const;  // Returns the actual Eluna instance
   };
   ```

3. **Access Patterns:**
   ```cpp
   // Global Eluna state
   Eluna* eluna = sWorld->GetEluna();
   
   // Map-specific state
   Eluna* eluna = map->GetEluna();
   
   // Creature context (uses map's Eluna)
   Eluna* eluna = creature->GetEluna();
   ```

### Why No `sEluna` Global?

**By Design:**
- Supports per-map scripting isolation
- Different instances can have different scripts
- Allows map-specific event handling

**Current Singletons:**
- `sElunaMgr` - Instance manager
- `sElunaLoader` - Script loader
- `sElunaConfig` - Configuration

## AzerothCore Comparison

AzerothCore (3.3.5 base) likely used a simpler architecture with a global `sEluna` because:
1. Older TrinityCore version (pre-multi-instance support)
2. Simpler requirements (no per-instance scripting)
3. Less complex hook system

## Our Options for ChatHandler

### Option 1: Use World's Global Eluna ✅ RECOMMENDED

**Implementation:**
```cpp
// In ChatHandler.cpp
Eluna* eluna = sWorld->GetEluna();
if (eluna)
{
    eluna->OnAddonMessage(sender, type, fullMessage, receiver, nullptr, nullptr, nullptr);
}
```

**Pros:**
- ✅ Uses existing architecture correctly
- ✅ No new globals needed
- ✅ Works for global addon messages (which AIO is)
- ✅ Clean and maintainable
- ✅ Follows TrinityCore patterns

**Cons:**
- ⚠️ Requires including World.h in ChatHandler.cpp
- ⚠️ Slightly more verbose than `sEluna`

**Best For:** Our use case (AIO is global, not per-instance)

---

### Option 2: Use Player's Map Eluna

**Implementation:**
```cpp
// In ChatHandler.cpp
Eluna* eluna = sender->GetMap()->GetEluna();
if (eluna)
{
    eluna->OnAddonMessage(sender, type, fullMessage, receiver, nullptr, nullptr, nullptr);
}
```

**Pros:**
- ✅ Respects per-map isolation
- ✅ Uses existing architecture
- ✅ Player context is already available

**Cons:**
- ❌ AIO messages are global, not per-instance
- ❌ Could cause confusion if players in different instances
- ❌ Maps might not have Eluna if global-only scripts

**Best For:** If we wanted per-instance AIO (we don't)

---

### Option 3: Create `sEluna` Global Singleton ❌ NOT RECOMMENDED

**Implementation:**
```cpp
// In some header
#define sEluna (sWorld->GetEluna())
```

**Pros:**
- ✅ Short syntax
- ✅ Matches AzerothCore style

**Cons:**
- ❌ Hides the multi-instance architecture
- ❌ Could return nullptr if world not initialized
- ❌ Macro doesn't check for null
- ❌ Contradicts TrinityCore's design intent
- ❌ Makes future per-instance work harder

**Best For:** Nothing - this is a trap

---

### Option 4: Create True Eluna Singleton (Major Refactor) ❌ OVERKILL

**Implementation:**
- Refactor ElunaMgr to only manage one global instance
- Remove per-map Eluna support
- Create `sEluna` singleton

**Pros:**
- ✅ Simpler architecture (maybe)
- ✅ Matches older codebases

**Cons:**
- ❌ Huge refactor of existing code
- ❌ Breaks per-instance scripting
- ❌ Loses TrinityCore's architectural benefits
- ❌ Could break other Eluna features
- ❌ Not maintainable long-term

**Best For:** Complete rewrite (which we don't need)

---

## Recommendation: Option 1 (World's Global Eluna)

### Rationale

1. **Matches Project Philosophy**
   - We embrace custom C++ changes
   - We want proper, maintainable solutions
   - We're not afraid of "doing it right"

2. **Correct for AIO's Use Case**
   - AIO is a **global communication system**
   - Not tied to specific maps or instances
   - Should use global Eluna state

3. **Future-Proof**
   - If we ever want per-instance Lua scripts, that still works
   - Doesn't lock us into a simplified architecture
   - Follows TrinityCore patterns

4. **Clean Implementation**
   - One extra include
   - Clear, explicit access pattern
   - Easy to debug and understand

### Implementation

**In ChatHandler.cpp:**

```cpp
// At top with other includes
#include "World/World.h"

// In HandleChatAddonMessage, replace sEluna with:
Eluna* eluna = sWorld->GetEluna();
if (eluna)
{
    // Use eluna...
}
```

### Future Considerations

**If we ever need per-player/per-instance AIO:**
- We can switch to `sender->GetMap()->GetEluna()`
- Or create a decision function: `GetElunaForAIO(sender)`
- The architecture supports it

**If global Eluna is null:**
- This means Eluna is disabled or not initialized
- Our null check handles it gracefully
- Could add logging: "Eluna not available for AIO"

## Alternative: Helper Function (Optional Enhancement)

If we end up needing Eluna in many places, we could add a helper:

```cpp
// In a new ElunaUtil.h or in WorldSession.h
inline Eluna* GetGlobalEluna()
{
    return sWorld->GetEluna();
}
```

This gives us:
- Short syntax: `GetGlobalEluna()`
- Clear intent: "global" vs per-map
- Can add logging/safety checks in one place
- Can change implementation later

## Summary

**Do This:**
```cpp
#include "World/World.h"

Eluna* eluna = sWorld->GetEluna();
if (eluna)
{
    eluna->OnAddonMessage(...);
}
```

**Don't Do This:**
```cpp
// ❌ This doesn't exist
if (sEluna)
{
    sEluna->OnAddonMessage(...);
}
```

**Why:**
- Correct architecture usage
- Maintainable long-term
- Clear and explicit
- Works with TrinityCore's design
- Future-proof

## Decision

✅ **Use Option 1: `sWorld->GetEluna()`**

This is the right solution for our project's goals and Eluna's architecture.
