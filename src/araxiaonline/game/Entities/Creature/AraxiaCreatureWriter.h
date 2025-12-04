/*
 * AraxiaCreatureWriter.h
 * 
 * Araxia Online - Custom Write Operations for Creatures
 * 
 * This class wraps all database write operations for creatures.
 * All modifications to creature data should go through this class to enable:
 * - Centralized logging of changes
 * - Easy export of modifications
 * - Audit trail for content changes
 * - Future rollback capabilities
 */

#ifndef ARAXIA_CREATURE_WRITER_H
#define ARAXIA_CREATURE_WRITER_H

#include "Define.h"
#include "ObjectGuid.h"
#include <string>

class Creature;
class Player;

namespace Araxia
{

// Result structure for write operations
struct WriteResult
{
    bool success;
    std::string message;
    uint64 changeId;  // Future: unique ID for tracking changes
};

class CreatureWriter
{
public:
    static CreatureWriter& Instance();
    
    // Prevent copying
    CreatureWriter(CreatureWriter const&) = delete;
    CreatureWriter& operator=(CreatureWriter const&) = delete;
    
    // ========================================================================
    // Movement Settings
    // ========================================================================
    
    /**
     * Set the wander distance for a creature spawn.
     * Only applies to creatures with MovementType = 1 (Random).
     * 
     * @param spawnGuid The creature spawn GUID (from creature table)
     * @param distance The wander radius in yards
     * @param changedBy Optional player who made the change (for logging)
     * @return WriteResult with success status and any error message
     */
    WriteResult SetWanderDistance(ObjectGuid::LowType spawnGuid, float distance, Player* changedBy = nullptr);
    
    /**
     * Get the current wander distance for a creature spawn.
     * 
     * @param spawnGuid The creature spawn GUID
     * @return The wander distance, or 0 if not found
     */
    float GetWanderDistance(ObjectGuid::LowType spawnGuid);
    
    /**
     * Set the movement type for a creature spawn.
     * 
     * @param spawnGuid The creature spawn GUID
     * @param movementType 0=Idle, 1=Random, 2=Waypoint
     * @param changedBy Optional player who made the change
     * @return WriteResult with success status
     */
    WriteResult SetMovementType(ObjectGuid::LowType spawnGuid, uint8 movementType, Player* changedBy = nullptr);
    
    /**
     * Get the movement type for a creature spawn.
     * 
     * @param spawnGuid The creature spawn GUID
     * @return Movement type (0=Idle, 1=Random, 2=Waypoint), or 0 if not found
     */
    uint8 GetMovementType(ObjectGuid::LowType spawnGuid);

private:
    CreatureWriter();
    ~CreatureWriter() = default;
    
    // Log a change for future export capabilities
    void LogChange(std::string const& table, std::string const& field, 
                   std::string const& oldValue, std::string const& newValue,
                   ObjectGuid::LowType affectedGuid, Player* changedBy);
};

#define sAraxiaCreatureWriter Araxia::CreatureWriter::Instance()

} // namespace Araxia

#endif // ARAXIA_CREATURE_WRITER_H
