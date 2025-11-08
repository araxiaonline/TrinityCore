# This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

# Eluna/LuaEngine test suite
# These tests validate Eluna functionality including:
# - Core Lua execution
# - Data type conversions
# - Event system
# - Method bindings
# - Integration scenarios
# - Edge cases and error handling

message(STATUS "Adding Eluna/LuaEngine tests")

set(LUAENGINE_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/LuaEngineTestFixture.h
  ${CMAKE_CURRENT_SOURCE_DIR}/MockServerObjects.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ElunaTestSetup.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GlobalElunaTests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ServerIntegration.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/LuaEngineCore.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/LuaEngineAdvanced.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/LuaEngineIntegration.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/CoreLuaExecution.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/DataTypeConversions.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/EventSystem.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Integration.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/EdgeCases.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/MethodBindings.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/LuaCommandTests.cpp
)

set(TEST_SOURCES ${TEST_SOURCES} ${LUAENGINE_TEST_SOURCES} PARENT_SCOPE)
