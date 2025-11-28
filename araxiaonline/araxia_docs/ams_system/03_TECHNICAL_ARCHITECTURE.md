# AIO Technical Architecture

## System Overview

AIO (Addon I/O) provides bidirectional Lua communication between WoW client addons and server-side Lua scripts via the game's addon message system.

```
┌─────────────────────────────────────────────────────────────────────┐
│                          WoW Client (11.2.5)                        │
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                    AIO_Client Addon                          │   │
│  │  ┌────────────────────────────────────────────────────────┐  │   │
│  │  │ AIO.lua (Client-side)                                  │  │   │
│  │  │  - Message sending (SendAddonMessage)                  │  │   │
│  │  │  - Message receiving (CHAT_MSG_ADDON_LOGGED)           │  │   │
│  │  │  - Handler registration (AddHandlers)                  │  │   │
│  │  │  - Message queueing                                    │  │   │
│  │  └────────────────────────────────────────────────────────┘  │   │
│  └─────────────────────────────────────────────────────────────┘   │
│                              ↕                                      │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │              WoW Client API (FrameXML)                      │   │
│  │  - C_ChatInfo.SendAddonMessage()                            │   │
│  │  - CHAT_MSG_ADDON / CHAT_MSG_ADDON_LOGGED events            │   │
│  └─────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────┘
                              ↕
                    (Network: Addon Message Packets)
                              ↕
┌─────────────────────────────────────────────────────────────────────┐
│                    TrinityCore Server (11.2.5)                      │
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │              WorldSession / ChatHandler (C++)                │   │
│  │  ┌────────────────────────────────────────────────────────┐  │   │
│  │  │ ChatHandler.cpp                                        │  │   │
│  │  │  - HandleChatAddonMessage()                            │  │   │
│  │  │  - HandleChatAddonMessageTargeted()                    │  │   │
│  │  │  - Validates prefix, length, permissions               │  │   │
│  │  │  - Routes to Guild/Whisper/Party/Channel handlers      │  │   │
│  │  └────────────────────────────────────────────────────────┘  │   │
│  └─────────────────────────────────────────────────────────────┘   │
│                              ↕                                      │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                  Eluna Lua Engine (C++)                      │   │
│  │  ┌────────────────────────────────────────────────────────┐  │   │
│  │  │ PlayerHooks.cpp                                        │  │   │
│  │  │  - OnChat() detects LANG_ADDON                         │  │   │
│  │  │  - Calls OnAddonMessage()                              │  │   │
│  │  └────────────────────────────────────────────────────────┘  │   │
│  │  ┌────────────────────────────────────────────────────────┐  │   │
│  │  │ ServerHooks.cpp                                        │  │   │
│  │  │  - OnAddonMessage() parses prefix/message              │  │   │
│  │  │  - Fires ADDON_EVENT_ON_MESSAGE                        │  │   │
│  │  └────────────────────────────────────────────────────────┘  │   │
│  │  ┌────────────────────────────────────────────────────────┐  │   │
│  │  │ PlayerMethods.h                                        │  │   │
│  │  │  - SendAddonMessage() sends to client                  │  │   │
│  │  └────────────────────────────────────────────────────────┘  │   │
│  └─────────────────────────────────────────────────────────────┘   │
│                              ↕                                      │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │              Server-Side Lua Scripts (Eluna)                 │   │
│  │  ┌────────────────────────────────────────────────────────┐  │   │
│  │  │ AIO_Server/AIO.lua                                     │  │   │
│  │  │  - Registers ADDON_EVENT_ON_MESSAGE (Event 30)         │  │   │
│  │  │  - Message receiving and routing                       │  │   │
│  │  │  - Handler registration                                │  │   │
│  │  │  - Message queueing                                    │  │   │
│  │  │  - Addon code distribution                             │  │   │
│  │  └────────────────────────────────────────────────────────┘  │   │
│  └─────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────┘
```

## Message Flow Details

### Client → Server Flow

