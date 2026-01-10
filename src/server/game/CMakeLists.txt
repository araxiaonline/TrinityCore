# This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

CollectIncludeDirectories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC_INCLUDES
  EXCLUDE
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)

# Provide an interface target for the game project to allow
# dependent projects to build meanwhile.
add_library(game-interface INTERFACE)

target_include_directories(game-interface
  INTERFACE
    ${PUBLIC_INCLUDES})

target_link_libraries(game-interface
  INTERFACE
    shared
    mmaps_common)

# Create lualib interface target for LuaEngine
set(LUA_VERSION "luajit")
add_library(lualib INTERFACE)

# Set LuaJIT paths based on platform
if(APPLE)
  # macOS with Homebrew - detect Homebrew prefix dynamically
  execute_process(
    COMMAND brew --prefix luajit
    OUTPUT_VARIABLE HOMEBREW_LUAJIT_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
  if(HOMEBREW_LUAJIT_PREFIX)
    set(LUAJIT_INCLUDE_DIR "${HOMEBREW_LUAJIT_PREFIX}/include/luajit-2.1")
    set(LUAJIT_LIBRARY_DIR "${HOMEBREW_LUAJIT_PREFIX}/lib")
  else()
    # Fallback to common Homebrew path
    set(LUAJIT_INCLUDE_DIR "/opt/homebrew/opt/luajit/include/luajit-2.1")
    set(LUAJIT_LIBRARY_DIR "/opt/homebrew/opt/luajit/lib")
  endif()
else()
  # Linux - standard system paths
  set(LUAJIT_INCLUDE_DIR "/usr/include/luajit-2.1")
  set(LUAJIT_LIBRARY_DIR "/usr/lib")
endif()

target_include_directories(lualib INTERFACE 
  ${LUAJIT_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}/dep/efsw/include)
target_link_directories(lualib INTERFACE ${LUAJIT_LIBRARY_DIR})
target_link_libraries(lualib INTERFACE luajit-5.1 efsw)
target_compile_definitions(lualib INTERFACE ELUNA LUAJIT_VERSION ELUNA_TRINITY ELUNA_EXPANSION=9)

add_library(game
  ${PRIVATE_SOURCES})

# upstream from TC
# add_library(game)

CollectAndAddSourceFiles(
  game
  ${CMAKE_CURRENT_SOURCE_DIR}
  EXCLUDE
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)

target_include_directories(game
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${LUAJIT_INCLUDE_DIR})

target_link_libraries(game
  PRIVATE
    trinity-core-interface
    lualib
  PUBLIC
    game-interface)

if(TARGET efsw)
  target_link_libraries(game
    PRIVATE
      efsw)
endif()

set_target_properties(game
  PROPERTIES
    COMPILE_WARNING_AS_ERROR ${WITH_WARNINGS_AS_ERRORS}
    DEFINE_SYMBOL TRINITY_API_EXPORT_GAME
    FOLDER "server")

if(BUILD_SHARED_LIBS)
  if(UNIX)
    install(TARGETS game
      LIBRARY
        DESTINATION lib)
  elseif(WIN32)
    install(TARGETS game
      RUNTIME
        DESTINATION "${CMAKE_INSTALL_PREFIX}")
  endif()
endif()

# Add Araxia Online custom code
add_subdirectory(${CMAKE_SOURCE_DIR}/src/araxiaonline ${CMAKE_BINARY_DIR}/araxiaonline)

# Generate precompiled header
if(USE_COREPCH)
  add_cxx_pch(game PrecompiledHeaders/gamePCH.h)
endif()
