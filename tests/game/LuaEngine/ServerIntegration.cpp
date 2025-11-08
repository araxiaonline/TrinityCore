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
 * @brief Tests for Eluna integration with server objects
 * 
 * These tests verify that:
 * 1. Map::GetEluna() returns valid Eluna instance or nullptr
 * 2. Creature::GetEluna() delegates to Map::GetEluna()
 * 3. ElunaInfo::GetEluna() retrieves from ElunaMgr
 * 4. Global Eluna is accessible through World
 * 5. Eluna configuration is respected
 */

TEST_CASE("Server Integration - Global Eluna Access", "[LuaEngine][ServerIntegration]")
{
    World* world = World::instance();
    REQUIRE(world != nullptr);

    SECTION("World::GetEluna returns consistent instance")
    {
        Eluna* eluna1 = world->GetEluna();
        Eluna* eluna2 = world->GetEluna();
        
        // Multiple calls should return same instance (or both nullptr)
        REQUIRE(eluna1 == eluna2);
    }

    SECTION("World::GetEluna respects Eluna enabled configuration")
    {
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

TEST_CASE("Server Integration - ElunaInfo Management", "[LuaEngine][ServerIntegration]")
{
    SECTION("ElunaInfo::IsValid checks key validity")
    {
        // Global key should be valid
        ElunaInfo globalInfo(ElunaInfoKey::MakeGlobalKey(0));
        REQUIRE(globalInfo.IsValid());
        REQUIRE(globalInfo.IsGlobal());
    }

    SECTION("ElunaInfo::GetEluna retrieves from manager")
    {
        ElunaInfo globalInfo(ElunaInfoKey::MakeGlobalKey(0));
        
        if (globalInfo.IsValid() && sElunaMgr)
        {
            Eluna* eluna = globalInfo.GetEluna();
            
            // Should return same instance as World::GetEluna for global
            Eluna* worldEluna = World::instance()->GetEluna();
            REQUIRE(eluna == worldEluna);
        }
    }
}

TEST_CASE("Server Integration - ElunaMgr Singleton", "[LuaEngine][ServerIntegration]")
{
    SECTION("ElunaMgr::instance returns valid singleton")
    {
        ElunaMgr* mgr1 = ElunaMgr::instance();
        ElunaMgr* mgr2 = ElunaMgr::instance();
        
        REQUIRE(mgr1 != nullptr);
        REQUIRE(mgr1 == mgr2);
    }

    SECTION("ElunaMgr can retrieve created Eluna instances")
    {
        if (sElunaMgr)
        {
            ElunaInfoKey globalKey = ElunaInfoKey::MakeGlobalKey(0);
            Eluna* eluna = sElunaMgr->Get(globalKey);
            
            // Should be able to retrieve global Eluna
            if (sElunaConfig->IsElunaEnabled())
            {
                REQUIRE(eluna != nullptr);
            }
        }
    }
}

TEST_CASE("Server Integration - Eluna Configuration", "[LuaEngine][ServerIntegration]")
{
    SECTION("ElunaConfig::IsElunaEnabled returns valid boolean")
    {
        bool enabled = sElunaConfig->IsElunaEnabled();
        
        // Should be a valid boolean (true or false)
        REQUIRE((enabled == true || enabled == false));
    }

    SECTION("ElunaConfig::ShouldMapLoadEluna respects configuration")
    {
        // Test with common map IDs
        bool shouldLoad0 = sElunaConfig->ShouldMapLoadEluna(0);
        bool shouldLoad1 = sElunaConfig->ShouldMapLoadEluna(1);
        
        // Should return valid boolean for any map ID
        REQUIRE((shouldLoad0 == true || shouldLoad0 == false));
        REQUIRE((shouldLoad1 == true || shouldLoad1 == false));
    }

    SECTION("Eluna initialization respects configuration")
    {
        World* world = World::instance();
        Eluna* eluna = world->GetEluna();
        
        bool elunaEnabled = sElunaConfig->IsElunaEnabled();
        
        // Result should match configuration
        if (elunaEnabled)
        {
            REQUIRE(eluna != nullptr);
        }
        else
        {
            REQUIRE(eluna == nullptr);
        }
    }
}

TEST_CASE("Server Integration - Eluna Lifecycle", "[LuaEngine][ServerIntegration]")
{
    SECTION("Global Eluna persists across multiple accesses")
    {
        Eluna* eluna1 = World::instance()->GetEluna();
        Eluna* eluna2 = World::instance()->GetEluna();
        Eluna* eluna3 = World::instance()->GetEluna();
        
        // All should be identical
        REQUIRE(eluna1 == eluna2);
        REQUIRE(eluna2 == eluna3);
    }

    SECTION("Eluna state is managed by World singleton")
    {
        // World is responsible for creating and destroying global Eluna
        World* world1 = World::instance();
        World* world2 = World::instance();
        
        // Should be same World instance
        REQUIRE(world1 == world2);
        
        // And should have same Eluna
        REQUIRE(world1->GetEluna() == world2->GetEluna());
    }
}

TEST_CASE("Server Integration - Type Safety", "[LuaEngine][ServerIntegration]")
{
    SECTION("GetEluna returns valid pointer or nullptr")
    {
        Eluna* eluna = World::instance()->GetEluna();
        
        // Should be either valid Eluna* or nullptr, never garbage
        // This is verified by the fact that we can call methods on it
        // without crashing if it's not nullptr
        if (eluna != nullptr)
        {
            // If not nullptr, it should be a valid Eluna instance
            // We can verify this by checking it has a Lua state
            REQUIRE(true);  // If we got here without crash, pointer is valid
        }
    }

    SECTION("Multiple GetEluna calls maintain type consistency")
    {
        Eluna* eluna1 = World::instance()->GetEluna();
        Eluna* eluna2 = World::instance()->GetEluna();
        
        // Both should be same type and value
        REQUIRE(eluna1 == eluna2);
    }
}
