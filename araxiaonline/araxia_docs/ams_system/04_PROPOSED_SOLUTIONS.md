# AIO Integration - Proposed Solutions

This document outlines potential solutions for getting AIO working on TrinityCore 11.2.5.

## Solution Philosophy

Given the massive gap between 3.3.5 and 11.2.5, we should:

1. **Start Simple** - Get a minimal proof-of-concept working first
2. **Verify Each Layer** - Don't skip ahead to AIO before basic addon messages work
3. **Be Willing to Rewrite** - Don't force 3.3.5 code to work if it doesn't fit 11.2.5
4. **Focus on Reliability** - One working channel is better than five broken ones
5. **Document Everything** - Future us will thank present us

## Phase 1: Establish Basic Communication

### Goal
Get ANY message flowing client↔server, even if it's hacky.

### Approach 1A: Verify Whisper-to-Self Works

**Steps:**
1. Add C++ logging to `ChatHandler::HandleChatAddonMessage()`
2. Test raw client send: `/run C_ChatInfo.SendAddonMessage("TEST", "Hello", "WHISPER", UnitName("player"))`
3. Check if message appears in server logs

**If Success:** Proceed to hook into Eluna
**If Failure:** Try Approach 1B

### Approach 1B: Use GUILD Channel Instead

**Rationale:** 
- Guild channel may be more reliable
- Already has special Warden handling (proves it works)
- Easier to debug (broadcasts to all guild members)

**Steps:**
1. Require player to be in a guild for AIO
2. Change AIO to use GUILD channel
3. Test: `/run C_ChatInfo.SendAddonMessage("TEST", "Hello", "GUILD")`

**Considerations:**
- ✅ More likely to work
- ✅ Easier to debug (see in guild chat)
- ⚠️ Requires guild membership (easily managed)
- ⚠️ Other guild members could see messages (can use test guild)

### Approach 1C: Use PARTY Channel

**Rationale:**
- Party-to-self might work where whisper doesn't
- Can create a party of one

**Steps:**
1. Have player create solo party
2. Use PARTY channel
3. Test: `/run C_ChatInfo.SendAddonMessage("TEST", "Hello", "PARTY")`

**Considerations:**
- ✅ Don't need guild
- ⚠️ Have to manage party state (simple to handle)
- ⚠️ Might not work solo (need to test)

## Phase 2: Hook into Eluna

### Goal
Get addon messages to fire Eluna's ADDON_EVENT_ON_MESSAGE

### Approach 2A: Fix LANG_ADDON Detection

**Hypothesis:** Eluna's `OnChat()` isn't being called with LANG_ADDON

**Investigation Steps:**
1. Add logging to `PlayerHooks::OnChat()` to see all calls
2. Check what `lang` value addon messages have
3. Verify LANG_ADDON constant value in 11.2.5

**Possible Fixes:**

**Fix 2A.1: LANG_ADDON Value Changed**
```cpp
// In PlayerHooks.cpp, OnChat()
// OLD: if (lang == LANG_ADDON)
// NEW: if (lang == LANG_ADDON || lang == NEW_LANG_ADDON_VALUE)
```

**Fix 2A.2: Chat Routing Broken**
```cpp
// Add explicit hook in ChatHandler.cpp
void WorldSession::HandleChatAddonMessage(...) {
    // ... existing validation ...
    
    // NEW: Explicitly call Eluna hook
    if (sEluna->OnAddonMessage(sender, type, fullMessage, receiver, guild, group, channel)) {
        // Eluna handled it
    }
    
    // ... existing routing ...
}
```

### Approach 2B: Create New Eluna Hook Path

**Rationale:** Bypass chat system entirely, hook directly in addon message handler

**Implementation:**
```cpp
// In ChatHandler.cpp
void WorldSession::HandleChatAddonMessage(ChatMsg type, std::string prefix, std::string text, ...) {
    Player* sender = GetPlayer();
    
    // ... validation ...
    
    // NEW: Direct Eluna hook for addon messages
    std::string fullMessage = prefix + "\t" + text;
    
    if (type == CHAT_MSG_WHISPER && target == sender->GetName()) {
        // Self-whisper addon message
        if (!sEluna->OnAddonMessage(sender, type, fullMessage, sender, nullptr, nullptr, nullptr)) {
            return; // Eluna blocked it
        }
    }
    
    // ... rest of handling ...
}
```

