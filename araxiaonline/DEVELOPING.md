# TrinityCore Development Quick Start

## Start the Container

```bash
docker compose --env-file .env up -d
```

## Attach to the Container

```bash
docker exec -it trinitycore-dev bash
```

## Build TrinityCore

```bash
cd /workspace/build
bash ../araxiaonline/cmake_setup.sh
cmake --build . -j$(nproc)
```

## Run Worldserver

```bash
/opt/trinitycore/bin/worldserver
```

## Notes

- The container runs `sleep infinity` - it does **not** auto-start worldserver
- This is a development environment, not a production server
- Build cache persists between container restarts
- Config files are mounted from `TrinityServerBits/etc/`
- Lua scripts are mounted from `TrinityServerBits/lua_scripts/`
