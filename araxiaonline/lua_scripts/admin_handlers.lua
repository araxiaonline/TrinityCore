--[[
    AraxiaTrinityAdmin Server Handlers
    
    Provides server-side data access for the AraxiaTrinityAdmin addon.
    Uses AMS (Araxia Messaging System) for client-server communication.
]]

print("[Admin Handlers] Loading...")

-- Check if AMS is available
if not AMS then
    print("[Admin Handlers] ERROR: AMS not loaded! Make sure AMS_Server.lua is loaded first.")
    return
end

-- ============================================================================
-- Helper Functions
-- ============================================================================

-- Convert copper to formatted gold string
local function FormatGold(copper)
    if not copper or copper == 0 then
        return "0 copper"
    end
    
    local gold = math.floor(copper / 10000)
    local silver = math.floor((copper % 10000) / 100)
    local copperLeft = copper % 100
    
    local parts = {}
    if gold > 0 then table.insert(parts, gold .. "g") end
    if silver > 0 then table.insert(parts, silver .. "s") end
    if copperLeft > 0 then table.insert(parts, copperLeft .. "c") end
    
    return table.concat(parts, " ")
end

-- Get spell school name
local function GetSpellSchoolName(school)
    local schools = {
        [0] = "Physical",
        [1] = "Holy",
        [2] = "Fire",
        [3] = "Nature",
        [4] = "Frost",
        [5] = "Shadow",
        [6] = "Arcane"
    }
    return schools[school] or "Unknown"
end

-- Get stat name
local function GetStatName(stat)
    local stats = {
        [0] = "Strength",
        [1] = "Agility",
        [2] = "Stamina",
        [3] = "Intellect",
        [4] = "Spirit"
    }
    return stats[stat] or "Unknown"
end

-- Get rank name
local function GetRankName(rank)
    local ranks = {
        [0] = "Normal",
        [1] = "Elite",
        [2] = "Rare Elite",
        [3] = "Boss",
        [4] = "Rare"
    }
    return ranks[rank] or "Unknown"
end

-- ============================================================================
-- NPC Data Handler
-- ============================================================================

