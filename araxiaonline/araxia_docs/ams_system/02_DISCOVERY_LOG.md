# AIO Integration - Discovery Log

This document tracks our findings as we investigate the AIO communication issue.

## Session: Nov 27, 2025 - Initial Analysis

### Environment Setup
- **Server:** TrinityCore 11.2.5 with Eluna enabled
- **Client:** WoW 11.2.5.83634
- **AIO_Server:** Loaded successfully by Eluna (confirmed in logs)
- **AIO_Client:** Loads without errors

### Initial Code Review Findings

#### 1. Client-Side Implementation (AIO_Client/AIO.lua)

**Message Sending:**
```lua
-- Line 509-517
local function AIO_SendAddonMessage(msg, player)
    if AIO_SERVER then
        -- server -> client
        player:SendAddonMessage(AIO_ServerPrefix, msg, 7, player)
    else
        -- client -> server
        SendAddonMessageCompat(AIO_ClientPrefix, msg, "WHISPER", UnitName("player"))
    end
end
```

**Observations:**
- Uses compatibility wrapper `SendAddonMessageCompat()` for API version handling
- Sends to "WHISPER" channel targeting self (`UnitName("player")`)
- Client prefix is `AIO_ClientPrefix` (need to verify value)
- Server prefix is `AIO_ServerPrefix` (need to verify value)

**Message Reception:**
```lua
-- Line 1054-1068
local function ONADDONMSG(self, event, prefix, msg, Type, sender)
    if prefix == AIO_ServerPrefix then
        if (event == "CHAT_MSG_ADDON" or event == "CHAT_MSG_ADDON_LOGGED") and sender == UnitName("player") then
            -- Normal AIO message handling from addon messages
            AIO_HandleIncomingMsg(msg, sender)
        end
    end
end
local MsgReceiver = CreateFrame("Frame")
MsgReceiver:RegisterEvent("CHAT_MSG_ADDON") -- Legacy event (pre-8.0)
MsgReceiver:RegisterEvent("CHAT_MSG_ADDON_LOGGED") -- Modern event (8.0+)
MsgReceiver:SetScript("OnEvent", ONADDONMSG)
```

**Observations:**
- Registers BOTH addon message events (good for compatibility)
- Checks sender == self (whisper-to-self pattern)
- Only accepts messages with `AIO_ServerPrefix`

#### 2. Server-Side Implementation (AIO_Server/AIO.lua)

**Message Sending:**
```lua
-- Line 485-493
local function AIO_SendAddonMessage(msg, player)
    if AIO_SERVER then
        -- server -> client
        player:SendAddonMessage(AIO_ServerPrefix, msg, 7, player)
    else
        -- client -> server
        SendAddonMessage(AIO_ClientPrefix, msg, "WHISPER", UnitName("player"))
    end
end
```

**Observations:**
- Server code matches client structure
- Uses Eluna's `Player:SendAddonMessage()` method
- Channel parameter is `7` (need to determine what this maps to)
- No compatibility wrapper on server (relies on Eluna implementation)

**Critical Question:** How does server code receive messages?
- AIO_Server/AIO.lua doesn't show explicit message reception registration
- Should be handled by Eluna's `ADDON_EVENT_ON_MESSAGE` hook (event 30)
- Need to verify if AIO is registering this hook

#### 3. Eluna Server-Side Hooks

**Addon Message Hook Definition:**
```cpp
// hooks/Hooks.h line 95
ADDON_EVENT_ON_MESSAGE = 30, // (event, sender, type, prefix, msg, target)
```

**Hook Implementation:**
```cpp
// hooks/ServerHooks.cpp line 29-61
bool Eluna::OnAddonMessage(Player* sender, uint32 type, std::string& msg, 
                           Player* receiver, Guild* guild, Group* group, Channel* channel)
{
    START_HOOK_WITH_RETVAL(ADDON_EVENT_ON_MESSAGE, true);
    HookPush(sender);
    HookPush(type);
    
    // Parse prefix from message (tab-delimited)
    auto delimeter_position = msg.find('\t');
    if (delimeter_position == std::string::npos) {
        HookPush(msg); // prefix
        HookPush(); // msg
    } else {
        std::string prefix = msg.substr(0, delimeter_position);
        std::string content = msg.substr(delimeter_position + 1, std::string::npos);
        HookPush(prefix);
        HookPush(content);
    }
    
    // Push target (receiver, guild, group, or channel)
    if (receiver) HookPush(receiver);
    else if (guild) HookPush(guild);
    else if (group) HookPush(group);
    else if (channel) HookPush(channel->GetChannelId());
    else HookPush();
    
    return CallAllFunctionsBool(binding, key, true);
}
```

