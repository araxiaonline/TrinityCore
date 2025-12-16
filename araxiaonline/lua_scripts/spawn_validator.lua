--[[
    Spawn Validator - Headless creature spawn validation for MCP
    
    Allows MCP to validate creature spawns without requiring a player in-game.
    Uses server-side APIs to check if creatures exist and are spawned correctly.
    
    Shared Data Keys:
    - mcp_spawn_validation_request: Request from MCP to validate spawns
    - mcp_spawn_validation_result: Results of validation
    - mcp_spawn_query_result: Results of spawn queries
]]

local Smallfolk = require("Smallfolk")

-- Configuration
local VALIDATION_INTERVAL = 1000  -- Check for requests every 1 second

-- Initialize shared data keys
local function InitSharedData()
    if not HasSharedData("mcp_spawn_validation_request") then
        SetSharedData("mcp_spawn_validation_request", "")
    end
    if not HasSharedData("mcp_spawn_validation_result") then
        SetSharedData("mcp_spawn_validation_result", "")
    end
    if not HasSharedData("mcp_spawn_query_result") then
        SetSharedData("mcp_spawn_query_result", "")
    end
    if not HasSharedData("mcp_force_spawn_request") then
        SetSharedData("mcp_force_spawn_request", "")
    end
end

-- Get all creatures in a specific area
local function GetCreaturesInArea(mapId, centerX, centerY, centerZ, radius)
    local creatures = {}
    
    -- Use GetCreaturesInWorld to find creatures
    local allCreatures = GetCreaturesInWorld(mapId)
    
    if allCreatures then
        for _, creature in ipairs(allCreatures) do
            local cx, cy, cz = creature:GetLocation()
            local dx = cx - centerX
            local dy = cy - centerY
            local dz = cz - centerZ
            local dist = math.sqrt(dx*dx + dy*dy + dz*dz)
            
            if dist <= radius then
                table.insert(creatures, {
                    guid = creature:GetGUIDLow(),
                    entry = creature:GetEntry(),
                    name = creature:GetName(),
                    x = cx,
                    y = cy,
                    z = cz,
                    distance = dist,
                    isAlive = creature:IsAlive(),
                    level = creature:GetLevel()
                })
            end
        end
    end
    
    return creatures
end

-- Get creature by entry ID
local function GetCreaturesByEntry(mapId, entryId)
    local creatures = {}
    local allCreatures = GetCreaturesInWorld(mapId)
    
    if allCreatures then
        for _, creature in ipairs(allCreatures) do
            if creature:GetEntry() == entryId then
                local x, y, z = creature:GetLocation()
                table.insert(creatures, {
                    guid = creature:GetGUIDLow(),
                    entry = creature:GetEntry(),
                    name = creature:GetName(),
                    x = x,
                    y = y,
                    z = z,
                    isAlive = creature:IsAlive(),
                    level = creature:GetLevel()
                })
            end
        end
    end
    
    return creatures
end

-- Count all creatures on a map
local function CountCreaturesOnMap(mapId)
    local allCreatures = GetCreaturesInWorld(mapId)
    return allCreatures and #allCreatures or 0
end

-- Force spawn a creature at a location
local function ForceSpawnCreature(mapId, entryId, x, y, z, orientation)
    -- SpawnCreature(entry, map, x, y, z, o, despawnType, despawnTime)
    -- despawnType: 0 = manual, 1 = timed
    local creature = PerformIngameSpawn(1, entryId, mapId, 0, x, y, z, orientation or 0, false, 0)
    
    if creature then
        return {
            success = true,
            guid = creature:GetGUIDLow(),
            entry = creature:GetEntry(),
            name = creature:GetName(),
            x = x,
            y = y,
            z = z
        }
    else
        return {
            success = false,
            error = "Failed to spawn creature " .. entryId
        }
    end
end

