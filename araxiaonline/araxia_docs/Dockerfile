# TrinityCore Multi-Stage Docker Build
# This Dockerfile builds TrinityCore from source in a clean environment

# ============================================================================
# Build Stage: Compile TrinityCore from source
# ============================================================================
FROM ubuntu:24.04 AS builder

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies (Ubuntu 24.04 has CMake 3.28+)
RUN apt-get update && apt-get install -y \
    git \
    clang \
    cmake \
    make \
    gcc \
    g++ \
    libmysqlclient-dev \
    libssl-dev \
    libbz2-dev \
    libreadline-dev \
    libncurses-dev \
    libboost-all-dev \
    p7zip-full \
    && rm -rf /var/lib/apt/lists/*

# Set compiler to clang (recommended by TrinityCore)
RUN update-alternatives --install /usr/bin/cc cc /usr/bin/clang 100 && \
    update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++ 100

# Create build directory
WORKDIR /build

# Copy source code
COPY . /build/TrinityCore

# Create build directory and compile
WORKDIR /build/TrinityCore/build

# Configure with CMake
# Build options:
# - CMAKE_INSTALL_PREFIX: Where to install the compiled binaries
# - CMAKE_BUILD_TYPE: Release build for production
# - WITH_WARNINGS: Disable warnings to speed up compilation
# - TOOLS: Build extraction tools (mapextractor, vmap4extractor, etc.)
# - SERVERS: Build server binaries (worldserver, authserver)
RUN cmake ../ \
    -DCMAKE_INSTALL_PREFIX=/opt/trinitycore \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DTOOLS=1 \
    -DSERVERS=1 \
    -DSCRIPTS=static \
    -DWITH_WARNINGS=0

# Compile (use all available cores, adjust -j if low on RAM)
# If you have less than 4GB RAM, use -j2 or -j1
RUN make -j$(nproc) && make install

# ============================================================================
# Runtime Stage: Create minimal runtime image
# ============================================================================
FROM ubuntu:24.04 AS runtime

# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install only runtime dependencies
RUN apt-get update && apt-get install -y \
    libmysqlclient21 \
    mysql-client \
    libssl3t64 \
    libbz2-1.0 \
    libreadline8t64 \
    libncurses6 \
    libboost-all-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Create trinitycore user and directories
RUN useradd -m -s /bin/bash trinitycore && \
    mkdir -p /opt/trinitycore/etc \
             /opt/trinitycore/logs \
             /opt/trinitycore/data \
             /opt/trinitycore/sql \
             /build/TrinityCore && \
    chown -R trinitycore:trinitycore /opt/trinitycore /build/TrinityCore

# Copy compiled binaries from builder stage
COPY --from=builder --chown=trinitycore:trinitycore /opt/trinitycore /opt/trinitycore

# Copy SQL files to both locations:
# - /opt/trinitycore/sql for docker-compose volume mount
# - /build/TrinityCore/sql for hardcoded paths in binaries
COPY --chown=trinitycore:trinitycore sql /opt/trinitycore/sql
COPY --chown=trinitycore:trinitycore sql /build/TrinityCore/sql

WORKDIR /opt/trinitycore
USER trinitycore

# Expose default ports
# 1119: Auth server (bnetserver)
# 8085: World server
# 3724: Auth server (legacy)
# 8086: World server SOAP
EXPOSE 1119 8085 3724 7878

# Volume mount points for configuration and data
VOLUME ["/opt/trinitycore/etc", "/opt/trinitycore/data", "/opt/trinitycore/logs"]

# Default command - shows available binaries
CMD ["sh", "-c", "echo 'TrinityCore Docker Image'; echo 'Available binaries in /opt/trinitycore/bin:'; ls -la /opt/trinitycore/bin/; echo ''; echo 'To run worldserver: docker run --entrypoint=/opt/trinitycore/bin/worldserver <image>'; echo 'To run authserver: docker run --entrypoint=/opt/trinitycore/bin/authserver <image>'"]
