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

-- Load Smallfolk for serialization (used by shared data)
local Smallfolk = require("smallfolk")

-- ============================================================================
-- Configurable Display IDs (can be changed via shared data without recompile)
-- ============================================================================

-- Default display IDs - these can be overridden via SetSharedData
local DEFAULT_DISPLAYS = {
    waypoint_marker = 1824,       -- Elven Wisp (works in 11.2.5)
    waypoint_highlight = 1824,    -- Same as marker but scaled up (highlight via size)
    spawn_marker = 31366,         -- Green targeting circle
}

-- Get a display ID, checking shared data first, then falling back to default
local function GetDisplayId(key)
    local sharedKey = "config_display_" .. key
    local value = GetSharedData(sharedKey)
    if value and value ~= "" then
        local num = tonumber(value)
        if num then
            return num
        end
    end
    return DEFAULT_DISPLAYS[key] or 17188
end

-- Set a display ID in shared data (persists until server restart)
local function SetDisplayIdConfig(key, displayId)
    local sharedKey = "config_display_" .. key
    SetSharedData(sharedKey, tostring(displayId))
    print("[Admin Handlers] Display config updated: " .. key .. " = " .. displayId)
end

-- Initialize display configs from defaults (only if not already set)
local function InitDisplayConfigs()
    for key, defaultValue in pairs(DEFAULT_DISPLAYS) do
        local sharedKey = "config_display_" .. key
        if not HasSharedData(sharedKey) then
            SetSharedData(sharedKey, tostring(defaultValue))
        end
    end
    print("[Admin Handlers] Display configs initialized")
end

-- Initialize on load
InitDisplayConfigs()

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
    
    -- Get configurable display ID for waypoint markers
    local displayId = GetDisplayId("waypoint_marker")
    print("[Admin Handlers] SHOW_WAYPOINTS: Using displayId " .. tostring(displayId))
    
    -- Visualize the path (spawns marker creatures at each waypoint)
    -- Pass player to inherit their phase, and displayId for marker appearance
    local success, err = pcall(function() return creature:VisualizeWaypointPath(player, displayId) end)
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
-- Spawn Point Marker Operations
-- ============================================================================

--[[
    SHOW_SPAWN_MARKER - Show a marker at the selected creature's spawn point
    Request: { displayId = optional number }
    Response: { success = bool, x, y, z, o }
]]
AMS.RegisterHandler("SHOW_SPAWN_MARKER", function(player, data)
    print("[Admin Handlers] SHOW_SPAWN_MARKER request received")
    
    local creature = player:GetSelection()
    if not creature then
        AMS.Send(player, "SPAWN_MARKER_RESPONSE", { success = false, error = "No target selected" })
        return
    end
    
    creature = creature:ToCreature()
    if not creature then
        AMS.Send(player, "SPAWN_MARKER_RESPONSE", { success = false, error = "Target is not a creature" })
        return
    end
    
    -- Get home position for response
    local x, y, z, o = creature:GetHomePosition()
    
    -- Show spawn marker (pass player for phase, optional displayId)
    local displayId = data and data.displayId or nil
    local success, marker = pcall(function() 
        if displayId then
            return creature:ShowSpawnPointMarker(player, displayId)
        else
            return creature:ShowSpawnPointMarker(player)
        end
    end)
    
    if success and marker then
        print("[Admin Handlers] SHOW_SPAWN_MARKER: Marker created at " .. x .. ", " .. y .. ", " .. z)
        AMS.Send(player, "SPAWN_MARKER_RESPONSE", { 
            success = true, 
            x = x, y = y, z = z, o = o,
            message = "Spawn marker shown"
        })
    else
        print("[Admin Handlers] SHOW_SPAWN_MARKER: Failed to create marker")
        AMS.Send(player, "SPAWN_MARKER_RESPONSE", { 
            success = false, 
            error = "Failed to show marker (rebuild server required)"
        })
    end
end)