-- Process validation requests from MCP
local function ProcessValidationRequest()
    local requestData = GetSharedData("mcp_spawn_validation_request")
    
    if not requestData or requestData == "" then
        return
    end
    
    -- Clear the request
    SetSharedData("mcp_spawn_validation_request", "")
    
    -- Parse request (simple JSON-like format)
    -- Format: {"action":"query_area","mapId":870,"x":1606,"y":-1733,"z":274,"radius":50}
    local action = requestData:match('"action":"([^"]*)"')
    local mapId = tonumber(requestData:match('"mapId":(%d+)'))
    
    local result = {}
    
    if action == "query_area" then
        local x = tonumber(requestData:match('"x":([%-%.%d]+)'))
        local y = tonumber(requestData:match('"y":([%-%.%d]+)'))
        local z = tonumber(requestData:match('"z":([%-%.%d]+)'))
        local radius = tonumber(requestData:match('"radius":(%d+)')) or 50
        
        if mapId and x and y and z then
            local creatures = GetCreaturesInArea(mapId, x, y, z, radius)
            result = {
                success = true,
                action = "query_area",
                mapId = mapId,
                center = {x = x, y = y, z = z},
                radius = radius,
                count = #creatures,
                creatures = creatures
            }
            print("[Spawn Validator] Found " .. #creatures .. " creatures in area")
        else
            result = {success = false, error = "Missing coordinates"}
        end
        
    elseif action == "query_entry" then
        local entryId = tonumber(requestData:match('"entryId":(%d+)'))
        
        if mapId and entryId then
            local creatures = GetCreaturesByEntry(mapId, entryId)
            result = {
                success = true,
                action = "query_entry",
                mapId = mapId,
                entryId = entryId,
                count = #creatures,
                creatures = creatures
            }
            print("[Spawn Validator] Found " .. #creatures .. " creatures with entry " .. entryId)
        else
            result = {success = false, error = "Missing mapId or entryId"}
        end
        
    elseif action == "count_map" then
        if mapId then
            local count = CountCreaturesOnMap(mapId)
            result = {
                success = true,
                action = "count_map",
                mapId = mapId,
                count = count
            }
            print("[Spawn Validator] Map " .. mapId .. " has " .. count .. " creatures")
        else
            result = {success = false, error = "Missing mapId"}
        end
        
    elseif action == "force_spawn" then
        local entryId = tonumber(requestData:match('"entryId":(%d+)'))
        local x = tonumber(requestData:match('"x":([%-%.%d]+)'))
        local y = tonumber(requestData:match('"y":([%-%.%d]+)'))
        local z = tonumber(requestData:match('"z":([%-%.%d]+)'))
        local o = tonumber(requestData:match('"orientation":([%-%.%d]+)')) or 0
        
        if mapId and entryId and x and y and z then
            result = ForceSpawnCreature(mapId, entryId, x, y, z, o)
            result.action = "force_spawn"
            print("[Spawn Validator] Force spawn result: " .. (result.success and "success" or "failed"))
        else
            result = {success = false, error = "Missing spawn parameters"}
        end
        
    else
        result = {success = false, error = "Unknown action: " .. (action or "nil")}
    end
    
    -- Store result as JSON
    local resultJson = string.format(
        '{"success":%s,"action":"%s","mapId":%d,"count":%d,"error":"%s","timestamp":%d}',
        result.success and "true" or "false",
        result.action or "unknown",
        result.mapId or 0,
        result.count or 0,
        result.error or "",
        os.time()
    )
    
    -- For creature lists, append them
    if result.creatures and #result.creatures > 0 then
        local creatureStrs = {}
        for _, c in ipairs(result.creatures) do
            table.insert(creatureStrs, string.format(
                '{"guid":%d,"entry":%d,"name":"%s","x":%.2f,"y":%.2f,"z":%.2f,"level":%d,"alive":%s}',
                c.guid or 0,
                c.entry or 0,
                c.name or "unknown",
                c.x or 0,
                c.y or 0,
                c.z or 0,
                c.level or 0,
                c.isAlive and "true" or "false"
            ))
        end
        resultJson = resultJson:gsub('}$', ',"creatures":[' .. table.concat(creatureStrs, ',') .. ']}')
    end
    
    SetSharedData("mcp_spawn_validation_result", resultJson)
end

-- Register a world update hook to check for requests
local function OnWorldUpdate(event, diff)
    ProcessValidationRequest()
end

-- Initialize
InitSharedData()

-- Register world update event (fires every server tick)
RegisterServerEvent(14, OnWorldUpdate)  -- WORLD_EVENT_ON_UPDATE

print("[Spawn Validator] Loaded - MCP can now validate spawns without a player")
print("[Spawn Validator] Actions: query_area, query_entry, count_map, force_spawn")