**Benefits:**
- ✅ Direct, explicit control
- ✅ Easy to debug
- ✅ Full control over implementation
- ✅ Can optimize specifically for our use case

## Phase 3: Server→Client Messages

### Goal
Get server Lua to send messages that client receives

### Approach 3A: Fix SendAddonMessage Packet Format

**Hypothesis:** Packet structure doesn't match 11.2.5 client expectations

**Investigation Steps:**
1. Add logging to `PlayerMethods::SendAddonMessage()`
2. Log exact packet contents
3. Capture network traffic and compare to working addon messages
4. Check TrinityCore source for correct packet format

**Possible Fixes:**

**Fix 3A.1: Initialize() Parameters Wrong**
```cpp
// Current code
WorldPackets::Chat::Chat chat;
chat.Initialize(channel, LANG_ADDON, player, receiver, fullmsg, 0, "", DEFAULT_LOCALE, prefix);

// Maybe needs different parameters?
// Check TrinityCore ChatHandler.cpp for examples of correct usage
```

**Fix 3A.2: Need Different Packet Type**
```cpp
// Maybe addon messages require a specific packet type, not generic Chat packet?
// Research: WorldPackets::Chat::ChatAddon or similar
```

### Approach 3B: Use Known-Working Code Path

**Rationale:** Find how server sends addon messages normally, copy that

**Steps:**
1. Search for existing addon message sends in TrinityCore
2. Find Warden or other system that sends addon messages
3. Copy that exact pattern into Eluna SendAddonMessage
4. Test

**Example from ChatHandler:**
```cpp
// In ChatHandler.cpp, guild broadcast code:
WorldPackets::Chat::Chat packet;
packet.Initialize(type, isLogged ? LANG_ADDON_LOGGED : LANG_ADDON, sender, nullptr, text, 0, "", DEFAULT_LOCALE, prefix);
guild->BroadcastAddonMessagePacket(packet.Write(), prefix, true, subGroup, sender->GetGUID());
```

**Adaptation:**
```cpp
// In PlayerMethods.h, SendAddonMessage
WorldPackets::Chat::Chat packet;
packet.Initialize(channel, LANG_ADDON, player, receiver, message, 0, "", DEFAULT_LOCALE, prefix);
receiver->GetSession()->SendPacket(packet.Write());
```

## Phase 4: AIO Protocol Adaptation

### Goal
Make AIO's higher-level protocol work reliably on 11.2.5

### Approach 4A: Minimal AIO Changes

**Strategy:** Keep as much of AIO as possible, only fix broken parts

**Changes:**
1. Lock to single channel (no multi-channel support)
2. Simplify prefix handling
3. Update message format if needed
4. Remove features we don't use (dynamic addon loading)

**Implementation:**
```lua
-- In AIO.lua (both client and server)

-- Simplified SendAddonMessage
local function AIO_SendAddonMessage(msg, player)
    if AIO_SERVER then
        -- Server -> Client (use WHISPER or whatever works)
        player:SendAddonMessage(AIO_ServerPrefix, msg, WORKING_CHANNEL, player)
    else
        -- Client -> Server (use WHISPER or whatever works)
        C_ChatInfo.SendAddonMessage(AIO_ClientPrefix, msg, "WHISPER", UnitName("player"))
    end
end

-- Simplified message receiving (remove multi-channel handling)
-- Focus on making ONE channel rock-solid
```

### Approach 4B: Rewrite AIO for 11.2.5

**Strategy:** Keep the API surface, rewrite internals for 11.2.5

**Keep:**
- `AIO.AddHandlers(name, handlers)` API
- `AIO.Handle(player, name, handler, ...)` API
- `AIO.Msg():Add():Send()` API
- Message compression and encoding
- Handler routing

**Rewrite:**
- Communication layer (use what actually works)
- Message format (adapt to 11.2.5 quirks)
- Channel handling (focus on one reliable channel)
- Packet handling (match 11.2.5 expectations)

