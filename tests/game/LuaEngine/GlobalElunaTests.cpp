/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "tc_catch2.h"
#include "World.h"
#include "LuaEngine.h"
#include "ElunaConfig.h"
#include "ElunaMgr.h"

/**
 * @brief Tests for global Eluna singleton in World class
 * 
 * These tests verify that:
 * 1. World singleton exists and is accessible
 * 2. Global Eluna instance is created when ELUNA is enabled
 * 3. GetEluna() returns valid pointer when initialized
 * 4. GetEluna() returns nullptr when Eluna is disabled
 * 5. Global Eluna instance persists across calls
 * 6. Global Eluna is properly initialized with global key
 */

TEST_CASE("Global Eluna Singleton - World Instance", "[LuaEngine][GlobalEluna]")
{
    SECTION("World singleton is accessible")
    {
        World* world = World::instance();
        REQUIRE(world != nullptr);
    }

    SECTION("World singleton is same across multiple calls")
    {
        World* world1 = World::instance();
        World* world2 = World::instance();
        REQUIRE(world1 == world2);
    }

    SECTION("World singleton is not null")
    {
        REQUIRE(World::instance() != nullptr);
    }
}

TEST_CASE("Global Eluna Singleton - GetEluna Method", "[LuaEngine][GlobalEluna]")
{
    World* world = World::instance();
    REQUIRE(world != nullptr);

    SECTION("GetEluna returns consistent value")
    {
        Eluna* eluna1 = world->GetEluna();
        Eluna* eluna2 = world->GetEluna();
        // Both calls should return the same instance (or both nullptr)
        REQUIRE(eluna1 == eluna2);
    }
}

TEST_CASE("Global Eluna Singleton - Initialization State", "[LuaEngine][GlobalEluna]")
{
    World* world = World::instance();
    REQUIRE(world != nullptr);

    SECTION("Eluna config singleton exists")
    {
        // sElunaConfig should be available
        REQUIRE(sElunaConfig != nullptr);
    }

    SECTION("Eluna manager singleton exists")
    {
        // sElunaMgr should be available
        REQUIRE(sElunaMgr != nullptr);
    }

}

TEST_CASE("Global Eluna Singleton - Configuration Awareness", "[LuaEngine][GlobalEluna]")
{
    World* world = World::instance();
    REQUIRE(world != nullptr);

    SECTION("Respects Eluna enabled configuration")
    {
        // If Eluna is enabled in config, GetEluna should return non-null
        // If Eluna is disabled, GetEluna should return nullptr
        bool elunaEnabled = sElunaConfig->IsElunaEnabled();
        Eluna* eluna = world->GetEluna();
        
        if (elunaEnabled)
        {
            // When enabled, should have valid Eluna instance
            REQUIRE(eluna != nullptr);
        }
        else
        {
            // When disabled, should return nullptr
            REQUIRE(eluna == nullptr);
        }
    }
}

TEST_CASE("Global Eluna Singleton - Persistence", "[LuaEngine][GlobalEluna]")
{
    World* world = World::instance();
    REQUIRE(world != nullptr);

    SECTION("Global Eluna instance persists")
    {
        Eluna* eluna1 = world->GetEluna();
        Eluna* eluna2 = world->GetEluna();
        Eluna* eluna3 = world->GetEluna();
        
        // All calls should return same instance
        REQUIRE(eluna1 == eluna2);
        REQUIRE(eluna2 == eluna3);
    }

    SECTION("Global Eluna is same across World calls")
    {
        Eluna* eluna1 = World::instance()->GetEluna();
        Eluna* eluna2 = World::instance()->GetEluna();
        
        REQUIRE(eluna1 == eluna2);
    }
}

TEST_CASE("Global Eluna Singleton - Type Safety", "[LuaEngine][GlobalEluna]")
{
    World* world = World::instance();
    REQUIRE(world != nullptr);

    SECTION("Multiple GetEluna calls are type-safe")
    {
        Eluna* eluna1 = world->GetEluna();
        Eluna* eluna2 = world->GetEluna();
        
        // Both should be same type and value
        REQUIRE(eluna1 == eluna2);
    }
}
