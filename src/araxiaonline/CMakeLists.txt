# Araxia Online - Custom Code
# This folder contains all Araxia-specific customizations

CollectSourceFiles(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE_SOURCES)

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

# Add include directories for araxiaonline code
target_include_directories(game
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/game
    ${CMAKE_CURRENT_SOURCE_DIR}/game/Entities
    ${CMAKE_CURRENT_SOURCE_DIR}/game/Entities/Creature
    ${CMAKE_CURRENT_SOURCE_DIR}/mcp
)

# Add sources to the game target
target_sources(game
  PRIVATE
    ${PRIVATE_SOURCES}
)

message(STATUS "[Araxia] Custom code sources added")