```
1. Client Lua calls AIO.Handle() or AIO.Msg():Send()
   ↓
2. AIO_SendAddonMessage() called
   ↓
3. SendAddonMessageCompat() wrapper called
   ↓
4. C_ChatInfo.SendAddonMessage(prefix, message, "WHISPER", UnitName("player"))
   ↓
5. WoW Client builds CMSG_CHAT_ADDON_MESSAGE packet
   ↓
6. Packet sent to server over network
   ↓
7. Server receives and routes to WorldSession::HandleChatAddonMessage()
   ↓
8. Validation: prefix length, spam check, config check
   ↓
9. Routed by channel type (WHISPER in AIO's case)
   ↓
10. Player::WhisperAddon() or equivalent called
   ↓
11. ??? (This is where we need to investigate)
   ↓
12. Eventually should reach Eluna::OnChat() with LANG_ADDON
   ↓
13. Eluna::OnAddonMessage() called
   ↓
14. ADDON_EVENT_ON_MESSAGE fired to Lua
   ↓
15. AIO_Server Lua handler processes message
```

**Current Issue:** Flow breaks somewhere between step 10 and step 12.

### Server → Client Flow

```
1. Server Lua calls player:SendAddonMessage(prefix, message, channel, player)
   ↓
2. Eluna PlayerMethods::SendAddonMessage() called
   ↓
3. Creates fullmsg = prefix + "\t" + message
   ↓
4. WorldPackets::Chat::Chat.Initialize() called with:
   - channel type
   - LANG_ADDON
   - sender (player)
   - receiver (player)
   - fullmsg
   - prefix (separate parameter)
   ↓
5. Packet written and sent to receiver's session
   ↓
6. Client receives SMSG_CHAT packet
   ↓
7. ??? (This is where we need to investigate)
   ↓
8. Should fire CHAT_MSG_ADDON_LOGGED event
   ↓
9. AIO_Client frame handler receives event
   ↓
10. Checks prefix matches AIO_ServerPrefix
   ↓
11. Checks sender matches UnitName("player")
   ↓
12. AIO_HandleIncomingMsg() processes message
```

**Current Issue:** Flow breaks somewhere between step 6 and step 8.

## Data Structures

### AIO Message Format

**Client→Server:**
```
SendAddonMessage Parameters:
- prefix: "AIO_C" (AIO_ClientPrefix)
- message: <compressed and encoded payload>
- channel: "WHISPER"
- target: UnitName("player") (self)
```

**Server→Client:**
```
SendAddonMessage Parameters:
- prefix: "AIO_S" (AIO_ServerPrefix)  
- message: <compressed and encoded payload>
- channel: 7 (ChatMsg enum value)
- target: Player object
```

### Message Payload Structure

AIO messages use a structured protocol:

```lua
-- Short message (≤ 255 chars)
payload = "s" .. compressedData

-- Long message (> 255 chars)
-- Sent as multiple packets with headers
header = partCount (2 bytes) .. msgID (2 bytes)
packet = header .. partNumber (2 bytes) .. dataPart
```

### Handler System

**Client-Side:**
```lua
AIO.AddHandlers("HandlerName", {
    FunctionName = function(player, arg1, arg2, ...) 
        -- handler code
    end
})
```

**Server-Side:**
```lua
AIO.AddHandlers("HandlerName", {
    FunctionName = function(player, arg1, arg2, ...) 
        -- handler code
    end
})
```

When message received:
```lua
-- Message format: "HandlerName\tFunctionName\targ1\targ2\t..."
-- Parsed and routed to: Handlers["HandlerName"]["FunctionName"](player, arg1, arg2, ...)
```

## Key Components

### 1. Message Encoding/Compression

**Libraries Used:**
- **Smallfolk** - Lua table serialization
- **LZW** - LZ compression (lualzw-zeros)
- **CRC32** - Message integrity checking (server-side)
- **Base64-like encoding** - For binary data transmission

**Flow:**
```
Lua table → Smallfolk serialize → LZW compress → Encode → Send
Receive → Decode → LZW decompress → Smallfolk deserialize → Lua table
```

### 2. Message Queueing

**Purpose:** Handle messages that can't fit in single 255-byte packet

