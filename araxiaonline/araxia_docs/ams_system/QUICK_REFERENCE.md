# AIO Integration - Quick Reference

One-page reference for common tasks and information.

## The Problem (TL;DR)

**What:** Client↔Server addon messages not working on TrinityCore 11.2.5
**Why:** AIO designed for 3.3.5, massive protocol changes since then
**Goal:** Get bidirectional Lua communication working reliably

## Current Status

✅ Server compiles with Eluna
✅ AIO_Server loads successfully  
✅ AIO_Client loads without errors
❌ Client→Server messages NOT working
❌ Server→Client messages NOT working

## Quick Diagnosis Commands

### Test Client→Server

```lua
-- In-game, run this:
/run C_ChatInfo.SendAddonMessage("TEST", "Hello", "WHISPER", UnitName("player"))

-- Check server logs for the message
-- If you see it: ChatHandler works, need to hook Eluna
-- If you don't: ChatHandler problem or channel issue
```

### Test Server→Client

```lua
-- In server Lua (e.g., test_aio.lua):
local function OnLogin(event, player)
    player:SendAddonMessage("TEST", "Hello from server", 7, player)
end
RegisterPlayerEvent(3, OnLogin)

-- In client addon or /run command:
local f = CreateFrame("Frame")
f:RegisterEvent("CHAT_MSG_ADDON_LOGGED")
f:SetScript("OnEvent", function(self, event, prefix, msg, channel, sender)
    print(string.format("[Test] %s: %s = %s", event, prefix, msg))
end)

-- Login and check client chat for message
-- If you see it: Packet sending works
-- If you don't: Packet format problem
```

## Key Files

### Client
```
q:\Araxia Online\...\Interface\AddOns\AIO_Client\
  ├── AIO.lua              (main client logic)
  └── AIO_Client.toc       (addon manifest)
```

### Server Lua
```
q:\github.com\...\TrinityServerBits\lua_scripts\
  ├── AIO_Server\AIO.lua   (main server logic)
  └── init.lua             (server startup)
```

### Server C++
```
q:\github.com\...\TrinityCore\src\server\game\
  ├── Handlers\ChatHandler.cpp              (receives addon messages)
  ├── LuaEngine\hooks\PlayerHooks.cpp       (routes to Eluna)
  ├── LuaEngine\hooks\ServerHooks.cpp       (OnAddonMessage implementation)
  └── LuaEngine\methods\...\PlayerMethods.h (SendAddonMessage method)
```

### Logs
```
q:\github.com\...\TrinityServerBits\logs\
  ├── Server.log           (general server log)
  └── Eluna.log            (Eluna-specific log)
```

## Critical Code Locations

### Client Sending (AIO_Client/AIO.lua ~line 515)
```lua
SendAddonMessageCompat(AIO_ClientPrefix, msg, "WHISPER", UnitName("player"))
```

### Client Receiving (AIO_Client/AIO.lua ~line 1065)
```lua
MsgReceiver:RegisterEvent("CHAT_MSG_ADDON")
MsgReceiver:RegisterEvent("CHAT_MSG_ADDON_LOGGED")
```

### Server Receiving (lua_scripts)
```lua
RegisterServerEvent(30, OnAddonMessageHandler) -- ADDON_EVENT_ON_MESSAGE
```

### Server Sending (lua_scripts)
```lua
player:SendAddonMessage(prefix, message, channel, player)
```

### C++ Addon Message Handler (ChatHandler.cpp ~line 506)
```cpp
void WorldSession::HandleChatAddonMessage(ChatMsg type, std::string prefix, 
                                          std::string text, bool isLogged, ...)
```

### C++ LANG_ADDON Detection (PlayerHooks.cpp ~line 529)
```cpp
if (lang == LANG_ADDON)
    return OnAddonMessage(pPlayer, type, msg, NULL, NULL, NULL, NULL);
```

### C++ Send Method (PlayerMethods.h ~line 3631)
```cpp
int SendAddonMessage(Eluna* E, Player* player) {
    // Creates packet and sends to client
}
```

## Message Flow (Expected)

