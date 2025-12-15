# This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

# AraxiaEventBus - ZeroMQ-based event bus for server events
# See: openspec/changes/add-zeromq-event-bus/

set(EVENTBUS_SOURCES
  AraxiaEventBus.cpp
  AraxiaEventBus.h
  AraxiaEventBusConfig.cpp
  AraxiaEventBusConfig.h
  AraxiaEvents.cpp
  AraxiaEvents.h
)

add_library(araxia-eventbus STATIC
  ${EVENTBUS_SOURCES})

target_include_directories(araxia-eventbus
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(araxia-eventbus
  PUBLIC
    trinity-core-interface
    common
  PRIVATE
    zeromq)

set_target_properties(araxia-eventbus
  PROPERTIES
    FOLDER "server/araxiaonline")