**Implementation:**
- Splits messages into chunks
- Assigns message ID
- Sends with part numbers
- Receiver reassembles in order

**File:** `queue.lua` (both client and server)

### 3. Addon Code Distribution

**Server-Side Feature:**
- Server can send Lua code to client
- Code is cached on client by CRC
- Updates sent only when code changes
- Uses `AIO.AddAddon()` or `AIO.AddAddonCode()`

**Not Currently Used:** Our project focuses on communication, not dynamic addon loading

### 4. Handler Registration

**Pattern:**
```lua
local handlers = AIO.AddHandlers("MyAddon", {})

handlers.DoSomething = function(player, arg1, arg2)
    print("Received:", arg1, arg2)
    -- Send response
    AIO.Handle(player, "MyAddon", "OnResponse", "result")
end
```

**Invocation:**
```lua
-- From other side
AIO.Handle(player, "MyAddon", "DoSomething", "hello", "world")
```

## Critical Design Decisions (3.3.5)

These decisions were made for AzerothCore 3.3.5 and may not work for 11.2.5:

### 1. Whisper-to-Self Communication

**Rationale:** 
- Simple, direct player↔server link
- No need for guild/party membership
- Private communication

**Assumption:** Server allows whispering to yourself

**Risk for 11.2.5:** May be blocked or behave differently

### 2. LANG_ADDON Detection

**Assumption:** Addon messages use LANG_ADDON language code

**Risk for 11.2.5:** Protocol may have changed

### 3. Tab-Delimited Prefix/Message

**Assumption:** Prefix and message separated by `\t`

**Risk for 11.2.5:** Packet format may differ

### 4. 255-Byte Message Limit

**Assumption:** Addon messages capped at 255 bytes

**Risk for 11.2.5:** Limit may have changed

### 5. Channel Type Enum Values

**Assumption:** Channel type `7` is WHISPER

**Risk for 11.2.5:** Enum values may have changed

## Known Differences: 3.3.5 vs 11.2.5

### Client API Changes

| 3.3.5 | 11.2.5 | Status |
|-------|--------|--------|
| `SendAddonMessage(...)` | `C_ChatInfo.SendAddonMessage(...)` | ✅ Wrapped |
| `CHAT_MSG_ADDON` event | `CHAT_MSG_ADDON` + `CHAT_MSG_ADDON_LOGGED` | ✅ Both registered |
| Simple event parameters | Complex event parameters | ⚠️ Need to verify |

### Server Changes

| 3.3.5 | 11.2.5 | Status |
|-------|--------|--------|
| `WorldPacket` API | `WorldPackets::Chat::Chat` API | ✅ Used in code |
| Simple chat packet | Complex chat packet with multiple fields | ⚠️ Need to verify |
| `BuildChatPacket()` | `Chat::Initialize()` | ✅ Used in code |

### Protocol Changes

| Aspect | 3.3.5 | 11.2.5 | Status |
|--------|-------|--------|--------|
| Addon message opcode | CMSG_MESSAGECHAT | CMSG_CHAT_ADDON_MESSAGE | ⚠️ Different |
| Packet structure | Simple | Complex | ⚠️ Need to verify |
| Prefix handling | In message body | Separate field | ⚠️ Need to verify |
| IsLogged parameter | N/A | Added in 8.0 | ⚠️ Need to handle |

## Unknowns & Investigation Needed

### Question 1: How does ChatHandler route to Eluna?

**Current Understanding:** 
- ChatHandler processes addon messages
- Somewhere it should call into Eluna with LANG_ADDON
- Not clear where this connection happens

**Need to Find:**
- Where does `Player::WhisperAddon()` go?
- Does it eventually call `OnChat()` with LANG_ADDON?
- Is there a missing link?

### Question 2: What is channel type 7?

**Current Code:** `player:SendAddonMessage(prefix, msg, 7, player)`

**Need to Verify:**
- Is 7 == CHAT_MSG_WHISPER in 11.2.5?
- Should we use a different channel?
- What values are valid?

### Question 3: Does whisper-to-self work?

