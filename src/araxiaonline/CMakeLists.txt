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

# Install araxia.conf.dist to the config directory
# It will be loaded after worldserver.conf via the .conf.d mechanism
# Note: We use a custom target since game target is defined elsewhere
if(COPY_CONF)
  if(UNIX)
    # Copy to build output during build
    add_custom_target(araxia-conf ALL
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../etc/worldserver.conf.d
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/araxia.conf.dist ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../etc/worldserver.conf.d/araxia.conf.dist
      COMMENT "[Araxia] Copying araxia.conf.dist to etc/worldserver.conf.d/"
    )
    # Install to system config directory
    install(FILES araxia.conf.dist DESTINATION ${CONF_DIR}/worldserver.conf.d)
  elseif(WIN32)
    add_custom_target(araxia-conf ALL
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/worldserver.conf.d
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/araxia.conf.dist ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/worldserver.conf.d/
      COMMENT "[Araxia] Copying araxia.conf.dist to worldserver.conf.d/"
    )
    install(FILES araxia.conf.dist DESTINATION "${CMAKE_INSTALL_PREFIX}/worldserver.conf.d")
  endif()
endif()

message(STATUS "[Araxia] Custom code sources added")
