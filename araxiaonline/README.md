# Araxia Online TrinityCore

## Introduction

Araxia Online is a MMORPG server based on TrinityCore. Our goal is to pin the core at the Midnight expansion (just to get player housing!) and focus on creating a stable, playable game environment. Our primary goals are:

1. Create a stable, maintained fork that is no longer focused on staying current but is instead focused on a quality playing experience across Azeroth from 1.x -> 12.x.
2. Create tools for assiting in creating world content. These tools will be a mix of server side, client side, and separate apps and other tools to simplify the process of creating quality content.
3. Build a core that WE at Araxia want to play. This will not be a pure Bliz like experience and we are NOT wanting to try to replicate all retail WoW content ala AzerothCore (which we love!).

Initial goals of this project are:
1. Create a nice docker build/deploy process that's easy to use.
2. Make it easy to get a server going and get a client connected.
3. Embed Eluna into the engine as a first class citizen.
4. Get AIO style communication bettween the server/client so that we can start to build client side tooling that has direct access to any/all client side functionality we want.
5. Build out @AraxiaTrinityAdmin to provide a UI for managing the server and world content.
6. Build content, make it fun.
7. Play the game!

## Repository Structure

This is a **multi-repository workspace**. The main repositories are:

- **`q:\github.com\araxiaonline\TrinityCore`** - TrinityCore fork (this repo)
  - `araxiaonline/` - Custom Docker setup, configs, and documentation
  - `src/server/game/LuaEngine/` - Eluna integration (custom)
- **`q:\github.com\araxiaonline\TrinityServerBits`** - Server runtime files
  - `etc/` - **Server config files** (worldserver.conf, authserver.conf)
  - `lua_scripts/` - Server-side Lua scripts (Eluna)
  - `client_files_11.2.5.63834/` - Game data files (DBC, maps, etc.)
  - `logs/` - **Server logs** (Server.log, Eluna.log, DBErrors.log, GM.log)
- **`q:\Araxia Online\World of Warcraft Araxia Trinity 11.2.5.83634\_retail_\Interface`** - WoW client
  - `AddOns/AraxiaTrinityAdmin/` - Content creation UI addon
  - `AddOns/AIO_Client/` - Client-side AIO communication
  - `AddOns/AIO_Test/` - **Test addon for debugging AIO messages**

## Technical Stack

- **TrinityCore 11.2.5** (Midnight expansion)
- **Eluna Lua Engine** - Embedded as first-class citizen (via `-DWITH_ELUNA=1`)
- **AIO (Addon I/O)** - Client↔Server Lua communication (based on Rochet2's work)
- **Docker** - Development environment (Ubuntu 24.04, Clang, ccache)
- **MySQL** - External database at `192.168.63.11:3456`
- **WoW Client 11.2.5.83634** - Retail client

## Key Components

### AraxiaTrinityAdmin Addon
**Location:** `Interface/AddOns/AraxiaTrinityAdmin/`

Content creation UI for managing NPCs, spawns, and world content. Built with WoW's FrameXML API.

**Current Features:**
- NPC info panel with target inspection
- Add NPC panel for spawning creatures
- Minimap button for quick access

### AIO (Addon I/O) System
**Client:** `Interface/AddOns/AIO_Client/`  
**Server:** `lua_scripts/AIO_Server/`

Bidirectional Lua communication between client and server. Allows custom client UI to call server-side functions and vice versa.

**Based on:** https://github.com/Rochet2/AIO

### Eluna Integration
**Location:** `src/server/game/LuaEngine/`

Embedded Lua scripting engine for TrinityCore. Provides hooks for game events, creature AI, and custom content.

**Key Files:**
- `lua_scripts/init.lua` - Main entry point
- `lua_scripts/integration_tests/` - Test suite for Eluna bindings
- `lua_scripts/run_tests.lua` - Test runner

## Development Workflow

See [DEVELOPING.md](./DEVELOPING.md) for Docker setup and build instructions.

**Quick Start:**
```bash
# Start container
docker compose --env-file .env up -d

# Attach and build
docker exec -it trinitycore-dev bash
cd /workspace/build
bash ../araxiaonline/cmake_setup.sh
cmake --build . -j$(nproc)

# Run server
/opt/trinitycore/bin/worldserver
```

## Important Notes for AI/LLM Developers

1. **Eluna is enabled via CMake:** Use `-DWITH_ELUNA=1`, not `-DELUNA=1`
2. **Multi-repo workspace:** Changes may span TrinityCore, TrinityServerBits, and Interface folders
3. **No upstream README changes:** Keep `TrinityCore/README.md` untouched to avoid merge conflicts
4. **Docker-first development:** All builds happen inside the container
5. **External database:** Server connects to external MySQL, not containerized DB
6. **Paths differ by context:**
   - Container paths: `/opt/trinitycore/`, `/workspace/`
   - Windows paths: `q:\github.com\araxiaonline\`, `q:\Araxia Online\`
7. **AIO communication:** Client and server Lua must stay in sync
8. **WoW API version:** Interface 110205 (11.2.5)
9. **Live addon development:** `q:\Araxia Online\...\Interface\AddOns\` is the LIVE client addons folder. Changes are immediately available after `/reload` in-game - no client restart needed. Excellent for rapid debugging.

## Runtime Files & Logs

### Server Logs (TrinityServerBits/logs/)
**Accessible via workspace context `@logs`**
- `Server.log` - Main server log (includes our `aio.debug` logging)
- `Eluna.log` - Eluna Lua engine log (script loading, Lua errors)
- `DBErrors.log` - Database query errors
- `GM.log` - GM command history

**Monitoring logs in real-time:**
```bash
# In container
tail -f /opt/trinitycore/logs/Server.log | grep "aio.debug"
tail -f /opt/trinitycore/logs/Eluna.log
```

### Server Config (TrinityServerBits/etc/)
**Accessible via workspace context `@etc`**
- `worldserver.conf` - Main server configuration
  - `AddonChannel.Enable` - Must be `1` for addon messages to work
  - Logging levels, database connections, etc.
- `authserver.conf` - Auth server configuration

**Important config settings for AIO:**
```conf
# In worldserver.conf
AddonChannel.Enable = 1
```

## Documentation

Additional docs in `araxiaonline/araxia_docs/`:
- `ELUNA_INTEGRATION_COMPLETE.md` - Eluna setup details
- `ELUNA_CONFIG_GUIDE.md` - Configuration guide
- `DOCKER_BUILD_README.md` - Docker build process
- `ARAXIA_TODO.md` - Project roadmap

## License


