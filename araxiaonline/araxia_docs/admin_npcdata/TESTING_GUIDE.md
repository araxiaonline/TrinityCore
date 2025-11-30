# NPC Server Data - Testing Guide

**Feature:** Server-side NPC data in AraxiaTrinityAdmin  
**Status:** Phase 1 Complete - Ready for Testing

---

## Quick Start

### 1. Server Setup

**Restart worldserver** (`.reload eluna` has caching issues):
```bash
# In container or server console
# Stop worldserver, then restart
```

**Verify server logs** show:
```
[AMS Server] AMS Server v1.0.0-alpha initialized
[Admin Handlers] Loaded successfully!
[Admin Handlers] Registered handlers:
  - GET_NPC_DATA
```

### 2. Client Setup

**Reload addons** in-game:
```
/reload
```

**Verify addons loaded:**
```
/ams echo test         # Should work (AMS_Test)
```

**Check for errors:**
- Look for Lua errors on screen
- Check that AMS_Client and AraxiaTrinityAdmin both loaded

### 3. Test the Feature

1. **Open AraxiaTrinityAdmin:**
   - Click minimap button
   - Select "NPC Info" tab

2. **Target an NPC:**
   - Target any creature/NPC in the world

3. **Click "Refresh":**
   - Should see "Loading from server..." briefly
   - Then server data should appear below client data

**Expected Output:**
```
=== Basic Information ===
Name: Oathsworn Peacekeeper
NPC ID: 219014
GUID: Creature-0-3-2552-0-219014-0000000995

=== Stats ===
Level: 81
Health: 64598127 / 64598127 (100.0%)
Classification: Normal
Reaction: Friendly

=== Additional Info ===
Creature Type: Humanoid
Faction: 35

=== TrinityCore Commands ===
...

Click 'Refresh' to fetch detailed server data

[After clicking Refresh]

=== Server Data ===
Fetched: 10:15:23

Base Stats:
  Strength: 120
  Agility: 80
  Stamina: 150
  Intellect: 100
  Spirit: 90

Vitals:
  Health: 64598127 / 64598127 (100.0%)

Movement Speeds:
  Walk: 1.00
  Run: 1.14
  Fly: 1.14

Spell Power:
  Physical: 0
  Holy: 0
  ...

Scripts & AI:
  AI: SmartAI
  
Behavior:
  Respawn: 120 seconds
  Wander: 10.0 yards
  Flags: Regen HP

--- Notes ---
Combat stats (armor, damage, resistances) require custom Eluna methods
Template data (gold, loot, flags) requires CreatureTemplate access
These will be added in Phase 2/3 of development
```

---

## Debugging

### Server Side

**Check server logs:**
```
tail -f /opt/trinitycore/logs/Server.log | grep -i "admin\|ams"
```

**Look for:**
- `[Admin Handlers] GET_NPC_DATA: Fetching data for GUID: ...`
- `[AMS Server] Received message from <player>`
- Any ERROR messages

**Common Issues:**

**Issue:** Handler not registered
```
[AMS Server] ERROR: No handler registered for GET_NPC_DATA
```
**Fix:** Make sure `admin_handlers.lua` is loaded in `init.lua`

**Issue:** Creature not found
```
[Admin Handlers] GET_NPC_DATA: Creature not found: <guid>
```
**Fix:** GUID parsing issue or creature despawned

### Client Side

**Enable AMS debug:**
```lua
-- In AMS_Client.lua
local AMS_DEBUG = true
```

**Check for errors:**
- Look for red error text on screen
- `/console scriptErrors 1` to see Lua errors

**Common Issues:**

**Issue:** "AMS not loaded"
```
[AraxiaTrinityAdmin] ERROR: AMS not loaded!
```
**Fix:** Make sure AMS_Client addon is enabled and loaded before AraxiaTrinityAdmin

**Issue:** Nothing happens on Refresh
- Check if AMS.Send is being called (enable AMS_DEBUG)
- Make sure you have a valid target
- Check server logs for received message

---

## Test Cases

### Test Case 1: Normal NPC
- **Target:** Any regular mob
- **Expected:** All data populates, rank = Normal

### Test Case 2: Elite NPC
- **Target:** Elite mob
- **Expected:** Classification shows "Elite", behavior flags include "Elite"

### Test Case 3: Boss
- **Target:** Dungeon/raid boss
- **Expected:** Classification shows "Boss", special flags

### Test Case 4: No Target
- **Action:** Click Refresh with no target
- **Expected:** "No valid NPC target found"

### Test Case 5: Player Target
- **Target:** Another player
- **Expected:** "No valid NPC target found" (not a creature)

### Test Case 6: Despawned Creature
- **Target:** NPC, then it despawns, then click Refresh
- **Expected:** Error message "Creature not found in world"

---

## Performance Testing

**Response Time:**
- Should be <100ms for local server
- May be higher for remote servers

**Memory:**
- Check addon memory: `/run print(GetAddOnMemoryUsage("AraxiaTrinityAdmin"))`
- Should be <5MB for typical usage

**Network:**
- AMS messages are text-based, should be small
- Check message size in debug logs

---

## Success Criteria

✅ Server handler loads without errors  
✅ Client addon loads without errors  
✅ Clicking Refresh shows "Loading..." state  
✅ Server data appears after brief delay  
✅ Data is formatted and readable  
✅ Multiple refreshes work correctly  
✅ Targeting different NPCs updates data  
✅ Error cases handled gracefully  

---

## Known Limitations (Phase 1)

⚠️ **Missing Combat Stats:**
- Armor, Attack Power, Damage, Attack Speed
- Resistances (Holy, Fire, Nature, Frost, Shadow, Arcane)
- Crit %, Hit %, Dodge %, Parry %

⚠️ **Missing Template Data:**
- Gold drop (min/max)
- Loot table ID
- Unit/NPC/Type flags
- Immunity masks

These will be added in **Phase 2** with custom Eluna C++ methods.

---

## Next Steps After Testing

1. ✅ **If it works:** Proceed to Phase 2 (custom Eluna methods)
2. 🐛 **If bugs found:** Debug and fix issues
3. 📝 **Document findings:** Update this guide with any discoveries

---

## Useful Commands

**Server:**
```bash
# Restart worldserver
docker exec -it trinitycore bash
supervisorctl restart worldserver

# Watch logs
tail -f /opt/trinitycore/logs/Server.log

# Check Eluna logs
tail -f /opt/trinitycore/logs/Eluna.log
```

**Client:**
```
/reload                          # Reload UI
/ams echo test                   # Test AMS
/console scriptErrors 1          # Show Lua errors
/run print(AMS and "AMS loaded" or "AMS NOT loaded")
/run print(AraxiaTrinityAdmin.ServerData and "ServerData loaded" or "ServerData NOT loaded")
```

**Debug Output:**
```lua
-- Temporarily add to ServerDataModule.lua or admin_handlers.lua
print("DEBUG:", variableName, value)
```

---

## Support

If you encounter issues, check:
1. Server logs for handler errors
2. Client for Lua errors
3. AMS debug output (if enabled)
4. This testing guide for common issues

**Remember:** Phase 1 only shows data available through existing Eluna methods. Full combat stats require Phase 2 (C++ extensions).
