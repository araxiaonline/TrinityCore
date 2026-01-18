#!/bin/bash

# run this script from the build directory with
# bash ../araxiaonline/cmake_setup.sh
cmake .. \
    -DCMAKE_INSTALL_PREFIX="/opt/araxiaonline/araxia-trinity-drwow/server" \
    -DCONF_DIR="/opt/araxiaonline/araxia-trinity-drwow/server/etc" \
    -DCMAKE_BUILD_TYPE="RelWithDebInfo" \
    -DTOOLS=1 \
    -DSERVERS=1 \
    -DSCRIPTS=static \
    -DWITH_WARNINGS=1 \
    -DBUILD_EFSW=1 \
    -DCMAKE_C_COMPILER_LAUNCHER=ccache \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache


#    -DWITH_ELUNA=1