### Client → Server
```
Client Lua (SendAddonMessage)
  ↓
WoW Client API (C_ChatInfo.SendAddonMessage)
  ↓
Network Packet (CMSG_CHAT_ADDON_MESSAGE)
  ↓
Server ChatHandler (HandleChatAddonMessage)
  ↓
??? (Mystery step - needs investigation)
  ↓
Eluna OnChat (with LANG_ADDON)
  ↓
Eluna OnAddonMessage
  ↓
Server Lua (ADDON_EVENT_ON_MESSAGE handler)
```

**Breaks somewhere between ChatHandler and OnChat**

### Server → Client
```
Server Lua (player:SendAddonMessage)
  ↓
Eluna PlayerMethods (SendAddonMessage)
  ↓
WorldPackets::Chat::Chat (packet construction)
  ↓
Network Packet (SMSG_CHAT)
  ↓
??? (Mystery step - needs investigation)
  ↓
Client Event (CHAT_MSG_ADDON_LOGGED)
  ↓
Client Lua (event handler)
```

**Breaks somewhere between packet send and client event**

## Key Questions to Answer

1. ❓ Do addon messages reach ChatHandler.cpp?
2. ❓ Is LANG_ADDON set correctly for 11.2.5?
3. ❓ Does OnChat get called for addon messages?
4. ❓ Does whisper-to-self work in 11.2.5?
5. ❓ What is channel type 7 in 11.2.5?
6. ❓ Is packet format correct for 11.2.5 client?
7. ❓ Are AIO prefixes registered properly?

## Next Steps (Recommended)

1. **Add C++ logging** to ChatHandler to see if messages arrive
2. **Add C++ logging** to PlayerHooks to see if LANG_ADDON detected
3. **Add Lua logging** to ADDON_EVENT_ON_MESSAGE handler
4. **Test multiple channels** (WHISPER, GUILD, PARTY) to find what works
5. **Compare working addon messages** (e.g., DBM) to see correct format

## Build & Test Cycle

```bash
# 1. Make C++ changes in TrinityCore/src/
# 2. Build
docker exec -it trinitycore-dev bash
cd /workspace/build
cmake --build . -j$(nproc)

# 3. Restart server
pkill worldserver
/opt/trinitycore/bin/worldserver

# 4. Make Lua changes in TrinityServerBits/lua_scripts/
# 5. Reload Lua (in-game)
.reload eluna

# 6. Make client addon changes in Interface/AddOns/AIO_Client/
# 7. Reload UI (in-game)
/reload

# 8. Test
# (run test commands from above)

# 9. Check logs
tail -f /opt/trinitycore/logs/Server.log
tail -f /opt/trinitycore/logs/Eluna.log
```

## Common Issues

### "SendAddonMessage is not a function"
- Client: Use `C_ChatInfo.SendAddonMessage`
- Check API version compatibility

### Messages not appearing in logs
- Check server config: `AddonChannel.Enable = 1`
- Verify prefix length ≤ 16 characters
- Check spam throttling

### Client errors on load
- Check .toc file syntax
- Verify all dependencies loaded
- Look for Lua errors in-game

### Server crashes
- Check C++ compile errors
- Verify null pointer checks
- Look for Eluna state issues

## Tips

- **Test with guild channel first** - easier to see messages
- **Use unique test prefixes** - helps distinguish messages
- **Log at every layer** - find exactly where it breaks
- **Test one direction at a time** - client→server first
- **Verify assumptions** - 11.2.5 ≠ 3.3.5

## Documentation

- **Overview:** `00_OVERVIEW.md`
- **Discovery Plan:** `01_DISCOVERY_PLAN.md`
- **Discovery Log:** `02_DISCOVERY_LOG.md`
- **Architecture:** `03_TECHNICAL_ARCHITECTURE.md`
- **Solutions:** `04_PROPOSED_SOLUTIONS.md`
- **Implementation:** `05_IMPLEMENTATION_LOG.md`

## External Resources

- **Eluna API:** https://elunaluaengine.github.io/
- **WoW API:** https://wowpedia.fandom.com/wiki/World_of_Warcraft_API
- **Original AIO:** https://github.com/Rochet2/AIO
- **TrinityCore:** https://github.com/TrinityCore/TrinityCore

---

**Remember:** Don't get lost in the weeds. Get ONE message through first, then build on that.