**Observations:**
- Hook parses prefix/message from tab-delimited format
- Expected format: `prefix\tmessage`
- Can handle different target types (player, guild, group, channel)
- Returns true by default (allows message)

**How OnAddonMessage gets called:**
```cpp
// hooks/PlayerHooks.cpp line 527-555
bool Eluna::OnChat(Player* pPlayer, uint32 type, uint32 lang, std::string& msg)
{
    if (lang == LANG_ADDON)
        return OnAddonMessage(pPlayer, type, msg, NULL, NULL, NULL, NULL);
    
    // ... rest of chat handling
}
```

**Critical Finding:** 
- Addon messages are detected by `lang == LANG_ADDON`
- OnChat is called from game's chat system
- Need to verify LANG_ADDON is being set for 11.2.5 addon messages

#### 4. Server Chat Handler (C++ Core)

**Addon Message Handler:**
```cpp
// Handlers/ChatHandler.cpp line 506-608
void WorldSession::HandleChatAddonMessage(ChatMsg type, std::string prefix, 
                                          std::string text, bool isLogged, 
                                          std::string target, Optional<ObjectGuid> targetGuid)
{
    Player* sender = GetPlayer();
    
    // Validation
    if (prefix.empty() || prefix.length() > 16) return;
    
    // Warden special case (GUILD channel)
    if (type == CHAT_MSG_GUILD) {
        if (_warden && _warden->ProcessLuaCheckResponse(text)) return;
    }
    
    // Config check
    if (!sWorld->getBoolConfig(CONFIG_ADDON_CHANNEL)) return;
    
    // Spam check
    if (!CanSpeak()) return;
    sender->UpdateSpeakTime(Player::ChatFloodThrottle::ADDON);
    
    // Special command handler
    if (prefix == AddonChannelCommandHandler::PREFIX && 
        AddonChannelCommandHandler(this).ParseCommands(text.c_str()))
        return;
    
    // Length check
    if (text.length() > 255) return;
    
    // Route by channel type
    switch (type) {
        case CHAT_MSG_GUILD:
        case CHAT_MSG_OFFICER:
            // Guild broadcast
            break;
        case CHAT_MSG_WHISPER:
            // Find receiver and send
            sender->WhisperAddon(text, prefix, isLogged, receiver);
            break;
        case CHAT_MSG_PARTY:
        case CHAT_MSG_RAID:
        case CHAT_MSG_INSTANCE_CHAT:
            // Group broadcast
            break;
        case CHAT_MSG_CHANNEL:
            // Channel broadcast
            break;
    }
}
```

**Critical Issues Found:**

1. **Whisper-to-self not handled properly?**
   - Code looks for receiver by name/GUID
   - If targeting self, does it find the player?
   - Does `WhisperAddon()` work for self-whisper?

2. **No LANG_ADDON setting visible**
   - ChatHandler doesn't explicitly set `LANG_ADDON`
   - How does Eluna's `OnChat` detect addon messages?
   - Is there a mismatch between packet handling and Eluna hook?

3. **Message format mismatch?**
   - ChatHandler uses `prefix` and `text` as separate parameters
   - Eluna expects `prefix\ttext` format
   - Who is responsible for combining them?

#### 5. Eluna SendAddonMessage Method

**Implementation:**
```cpp
// methods/TrinityCore/PlayerMethods.h line 3631-3648
int SendAddonMessage(Eluna* E, Player* player)
{
    std::string prefix = E->CHECKVAL<std::string>(2);
    std::string message = E->CHECKVAL<std::string>(3);
    ChatMsg channel = ChatMsg(E->CHECKVAL<uint8>(4));
    Player* receiver = E->CHECKOBJ<Player>(5);
    std::string fullmsg = prefix + "\t" + message;
    
    WorldPackets::Chat::Chat chat;
    chat.Initialize(channel, LANG_ADDON, player, receiver, fullmsg, 0, "", DEFAULT_LOCALE, prefix);
    receiver->GetSession()->SendPacket(chat.Write());
    
    return 0;
}
```