AMS.RegisterHandler("GET_NPC_DATA", function(player, data)
    local npcGUID = data.npcGUID
    
    if not npcGUID then
        print("[Admin Handlers] GET_NPC_DATA: No GUID provided")
        AMS.Send(player, "NPC_DATA_RESPONSE", {
            success = false,
            error = "No NPC GUID provided"
        })
        return
    end
    
    print("[Admin Handlers] GET_NPC_DATA: Fetching data for GUID:", npcGUID)
    
    local creature = player:GetSelection()
    
    if not creature then
        print("[Admin Handlers] GET_NPC_DATA: No creature selected")
        AMS.Send(player, "NPC_DATA_RESPONSE", {
            success = false,
            error = "No creature selected",
            guid = npcGUID
        })
        return
    end
    
    -- Verify it's a creature (not a player or gameobject)
    creature = creature:ToCreature()
    if not creature then
        print("[Admin Handlers] GET_NPC_DATA: Selected object is not a creature")
        AMS.Send(player, "NPC_DATA_RESPONSE", {
            success = false,
            error = "Selected object is not a creature",
            guid = npcGUID
        })
        return
    end
    
    -- Helper to safely call creature methods (converts userdata to string)
    local function SafeGet(fn, default)
        local success, result = pcall(fn)
        if success then
            -- Convert userdata to string to avoid serialization issues
            if type(result) == "userdata" then
                return tostring(result)
            end
            return result
        else
            print("[Admin Handlers] SafeGet failed:", result)
            return default
        end
    end
    
    print("[Admin Handlers] Building response data...")
    
    -- Build response data using available Eluna methods
    local response = {
        success = true,
        guid = npcGUID,
        timestamp = os.time(),
        
        -- Basic Information
        basic = {
            entry = SafeGet(function() return creature:GetEntry() end, 0),
            name = SafeGet(function() return creature:GetName() end, "Unknown"),
            level = SafeGet(function() return creature:GetLevel() end, 0),
            displayId = SafeGet(function() return creature:GetDisplayId() end, 0),
            nativeDisplayId = SafeGet(function() return creature:GetNativeDisplayId() end, 0),
            scale = SafeGet(function() return creature:GetScale() end, 1.0),
            faction = SafeGet(function() return creature:GetFaction() end, 0),
            creatureType = SafeGet(function() return creature:GetCreatureType() end, 0),
            rank = SafeGet(function() return creature:GetRank() end, 0),
            rankName = SafeGet(function() return GetRankName(creature:GetRank()) end, "Normal")
        },
        
        -- Health & Power
        vitals = {
            health = SafeGet(function() return creature:GetHealth() end, 0),
            maxHealth = SafeGet(function() return creature:GetMaxHealth() end, 1),
            healthPercent = SafeGet(function() return (creature:GetHealth() / creature:GetMaxHealth()) * 100 end, 0),
            power = SafeGet(function() return creature:GetPower(0) end, 0),
            maxPower = SafeGet(function() return creature:GetMaxPower(0) end, 0),
            powerType = SafeGet(function() return creature:GetPowerType() end, 0)
        },
        
        -- Base Stats (STR, AGI, STA, INT, SPI)
        stats = {},
        
        -- Movement Speeds
        speeds = {
            walk = SafeGet(function() return creature:GetSpeed(0) end, 0),
            run = SafeGet(function() return creature:GetSpeed(1) end, 0),
            runBack = SafeGet(function() return creature:GetSpeed(2) end, 0),
            swim = SafeGet(function() return creature:GetSpeed(3) end, 0),
            swimBack = SafeGet(function() return creature:GetSpeed(4) end, 0),
            fly = SafeGet(function() return creature:GetSpeed(6) end, 0),
            flyBack = SafeGet(function() return creature:GetSpeed(7) end, 0)
        },
        
        -- Spell Power per school
        spellPower = {},
        
        -- AI & Scripts
        scripts = {
            aiName = SafeGet(function() return creature:GetAIName() end, ""),
            scriptName = SafeGet(function() return creature:GetScriptName() end, ""),
            scriptId = SafeGet(function() return creature:GetScriptId() end, 0)
        },
        
        -- Behavior
        behavior = {
            respawnDelay = SafeGet(function() return creature:GetRespawnDelay() end, 0),
            wanderRadius = SafeGet(function() return creature:GetWanderRadius() end, 0),
            isInCombat = SafeGet(function() return creature:IsInCombat() end, false),
            isRegeneratingHealth = SafeGet(function() return creature:IsRegeneratingHealth() end, false),
            isElite = SafeGet(function() return creature:IsElite() end, false),
            isWorldBoss = SafeGet(function() return creature:IsWorldBoss() end, false),
            isCivilian = SafeGet(function() return creature:IsCivilian() end, false)
        },
        
        -- Movement info (0=Idle, 1=Random, 2=Waypoint)
        movement = {
            defaultType = SafeGet(function() return creature:GetDefaultMovementType() end, 0),
            currentType = SafeGet(function() return creature:GetMovementType() end, 0),
            currentWaypointId = SafeGet(function() return creature:GetCurrentWaypointId() end, 0),
            respawnTime = SafeGet(function() return creature:GetRespawnDelay() end, 0),
            waypointPath = SafeGet(function() return creature:GetWaypointPathData() end, nil)
        }
    }
    
    print("[Admin Handlers] Response data built successfully")
    
    -- Use new safe C++ methods for combat stats (Phase 2)
    print("[Admin Handlers] Getting combat stats via safe C++ methods...")
    
    -- Get armor using safe C++ method
    response.combat = {
        armor = SafeGet(function() return creature:GetArmor() end, 0),
        baseAttackTime = SafeGet(function() return creature:GetBaseAttackTime(0) end, 0),
        offhandAttackTime = SafeGet(function() return creature:GetBaseAttackTime(1) end, 0),
        rangedAttackTime = SafeGet(function() return creature:GetBaseAttackTime(2) end, 0)
    }
    
    -- Get resistances using safe C++ method (0=Physical/Armor, 1=Holy, 2=Fire, 3=Nature, 4=Frost, 5=Shadow, 6=Arcane)
    response.resistances = {}
    for i = 0, 6 do
        response.resistances[GetSpellSchoolName(i)] = SafeGet(function() return creature:GetResistance(i) end, 0)
    end
    
    -- Get creature template data using safe C++ method
    -- Note: This can be large for some creatures (several KB), contributing to payload size
    response.template = SafeGet(function() return creature:GetCreatureTemplateData() end, nil)
    
    -- Stats - using safe C++ GetStat method
    for i = 0, 4 do
        response.stats[GetStatName(i)] = SafeGet(function() return creature:GetStat(i) end, 0)
    end
    
    -- Spell power - still skip as it crashes on creatures
    for i = 0, 6 do
        response.spellPower[GetSpellSchoolName(i)] = 0
    end
    
    -- Add note about available data
    response.notes = {
        "Phase 2: Stats, armor, resistances, attack times, template data available",
        "Spell power disabled - crashes on creatures"
    }
    
    local creatureName = SafeGet(function() return creature:GetName() end, "Unknown")
    print("[Admin Handlers] GET_NPC_DATA: Sending response for", creatureName)
    
    -- Send response back to client
    print("[Admin Handlers] Calling AMS.Send...")
    AMS.Send(player, "NPC_DATA_RESPONSE", response)
    print("[Admin Handlers] AMS.Send completed")
end)

-- ============================================================================
-- Waypoint Visualization Handlers
-- ============================================================================

