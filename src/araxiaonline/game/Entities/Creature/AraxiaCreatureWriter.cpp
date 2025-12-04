/*
 * AraxiaCreatureWriter.cpp
 * 
 * Araxia Online - Custom Write Operations for Creatures
 */

#include "AraxiaCreatureWriter.h"
#include "Creature.h"
#include "Player.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Map.h"

namespace Araxia
{

CreatureWriter& CreatureWriter::Instance()
{
    static CreatureWriter instance;
    return instance;
}

CreatureWriter::CreatureWriter()
{
    TC_LOG_INFO("server.loading", "[Araxia] CreatureWriter initialized");
}

void CreatureWriter::LogChange(std::string const& table, std::string const& field,
                                std::string const& oldValue, std::string const& newValue,
                                ObjectGuid::LowType affectedGuid, Player* changedBy)
{
    // TODO: Store changes in araxia_change_log table for export
    // For now, just log to the server log
    std::string changerName = changedBy ? changedBy->GetName() : "SYSTEM";
    
    TC_LOG_INFO("araxia.changes", "[Araxia Change] Table: %s, Field: %s, GUID: " UI64FMTD ", "
                "Old: %s, New: %s, ChangedBy: %s",
                table.c_str(), field.c_str(), affectedGuid,
                oldValue.c_str(), newValue.c_str(), changerName.c_str());
}

WriteResult CreatureWriter::SetWanderDistance(ObjectGuid::LowType spawnGuid, float distance, Player* changedBy)
{
    WriteResult result;
    result.success = false;
    result.changeId = 0;
    
    // Validate distance
    if (distance < 0.0f)
    {
        result.message = "Wander distance cannot be negative";
        return result;
    }
    
    if (distance > 100.0f)
    {
        result.message = "Wander distance too large (max 100 yards)";
        return result;
    }
    
    // Get current value for logging
    float oldDistance = GetWanderDistance(spawnGuid);
    
    // Update database
    WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_UPD_CREATURE_WANDER_DISTANCE);
    stmt->setFloat(0, distance);
    stmt->setUInt64(1, spawnGuid);
    WorldDatabase.Execute(stmt);
    
    // Log the change
    LogChange("creature", "wander_distance", 
              std::to_string(oldDistance), std::to_string(distance),
              spawnGuid, changedBy);
    
    // CRITICAL: Also update the ObjectMgr cache so respawned creatures get the new value
    CreatureData const* data = sObjectMgr->GetCreatureData(spawnGuid);
    if (data)
    {
        // Cast away const to update the cached value
        // This is safe because we own the data and are updating it consistently with DB
        CreatureData* mutableData = const_cast<CreatureData*>(data);
        mutableData->wander_distance = distance;
        TC_LOG_DEBUG("araxia.changes", "[Araxia] Updated ObjectMgr cache for spawn " UI64FMTD " wander_distance to %.2f", spawnGuid, distance);
    }
    
    result.success = true;
    result.message = "Wander distance updated successfully";
    
    TC_LOG_INFO("araxia.changes", "[Araxia] SetWanderDistance: GUID " UI64FMTD " set to %.2f yards by %s",
                spawnGuid, distance, changedBy ? changedBy->GetName().c_str() : "SYSTEM");
    
    return result;
}

float CreatureWriter::GetWanderDistance(ObjectGuid::LowType spawnGuid)
{
    CreatureData const* data = sObjectMgr->GetCreatureData(spawnGuid);
    if (data)
        return data->wander_distance;
    
    return 0.0f;
}

WriteResult CreatureWriter::SetMovementType(ObjectGuid::LowType spawnGuid, uint8 movementType, Player* changedBy)
{
    WriteResult result;
    result.success = false;
    result.changeId = 0;
    
    // Validate movement type
    if (movementType > 2)
    {
        result.message = "Invalid movement type (must be 0=Idle, 1=Random, 2=Waypoint)";
        return result;
    }
    
    // Get current value for logging
    uint8 oldType = GetMovementType(spawnGuid);
    
    // Update database
    WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_UPD_CREATURE_MOVEMENT_TYPE);
    stmt->setUInt8(0, movementType);
    stmt->setUInt64(1, spawnGuid);
    WorldDatabase.Execute(stmt);
    
    // Log the change
    LogChange("creature", "MovementType",
              std::to_string(oldType), std::to_string(movementType),
              spawnGuid, changedBy);
    
    // CRITICAL: Also update the ObjectMgr cache so respawned creatures get the new value
    CreatureData const* data = sObjectMgr->GetCreatureData(spawnGuid);
    if (data)
    {
        CreatureData* mutableData = const_cast<CreatureData*>(data);
        mutableData->movementType = movementType;
        TC_LOG_DEBUG("araxia.changes", "[Araxia] Updated ObjectMgr cache for spawn " UI64FMTD " movementType to %u", spawnGuid, movementType);
    }
    
    result.success = true;
    result.message = "Movement type updated successfully";
    
    TC_LOG_INFO("araxia.changes", "[Araxia] SetMovementType: GUID " UI64FMTD " set to %u by %s",
                spawnGuid, movementType, changedBy ? changedBy->GetName().c_str() : "SYSTEM");
    
    return result;
}

uint8 CreatureWriter::GetMovementType(ObjectGuid::LowType spawnGuid)
{
    CreatureData const* data = sObjectMgr->GetCreatureData(spawnGuid);
    if (data)
        return data->movementType;
    
    return 0;
}

} // namespace Araxia