**Test Needed:**
```lua
-- Client
C_ChatInfo.SendAddonMessage("TEST", "msg", "WHISPER", UnitName("player"))

-- Does server receive it?
```

### Question 4: What is LANG_ADDON value?

**Need to Find:**
- What is the numeric value of LANG_ADDON?
- Is it set by client or server?
- How does server detect it?

### Question 5: Message format compatibility

**Need to Verify:**
- Client sends: prefix + message (separate parameters)
- Server expects: prefix\tmessage (tab-delimited)
- Who combines them?
- Is tab preserved in packet encoding?

## Testing Strategy

### Unit Tests (Minimal)

1. **Message Encoding/Decoding**
   - Verify Smallfolk serialize/deserialize
   - Verify LZW compress/decompress
   - Verify round-trip preserves data

2. **Message Splitting/Reassembly**
   - Test queue system with long messages
   - Verify parts arrive in order
   - Verify reassembly matches original

### Integration Tests

1. **Client→Server Raw Message**
   - Send addon message from client
   - Verify server receives it
   - Log at each layer

2. **Server→Client Raw Message**
   - Send addon message from server
   - Verify client receives it
   - Log at each layer

3. **AIO Handler System**
   - Register handler on server
   - Call from client
   - Verify handler executes
   - Verify response returns

4. **Full Round-Trip**
   - Client calls server handler
   - Server processes and responds
   - Client receives response
   - Client processes response

### Load Tests (Future)

- Multiple clients sending simultaneously
- Large message handling
- Rapid message bursts
- Memory leak detection

## Performance Considerations

### Message Size Limits

- Single packet: 255 bytes
- With overhead (compression, encoding): ~200 bytes usable
- Large messages: Multiple packets (slower)

**Recommendation:** Keep messages small, send only necessary data

### Compression Tradeoffs

- Small messages: Compression overhead > benefit
- Large messages: Compression saves bandwidth
- AIO threshold: 255 bytes (uses compression for long messages)

**Recommendation:** Profile and adjust threshold if needed

### Handler Lookup

- Current: String-based handler name lookup
- O(1) for direct table access
- Acceptable performance

**Optimization:** Pre-register handler IDs (numbers) instead of strings

### Queue Management

- Unbounded queue could cause memory issues
- Need timeout for incomplete messages
- Need cleanup for disconnected players

**Recommendation:** Add queue limits and timeouts

## Security Considerations

### 1. Addon Message Spam

**Protection:** Server has `UpdateSpeakTime()` throttling

**Risk:** Malicious client could spam server with messages

**Mitigation:** Monitor and adjust throttle limits

### 2. Malicious Payloads

**Risk:** Client could send crafted messages to exploit server Lua

**Protection:** 
- Smallfolk has some safety features
- Lua sandboxing in Eluna

**Mitigation:** Validate all inputs in handlers

### 3. Handler Injection

**Risk:** Client could call any registered handler

**Protection:** Server-side validation in handlers

**Mitigation:** Only expose safe handlers, validate permissions

### 4. Code Injection via AddAddon

**Risk:** Malicious server could send harmful code to client

**Protection:** Client can disable dynamic addon loading

**Mitigation:** Don't use AddAddon feature, use static addons only

## Future Enhancements

### 1. Multiple Channels

**Current:** Only whisper-to-self
**Enhancement:** Support guild, party, channel for multiplayer features

### 2. Async/Promise Pattern

**Current:** Fire-and-forget handlers
**Enhancement:** Return values, promise-style callbacks

### 3. Typed Messages

**Current:** Everything is Lua tables
**Enhancement:** Schema validation, type checking

### 4. Compression Improvements

**Current:** LZW
**Enhancement:** Better compression (zlib, etc.) if available

### 5. Monitoring/Debugging

**Current:** Print statements
**Enhancement:** Structured logging, metrics, debugging UI

## References

- **Original AIO:** https://github.com/Rochet2/AIO
- **Eluna:** https://github.com/ElunaLuaEngine/Eluna
- **TrinityCore:** https://github.com/TrinityCore/TrinityCore
- **WoW API Docs:** https://wowpedia.fandom.com/wiki/World_of_Warcraft_API