**Example:**
```lua
-- New internal API
local AIO_Protocol = {
    version = "2.0-TC11",
    channel = "WHISPER", -- or whatever works
    
    Send = function(prefix, message, target)
        -- 11.2.5-specific implementation
    end,
    
    Receive = function(prefix, message, sender)
        -- 11.2.5-specific implementation
    end
}

-- Public API unchanged
AIO.Handle = function(...)
    -- Uses AIO_Protocol internally
end
```

## Phase 5: Production Hardening

### Goal
Make the system robust enough for real use

### Requirements

1. **Error Handling**
   - Graceful failure when messages lost
   - Timeout for long messages
   - Recovery from partial messages

2. **Logging**
   - Structured, filterable logs
   - Separate debug/info/warn/error levels
   - Performance metrics

3. **Testing**
   - Unit tests for message encoding
   - Integration tests for client↔server
   - Load tests for multiple clients
   - Chaos tests for packet loss

4. **Documentation**
   - API documentation for addon developers
   - Internal documentation for maintenance
   - Troubleshooting guide
   - Examples and tutorials

## Recommended Solution Path

Based on the analysis, here's the recommended sequence:

### Step 1: Establish Basic Link (1-2 hours)
1. Add comprehensive C++ logging to ChatHandler
2. Test client→server addon message with multiple channels
3. Identify which channel actually works
4. Get server to log received messages

**Success Criteria:** See addon message in server logs

### Step 2: Hook into Eluna (2-4 hours)
1. Trace LANG_ADDON detection in existing code
2. Add Eluna hook call in ChatHandler if missing
3. Verify ADDON_EVENT_ON_MESSAGE fires
4. Test server Lua receives messages

**Success Criteria:** Server Lua script logs received message

### Step 3: Server→Client Path (2-4 hours)
1. Find working example of server sending addon message
2. Copy pattern into Eluna SendAddonMessage
3. Test client receives message
4. Verify client event fires

**Success Criteria:** Client addon logs received message

### Step 4: AIO Integration (4-8 hours)
1. Update AIO to use working channel
2. Simplify to single-channel
3. Test handler system
4. Build example interaction

**Success Criteria:** AraxiaTrinityAdmin can call server function

### Step 5: Polish (4-8 hours)
1. Add error handling
2. Clean up debug code
3. Write documentation
4. Create examples

**Success Criteria:** Another developer can use the system

**Total Estimated Time:** 13-26 hours

## Contingency Plans

### If Whisper-to-Self Doesn't Work
→ Use GUILD channel
→ Require guild membership for AIO features
→ Consider creating system guild automatically

### If No Channel Works Reliably
→ Investigate custom packet types
→ Consider using game mail system (slow but reliable)
→ Look into using different game systems (e.g., trade window)

### If Eluna Hooks Don't Fire
→ Add custom C++ hooks in TrinityCore
→ Fork and maintain custom Eluna if needed
→ Document changes for future updates

### If Packet Format Incompatible
→ Reverse engineer working addon messages
→ Build packet manually if needed
→ Consider alternative transport mechanisms

### If AIO Protocol Incompatible
→ Rewrite AIO for 11.2.5
→ Keep API surface, new internals
→ Maintain compatibility layer if needed

## Decision Criteria

When choosing between approaches:

1. **Reliability > Features**
   - One working channel beats five broken ones
   
2. **Simplicity > Cleverness**
   - Straightforward solution beats elegant but complex one
   
3. **Maintainability > Performance**
   - Easy to understand beats micro-optimized
   
4. **Tested > Theoretical**
   - Proven to work beats should work in theory
   
5. **Documented > Undocumented**
   - Well-explained beats figure it out yourself

## Next Actions

1. Review this solutions document
2. Choose starting approach (recommend: Step 1 → 2 → 3 → 4 → 5)
3. Begin with logging and discovery
4. Document findings in `05_IMPLEMENTATION_LOG.md` as we go
5. Update this document with actual solutions as we find them

## Success Metrics

We'll know we're on the right track when:

- ✅ Can send raw addon message from client, see on server (< 1 hour)
- ✅ Server Lua receives message via Eluna hook (< 4 hours)
- ✅ Server can send message, client receives it (< 8 hours)
- ✅ Full AIO handler round-trip works (< 16 hours)
- ✅ AraxiaTrinityAdmin working with server (< 24 hours)

If any step takes significantly longer, reassess approach.
