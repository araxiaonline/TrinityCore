# AIO Integration Overview

## Project Goal
Establish working bidirectional communication between WoW 11.2.5 client and TrinityCore server using AIO (Addon I/O) to enable custom content creation tools.

## Current Status (Nov 27, 2025)

### ✅ What's Working
- **TrinityCore server** compiles and runs with Eluna enabled (`-DWITH_ELUNA=1`)
- **Eluna integration** partially functional - hooks and basic scripting works
- **AIO_Server** loaded successfully by Eluna at server startup
- **AIO_Client** addon loads without errors on WoW 11.2.5 client
- **Server-side Eluna hooks** for addon messages (`ADDON_EVENT_ON_MESSAGE`) are present

### ❌ What's NOT Working
- **Client→Server messages** not being received
- **Server→Client messages** not being delivered
- **Bidirectional communication** completely broken

## The Core Problem

AIO was designed for **AzerothCore 3.3.5** (WotLK client/server). We're running **TrinityCore 11.2.5** (Midnight expansion). The gap is **~15 years** of WoW protocol changes.

### Major Differences Between 3.3.5 and 11.2.5

1. **Addon Message API Changed (WoW 8.0+)**
   - 3.3.5: `SendAddonMessage(prefix, message, chatType, target)`
   - 11.2.5: `C_ChatInfo.SendAddonMessage(prefix, message, chatType, target)`
   - Client code has compatibility wrapper, but behavior differs

2. **Addon Message Events Changed (WoW 8.0+)**
   - 3.3.5: `CHAT_MSG_ADDON` event
   - 11.2.5: `CHAT_MSG_ADDON_LOGGED` event (logged), `CHAT_MSG_ADDON` (filtered)
   - Client registers both, but event parameters may differ

3. **Chat Channel System Overhaul (Multiple Expansions)**
   - Channel types changed
   - Whisper targeting changed (GUID-based vs name-based)
   - LANG_ADDON handling changed server-side

4. **Server-Side Addon Handling (TrinityCore Evolution)**
   - WorldPackets structure different
   - `ChatHandler.cpp` implementation different
   - Addon message throttling added
   - Security checks added (Warden integration)

## Repository Structure

### Client Side
**Location:** `q:\Araxia Online\World of Warcraft Araxia Trinity 11.2.5.83634\_retail_\Interface\AddOns\AIO_Client\`

**Key Files:**
- `AIO.lua` - Main client-side AIO implementation
- `AIO_Client.toc` - Addon manifest
- Dependencies: Smallfolk (serialization), LZW compression, LibWindow

**Current Implementation:**
- Uses `SendAddonMessageCompat()` wrapper for API compatibility
- Registers `CHAT_MSG_ADDON` and `CHAT_MSG_ADDON_LOGGED` events
- Sends messages via `WHISPER` channel to self (`UnitName("player")`)

### Server Side
**Location:** `q:\github.com\araxiaonline\TrinityServerBits\lua_scripts\AIO_Server\`

**Key Files:**
- `AIO.lua` - Main server-side AIO implementation
- `queue.lua` - Message queuing
- Dependencies: Smallfolk, LZW, LuaSrcDiet, crc32lua

**Current Implementation:**
- Uses `Player:SendAddonMessage(prefix, msg, channel, target)` Eluna method
- Should receive messages via `ADDON_EVENT_ON_MESSAGE` hook (event 30)
- Expects messages from `OnChat()` hooks with `LANG_ADDON`

### Core Server Integration
**Location:** `q:\github.com\araxiaonline\TrinityCore\src\server\game\`

**Key Files:**
- `Handlers/ChatHandler.cpp` - Processes incoming addon messages
- `LuaEngine/hooks/PlayerHooks.cpp` - Routes LANG_ADDON messages to `OnAddonMessage`
- `LuaEngine/hooks/ServerHooks.cpp` - `OnAddonMessage()` implementation
- `LuaEngine/methods/TrinityCore/PlayerMethods.h` - `SendAddonMessage()` method

## Why Previous Attempts Failed

The previous attempt got **lost in the rabbit hole** of trying to make AIO work across multiple chat channels (whisper, guild, party, channel) without understanding:

1. **How addon messages flow in WoW 11.2.5**
   - What opcodes are actually sent?
   - What packet structure is used?
   - How does the server parse them?

2. **How Eluna receives addon messages**
   - Which hooks fire when?
   - What parameters do they receive?
   - How does LANG_ADDON detection work?

3. **What actually needs to change**
   - Focus was on AIO.lua client/server compatibility
   - Should have focused on C++ server-side message routing
   - Didn't verify messages were even reaching the server

## The Path Forward

### Phase 1: Understand Current Message Flow
1. Add comprehensive logging at every layer
2. Test simple client→server message
3. Verify where messages break
4. Document actual vs expected behavior

### Phase 2: Fix Server-Side Reception
1. Verify `ChatHandler.cpp` processes addon messages correctly
2. Ensure LANG_ADDON detection works for 11.2.5 protocol
3. Verify `OnAddonMessage` hook fires with correct parameters
4. Test Eluna Lua receives messages

### Phase 3: Fix Server→Client Transmission
1. Verify `Player:SendAddonMessage()` sends correct packet format
2. Ensure packet reaches client
3. Verify client event fires
4. Test client Lua receives messages

### Phase 4: Full AIO Integration
1. Restore AIO message queueing
2. Implement addon code distribution
3. Test handler registration and calling
4. Build example client↔server interaction

### Phase 5: Production Readiness
1. Add error handling and logging
2. Performance testing
3. Security review
4. Documentation for content creators

## Success Criteria

We'll know this is working when:
1. ✅ Client can send a simple message to server
2. ✅ Server Lua script receives and logs the message
3. ✅ Server can send a simple message to client
4. ✅ Client Lua addon receives and logs the message
5. ✅ Full AIO handler system works bidirectionally
6. ✅ AraxiaTrinityAdmin addon can call server functions

## Non-Goals (For Now)

- Supporting all chat channel types (whisper, guild, party, etc.)
- Backwards compatibility with 3.3.5
- Multiple AIO prefixes/namespaces
- Dynamic addon loading (initially)

## Critical Principles

1. **Simple First** - Get one channel working perfectly before adding complexity
2. **Test Each Layer** - Verify message flow at C++ level before Lua level
3. **Log Everything** - Can't fix what you can't see
4. **Read the Code** - 11.2.5 != 3.3.5, verify every assumption
5. **Be Willing to Rewrite** - Don't force 3.3.5 code to work on 11.2.5

## Next Steps

See `01_DISCOVERY_PLAN.md` for detailed investigation plan.
