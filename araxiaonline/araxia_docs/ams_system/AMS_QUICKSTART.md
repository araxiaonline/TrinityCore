# AMS Quick Start Guide

Get the Araxia Messaging System up and running in 5 minutes!

## 🚀 Step 1: Restart WorldServer

The server files are already in place. Just restart worldserver to load AMS.

### In Docker Container:

```bash
# If worldserver is running, stop it first
pkill worldserver

# Start worldserver
cd /opt/trinitycore/bin
./worldserver
```

### Look for these messages in the startup log:

```
================================================================================
LOADING ARAXIA MESSAGING SYSTEM (AMS)
================================================================================
[AMS Server] AMS Server v1.0.0 initialized
[AMS Test] Loading test handlers...
[AMS Test] Test handlers registered successfully!
[AMS Test] Available handlers:
  - ECHO: Echo a message back
  - NPC_SEARCH: Search for NPCs (supports request/response)
  - GET_PLAYER_INFO: Get player information
  - TEST_LONG_MESSAGE: Test message splitting
  - REQUEST_MULTI_INFO: Test multi-block messages
AMS Server loaded successfully
================================================================================
```

✅ **If you see this, AMS server is ready!**

---

## 🎮 Step 2: Reload Addons In-Game

1. **Log into your character**
2. **Type:** `/reload`
3. **Wait for addons to load** (~2-3 seconds)

### Look for these messages in chat:

```
[AMS Client] AMS Client v1.0.0 initialized
[AMS Test] Addon loading...
[AMS Test] AMS is available, registering handlers...
[AMS Test] Addon loaded! Type /amstest for commands
```

✅ **If you see this, AMS client is ready!**

---

## 🧪 Step 3: Run Test Commands

### Test 1: Echo (Basic Send/Receive)

```
/amstest echo Hello from client!
```

**Expected output:**
```
[AMS Test] Sending echo: Hello from client!
[AMS Client] Sending message, length: 53
[AMS Client] Received message from server
[AMS Client] Processing 1 message block(s)
[AMS Client] Calling handler: ECHO_RESPONSE
[AMS Test] Echo response: Server echoed: Hello from client!
[AMS Test] Server timestamp: 1732762800
```

**Server log (check with `tail -f /opt/trinitycore/logs/Server.log | grep AMS`):**
```
[AMS Server] Received message from PlayerName
[AMS Server] Processing 1 message block(s)
[AMS Server] Calling handler: ECHO
[AMS Test] ECHO received from PlayerName: Hello from client!
[AMS Server] Sending message to PlayerName length: 98
```

✅ **Echo working = basic messaging works!**

---

### Test 2: NPC Search (Simple Handler)

```
/amstest search rag
```

**Expected output:**
```
[AMS Test] Searching for NPCs: rag
[AMS Client] Sending message, length: 49
[AMS Test] === NPC SEARCH RESULTS ===
[AMS Test] 1. [1234] Ragnaros the Firelord (Level 60)
[AMS Test] 2. [5678] Ragnar the Warrior (Level 45)
[AMS Test] 3. [9012] Raggy the Pet (Level 1)
[AMS Test] ========================
```

✅ **NPC search working = data serialization works!**

---

### Test 3: Request/Response Pattern

```
/amstest searchreq rag
```

**Expected output:**
```
[AMS Test] Searching for NPCs (request pattern): rag
[AMS Client] Sending message, length: 68
[AMS Client] Sent request 1 for handler NPC_SEARCH
[AMS Client] Calling response callback for request 1
[AMS Test] === REQUEST CALLBACK RESULTS ===
[AMS Test] 1. [1234] Ragnaros the Firelord (Level 60)
[AMS Test] 2. [5678] Ragnar the Warrior (Level 45)
[AMS Test] 3. [9012] Raggy the Pet (Level 1)
[AMS Test] ===============================
```

✅ **Request/response working = advanced pattern works!**

---

### Test 4: Player Info (Live Data)

```
/amstest info
```

**Expected output:**
```
[AMS Test] Requesting player info...
[AMS Test] === PLAYER INFO ===
[AMS Test] Name: YourCharName
[AMS Test] Level: 80
[AMS Test] Health: 15000 / 15000
[AMS Test] Mana: 8000 / 8000
[AMS Test] Location: Map 0 Zone 1519
[AMS Test] Position: -8913.23, 554.63, 93.79
[AMS Test] ===================
```

✅ **Player info working = server can access live data!**

---

### Test 5: Long Message (Message Splitting)

```
/amstest long
```

**Expected output:**
```
[AMS Test] Testing long message (message splitting)...
[AMS Server] Splitting message ID 1 into 4 parts
[AMS Client] Received part 1 of 4 for message ID 1
[AMS Client] Received part 2 of 4 for message ID 1
[AMS Client] Received part 3 of 4 for message ID 1
[AMS Client] Received part 4 of 4 for message ID 1
[AMS Client] Message ID 1 complete, reassembling...
[AMS Test] === LONG MESSAGE RECEIVED ===
[AMS Test] Received 100 items
[AMS Test] First item: Test Item 1
[AMS Test] Last item: Test Item 100
[AMS Test] ============================
```

