# TrinityCore Development Container
# Based on Ubuntu 24.04 with all build dependencies and development tools
FROM ubuntu:24.04

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies and development tools
RUN apt-get update && apt-get install -y \
    # Build essentials
    git \
    clang \
    cmake \
    make \
    gcc \
    g++ \
    gdb \
    ccache \
    ninja-build \
    # TrinityCore dependencies
    libmysqlclient-dev \
    libssl-dev \
    libbz2-dev \
    libreadline-dev \
    libncurses-dev \
    libboost-all-dev \
    p7zip-full \
    # Lua dependencies
    libluajit-5.1-dev \
    luajit \
    # Database client
    mysql-client \
    # Development tools
    vim \
    nano \
    curl \
    wget \
    htop \
    tmux \
    bash-completion \
    # Debugging and profiling
    valgrind \
    strace \
    # Documentation
    doxygen \
    graphviz \
    # Locale support
    locales \
    && rm -rf /var/lib/apt/lists/*

# Configure locale to fix runtime locale errors
RUN sed -i '/en_US.UTF-8/s/^# //g' /etc/locale.gen && \
    locale-gen en_US.UTF-8

ENV LANG=en_US.UTF-8
ENV LANGUAGE=en_US:en
ENV LC_ALL=en_US.UTF-8

# Set compiler to clang (recommended by TrinityCore)
RUN update-alternatives --install /usr/bin/cc cc /usr/bin/clang 100 && \
    update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++ 100

# Create trinitycore user with sudo privileges
RUN useradd -m -s /bin/bash -G sudo trinitycore && \
    echo "trinitycore ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# Create necessary directories
RUN mkdir -p \
    /opt/trinitycore/bin \
    /opt/trinitycore/etc \
    /opt/trinitycore/logs \
    /opt/trinitycore/data \
    /opt/trinitycore/lua_scripts \
    /workspace/build && \
    chown -R trinitycore:trinitycore /opt/trinitycore /workspace

# Configure ccache for faster rebuilds
ENV CCACHE_DIR=/workspace/.ccache
ENV CCACHE_MAXSIZE=5G
RUN mkdir -p /workspace/.ccache && \
    chown -R trinitycore:trinitycore /workspace/.ccache

# Set working directory
WORKDIR /workspace

# Switch to trinitycore user
USER trinitycore

# Set up bash completion and helpful aliases
RUN echo 'alias ll="ls -lah"' >> ~/.bashrc && \
    echo 'alias build="cd /workspace/build && cmake --build . -j\$(nproc)"' >> ~/.bashrc && \
    echo 'alias rebuild="cd /workspace/build && cmake --build . --clean-first -j\$(nproc)"' >> ~/.bashrc && \
    echo 'alias worldserver="/opt/trinitycore/bin/worldserver"' >> ~/.bashrc && \
    echo 'export PS1="\[\033[01;32m\]\u@\h\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]\$ "' >> ~/.bashrc

# Default command
CMD ["/bin/bash"]
