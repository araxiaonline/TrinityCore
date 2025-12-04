# TrinityCore Docker Build Guide

This guide explains how to build and run TrinityCore using Docker containers.

## Overview

This Docker setup provides a clean, reproducible build environment for TrinityCore with:
- **Multi-stage builds** for minimal image sizes
- **Separate runtime images** for worldserver and authserver
- **Docker Compose** orchestration for easy deployment
- **Volume mounts** for configuration and data persistence

## Recent Changes

### Fixed Missing libboost-locale Dependency (October 19, 2025)

**Issue:** Runtime error when starting authserver/worldserver:
```
libboost_locale.so.1.83.0: cannot open shared object file: No such file or directory
```

**Fix:** Added `libboost-locale1.83.0` to runtime dependencies in Dockerfile (line 82).

**Action Required:** Rebuild your Docker images using the instructions below to include this fix.

## Prerequisites

- Docker Engine 20.10+ 
- Docker Compose 2.0+
- At least 4GB RAM (8GB+ recommended for faster builds)
- ~10GB free disk space for images and data

## Quick Start

### 1. Build the Docker Image

#### Using the Build Helper Script (Recommended)

The `docker-build.sh` script automates the build process:

```bash
# For local testing (single architecture)
./docker-build.sh -r ghcr.io/araxiaonline -n trinitycore -t 11.2.5.63834

# For production (multi-architecture with push to registry)
./docker-build.sh \
  -r ghcr.io/araxiaonline \
  -n trinitycore \
  -t 11.2.5.63834 \
  --platform linux/amd64,linux/arm64 \
  --multi-arch \
  --push

# For ARM64 systems
./docker-build.sh --platform linux/arm64
```

**Script Options:**
- `-r, --registry`: Container registry (e.g., ghcr.io/araxiaonline)
- `-n, --name`: Image name (default: trinitycore)
- `-t, --tag`: Image tag/version (default: latest)
- `-p, --platform`: Target platform (default: linux/amd64)
- `-m, --multi-arch`: Enable multi-architecture build
- `--push`: Push image to registry after build

#### Using Docker Compose Directly

```bash
# Build all stages (builder + runtime)
docker compose build

# Or build a specific stage
docker build --target runtime -t trinitycore:runtime .
```

### 2. Prepare Configuration

```bash
# Create configuration directory
mkdir -p etc logs data

# Copy default configuration files from the repository
# (Adjust paths based on your TrinityCore branch)
cp src/server/worldserver/worldserver.conf.dist etc/worldserver.conf
cp src/server/authserver/authserver.conf.dist etc/authserver.conf

# Edit configuration files to point to MySQL container
# In both conf files, set:
#   LoginDatabaseInfo = "mysql;3306;trinity;trinity;auth"
#   WorldDatabaseInfo = "mysql;3306;trinity;trinity;world"  
#   CharacterDatabaseInfo = "mysql;3306;trinity;trinity;characters"
```

### 3. Set Environment Variables

```bash
# Create .env file for Docker Compose
cat > .env << EOF
MYSQL_ROOT_PASSWORD=trinity_root_password
MYSQL_USER=trinity
MYSQL_PASSWORD=trinity
MYSQL_AUTH_DATABASE=auth
EOF
```

### 4. Initialize Database

```bash
# Start MySQL container
docker compose up -d mysql

# Wait for MySQL to be ready
sleep 30

# Import SQL files
# First, create the databases
docker compose exec mysql mysql -uroot -ptrinity_root_password -e "CREATE DATABASE IF NOT EXISTS auth;"
docker compose exec mysql mysql -uroot -ptrinity_root_password -e "CREATE DATABASE IF NOT EXISTS characters;"
docker compose exec mysql mysql -uroot -ptrinity_root_password -e "CREATE DATABASE IF NOT EXISTS world;"

# Import base SQL files
docker compose exec -T mysql mysql -uroot -ptrinity_root_password auth < sql/base/auth_database.sql
docker compose exec -T mysql mysql -uroot -ptrinity_root_password characters < sql/base/characters_database.sql
# World database import depends on your TDB version - download separately
```

### 5. Extract Client Data

Before running the worldserver, you need to extract data from your WoW client:

```bash
# Run the extraction tools from the container
docker run --rm -it \
  -v /path/to/wow/client:/client \
  -v $(pwd)/data:/opt/trinitycore/data \
  trinitycore:runtime \
  /opt/trinitycore/bin/mapextractor -i /client -o /opt/trinitycore/data

# Extract VMaps
docker run --rm -it \
  -v /path/to/wow/client:/client \
  -v $(pwd)/data:/opt/trinitycore/data \
  trinitycore:runtime \
  /opt/trinitycore/bin/vmap4extractor -d /client -o /opt/trinitycore/data

# Assemble VMaps
docker run --rm -it \
  -v $(pwd)/data:/opt/trinitycore/data \
  trinitycore:runtime \
  /opt/trinitycore/bin/vmap4assembler /opt/trinitycore/data/Buildings /opt/trinitycore/data/vmaps

# Extract MMaps (this takes a long time!)
docker run --rm -it \
  -v $(pwd)/data:/opt/trinitycore/data \
  trinitycore:runtime \
  /opt/trinitycore/bin/mmaps_generator
```

### 6. Start Services

```bash
# Start all services (MySQL, authserver, worldserver)
docker compose up -d

# View logs
docker compose logs -f

# View logs for specific service
docker compose logs -f worldserver
docker compose logs -f authserver
```

## Build Options

### Custom CMake Options

Edit the `Dockerfile` and modify the CMake configuration line:

```dockerfile
RUN cmake ../ \
    -DCMAKE_INSTALL_PREFIX=/opt/trinitycore \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DTOOLS=1 \
    -DSERVERS=1 \
    -DSCRIPTS=static \
    -DWITH_WARNINGS=0 \
    # Add your custom options here
```

Common options:
- `-DTOOLS=0/1`: Build extraction tools
- `-DSERVERS=0/1`: Build server binaries
- `-DSCRIPTS=static/dynamic`: Link scripts statically or dynamically
- `-DCMAKE_BUILD_TYPE=Debug/Release/RelWithDebInfo`: Build type
- `-DWITH_WARNINGS=0/1`: Enable/disable compiler warnings

### Memory-Constrained Builds

If you have less than 4GB RAM, modify the `make` command in the Dockerfile:

```dockerfile
# Use only 1 or 2 cores
RUN make -j2 && make install
```

## Directory Structure

```
TrinityCore/
├── Dockerfile              # Multi-stage build definition
├── docker-compose.yml      # Service orchestration
├── .dockerignore          # Files to exclude from build
├── .env                   # Environment variables (create this)
├── etc/                   # Configuration files (create this)
│   ├── worldserver.conf
│   └── authserver.conf
├── data/                  # Extracted client data (create this)
│   ├── dbc/
│   ├── maps/
│   ├── mmaps/
│   └── vmaps/
└── logs/                  # Server logs (auto-created)
    ├── Server.log
    └── DBErrors.log
```

## Troubleshooting

### Missing libboost-locale Library Error

**Symptom:**
```
libboost_locale.so.1.83.0: cannot open shared object file: No such file or directory
```

**Solution:**
This issue has been fixed in the Dockerfile. Rebuild your image using the latest Dockerfile:

```bash
./docker-build.sh -r ghcr.io/araxiaonline -n trinitycore -t <version>
```

**Temporary Workaround (if you cannot rebuild immediately):**

If you're using an older image and need a temporary fix, you can use wrapper scripts that install the missing dependency at runtime. See the `trinity-tools/docker-cloudflare/scripts/` directory for examples.

In your docker-compose.yml:
```yaml
authserver:
  entrypoint: ["/scripts/start-bnetserver.sh"]
  user: root
  volumes:
    - ./scripts:/scripts:ro
    # ... other volumes

worldserver:
  entrypoint: ["/scripts/start-worldserver.sh"]
  user: root
  volumes:
    - ./scripts:/scripts:ro
    # ... other volumes
```

**After rebuilding with the fixed image**, update your docker-compose.yml:
```yaml
authserver:
  entrypoint: ["/opt/trinitycore/bin/bnetserver"]
  user: trinitycore
  # Remove the scripts volume mount

worldserver:
  entrypoint: ["/opt/trinitycore/bin/worldserver"]
  user: trinitycore
  # Remove the scripts volume mount
```

### Build fails with "Killed" error

This usually means you're running out of memory. Solutions:
1. Reduce parallel jobs: Change `-j$(nproc)` to `-j1` or `-j2` in Dockerfile
2. Add more swap space to your system
3. Use a machine with more RAM

### Database connection errors