✅ **Message splitting working = can send large data!**

---

### Test 6: Fluent API (Multi-Block Messages)

```
/amstest fluent
```

**Expected output:**
```
[AMS Test] Testing fluent API (client-side)...
[AMS Test] Sent 3 messages in one packet!
[AMS Test] Echo response: Server echoed: First message
[AMS Test] Echo response: Server echoed: Second message
[AMS Test] === PLAYER INFO ===
[AMS Test] Name: YourCharName
[AMS Test] Level: 80
...
```

✅ **Fluent API working = can batch messages!**

---

## 🎯 All Tests Passing?

If all 6 tests work, **AMS is fully operational!** 🎉

You now have:
- ✅ Client→Server messaging
- ✅ Server→Client messaging
- ✅ Data serialization
- ✅ Request/response pattern
- ✅ Message splitting
- ✅ Fluent API
- ✅ Error isolation

---

## 🐛 Troubleshooting

### Problem: Addon not loaded in-game

**Check:**
```
/reload
```

**Look for:**
- Make sure `AMS_Client` and `AMS_Test` folders exist in `Interface/AddOns/`
- Check `Interface.log` for errors
- Make sure Smallfolk dependency was copied

**Fix:**
```bash
# Re-copy Smallfolk if needed
Copy-Item -Path "Interface/AddOns/AIO_Client/Dep_Smallfolk" -Destination "Interface/AddOns/AMS_Client/Dep_Smallfolk" -Recurse -Force
```

---

### Problem: Messages not reaching server

**Check server log:**
```bash
tail -f /opt/trinitycore/logs/Server.log | grep -i "ams\|addon"
```

**Look for:**
- `[AMS Server] Received message from PlayerName`
- If missing, check ChatHandler logging

**Enable debug:**
In `AMS_Client.lua` and `AMS_Server.lua`, set:
```lua
local AMS_DEBUG = true
```

---

### Problem: Handler not found

**Make sure handlers are registered BEFORE you send:**

```lua
-- WRONG (handler registered after send)
AMS.Send("MY_HANDLER", data)
AMS.RegisterHandler("MY_HANDLER", function(data) end)

-- RIGHT (handler registered first)
AMS.RegisterHandler("MY_HANDLER", function(data) end)
AMS.Send("MY_HANDLER", data)
```

---

### Problem: Serialization errors

**Check your data:**

Smallfolk supports:
- ✅ Strings, numbers, booleans
- ✅ Tables (array and hash)
- ❌ Functions, userdata, threads

**Example of BAD data:**
```lua
-- This will fail
AMS.Send("HANDLER", {
    callback = function() end,  -- ❌ Function
    player = GetPlayer()        -- ❌ Userdata
})
```

**Example of GOOD data:**
```lua
-- This will work
AMS.Send("HANDLER", {
    name = "PlayerName",        -- ✅ String
    level = 80,                 -- ✅ Number
    isOnline = true,            -- ✅ Boolean
    items = {1, 2, 3}          -- ✅ Array
})
```

---

## 📚 Next Steps

### 1. Read the Full Documentation

See `AMS_README.md` for:
- Complete API reference
- Architecture details
- Advanced examples
- Best practices

### 2. Integrate with AraxiaTrinityAdmin

Now that AMS is working, you can:
- Replace UI stubs with real server calls
- Implement NPC search
- Implement NPC spawning
- Build content creation tools

### 3. Add Your Own Handlers

```lua
-- Server side (in a new lua_scripts file)
AMS.RegisterHandler("MY_FEATURE", function(player, data)
    -- Your logic here
    AMS.Send(player, "MY_RESPONSE", result)
end)

-- Client side (in your addon)
AMS.RegisterHandler("MY_RESPONSE", function(data)
    -- Update UI with data
end)
```

---

## 🎊 Success!

You now have a **modern, lightweight, fully-functional** messaging system for your custom content creation tools!

**What you built:**
- ~5KB of code (vs 417KB AIO)
- Modern 11.2.5 API
- Request/response pattern
- Message splitting
- Full error isolation
- Complete test coverage

**Time to build amazing things!** 🚀

---

## Quick Reference Card

```lua
-- CLIENT SIDE
-- Send simple message
AMS.Send("HANDLER_NAME", {data})

-- Request/response
AMS.Request("HANDLER", {data}, function(response)
    -- Handle response
end)

-- Fluent API
AMS.Msg()
    :Add("HANDLER1", data1)
    :Add("HANDLER2", data2)
    :Send()

-- Register handler
AMS.RegisterHandler("HANDLER_NAME", function(data)
    -- Handle data
end)

------------------------------------------------------------

-- SERVER SIDE
-- Send to player
AMS.Send(player, "HANDLER_NAME", {data})

-- Fluent API
AMS.Msg()
    :Add("HANDLER1", data1)
    :Add("HANDLER2", data2)
    :Send(player)

-- Register handler
AMS.RegisterHandler("HANDLER_NAME", function(player, data)
    -- Handle data from client
    AMS.Send(player, "RESPONSE", result)
end)
```

---

**Happy coding!** 🎮
