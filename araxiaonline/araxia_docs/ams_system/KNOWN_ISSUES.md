# AMS Known Issues & Investigation Notes

**Last Updated:** November 27, 2025  
**Status:** Active Development

---

## Issue #1: Eluna SendAddonMessage Packet Truncation

**Priority:** HIGH  
**Status:** ✅ RESOLVED  
**Affects:** All server → client messaging  
**Resolution Date:** November 28, 2025

### Problem Description
Server-side Lua successfully builds messages with 12-character hex headers, but the client receives only 4 characters. The remaining 8 characters are lost somewhere in the transmission pipeline.

### Evidence

**Server Logs:**
```
[AMS Server] Building packet - hex1: AAAA hex2: AAAA hex3: AAAA
[AMS Server] Header as string: AAAAAAAAAAAA length: 12
[AMS Server] Packet length: 102 first 20 chars: AAAAAAAAAAAA{{"ECHO
```

**Client Logs:**
```
[AMS Client] Received message from server, sender: Dastardly-AraxiaTrinity-Local
[AMS Client] Received part 43690 of 43690 for message ID 0
```
- 43690 decimal = 0xAAAA hex
- Client correctly parses "AAAA" as hex
- But only received 4 chars instead of 12

**Math:**
- Server sends: 102 bytes (12 header + 90 payload)
- Client receives: 94 bytes (4 header + 90 payload)
- Lost: 8 characters (exactly 2 × "AAAA")

### Root Cause: Double Prefix Stripping ✅ IDENTIFIED

The bug was in Eluna's `SendAddonMessage` implementation in `PlayerMethods.h`:

**Problematic Code:**
```cpp
std::string fullmsg = prefix + "\t" + message;  // Prepend "AMS\t" to message
chat.Initialize(channel, LANG_ADDON, player, receiver, fullmsg, 0, "", DEFAULT_LOCALE, prefix);
//                                                        ^^^^^^^                          ^^^^^^
//                                                   "AMS\tAAAA..."                        "AMS"
```

**What Happened:**
1. Eluna prepended `prefix + "\t"` to the message: `"AMS\t" + "000000000000{...}"`
2. Then passed BOTH `fullmsg` AND `prefix` to `chat.Initialize()`
3. WoW client's `LANG_ADDON` handler **automatically strips prefix+tab** from the ChatText field
4. Result: Client strips "AMS\t" (4 chars) from "AMS\t000000000000{...}", leaving only "000000000000{...}"
5. BUT the message was actually "AMS\tAAAAAAAAAAAA{...}" due to our test, so stripping behavior was inconsistent

**Comparison with Native Code:**

TrinityCore's native `Player::WhisperAddon()` does it correctly:
```cpp
// Player.cpp:22081
packet.Initialize(CHAT_MSG_WHISPER, LANG_ADDON, this, this, text, 0, "", DEFAULT_LOCALE, prefix);
//                                                              ^^^^                        ^^^^^^
//                                                          text only                   prefix separate
```

### Solution: Remove Duplicate Prefix ✅ FIXED

**File:** `src/server/game/LuaEngine/methods/TrinityCore/PlayerMethods.h:3631`

**Fixed Code:**
```cpp
int SendAddonMessage(Eluna* E, Player* player)
{
    std::string prefix = E->CHECKVAL<std::string>(2);
    std::string message = E->CHECKVAL<std::string>(3);
    ChatMsg channel = ChatMsg(E->CHECKVAL<uint8>(4));
    Player* receiver = E->CHECKOBJ<Player>(5);
#if ELUNA_EXPANSION < EXP_RETAIL
    // For classic/TBC/WotLK: Need to prepend prefix+tab to message
    std::string fullmsg = prefix + "\t" + message;
    WorldPacket data;
    ChatHandler::BuildChatPacket(data, channel, LANG_ADDON, player, receiver, fullmsg);
    receiver->GetSession()->SendPacket(&data);
#else
    // For retail: Pass message and prefix separately
    // Client automatically strips prefix+tab from LANG_ADDON messages
    // So we must NOT prepend it or it will be double-stripped
    WorldPackets::Chat::Chat chat;
    chat.Initialize(channel, LANG_ADDON, player, receiver, message, 0, "", DEFAULT_LOCALE, prefix);
    receiver->GetSession()->SendPacket(chat.Write());
#endif
    return 0;
}
```

**Key Changes:**
- Retail (11.x): Pass `message` directly without prepending prefix
- Classic/TBC/WotLK: Keep old behavior (may need testing on those versions)
- Matches native TrinityCore `WhisperAddon` implementation

### Test Results ✅ SUCCESSFUL