--[[
    HIDE_SPAWN_MARKER - Hide the spawn point marker for the selected creature
    Request: {}
    Response: { success = bool }
]]
AMS.RegisterHandler("HIDE_SPAWN_MARKER", function(player, data)
    print("[Admin Handlers] HIDE_SPAWN_MARKER request received")
    
    local creature = player:GetSelection()
    if not creature then
        AMS.Send(player, "SPAWN_MARKER_RESPONSE", { success = false, error = "No target selected" })
        return
    end
    
    creature = creature:ToCreature()
    if not creature then
        AMS.Send(player, "SPAWN_MARKER_RESPONSE", { success = false, error = "Target is not a creature" })
        return
    end
    
    local success = pcall(function() return creature:HideSpawnPointMarker() end)
    
    if success then
        print("[Admin Handlers] HIDE_SPAWN_MARKER: Marker hidden")
        AMS.Send(player, "SPAWN_MARKER_RESPONSE", { success = true, message = "Spawn marker hidden" })
    else
        print("[Admin Handlers] HIDE_SPAWN_MARKER: Failed to hide marker")
        AMS.Send(player, "SPAWN_MARKER_RESPONSE", { success = false, error = "Failed to hide marker" })
    end
end)

-- Local cache for wander radius markers (for same-session cleanup)
-- Also stored in ElunaSharedData for visibility, but Lua object refs only work same-session
local wanderRadiusMarkers = {}

