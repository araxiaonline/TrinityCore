-- AraxiaTrinityAdmin Server Data Module
-- Handles communication with server via AMS to fetch NPC data

local addonName = "AraxiaTrinityAdmin"

-- Module table
local ServerData = {}

-- Cache for server data (keyed by GUID)
ServerData.cache = {}

-- Loading state tracking
ServerData.loading = {}

-- Callbacks waiting for data
ServerData.callbacks = {}

-- ============================================================================
-- Flag Decoders
-- ============================================================================

-- NPC Flags (what the NPC can do)
local NPC_FLAGS = {
    [0x00000001] = "Gossip",
    [0x00000002] = "Quest Giver",
    [0x00000010] = "Trainer",
    [0x00000080] = "Vendor",
    [0x00001000] = "Repair",
    [0x00002000] = "Flight Master",
    [0x00004000] = "Spirit Healer",
    [0x00010000] = "Innkeeper",
    [0x00020000] = "Banker",
    [0x00080000] = "Tabard Designer",
    [0x00100000] = "Battlemaster",
    [0x00200000] = "Auctioneer",
    [0x00400000] = "Stable Master",
    [0x00800000] = "Guild Banker",
    [0x01000000] = "Spellclick",
    [0x04000000] = "Mailbox",
    [0x10000000] = "Transmogrifier",
    [0x20000000] = "Void Storage",
}

-- Unit Flags (combat/interaction behavior)
local UNIT_FLAGS = {
    [0x00000002] = "Non-Attackable",
    [0x00000100] = "Immune to PC",
    [0x00000200] = "Immune to NPC",
    [0x00004000] = "Can't Swim",
    [0x00008000] = "Can Swim",
    [0x00020000] = "Pacified",
    [0x00040000] = "Stunned",
    [0x00080000] = "In Combat",
    [0x02000000] = "Uninteractible",
    [0x04000000] = "Skinnable",
    [0x80000000] = "Immune",
}

-- Creature Extra Flags (special behaviors)
local EXTRA_FLAGS = {
    [0x00000001] = "Instance Bind",
    [0x00000002] = "Civilian",
    [0x00000004] = "No Parry",
    [0x00000010] = "No Block",
    [0x00000020] = "No Crushing",
    [0x00000040] = "No XP",
    [0x00000080] = "Trigger",
    [0x00000100] = "No Taunt",
    [0x00000400] = "Ghost Only",
    [0x00000800] = "Offhand Attack",
    [0x00001000] = "No Sell",
    [0x00002000] = "No Combat",
    [0x00004000] = "World Event",
    [0x00008000] = "Guard",
    [0x00010000] = "Ignore Feign Death",
    [0x00020000] = "No Crit",
    [0x00040000] = "No Skill Gains",
    [0x10000000] = "Dungeon Boss",
    [0x20000000] = "Ignore Pathfinding",
    [0x40000000] = "Immune Knockback",
}

-- Decode flags into a list of descriptions
local function DecodeFlags(value, flagTable)
    if not value or value == 0 then return nil end
    local results = {}
    for flag, name in pairs(flagTable) do
        if bit.band(value, flag) ~= 0 then
            table.insert(results, name)
        end
    end
    if #results == 0 then return nil end
    return results
end

-- Expose flag decoders for other modules
function ServerData:DecodeNPCFlags(value)
    local flags = DecodeFlags(value, NPC_FLAGS)
    return flags and table.concat(flags, ", ") or nil
end

function ServerData:DecodeUnitFlags(value)
    local flags = DecodeFlags(value, UNIT_FLAGS)
    return flags and table.concat(flags, ", ") or nil
end

function ServerData:DecodeExtraFlags(value)
    local flags = DecodeFlags(value, EXTRA_FLAGS)
    return flags and table.concat(flags, ", ") or nil
end

-- Mark that decoders are available
ServerData.DecodeFlags = true

-- ============================================================================
-- Helper Functions
-- ============================================================================