**After Fix:**
- ✅ Client receives full 90-byte payload (header stripped correctly by client)
- ✅ Deserialization succeeds
- ✅ ECHO_RESPONSE handler fires
- ✅ Full round-trip messaging works
- ✅ Message displayed: "Server echoed: Hello from client!"
- ✅ Timestamp received correctly

---

## Issue #2: `.reload eluna` Bytecode Caching

**Priority:** MEDIUM  
**Status:** Investigating  
**Affects:** Development workflow

### Problem Description
The `.reload eluna` command successfully executes but serves cached bytecode instead of reloading the updated Lua files. Full worldserver restart is required to load changes.

### Evidence

**Behavior:**
1. Edit `AMS_Server.lua` to add debug output
2. Run `.reload eluna` in-game
3. Server logs show "Loaded and precompiled 25 scripts"
4. Old code still executes (no new debug output)
5. Full server restart DOES load new code

**Eluna Log Output:**
```
[Eluna]: LoadScripts() called, current cache state: 1
[Eluna]: Searching for scripts in `/opt/trinitycore/lua_scripts`
[Eluna]: Loaded and precompiled 25 scripts in 122 ms
[Eluna]: Script cache state set to READY
```

### Possible Causes

1. **Bytecode Cache Not Cleared**
   - Eluna compiles to bytecode and caches it
   - `ReloadElunaForMap()` might not flush the cache

2. **Function Closure Persistence**
   - Lua closures might hold references to old code
   - Event handlers registered at startup persist

3. **Require Cache**
   - `package.loaded` table not cleared
   - Modules cached by `require()` not reloaded

### Investigation Steps

- [ ] Check `ElunaLoader.cpp::ReloadElunaForMap()` implementation
- [ ] See if there's a cache clear function
- [ ] Compare with AzerothCore mod-eluna reload implementation
- [ ] Check if `package.loaded` needs manual clearing
- [ ] Look for precompiled bytecode files (.luac)
- [ ] Check Eluna config for cache-related settings

### Workarounds

**Current:** Full server restart (works but slow)

**Potential Fix:** Add cache clearing to reload command:
```cpp
// In HandleReloadElunaCommand before calling ReloadElunaForMap
sElunaLoader->ClearCache(); // If such a function exists
```

---

## Testing Notes

### What Works ✅
- Client → Server messaging (prefix "AMS")
- Server → Client messaging (fixed!)
- Hex encoding/decoding
- Server-side Lua receives messages correctly
- Full round-trip request/response pattern
- Message serialization with Smallfolk
- Handler registration
- Debug logging
- ECHO test handler
- Smallfolk deserialization

### What Still Needs Testing
- Message splitting for long messages (>2500 bytes)
- Multiple handlers (NPC_SEARCH, GET_PLAYER_INFO, etc.)
- Request/Response pattern with callbacks
- Multi-block messages
- Error handling and edge cases

### What Doesn't Work ❌
- `.reload eluna` hot-reloading (caching issue - see Issue #2)

### Why Simple Tests Worked

**Simple tests bypassed the issue because:**
1. They sent plain text without complex headers
2. The prefix+tab stripping still worked, just stripped "CAIO\t" from short messages
3. No serialization meant no complex parsing on client side
4. Messages were short enough that losing a few characters didn't break functionality

**AMS exposed the bug because:**
1. Uses 12-character hex headers that must be preserved exactly
2. Serialized data is sensitive to byte corruption
3. Losing 8 characters breaks the header format completely
4. Smallfolk deserialization requires exact format

---

## Next Steps

1. ✅ **COMPLETED:** Debug the packet truncation issue
   - ✅ Found root cause in Eluna SendAddonMessage
   - ✅ Fixed by removing duplicate prefix prepending
   - ✅ Verified with full round-trip test

2. **Immediate:** Test remaining AMS features
   - Test long message splitting (>2500 bytes)
   - Test all example handlers
   - Test error handling
   - Clean up debug output

3. **Short-term:** Fix `.reload eluna` caching
   - Research Eluna cache mechanism
   - Implement cache clearing
   - Test hot-reload workflow

4. **Long-term:** Complete AMS implementation
   - Build out more handler examples
   - Integrate with AraxiaTrinityAdmin
   - Production-ready error handling
   - Performance testing

---

## References

- **Eluna Source:** `src/server/game/LuaEngine/`
- **Player Methods:** `methods/TrinityCore/PlayerMethods.h`
- **Reload Command:** `src/server/scripts/Commands/cs_reload.cpp`
- **AMS Implementation:** `lua_scripts/AMS_Server.lua`, `AddOns/AMS_Client/AMS_Client.lua`
- **Lessons Learned:** `araxia_docs/aio_integration/LESSONS_LEARNED_AMS.md`