--[[
    SHOW_WANDER_RADIUS - Show visual markers in a circle at the creature's wander radius
    Request: { segments = optional number (default 12) }
    Response: { success = bool, radius = number, x, y, z }
]]
AMS.RegisterHandler("SHOW_WANDER_RADIUS", function(player, data)
    print("[Admin Handlers] SHOW_WANDER_RADIUS request received")
    
    local creature = player:GetSelection()
    if not creature then
        AMS.Send(player, "WANDER_RADIUS_RESPONSE", { success = false, error = "No target selected" })
        return
    end
    
    creature = creature:ToCreature()
    if not creature then
        AMS.Send(player, "WANDER_RADIUS_RESPONSE", { success = false, error = "Target is not a creature" })
        return
    end
    
    local spawnId = creature:GetDBTableGUIDLow()
    local spawnIdStr = tostring(spawnId)  -- Convert to string for table key
    local wanderRadius = creature:GetWanderRadius()
    
    if wanderRadius <= 0 then
        AMS.Send(player, "WANDER_RADIUS_RESPONSE", { success = false, error = "Creature has no wander radius (0)" })
        return
    end
    
    -- Get spawn position
    local homeX, homeY, homeZ, homeO = creature:GetHomePosition()
    local markerKey = "wander_markers_" .. spawnIdStr
    
    -- Clear any existing markers for this creature (from local cache)
    if wanderRadiusMarkers[spawnIdStr] then
        for _, marker in ipairs(wanderRadiusMarkers[spawnIdStr]) do
            if marker and marker:IsInWorld() then
                marker:DespawnOrUnsummon()
            end
        end
        wanderRadiusMarkers[spawnIdStr] = nil
    end
    ClearSharedData(markerKey)  -- Also clear shared data
    
    -- Number of markers around the circle (default 12 = every 30 degrees)
    local segments = (data and data.segments) or 12
    if segments < 4 then segments = 4 end
    if segments > 36 then segments = 36 end
    
    local markers = {}
    local markerGuids = {}
    local angleStep = (2 * math.pi) / segments
    
    -- Spawn markers around the circle perimeter
    for i = 0, segments - 1 do
        local angle = i * angleStep
        local markerX = homeX + wanderRadius * math.cos(angle)
        local markerY = homeY + wanderRadius * math.sin(angle)
        local markerZ = homeZ  -- Keep at spawn height
        
        -- Spawn a visual marker at this position
        -- Using entry 1 (VISUAL_WAYPOINT) with a distinct display
        local marker = creature:SpawnCreature(1, markerX, markerY, markerZ, 0, 8)  -- 8 = TEMPSUMMON_MANUAL_DESPAWN
        if marker then
            marker:SetDisplayId(31366)  -- Green circle like spawn marker
            marker:SetScale(0.3)  -- Smaller for radius markers
            table.insert(markers, marker)
            table.insert(markerGuids, tostring(marker:GetGUIDLow()))
        end
    end
    
    -- Also spawn a marker at the center (spawn point)
    local centerMarker = creature:SpawnCreature(1, homeX, homeY, homeZ, homeO, 8)
    if centerMarker then
        centerMarker:SetDisplayId(31366)  -- Green circle
        centerMarker:SetScale(1.0)  -- Larger for center
        table.insert(markers, centerMarker)
        table.insert(markerGuids, tostring(centerMarker:GetGUIDLow()))
    end
    
    -- Store in local cache for same-session cleanup (use string key)
    wanderRadiusMarkers[spawnIdStr] = markers
    
    -- Also store GUIDs in shared data (for debugging/tracking)
    SetSharedData(markerKey, Smallfolk.dumps(markerGuids))
    
    print("[Admin Handlers] SHOW_WANDER_RADIUS: Created " .. #markers .. " markers for radius " .. wanderRadius)
    AMS.Send(player, "WANDER_RADIUS_RESPONSE", { 
        success = true, 
        radius = wanderRadius,
        segments = segments,
        markerCount = #markers,
        x = homeX, y = homeY, z = homeZ,
        message = "Wander radius shown (" .. wanderRadius .. " yards)"
    })
end)

--[[
    HIDE_WANDER_RADIUS - Hide the wander radius markers for the selected creature
    Request: {}
    Response: { success = bool }
]]
AMS.RegisterHandler("HIDE_WANDER_RADIUS", function(player, data)
    print("[Admin Handlers] HIDE_WANDER_RADIUS request received")
    
    local creature = player:GetSelection()
    if not creature then
        AMS.Send(player, "WANDER_RADIUS_RESPONSE", { success = false, error = "No target selected" })
        return
    end
    
    creature = creature:ToCreature()
    if not creature then
        AMS.Send(player, "WANDER_RADIUS_RESPONSE", { success = false, error = "Target is not a creature" })
        return
    end
    
    local spawnId = creature:GetDBTableGUIDLow()
    local spawnIdStr = tostring(spawnId)
    local markerKey = "wander_markers_" .. spawnIdStr
    
    if wanderRadiusMarkers[spawnIdStr] then
        local count = #wanderRadiusMarkers[spawnIdStr]
        for _, marker in ipairs(wanderRadiusMarkers[spawnIdStr]) do
            if marker and marker:IsInWorld() then
                marker:DespawnOrUnsummon()
            end
        end
        wanderRadiusMarkers[spawnIdStr] = nil
        ClearSharedData(markerKey)
        print("[Admin Handlers] HIDE_WANDER_RADIUS: Removed " .. count .. " markers")
        AMS.Send(player, "WANDER_RADIUS_RESPONSE", { success = true, message = "Wander radius hidden" })
    else
        -- No local cache - markers may have been lost on reload
        -- They'll despawn naturally, just clear any tracking data
        ClearSharedData(markerKey)
        AMS.Send(player, "WANDER_RADIUS_RESPONSE", { success = true, message = "Wander radius markers cleared (may already be despawned)" })
    end
end)

--[[
    CLEAR_WANDER_MARKERS - Clear both spawn marker and wander radius markers
    Request: {}
    Response: { success = bool }
]]
AMS.RegisterHandler("CLEAR_WANDER_MARKERS", function(player, data)
    print("[Admin Handlers] CLEAR_WANDER_MARKERS request received")
    
    local creature = player:GetSelection()
    if not creature then
        AMS.Send(player, "CLEAR_WANDER_MARKERS_RESPONSE", { success = false, error = "No target selected" })
        return
    end
    
    creature = creature:ToCreature()
    if not creature then
        AMS.Send(player, "CLEAR_WANDER_MARKERS_RESPONSE", { success = false, error = "Target is not a creature" })
        return
    end
    
    local spawnId = creature:GetDBTableGUIDLow()
    local spawnIdStr = tostring(spawnId)
    local cleared = 0
    local messages = {}
    
    -- Clear spawn marker (via C++ method if available)
    local spawnSuccess, spawnResult = pcall(function() return creature:HideSpawnPointMarker() end)
    if spawnSuccess and spawnResult then 
        cleared = cleared + 1 
        table.insert(messages, "spawn marker")
    end
    
    -- Clear wander radius markers (from local cache, use string key)
    if wanderRadiusMarkers[spawnIdStr] then
        local count = #wanderRadiusMarkers[spawnIdStr]
        for _, marker in ipairs(wanderRadiusMarkers[spawnIdStr]) do
            if marker and marker:IsInWorld() then
                marker:DespawnOrUnsummon()
                cleared = cleared + 1
            end
        end
        wanderRadiusMarkers[spawnIdStr] = nil
        table.insert(messages, count .. " radius markers")
    end
    
    -- Clear shared data
    ClearSharedData("wander_markers_" .. spawnIdStr)
    
    local msg = cleared > 0 and ("Cleared: " .. table.concat(messages, ", ")) or "No active markers in cache (may have been lost on reload)"
    print("[Admin Handlers] CLEAR_WANDER_MARKERS: " .. msg .. " for creature " .. tostring(spawnId))
    AMS.Send(player, "CLEAR_WANDER_MARKERS_RESPONSE", { success = true, cleared = cleared, message = msg })
end)

--[[
    CLEAR_NEARBY_MARKERS - Emergency clear of all VISUAL_WAYPOINT creatures near player
    This clears orphaned markers from before tracking was implemented
    Request: { range = optional number (default 100) }
    Response: { success = bool, cleared = number }
]]
AMS.RegisterHandler("CLEAR_NEARBY_MARKERS", function(player, data)
    print("[Admin Handlers] CLEAR_NEARBY_MARKERS request received")
    
    local range = (data and data.range) or 100
    local cleared = 0
    
    -- Get all creatures near player with entry 1 (VISUAL_WAYPOINT)
    local creatures = player:GetCreaturesInRange(range, 1)  -- entry 1 = VISUAL_WAYPOINT
    if creatures then
        for _, creature in ipairs(creatures) do
            if creature and creature:IsInWorld() then
                creature:DespawnOrUnsummon()
                cleared = cleared + 1
            end
        end
    end
    
    print("[Admin Handlers] CLEAR_NEARBY_MARKERS: Cleared " .. cleared .. " markers within " .. range .. " yards")
    AMS.Send(player, "CLEAR_NEARBY_MARKERS_RESPONSE", { success = true, cleared = cleared, message = "Cleared " .. cleared .. " orphaned markers" })
end)

-- Get detailed waypoint data for a path
AMS.RegisterHandler("GET_WAYPOINT_DETAILS", function(player, data)
    if not data or not data.pathId then
        print("[Admin Handlers] GET_WAYPOINT_DETAILS: No pathId provided")
        AMS.Send(player, "WAYPOINT_DETAILS_RESPONSE", { success = false, error = "No pathId" })
        return
    end
    
    local pathId = data.pathId
    print("[Admin Handlers] GET_WAYPOINT_DETAILS: Fetching details for path " .. pathId)
    
    -- Get the waypoint path using the C++ binding
    local path = GetWaypointPath(pathId)
    if not path or not path.nodes then
        print("[Admin Handlers] GET_WAYPOINT_DETAILS: Path not found")
        AMS.Send(player, "WAYPOINT_DETAILS_RESPONSE", { success = false, error = "Path not found" })
        return
    end
    
    print("[Admin Handlers] GET_WAYPOINT_DETAILS: Sending " .. #path.nodes .. " nodes")
    AMS.Send(player, "WAYPOINT_DETAILS_RESPONSE", { 
        success = true, 
        pathId = pathId,
        nodes = path.nodes
    })
end)

-- Highlight a specific waypoint in the world
AMS.RegisterHandler("SELECT_WAYPOINT", function(player, data)
    if not data or not data.pathId or not data.nodeId then
        print("[Admin Handlers] SELECT_WAYPOINT: Missing pathId or nodeId")
        return
    end
    
    local pathId = data.pathId
    local nodeId = data.nodeId
    print("[Admin Handlers] SELECT_WAYPOINT: Highlighting path " .. pathId .. " node " .. nodeId)
    
    -- Get the waypoint path
    local path = GetWaypointPath(pathId)
    if not path or not path.nodes then
        print("[Admin Handlers] SELECT_WAYPOINT: Path not found")
        return
    end
    
    -- Find the node
    local node = nil
    for _, n in ipairs(path.nodes) do
        if n.id == nodeId then
            node = n
            break
        end
    end
    
    if not node then
        print("[Admin Handlers] SELECT_WAYPOINT: Node not found")
        return
    end
    
    -- Notify player of selection
    print("[Admin Handlers] SELECT_WAYPOINT: Node " .. nodeId .. " at " .. node.x .. ", " .. node.y .. ", " .. node.z)
    
    -- Clear previous highlight if any (stored in shared data per player)
    local playerKey = "waypoint_highlight_" .. tostring(player:GetGUIDLow())
    local prevData = GetSharedData(playerKey)
    if prevData then
        local prev = Smallfolk.loads(prevData)
        if prev and prev.pathId and prev.nodeId then
            ClearWaypointMarkerAuras(player, prev.pathId, prev.nodeId)
        end
    end
    
    -- Highlight the new waypoint marker by changing its display
    -- Display ID is configurable via shared data (key: config_display_waypoint_highlight)
    local highlightDisplayId = GetDisplayId("waypoint_highlight")
    local success = HighlightWaypointMarker(player, pathId, nodeId, highlightDisplayId)
    
    if success then
        print("[Admin Handlers] SELECT_WAYPOINT: Highlighted marker with displayId " .. highlightDisplayId)
        -- Store current selection for later cleanup
        SetSharedData(playerKey, Smallfolk.dumps({ pathId = pathId, nodeId = nodeId }))
    else
        print("[Admin Handlers] SELECT_WAYPOINT: Could not find marker to highlight")
    end
    
    player:SendBroadcastMessage("Waypoint " .. nodeId .. " selected")
    
    -- Send response with node position
    AMS.Send(player, "WAYPOINT_SELECTED_RESPONSE", {
        success = true,
        pathId = pathId,
        nodeId = nodeId,
        x = node.x,
        y = node.y,
        z = node.z
    })
end)

-- Get waypoint node info from a visual waypoint GUID (when targeting a waypoint marker)
AMS.RegisterHandler("GET_WAYPOINT_FOR_GUID", function(player, data)
    if not data or not data.guid then
        print("[Admin Handlers] GET_WAYPOINT_FOR_GUID: No GUID provided")
        return
    end
    
    local pathId, nodeId = GetWaypointNodeForVisualGUID(data.guid)
    if pathId == 0 or nodeId == 0 then
        print("[Admin Handlers] GET_WAYPOINT_FOR_GUID: GUID not found in waypoint system")
        AMS.Send(player, "WAYPOINT_FOR_GUID_RESPONSE", { success = false })
        return
    end
    
    print("[Admin Handlers] GET_WAYPOINT_FOR_GUID: Found path " .. pathId .. " node " .. nodeId)
    AMS.Send(player, "WAYPOINT_FOR_GUID_RESPONSE", {
        success = true,
        pathId = pathId,
        nodeId = nodeId
    })
end)

-- Get player data (GM state, etc.)
AMS.RegisterHandler("GET_PLAYER_DATA", function(player, data)
    if not player then
        return
    end
    
    -- Send player data including GM state (IsGM is the correct Eluna method)
    AMS.Send(player, "PLAYER_DATA_RESPONSE", {
        success = true,
        isGM = player:IsGM()
    })
end)

-- Set display config (for MCP to change waypoint marker appearance)
-- Keys: waypoint_marker, waypoint_highlight, spawn_marker
AMS.RegisterHandler("SET_DISPLAY_CONFIG", function(player, data)
    if not data or not data.key or not data.displayId then
        print("[Admin Handlers] SET_DISPLAY_CONFIG: Missing key or displayId")
        return
    end
    
    local key = data.key
    local displayId = tonumber(data.displayId)
    
    if not displayId then
        print("[Admin Handlers] SET_DISPLAY_CONFIG: Invalid displayId")
        return
    end
    
    SetDisplayIdConfig(key, displayId)
    
    print("[Admin Handlers] SET_DISPLAY_CONFIG: Set " .. key .. " = " .. displayId)
    AMS.Send(player, "DISPLAY_CONFIG_RESPONSE", {
        success = true,
        key = key,
        displayId = displayId,
        message = "Display config updated. Re-show waypoints to see changes."
    })
end)

-- Get current display configs
AMS.RegisterHandler("GET_DISPLAY_CONFIGS", function(player, data)
    local configs = {}
    for key, _ in pairs(DEFAULT_DISPLAYS) do
        configs[key] = GetDisplayId(key)
    end
    
    AMS.Send(player, "DISPLAY_CONFIGS_RESPONSE", {
        success = true,
        configs = configs
    })
end)

-- Teleport player to a waypoint location
AMS.RegisterHandler("TELEPORT_TO_WAYPOINT", function(player, data)
    if not data or not data.x or not data.y or not data.z then
        print("[Admin Handlers] TELEPORT_TO_WAYPOINT: Missing coordinates")
        return
    end
    
    local x = data.x
    local y = data.y
    local z = data.z
    local orientation = data.orientation or 0
    
    print("[Admin Handlers] TELEPORT_TO_WAYPOINT: Teleporting to " .. x .. ", " .. y .. ", " .. z .. " facing " .. orientation)
    
    -- Teleport player to the waypoint location with correct orientation
    player:Teleport(player:GetMapId(), x, y, z, orientation)
    
    player:SendBroadcastMessage("Teleported to waypoint")
end)

-- ============================================================================
-- Creature Edit Operations (Araxia Write Operations)
-- ============================================================================

--[[
    SET_WANDER_DISTANCE - Set the wander distance for the selected creature
    Request: { distance = 10.0 }
    Response: { success = bool, message = string, newDistance = number }
    Note: Creature must be selected (same as GET_NPC_DATA)
]]
AMS.RegisterHandler("SET_WANDER_DISTANCE", function(player, data)
    if not data or data.distance == nil then
        print("[Admin Handlers] SET_WANDER_DISTANCE: Missing distance")
        AMS.Send(player, "SET_WANDER_DISTANCE_RESPONSE", {
            success = false,
            message = "Missing distance parameter"
        })
        return
    end
    
    print("[Admin Handlers] SET_WANDER_DISTANCE: Setting distance to " .. data.distance)
    
    -- Use player's selection (same pattern as GET_NPC_DATA)
    local creature = player:GetSelection()
    if not creature then
        AMS.Send(player, "SET_WANDER_DISTANCE_RESPONSE", {
            success = false,
            message = "No creature selected"
        })
        return
    end
    
    creature = creature:ToCreature()
    if not creature then
        AMS.Send(player, "SET_WANDER_DISTANCE_RESPONSE", {
            success = false,
            message = "Selected object is not a creature"
        })
        return
    end
    
    local spawnId = creature:GetDBTableGUIDLow()
    local spawnIdStr = tostring(spawnId)  -- Convert to string for table key
    local homeX, homeY, homeZ, homeO = creature:GetHomePosition()
    
    -- Check if markers were visible and hide them
    local hadSpawnMarker = false
    local hadRadiusMarkers = false
    
    -- Try to hide spawn marker (via C++) - check if it returns true
    local spawnHideSuccess, spawnHideResult = pcall(function() return creature:HideSpawnPointMarker() end)
    if spawnHideSuccess and spawnHideResult then
        hadSpawnMarker = true
        print("[Admin Handlers] SET_WANDER_DISTANCE: Hid spawn marker")
    end
    
    -- Hide wander radius markers if they exist (use string key)
    if wanderRadiusMarkers[spawnIdStr] then
        hadRadiusMarkers = true
        for _, marker in ipairs(wanderRadiusMarkers[spawnIdStr]) do
            if marker and marker:IsInWorld() then
                marker:DespawnOrUnsummon()
            end
        end
        wanderRadiusMarkers[spawnIdStr] = nil
        ClearSharedData("wander_markers_" .. spawnIdStr)
        print("[Admin Handlers] SET_WANDER_DISTANCE: Hid radius markers")
    end
    
    -- Use the Araxia writer method to save to database
    local success, message = creature:SaveWanderDistance(data.distance, player)
    
    print("[Admin Handlers] SET_WANDER_DISTANCE: Result - " .. tostring(success) .. ", " .. message)
    
    -- Despawn and respawn the creature so the new settings take effect immediately
    if success then
        -- Set a very short respawn delay (1 second), then despawn
        creature:SetRespawnDelay(1)
        creature:Respawn()  -- Mark for respawn
        creature:DespawnOrUnsummon(0)  -- Remove from world
        print("[Admin Handlers] SET_WANDER_DISTANCE: Creature despawned, respawning in 1 second")
        
        -- Tell client markers need to be re-shown (client will handle the delay)
        if hadSpawnMarker or hadRadiusMarkers then
            print("[Admin Handlers] SET_WANDER_DISTANCE: Markers will need to be re-shown after respawn")
        end
    end
    
    AMS.Send(player, "SET_WANDER_DISTANCE_RESPONSE", {
        success = success,
        message = message,
        newDistance = data.distance,
        markersCleared = hadSpawnMarker or hadRadiusMarkers,
        hadSpawnMarker = hadSpawnMarker,
        hadRadiusMarkers = hadRadiusMarkers
    })
end)

--[[
    SET_MOVEMENT_TYPE - Set the movement type for the selected creature
    Request: { movementType = 0|1|2 }
    Response: { success = bool, message = string, newMovementType = number }
    Note: Creature must be selected
]]
AMS.RegisterHandler("SET_MOVEMENT_TYPE", function(player, data)
    if not data or data.movementType == nil then
        print("[Admin Handlers] SET_MOVEMENT_TYPE: Missing movementType")
        AMS.Send(player, "SET_MOVEMENT_TYPE_RESPONSE", {
            success = false,
            message = "Missing movementType parameter"
        })
        return
    end
    
    print("[Admin Handlers] SET_MOVEMENT_TYPE: Setting type to " .. data.movementType)
    
    local creature = player:GetSelection()
    if not creature then
        AMS.Send(player, "SET_MOVEMENT_TYPE_RESPONSE", {
            success = false,
            message = "No creature selected"
        })
        return
    end
    
    creature = creature:ToCreature()
    if not creature then
        AMS.Send(player, "SET_MOVEMENT_TYPE_RESPONSE", {
            success = false,
            message = "Selected object is not a creature"
        })
        return
    end
    
    local success, message = creature:SaveMovementType(data.movementType, player)
    
    print("[Admin Handlers] SET_MOVEMENT_TYPE: Result - " .. tostring(success) .. ", " .. message)
    
    AMS.Send(player, "SET_MOVEMENT_TYPE_RESPONSE", {
        success = success,
        message = message,
        newMovementType = data.movementType
    })
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
print("  - SHOW_SPAWN_MARKER")
print("  - HIDE_SPAWN_MARKER")
print("  - SHOW_WANDER_RADIUS")
print("  - HIDE_WANDER_RADIUS")
print("  - CLEAR_WANDER_MARKERS")
print("  - CLEAR_NEARBY_MARKERS")
print("  - GET_WAYPOINT_DETAILS")
print("  - SELECT_WAYPOINT")
print("  - GET_WAYPOINT_FOR_GUID")
print("  - GET_PLAYER_DATA")
print("  - TELEPORT_TO_WAYPOINT")
print("  - SET_WANDER_DISTANCE")
print("  - SET_MOVEMENT_TYPE")
