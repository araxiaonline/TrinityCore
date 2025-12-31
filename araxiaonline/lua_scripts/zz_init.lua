-- Eluna Integration Tests Initialization
-- This script is loaded at server startup and runs the integration test suite

-- Only run tests in the global/world Eluna instance (mapId will be max uint32 for global)
-- Each map instance will also load this script, but we skip execution for those
local mapId = GetStateMapId()
-- Global/world state is -1 (or 4294967295 as unsigned). Skip for map instances.
if mapId ~= -1 and mapId ~= 4294967295 then
    return
end

print("\n" .. string.rep("=", 80))
print("ELUNA INTEGRATION TEST SUITE - AUTO-RUNNING AT STARTUP")
print(string.rep("=", 80) .. "\n")

-- Load the test runner
local testRunner = require("integration_tests/test_runner")

-- Load all test modules (they will register with TestRunner)
require("integration_tests/test_core_functionality")
require("integration_tests/test_events")
require("integration_tests/test_data_types")
require("integration_tests/test_bindings")

-- Run all registered tests
testRunner:RunAll()

print("\n" .. string.rep("=", 80))
print("ELUNA INTEGRATION TEST SUITE - COMPLETE")
print("Loading AMS Server...")
require("ab_AMS_Server.AMS_Server")
print("AMS Server loaded successfully")

-- Load AMS test handlers
require("ac_ams_test_handlers")

-- Load AraxiaTrinityAdmin server handlers (use dofile to force re-execution after AMS is loaded)
dofile("/opt/trinitycore/lua_scripts/ad_admin_handlers.lua")

print(string.rep("=", 80) .. "\n")
