# Araxia Online - Custom Code
# This folder contains all Araxia-specific customizations

# Build eventbus as separate library (has ZMQ dependency)
add_subdirectory(eventbus)

# Collect sources EXCLUDING eventbus (it's built as separate library)
CollectSourceFiles(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE_SOURCES
  SKIP_DIRS
    eventbus)

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

# Add include directories for araxiaonline code
target_include_directories(game
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/game
    ${CMAKE_CURRENT_SOURCE_DIR}/game/Entities
    ${CMAKE_CURRENT_SOURCE_DIR}/game/Entities/Creature
    ${CMAKE_CURRENT_SOURCE_DIR}/mcp
    ${CMAKE_CURRENT_SOURCE_DIR}/eventbus
)

# Add sources to the game target
target_sources(game
  PRIVATE
    ${PRIVATE_SOURCES}
)

# Link eventbus library (brings in ZMQ transitively)
target_link_libraries(game
  PRIVATE
    araxia-eventbus
    zeromq
)

message(STATUS "[Araxia] Custom code sources added")