-- Show waypoint markers in 3D space
AMS.RegisterHandler("SHOW_WAYPOINTS", function(player, data)
    print("[Admin Handlers] SHOW_WAYPOINTS request received")
    
    -- Get creature from player's current selection
    local creature = player:GetSelection()
    if not creature then
        print("[Admin Handlers] SHOW_WAYPOINTS: No target selected")
        AMS.Send(player, "WAYPOINTS_RESPONSE", { success = false, error = "No target selected" })
        return
    end
    
    creature = creature:ToCreature()
    if not creature then
        print("[Admin Handlers] SHOW_WAYPOINTS: Target is not a creature")
        AMS.Send(player, "WAYPOINTS_RESPONSE", { success = false, error = "Target is not a creature" })
        return
    end
    
    -- Check if creature has waypoints
    local pathId = creature:GetWaypointPath()
    if not pathId or pathId == 0 then
        print("[Admin Handlers] SHOW_WAYPOINTS: Creature has no waypoint path")
        AMS.Send(player, "WAYPOINTS_RESPONSE", { success = false, error = "No waypoint path" })
        return
    end
    
    -- Clear any existing visualization first (fixes state after Clear All)
    pcall(function() creature:DevisualizeWaypointPath() end)
    
    -- Visualize the path (spawns marker creatures at each waypoint)
    -- Pass player to inherit their phase (makes markers visible without GM mode)
    local success, err = pcall(function() return creature:VisualizeWaypointPath(player) end)
    if not success then
        print("[Admin Handlers] SHOW_WAYPOINTS: Error calling VisualizeWaypointPath:", err)
    end
    
    if success then
        print("[Admin Handlers] SHOW_WAYPOINTS: Visualization created for path", pathId)
        AMS.Send(player, "WAYPOINTS_RESPONSE", { 
            success = true, 
            pathId = pathId,
            message = "Waypoint markers spawned" 
        })
    else
        print("[Admin Handlers] SHOW_WAYPOINTS: Failed to create visualization")
        AMS.Send(player, "WAYPOINTS_RESPONSE", { success = false, error = "Failed to visualize (C++ method not available - rebuild required)" })
    end
end)

-- Hide waypoint markers
AMS.RegisterHandler("HIDE_WAYPOINTS", function(player, data)
    print("[Admin Handlers] HIDE_WAYPOINTS request received")
    
    -- Get creature from player's current selection
    local creature = player:GetSelection()
    if not creature then
        print("[Admin Handlers] HIDE_WAYPOINTS: No target selected")
        AMS.Send(player, "WAYPOINTS_RESPONSE", { success = false, error = "No target selected" })
        return
    end
    
    creature = creature:ToCreature()
    if not creature then
        print("[Admin Handlers] HIDE_WAYPOINTS: Target is not a creature")
        AMS.Send(player, "WAYPOINTS_RESPONSE", { success = false, error = "Target is not a creature" })
        return
    end
    
    -- Devisualize the path (removes marker creatures)
    local success, err = pcall(function() return creature:DevisualizeWaypointPath() end)
    if not success then
        print("[Admin Handlers] HIDE_WAYPOINTS: Error calling DevisualizeWaypointPath:", err)
    end
    
    if success then
        print("[Admin Handlers] HIDE_WAYPOINTS: Visualization removed")
        AMS.Send(player, "WAYPOINTS_RESPONSE", { 
            success = true, 
            message = "Waypoint markers removed" 
        })
    else
        print("[Admin Handlers] HIDE_WAYPOINTS: Failed to remove visualization")
        AMS.Send(player, "WAYPOINTS_RESPONSE", { success = false, error = "Failed to hide" })
    end
end)

-- Hide waypoints by GUID (for Clear All / Paths tab)
-- Note: Markers are tied to creatures - they'll despawn when creature respawns or server restarts
-- This handler logs the request for debugging
AMS.RegisterHandler("HIDE_WAYPOINTS_BY_GUID", function(player, data)
    if not data or not data.guid then
        print("[Admin Handlers] HIDE_WAYPOINTS_BY_GUID: No GUID provided")
        return
    end
    
    print("[Admin Handlers] HIDE_WAYPOINTS_BY_GUID: Acknowledged clear request for GUID:", data.guid)
    -- Note: If the creature is not the player's current target, we can't easily devisualize
    -- The client tracker is the source of truth; markers will despawn naturally
end)

-- Clear ALL waypoint markers and reset WaypointManager tracking state
AMS.RegisterHandler("CLEAR_ALL_WAYPOINT_MARKERS", function(player, data)
    print("[Admin Handlers] CLEAR_ALL_WAYPOINT_MARKERS: Clearing all waypoint visualizations")
    
    -- Use the C++ method to properly clear tracking state and despawn markers
    player:ClearAllWaypointVisualizations()
    
    print("[Admin Handlers] CLEAR_ALL_WAYPOINT_MARKERS: All visualizations cleared")
    AMS.Send(player, "CLEAR_WAYPOINTS_RESPONSE", { success = true })
end)

-- ============================================================================
-- Initialization
-- ============================================================================

print("[Admin Handlers] Loaded successfully!")
print("[Admin Handlers] Registered handlers:")
print("  - GET_NPC_DATA")
print("  - SHOW_WAYPOINTS")
print("  - HIDE_WAYPOINTS")
print("  - HIDE_WAYPOINTS_BY_GUID")
print("  - CLEAR_ALL_WAYPOINT_MARKERS")