Check that:
1. MySQL container is running: `docker compose ps`
2. Configuration files have correct database host (should be `mysql`, not `localhost`)
3. Database credentials match your `.env` file
4. Databases are created and populated with SQL

### Worldserver crashes on startup

Common causes:
1. Missing or incomplete client data in `./data` directory
2. Database not properly initialized
3. Configuration file errors

Check logs: `docker compose logs worldserver`

### Permission errors

The container runs as user `trinitycore` (non-root). Ensure mounted volumes have correct permissions:

```bash
# Fix ownership
sudo chown -R 1000:1000 etc/ data/ logs/
```

## Advanced Usage

### Running Individual Containers

```bash
# Run only authserver
docker compose up -d mysql authserver

# Run worldserver separately
docker run -d \
  --name trinitycore-worldserver \
  --network trinitycore_trinitycore \
  -v $(pwd)/etc:/opt/trinitycore/etc:ro \
  -v $(pwd)/data:/opt/trinitycore/data:ro \
  -v $(pwd)/logs:/opt/trinitycore/logs \
  -p 8085:8085 \
  trinitycore:worldserver
```

### Interactive Debugging

```bash
# Start container with shell instead of server
docker compose run --rm --entrypoint=/bin/bash worldserver

# Or attach to running container
docker compose exec worldserver /bin/bash
```

### Building Without Docker Compose

```bash
# Build base runtime image
docker build -t trinitycore:latest .

# Build specific server image
docker build --target worldserver -t trinitycore:worldserver .
docker build --target authserver -t trinitycore:authserver .
```

### Using Pre-built Images

If you want to push/pull images from a registry:

```bash
# Tag your image
docker tag trinitycore:latest yourregistry.com/trinitycore:latest

# Push to registry
docker push yourregistry.com/trinitycore:latest

# Pull on another machine
docker pull yourregistry.com/trinitycore:latest
```

## Maintenance

### Updating TrinityCore

```bash
# Pull latest code
git pull origin master  # or your branch

# Rebuild images using the build script
./docker-build.sh -r ghcr.io/araxiaonline -n trinitycore -t <new-version>

# Or rebuild with docker compose
docker compose build --no-cache

# Restart services
docker compose down
docker compose up -d
```

### Verifying the Build

After rebuilding with the libboost-locale fix, verify the library is present:

```bash
# Check that the library is present
docker run --rm --entrypoint=sh ghcr.io/araxiaonline/trinitycore:11.2.5.63834 \
  -c "ldconfig -p | grep libboost_locale"

# Should output:
# libboost_locale.so.1.83.0 (libc6,x86-64) => /lib/x86_64-linux-gnu/libboost_locale.so.1.83.0
```

### Backup Database

```bash
# Backup all databases
docker compose exec mysql mysqldump -uroot -ptrinity_root_password --all-databases > backup.sql

# Backup specific database
docker compose exec mysql mysqldump -uroot -ptrinity_root_password auth > auth_backup.sql
```

### View Resource Usage

```bash
# Check container stats
docker stats trinitycore-worldserver trinitycore-authserver trinitycore-mysql
```

## Performance Tuning

### MySQL Configuration

Create `my.cnf` and mount it:

```yaml
# In docker-compose.yml, add to mysql service:
volumes:
  - ./my.cnf:/etc/mysql/conf.d/custom.cnf:ro
```

Sample `my.cnf`:
```ini
[mysqld]
max_connections = 200
innodb_buffer_pool_size = 2G
innodb_log_file_size = 512M
```

### Container Resource Limits

In `docker-compose.yml`:

```yaml
worldserver:
  # ... other config ...
  deploy:
    resources:
      limits:
        cpus: '2'
        memory: 4G
      reservations:
        cpus: '1'
        memory: 2G
```

## Security Considerations

1. **Change default passwords** in `.env` file
2. **Don't expose MySQL port** to public internet (remove ports section from mysql service)
3. **Use read-only mounts** where possible (`:ro` flag)
4. **Keep images updated** regularly rebuild with latest base images
5. **Run as non-root** (already configured in Dockerfile)

## Support

For TrinityCore-specific questions, visit:
- Website: https://www.trinitycore.org/
- Forum: https://community.trinitycore.org/
- Discord: https://discord.gg/trinitycore

For Docker-related issues, check the Docker documentation:
- Docker: https://docs.docker.com/
- Docker Compose: https://docs.docker.com/compose/

## License

TrinityCore is licensed under GPL-2.0. See the main repository LICENSE file for details.