**Observations:**
- Combines prefix and message with tab delimiter
- Sets language to `LANG_ADDON`
- Uses modern WorldPackets::Chat::Chat structure
- Sends directly to receiver's session

**Questions:**
- Does `chat.Initialize()` expect prefix as separate parameter?
- Is the message format correct for client to parse?
- Does client receive packets with LANG_ADDON?

### Key Questions to Answer

1. **Are addon messages even reaching the server?**
   - Need to add logging to ChatHandler.cpp
   - Verify HandleChatAddonMessage is being called

2. **Is LANG_ADDON being set correctly?**
   - Where does LANG_ADDON come from in the packet?
   - Is it set by the client or server?

3. **Does Eluna OnChat get called for addon messages?**
   - Need to add logging before `if (lang == LANG_ADDON)` check
   - Verify OnAddonMessage is invoked

4. **Does whisper-to-self work in 11.2.5?**
   - May need to test different channel types
   - GUILD might be more reliable for testing

5. **Are AIO prefixes registered correctly?**
   - Need to verify `AIO_ClientPrefix` and `AIO_ServerPrefix` values
   - Check if they match on both sides

6. **Is the message format compatible?**
   - Client sends: prefix + message
   - Server expects: prefix\tmessage
   - Who adds the tab delimiter?

### Next Steps

1. **Add Debug Logging Script** (Server-Side)
   - Hook ADDON_EVENT_ON_MESSAGE directly
   - Log all parameters
   - Verify if ANY addon messages arrive

2. **Create Simple Test Addon** (Client-Side)
   - Minimal addon to send test message
   - Use /run command for easy testing
   - Log send attempt

3. **Add C++ Debug Logging**
   - ChatHandler.cpp - message receipt
   - PlayerHooks.cpp - LANG_ADDON detection
   - PlayerMethods.h - SendAddonMessage calls

4. **Analyze Logs**
   - Determine where message flow breaks
   - Identify root cause
   - Design fix

## Test Cases to Run

### Test 1: Raw Client Message
```lua
/run C_ChatInfo.SendAddonMessage("TEST", "Hello", "WHISPER", UnitName("player"))
```

**Expected:** Should appear in server logs if ChatHandler works

### Test 2: Raw Client Message (Guild)
```lua
/run C_ChatInfo.SendAddonMessage("TEST", "Hello", "GUILD")
```

**Expected:** Should broadcast if in guild, easier to debug

### Test 3: Server-Initiated Message
```lua
-- In server Lua on player login
player:SendAddonMessage("TEST", "Welcome", 7, player)
```

**Expected:** Client should receive and log

### Test 4: Full AIO Test
```lua
-- Client: Call AIO.Handle()
-- Server: Verify handler called
-- Server: Response back
-- Client: Verify response received
```

**Expected:** Full round-trip works

## Blockers & Risks

### Blocker 1: If addon messages don't reach server at all
**Risk:** May need to modify core TrinityCore C++ to route messages to Eluna
**Impact:** Low - C++ changes are straightforward, we have full control
**Solution:** Add hooks in ChatHandler to explicitly route to Eluna

### Blocker 2: If LANG_ADDON isn't set for 11.2.5 protocol
**Risk:** Need to understand new protocol format
**Impact:** High - may require packet parsing changes
**Mitigation:** Research 11.2.5 packet structure, may need to change detection logic

### Blocker 3: If whisper-to-self is blocked by server
**Risk:** May need different communication channel
**Impact:** Medium - requires AIO rewrite to use different channel
**Mitigation:** Test GUILD channel as alternative

### Blocker 4: If message format is incompatible
**Risk:** Tab delimiter may not be preserved through packet encoding
**Impact:** Low - can adjust parsing
**Mitigation:** Change delimiter or format

## Status

**Current Phase:** Phase 1.1 - Client→Server Message Flow Tracing
**Next Action:** Create debug logging infrastructure
**Blocked:** No
**Estimated Time to Completion:** Unknown - pending discovery results
