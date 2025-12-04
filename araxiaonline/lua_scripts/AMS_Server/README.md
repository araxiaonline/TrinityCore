# AMS Server - Araxia Messaging System (Server Side)

**Version:** 1.0.0-alpha

A lightweight, modern client-server messaging library for TrinityCore 11.2.5 with Eluna.

## Folder Structure

```
AMS_Server/
├── AMS_Server.lua   - Main AMS server implementation
├── smallfolk.lua    - Serialization library dependency
└── README.md        - This file
```

## Usage

The AMS_Server module is loaded automatically via:

```lua
require("AMS_Server.AMS_Server")  -- Loads AMS_Server/AMS_Server.lua
```

Note: Named `AMS_Server.lua` instead of `init.lua` to avoid name collision with the main `init.lua` file.

## Components

### AMS_Server.lua
Main AMS server implementation with:
- Handler registration system
- Message splitting/reassembly
- Smallfolk serialization integration
- Request/response patterns
- Error isolation via pcall

### smallfolk.lua
Lightweight Lua serialization library used for encoding/decoding messages between client and server.

## API

### Registering Handlers

```lua
AMS.RegisterHandler("HANDLER_NAME", function(player, data)
    -- Process request
    local response = ProcessData(data)
    
    -- Send response
    AMS.Send(player, "RESPONSE_NAME", response)
end)
```

### Sending Messages

**Simple send:**
```lua
AMS.Send(player, "UPDATE_NPC", {npcID = 1234, hp = 5000})
```

**Fluent API (multiple handlers in one message):**
```lua
AMS.Msg()
    :Add("UPDATE_NPC", {npcID = 1234})
    :Add("UPDATE_QUEST", {questID = 5678})
    :Send(player)
```

## Features

- ✅ Automatic message splitting for long payloads
- ✅ Message reassembly on both ends
- ✅ Handler registration system
- ✅ Error isolation with pcall
- ✅ Player disconnect cleanup
- ✅ Debug logging (toggle via AMS_DEBUG)

## Dependencies

- **TrinityCore 11.2.5** with Eluna
- **Smallfolk** - Included in this folder

## Related

- **Client:** `Interface/AddOns/AMS_Client/`
- **Documentation:** `araxiaonline/araxia_docs/ams_system/`

## Version History

**1.0.0-alpha** (Current)
- Initial release
- Handler registration
- Message splitting/reassembly
- Smallfolk serialization
- Error handling
