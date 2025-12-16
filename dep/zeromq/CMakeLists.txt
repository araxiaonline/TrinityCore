# This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

# ZeroMQ Event Bus Integration
# This provides pub/sub messaging for server events, encounter monitoring,
# and client addon communication via AMS bridge.
# See: openspec/changes/add-zeromq-event-bus/

# Find system libzmq
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)

# Create interface library for ZeroMQ
add_library(zeromq INTERFACE)

target_include_directories(zeromq
  INTERFACE
    ${ZMQ_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/cppzmq)

target_link_libraries(zeromq
  INTERFACE
    ${ZMQ_LIBRARIES})

target_link_directories(zeromq
  INTERFACE
    ${ZMQ_LIBRARY_DIRS})

target_compile_options(zeromq
  INTERFACE
    ${ZMQ_CFLAGS_OTHER})

message(STATUS "Found ZeroMQ: ${ZMQ_VERSION}")
