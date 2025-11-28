# AIO Integration - Discovery & Diagnosis Plan

## Objective
Systematically discover why client↔server addon messages aren't working and identify exactly what needs to be fixed.

## Phase 1: Message Flow Analysis 🔍

### 1.1 Client→Server Message Flow

**Goal:** Trace a message from client button click to server Lua script

**Steps:**
1. Create minimal test addon that sends a message on command
2. Add client-side logging in AIO.lua before/after SendAddonMessage
3. Capture network traffic (optional but helpful)
4. Add C++ logging in ChatHandler.cpp when addon message arrives
5. Add C++ logging in Eluna hooks when LANG_ADDON detected
6. Add Lua logging in ADDON_EVENT_ON_MESSAGE handler

**Expected Result:** We'll know exactly where messages stop flowing

**Test Script (Client):**
```lua
/run C_ChatInfo.SendAddonMessage("AIOTEST", "Hello from client", "WHISPER", UnitName("player"))
```

**Test Script (Server):**
```lua
RegisterServerEvent(30, function(event, sender, msgType, prefix, msg, target)
    print("[AIO Test] Message received:", prefix, msg)
end)
```

### 1.2 Server→Client Message Flow

**Goal:** Trace a message from server Lua script to client addon

**Steps:**
1. Create server Lua script that sends message on player login
2. Add Lua logging before calling Player:SendAddonMessage
3. Add C++ logging in PlayerMethods.h SendAddonMessage method
4. Add C++ logging in packet send
5. Add client logging in CHAT_MSG_ADDON_LOGGED event handler
6. Add client logging in AIO message processing

**Expected Result:** We'll know exactly where messages stop flowing

**Test Script (Server):**
```lua
local function OnLogin(event, player)
    player:SendAddonMessage("AIOTEST", "Hello from server", 7, player)
end
RegisterPlayerEvent(3, OnLogin) -- PLAYER_EVENT_ON_LOGIN
```

**Test Script (Client):**
```lua
local f = CreateFrame("Frame")
f:RegisterEvent("CHAT_MSG_ADDON_LOGGED")
f:SetScript("OnEvent", function(self, event, prefix, message, channel, sender)
    print("[AIO Test]", event, prefix, message, channel, sender)
end)
```

## Phase 2: Root Cause Identification 🔬

### 2.1 Server Reception Issues

**Hypothesis A: Addon messages not reaching ChatHandler**
- Check: WorldPackets parsing
- Check: Opcode handling
- Check: Session validity

**Hypothesis B: LANG_ADDON not being set correctly**
- Check: Prefix detection logic
- Check: Language field in packet
- Check: 11.2.5 protocol changes

**Hypothesis C: OnAddonMessage hook not firing**
- Check: Hook registration
- Check: Event binding
- Check: Parameter passing

**Hypothesis D: Eluna state not receiving messages**
- Check: Lua state isolation
- Check: Event callback registration
- Check: Error handling

### 2.2 Server Transmission Issues

**Hypothesis A: SendAddonMessage building wrong packet**
- Check: Packet structure for 11.2.5
- Check: LANG_ADDON field
- Check: Prefix encoding
- Check: Message length limits

**Hypothesis B: Packet not being sent to client**
- Check: Session send queue
- Check: Network layer
- Check: Packet serialization

**Hypothesis C: Client not receiving packet**
- Check: Client opcode handlers
- Check: Event registration
- Check: Addon channel filtering

**Hypothesis D: Client event not firing**
- Check: Event vs. Event_Logged
- Check: Sender name matching
- Check: Prefix filtering

## Phase 3: Solution Design 🛠️

Based on findings from Phases 1-2, we'll design targeted fixes.

### Potential Solutions (To Be Confirmed)

#### Solution A: Fix Server-Side Addon Message Reception
**If:** Messages aren't reaching Eluna at all
**Then:** 
1. Modify ChatHandler.cpp to properly route addon messages to Eluna
2. Ensure LANG_ADDON is properly detected for 11.2.5 protocol
3. Add OnAddonMessage hook invocation

#### Solution B: Fix Eluna SendAddonMessage Implementation
**If:** Messages sent but wrong packet format
**Then:**
1. Update PlayerMethods.h SendAddonMessage for 11.2.5 packet format
2. Verify WorldPackets::Chat::Chat structure usage
3. Test packet delivery to client

#### Solution C: Create Custom AIO Communication Layer
**If:** Addon message system is too broken for 11.2.5
**Then:**
1. Create custom client↔server communication mechanism
2. Use different WoW protocol feature (e.g., custom packets)
3. Bypass addon message system entirely

#### Solution D: Fix AIO.lua Protocol Mismatch
**If:** Addon messages work but AIO protocol is incompatible
**Then:**
1. Update AIO message format for 11.2.5
2. Simplify protocol (remove multi-channel support)
3. Focus on single reliable channel (WHISPER to self)

## Phase 4: Implementation & Testing 🚀

Once we know the root cause and solution, implement in order:

1. **Minimal Fix** - Get ANY message through (even if hacky)
2. **Verify Fix** - Test both directions work reliably
3. **Clean Up** - Remove debug code, add proper error handling
4. **AIO Integration** - Restore AIO features on top of working base
5. **Production Testing** - Test with real AraxiaTrinityAdmin addon

## Logging Strategy

### Client-Side Logging Points
```lua
-- In AIO.lua, AIO_SendAddonMessage function
print("[AIO Client] Sending:", prefix, #msg, channel, target)

-- In CHAT_MSG_ADDON_LOGGED handler
print("[AIO Client] Received:", prefix, #msg, channel, sender)

-- In AIO message handler
print("[AIO Client] Processing:", handlerName, params)
```

### Server-Side C++ Logging Points
```cpp
// In ChatHandler.cpp, HandleChatAddonMessage
TC_LOG_DEBUG("aio", "Addon message: prefix={}, length={}, type={}", prefix, text.length(), type);

// In PlayerHooks.cpp, OnChat (before OnAddonMessage)
TC_LOG_DEBUG("aio", "LANG_ADDON detected, calling OnAddonMessage");

// In ServerHooks.cpp, OnAddonMessage
TC_LOG_DEBUG("aio", "OnAddonMessage: sender={}, prefix={}, msg={}", sender->GetName(), prefix, msg);
```

### Server-Side Lua Logging Points
```lua
-- In AIO.lua, OnAddonMessage handler
print("[AIO Server] Message received:", event, sender:GetName(), prefix, #msg)

-- In AIO.lua, AIO_SendAddonMessage function
print("[AIO Server] Sending:", prefix, #msg, "to", player:GetName())
```

## Success Metrics

After each phase, we should be able to answer:

- **Phase 1:** Where exactly do messages break?
- **Phase 2:** What is the root cause?
- **Phase 3:** What is the correct fix?
- **Phase 4:** Does it work reliably?

## Time Estimates

- **Phase 1:** 1-2 hours (logging and testing)
- **Phase 2:** 1-2 hours (analysis and hypothesis testing)
- **Phase 3:** 1-4 hours (implementation depends on scope)
- **Phase 4:** 1-2 hours (testing and cleanup)

**Total:** 4-10 hours depending on complexity of fixes needed

## Next Steps

1. Review this plan
2. Start Phase 1.1 - Client→Server message tracing
3. Document findings in `02_DISCOVERY_LOG.md`
4. Proceed to Phase 1.2 once client→server flow is understood
