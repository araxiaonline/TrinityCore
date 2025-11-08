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

#ifndef TRINITY_MOCK_SERVER_OBJECTS_H
#define TRINITY_MOCK_SERVER_OBJECTS_H

#include "Common.h"
#include <memory>
#include <unordered_map>

/**
 * @brief Mock Eluna configuration for testing
 */
class MockElunaConfig
{
public:
    MockElunaConfig() = default;
    ~MockElunaConfig() = default;

    bool IsElunaEnabled() const { return true; }
    bool ShouldMapLoadEluna(uint32 mapId) const { return true; }
};

/**
 * @brief Mock Eluna Manager for testing
 * Provides minimal functionality needed for unit tests
 */
class MockElunaMgr
{
private:
    std::unordered_map<uint64, class Eluna*> elunaInstances;

public:
    MockElunaMgr() = default;
    ~MockElunaMgr() = default;

    /**
     * @brief Create an Eluna instance
     */
    void Create(class Map* map, struct ElunaInfo& info)
    {
        // In test context, we create minimal Eluna instances
        // The actual creation is handled by Eluna constructor
    }

    /**
     * @brief Get an Eluna instance
     */
    class Eluna* Get(const struct ElunaInfo& info)
    {
        // Return nullptr - actual Eluna will be created by constructor
        return nullptr;
    }

    /**
     * @brief Destroy an Eluna instance
     */
    void Destroy(const struct ElunaInfo& info)
    {
        // Cleanup handled by Eluna destructor
    }
};

/**
 * @brief Global mock instances for testing
 */
extern MockElunaConfig* sMockElunaConfig;
extern MockElunaMgr* sMockElunaMgr;

/**
 * @brief Initialize mock server objects for testing
 */
inline void InitializeMockServerObjects()
{
    if (!sMockElunaConfig)
        sMockElunaConfig = new MockElunaConfig();
    
    if (!sMockElunaMgr)
        sMockElunaMgr = new MockElunaMgr();
}

/**
 * @brief Cleanup mock server objects after testing
 */
inline void CleanupMockServerObjects()
{
    if (sMockElunaConfig)
    {
        delete sMockElunaConfig;
        sMockElunaConfig = nullptr;
    }

    if (sMockElunaMgr)
    {
        delete sMockElunaMgr;
        sMockElunaMgr = nullptr;
    }
}

#endif // TRINITY_MOCK_SERVER_OBJECTS_H