-- Format server data for display
function ServerData:FormatServerData(data)
    if not data then
        return "|cFFFF0000No server data available|r"
    end
    
    if not data.success then
        return string.format("|cFFFF0000Error: %s|r", data.error or "Unknown error")
    end
    
    local lines = {}
    
    -- Header
    table.insert(lines, "|cFFFFD700=== Server Data ===|r")
    table.insert(lines, string.format("|cFF888888Fetched: %s|r", date("%H:%M:%S", data.timestamp or 0)))
    table.insert(lines, "")
    
    -- Base Stats (only show if any stat > 0 - creatures typically don't have these)
    if data.stats then
        local hasNonZeroStat = false
        for _, value in pairs(data.stats) do
            if value and value > 0 then
                hasNonZeroStat = true
                break
            end
        end
        
        if hasNonZeroStat then
            table.insert(lines, "|cFFFFD700Base Stats:|r")
            for statName, value in pairs(data.stats) do
                if value and value > 0 then
                    table.insert(lines, string.format("  |cFF00FF00%s:|r %.0f", statName, value))
                end
            end
            table.insert(lines, "")
        end
    end
    
    -- Vitals
    if data.vitals then
        table.insert(lines, "|cFFFFD700Vitals:|r")
        table.insert(lines, string.format("  |cFF00FF00Health:|r %d / %d (%.1f%%)", 
            data.vitals.health or 0,
            data.vitals.maxHealth or 0,
            data.vitals.healthPercent or 0))
        
        if data.vitals.maxPower and data.vitals.maxPower > 0 then
            local powerTypes = {"Mana", "Rage", "Focus", "Energy"}
            local powerName = powerTypes[(data.vitals.powerType or 0) + 1] or "Power"
            table.insert(lines, string.format("  |cFF00FF00%s:|r %d / %d",
                powerName,
                data.vitals.power or 0,
                data.vitals.maxPower or 0))
        end
        table.insert(lines, "")
    end
    
    -- Movement Speeds
    if data.speeds then
        table.insert(lines, "|cFFFFD700Movement Speeds:|r")
        table.insert(lines, string.format("  |cFF00FF00Walk:|r %.2f", data.speeds.walk or 0))
        table.insert(lines, string.format("  |cFF00FF00Run:|r %.2f", data.speeds.run or 0))
        if data.speeds.fly and data.speeds.fly > 0 then
            table.insert(lines, string.format("  |cFF00FF00Fly:|r %.2f", data.speeds.fly or 0))
        end
        table.insert(lines, "")
    end
    
    -- Combat Stats (Phase 2)
    if data.combat then
        table.insert(lines, "|cFFFFD700Combat:|r")
        table.insert(lines, string.format("  |cFF00FF00Armor:|r %d", data.combat.armor or 0))
        if data.combat.baseAttackTime and data.combat.baseAttackTime > 0 then
            table.insert(lines, string.format("  |cFF00FF00Attack Time:|r %d ms", data.combat.baseAttackTime))
        end
        if data.combat.rangedAttackTime and data.combat.rangedAttackTime > 0 then
            table.insert(lines, string.format("  |cFF00FF00Ranged Time:|r %d ms", data.combat.rangedAttackTime))
        end
        table.insert(lines, "")
    end
    
    -- Resistances (Phase 2)
    if data.resistances then
        local hasResist = false
        local resistLines = {}
        local schoolOrder = {"Holy", "Fire", "Nature", "Frost", "Shadow", "Arcane"}
        for _, school in ipairs(schoolOrder) do
            local resist = data.resistances[school]
            if resist and resist > 0 then
                hasResist = true
                table.insert(resistLines, string.format("  |cFF00FF00%s:|r %d", school, resist))
            end
        end
        if hasResist then
            table.insert(lines, "|cFFFFD700Resistances:|r")
            for _, line in ipairs(resistLines) do
                table.insert(lines, line)
            end
            table.insert(lines, "")
        end
    end
    
    -- Template Data (Phase 2)
    if data.template then
        table.insert(lines, "|cFFFFD700Template:|r")
        if data.template.unitClass then
            local classes = {[1] = "Warrior", [2] = "Paladin", [4] = "Rogue", [8] = "Mage"}
            table.insert(lines, string.format("  |cFF00FF00Class:|r %s", classes[data.template.unitClass] or data.template.unitClass))
        end
        
        -- Decode NPC Flags into readable services
        local npcFlagsList = DecodeFlags(data.template.npcFlags, NPC_FLAGS)
        if npcFlagsList then
            table.insert(lines, "  |cFF00FF00Services:|r " .. table.concat(npcFlagsList, ", "))
        end
        
        -- Decode Unit Flags into readable behaviors
        local unitFlagsList = DecodeFlags(data.template.unitFlags, UNIT_FLAGS)
        if unitFlagsList then
            table.insert(lines, "  |cFF00FF00Behaviors:|r " .. table.concat(unitFlagsList, ", "))
        end
        
        -- Decode Extra Flags into readable properties
        local extraFlagsList = DecodeFlags(data.template.extraFlags, EXTRA_FLAGS)
        if extraFlagsList then
            table.insert(lines, "  |cFF00FF00Properties:|r " .. table.concat(extraFlagsList, ", "))
        end
        
        table.insert(lines, "")
    end
    
    -- Spell Power (often 0 for creatures)
    if data.spellPower then
        local hasSpellPower = false
        local schoolOrder = {"Physical", "Holy", "Fire", "Nature", "Frost", "Shadow", "Arcane"}
        for _, school in ipairs(schoolOrder) do
            local power = data.spellPower[school]
            if power and power > 0 then
                hasSpellPower = true
                break
            end
        end
        if hasSpellPower then
            table.insert(lines, "|cFFFFD700Spell Power:|r")
            for _, school in ipairs(schoolOrder) do
                local power = data.spellPower[school]
                if power and power > 0 then
                    table.insert(lines, string.format("  |cFF00FF00%s:|r %d", school, power))
                end
            end
            table.insert(lines, "")
        end
    end
    
    -- AI & Scripts
    if data.scripts then
        table.insert(lines, "|cFFFFD700Scripts & AI:|r")
        if data.scripts.aiName and data.scripts.aiName ~= "" then
            table.insert(lines, string.format("  |cFF00FF00AI:|r %s", data.scripts.aiName))
        end
        if data.scripts.scriptName and data.scripts.scriptName ~= "" then
            table.insert(lines, string.format("  |cFF00FF00Script:|r %s", data.scripts.scriptName))
        end
        table.insert(lines, "")
    end
    
    -- Behavior
    if data.behavior then
        table.insert(lines, "|cFFFFD700Behavior:|r")
        table.insert(lines, string.format("  |cFF00FF00Respawn:|r %d seconds", data.behavior.respawnDelay or 0))
        table.insert(lines, string.format("  |cFF00FF00Wander:|r %.1f yards", data.behavior.wanderRadius or 0))
        
        local flags = {}
        if data.behavior.isElite then table.insert(flags, "Elite") end
        if data.behavior.isWorldBoss then table.insert(flags, "World Boss") end
        if data.behavior.isCivilian then table.insert(flags, "Civilian") end
        if data.behavior.isRegeneratingHealth then table.insert(flags, "Regen HP") end
        
        if #flags > 0 then
            table.insert(lines, string.format("  |cFF00FF00Flags:|r %s", table.concat(flags, ", ")))
        end
        table.insert(lines, "")
    end
    
    -- Notes about missing data
    if data.notes then
        table.insert(lines, "|cFF888888--- Notes ---|r")
        for _, note in ipairs(data.notes) do
            table.insert(lines, "|cFF888888" .. note .. "|r")
        end
    end
    
    return table.concat(lines, "\n")
end

-- ============================================================================
-- AMS Integration
-- ============================================================================

-- Request NPC data from server
function ServerData:RequestNPCData(guid, callback)
    if not AMS then
        print("[AraxiaTrinityAdmin] ERROR: AMS not loaded!")
        if callback then
            callback(nil, "AMS not available")
        end
        return
    end
    
    if not guid then
        print("[AraxiaTrinityAdmin] ERROR: No GUID provided to RequestNPCData")
        if callback then
            callback(nil, "No GUID provided")
        end
        return
    end
    
    -- Check cache first
    if self.cache[guid] then
        print("[AraxiaTrinityAdmin] Using cached data for", guid)
        if callback then
            callback(self.cache[guid], nil)
        end
        return
    end
    
    -- Check if already loading
    if self.loading[guid] then
        print("[AraxiaTrinityAdmin] Already loading data for", guid)
        -- Add callback to queue
        if callback then
            if not self.callbacks[guid] then
                self.callbacks[guid] = {}
            end
            table.insert(self.callbacks[guid], callback)
        end
        return
    end
    
    -- Mark as loading
    self.loading[guid] = true
    if callback then
        self.callbacks[guid] = {callback}
    end
    
    print("[AraxiaTrinityAdmin] Requesting NPC data from server for GUID:", guid)
    
    -- Send request to server
    AMS.Send("GET_NPC_DATA", {npcGUID = guid})
end

-- Clear cached data for a GUID
function ServerData:ClearCache(guid)
    if guid then
        self.cache[guid] = nil
    else
        -- Clear all
        self.cache = {}
    end
end

-- Get cached data without requesting
function ServerData:GetCachedData(guid)
    return self.cache[guid]
end

-- Check if data is loading
function ServerData:IsLoading(guid)
    return self.loading[guid] == true
end

-- ============================================================================
-- AMS Response Handler
-- ============================================================================

-- Wait for AMS to be available, then register handler
local function InitAMSHandler()
    if not AMS then
        C_Timer.After(0.5, InitAMSHandler)
        return
    end
    
    -- Register response handler
    AMS.RegisterHandler("NPC_DATA_RESPONSE", function(data)
        local guid = data.guid
        
        if not guid then
            print("[AraxiaTrinityAdmin] Received NPC data without GUID")
            return
        end
        
        print("[AraxiaTrinityAdmin] Received NPC data for", guid)
        
        -- Cache the data
        ServerData.cache[guid] = data
        
        -- Mark as no longer loading
        ServerData.loading[guid] = nil
        
        -- Call any waiting callbacks
        if ServerData.callbacks[guid] then
            for _, callback in ipairs(ServerData.callbacks[guid]) do
                callback(data, nil)
            end
            ServerData.callbacks[guid] = nil
        end
    end)
    
    print("[AraxiaTrinityAdmin] Server Data Module initialized")
end

-- Start initialization
InitAMSHandler()

-- ============================================================================
-- Export
-- ============================================================================

-- Make available to addon
_G.AraxiaTrinityAdmin = _G.AraxiaTrinityAdmin or {}
_G.AraxiaTrinityAdmin.ServerData = ServerData
