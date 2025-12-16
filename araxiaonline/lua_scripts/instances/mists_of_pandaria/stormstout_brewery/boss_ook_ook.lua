--[[
    Stormstout Brewery - Ook-Ook Boss Script
    
    Entry: 56637
    Map: 961 (Stormstout Brewery)
    
    Abilities:
    - Ground Pound (106807) - Frontal cone, 3 sec channel, stuns 2 sec
    - Going Bananas (106651) - At 90/60/30% HP, +15% damage/speed, triggers barrels
    - Rolling Barrel (NPC 56682) - Rolls across room, players can ride
    - Brew Explosion (107351) - Barrel collision damage
    
    Notes:
    - Boss should spawn after 40 Hozen killed (not implemented here - needs instance script)
    - Barrel riding (vehicle) may not work without C++ support
]]

local OOK_OOK_ENTRY = 56637
local ROLLING_BARREL_ENTRY = 56682

-- SAFEGUARD: Script version check to handle stale timers after .reload eluna
-- Stale CreateLuaEvent timers from old script versions continue running after reload
-- This version number lets us detect and ignore callbacks from old timers
local SCRIPT_VERSION = 2
_G.OOK_OOK_SCRIPT_VERSION = SCRIPT_VERSION

-- Spell IDs
local SPELL_GROUND_POUND = 106807
local SPELL_GOING_BANANAS = 106651
local SPELL_BREW_EXPLOSION = 107351

-- Timers (in milliseconds)
local GROUND_POUND_TIMER = 8000      -- Every 8 seconds
local GROUND_POUND_INITIAL = 5000    -- First cast after 5 seconds

-- HP thresholds for Going Bananas (tracked to prevent re-triggering)
local BANANAS_90_TRIGGERED = false
local BANANAS_60_TRIGGERED = false
local BANANAS_30_TRIGGERED = false

-- Barrel spawn positions (around the brewhall stands)
-- These are approximate - may need adjustment based on actual room layout
local BARREL_SPAWN_POSITIONS = {
    {x = -755.0, y = 1356.0, z = 146.7, o = 4.7},  -- North stand
    {x = -770.0, y = 1341.0, z = 146.7, o = 0.0},  -- West stand
    {x = -755.0, y = 1326.0, z = 146.7, o = 1.5},  -- South stand
    {x = -740.0, y = 1341.0, z = 146.7, o = 3.1},  -- East stand
}

-- Boss center position for barrel targeting
local BOSS_CENTER = {x = -755.0, y = 1341.0, z = 146.7}

-- DEBUG/TESTING: Proximity trigger since 40-kill mechanic isn't implemented
-- Set to false to disable proximity aggro
local ENABLE_PROXIMITY_TRIGGER = true
local PROXIMITY_RANGE = 20  -- yards

--[[
    Reset phase tracking on combat start
]]
local function ResetPhaseTracking()
    BANANAS_90_TRIGGERED = false
    BANANAS_60_TRIGGERED = false
    BANANAS_30_TRIGGERED = false
end

--[[
    Spawn rolling barrels from the stands
    @param boss - The Ook-Ook creature
    @param count - Number of barrels to spawn
]]
local function SpawnBarrels(boss, count)
    if not boss or not boss:IsInWorld() then return end
    
    local map = boss:GetMap()
    if not map then return end
    
    for i = 1, count do
        -- Pick a random spawn position
        local spawnIdx = math.random(1, #BARREL_SPAWN_POSITIONS)
        local pos = BARREL_SPAWN_POSITIONS[spawnIdx]
        
        -- Spawn the barrel
        local barrel = boss:SpawnCreature(ROLLING_BARREL_ENTRY, pos.x, pos.y, pos.z, pos.o, 1, 30000)
        if barrel then
            -- Make barrel move toward boss center (simplified - real barrels roll in straight line)
            barrel:MoveFollow(boss, 0, 0)
            
            -- Register barrel collision check
            barrel:RegisterEvent(function(eventId, delay, repeats, barrelUnit)
                CheckBarrelCollision(barrelUnit, boss)
            end, 500, 0)  -- Check every 500ms
        end
    end
end

--[[
    Check if barrel has collided with anything
    @param barrel - The Rolling Barrel creature
    @param boss - The Ook-Ook boss (for debuff application)
]]
local function CheckBarrelCollision(barrel, boss)
    if not barrel or not barrel:IsInWorld() then return end
    
    -- Check distance to boss
    if boss and boss:IsInWorld() then
        local dist = barrel:GetDistance(boss)
        if dist < 3 then
            -- Hit the boss! Apply debuff and explode
            barrel:CastSpell(boss, SPELL_BREW_EXPLOSION, true)
            -- The spell should apply the vulnerability debuff
            barrel:DespawnOrUnsummon(100)
            return
        end
    end
    
    -- Check for nearby players
    local players = barrel:GetPlayersInRange(2)
    if players and #players > 0 then
        -- Hit a player - explode
        barrel:CastSpell(barrel, SPELL_BREW_EXPLOSION, true)
        barrel:DespawnOrUnsummon(100)
        return
    end
    
    -- TODO: Wall collision detection would need raycast or position bounds checking
end

--[[
    Handle Ground Pound ability
    @param eventId - Event ID
    @param delay - Delay before event
    @param repeats - Number of repeats remaining
    @param boss - The Ook-Ook creature
]]
local function CastGroundPound(eventId, delay, repeats, boss)
    if not boss or not boss:IsInWorld() or boss:IsDead() then return end
    if not boss:IsInCombat() then return end
    
    local victim = boss:GetVictim()
    if victim then
        boss:CastSpell(victim, SPELL_GROUND_POUND, false)
        -- Yell one of the quotes
        local quotes = {
            "Come on and get your Ook on!",
            "Get Ooking party started!",
            "We gonna Ook all night!",
        }
        boss:SendUnitYell(quotes[math.random(1, #quotes)], 0)
    end
end

--[[
    Check HP thresholds and trigger Going Bananas
    @param boss - The Ook-Ook creature
]]
local function CheckHPThresholds(boss)
    if not boss or not boss:IsInWorld() or boss:IsDead() then return end
    
    local hpPct = boss:GetHealthPct()
    
    -- 90% threshold
    if hpPct <= 90 and not BANANAS_90_TRIGGERED then
        BANANAS_90_TRIGGERED = true
        TriggerGoingBananas(boss, 1)
    end
    
    -- 60% threshold
    if hpPct <= 60 and not BANANAS_60_TRIGGERED then
        BANANAS_60_TRIGGERED = true
        TriggerGoingBananas(boss, 2)
    end
    
    -- 30% threshold
    if hpPct <= 30 and not BANANAS_30_TRIGGERED then
        BANANAS_30_TRIGGERED = true
        TriggerGoingBananas(boss, 3)
    end
end

--[[
    Trigger Going Bananas phase
    @param boss - The Ook-Ook creature
    @param stack - Which stack (1, 2, or 3)
]]
local function TriggerGoingBananas(boss, stack)
    if not boss or not boss:IsInWorld() then return end
    
    -- Cast Going Bananas buff on self
    boss:CastSpell(boss, SPELL_GOING_BANANAS, true)
    
    -- Yell
    boss:SendUnitYell("Me gonna ook you in the dooker!", 0)
    
    -- Spawn barrels based on stack (more barrels at lower HP)
    local barrelCount = 2 + stack  -- 3, 4, or 5 barrels
    SpawnBarrels(boss, barrelCount)
    
    PrintInfo(string.format("[Ook-Ook] Going Bananas triggered! Stack %d, spawning %d barrels", stack, barrelCount))
end

--[[
    EnterCombat handler
]]
local function OnEnterCombat(event, creature, target)
    if creature:GetEntry() ~= OOK_OOK_ENTRY then return end
    
    PrintInfo("[Ook-Ook] Combat started!")
    
    -- Reset tracking
    ResetPhaseTracking()
    
    -- Aggro yell
    creature:SendUnitYell("Me gonna ook you in the dooker!", 0)
    
    -- Schedule Ground Pound
    creature:RegisterEvent(CastGroundPound, GROUND_POUND_INITIAL, 0)
    
    -- Schedule HP threshold check (every 1 second)
    creature:RegisterEvent(function(eventId, delay, repeats, boss)
        CheckHPThresholds(boss)
    end, 1000, 0)
end

--[[
    LeaveCombat handler (wipe/evade)
]]
local function OnLeaveCombat(event, creature)
    if creature:GetEntry() ~= OOK_OOK_ENTRY then return end
    
    PrintInfo("[Ook-Ook] Combat ended (evade/wipe)")
    
    -- Clear events
    creature:RemoveEvents()
    
    -- Reset tracking
    ResetPhaseTracking()
end

--[[
    JustDied handler
]]
local function OnDied(event, creature, killer)
    if creature:GetEntry() ~= OOK_OOK_ENTRY then return end
    
    PrintInfo("[Ook-Ook] Defeated!")
    
    -- Clear events
    creature:RemoveEvents()
    
    -- Death yell
    creature:SendUnitYell("Ook! Oooook!!", 0)
    
    -- Reset tracking for next attempt
    ResetPhaseTracking()
end

--[[
    DamageTaken handler - for HP threshold checks
]]
local function OnDamageTaken(event, creature, attacker, damage)
    if creature:GetEntry() ~= OOK_OOK_ENTRY then return end
    
    -- Check HP thresholds after damage
    CheckHPThresholds(creature)
end

--[[
    Proximity trigger for testing (since 40-kill mechanic isn't implemented)
    Checks every 2 seconds if a player is within range
]]
local function OnSpawn(event, creature)
    if creature:GetEntry() ~= OOK_OOK_ENTRY then return end
    
    if not ENABLE_PROXIMITY_TRIGGER then return end
    
    PrintInfo("[Ook-Ook] Spawned - proximity trigger enabled at " .. PROXIMITY_RANGE .. " yards")
    
    -- Register periodic check for nearby players
    creature:RegisterEvent(function(eventId, delay, repeats, boss)
        if not boss or not boss:IsInWorld() or boss:IsDead() then return end
        if boss:IsInCombat() then return end  -- Already in combat
        
        local players = boss:GetPlayersInRange(PROXIMITY_RANGE)
        if players and #players > 0 then
            local target = players[1]
            if target and target:IsAlive() then
                PrintInfo("[Ook-Ook] Player detected within " .. PROXIMITY_RANGE .. " yards - engaging!")
                boss:SendUnitYell("You come to Ook's party?! NOW YOU GONNA GET OOKED!", 0)
                boss:AttackStart(target)
            end
        end
    end, 2000, 0)  -- Check every 2 seconds
end

-- Register event handlers
RegisterCreatureEvent(OOK_OOK_ENTRY, 1, OnEnterCombat)   -- CREATURE_EVENT_ON_ENTER_COMBAT
RegisterCreatureEvent(OOK_OOK_ENTRY, 2, OnLeaveCombat)   -- CREATURE_EVENT_ON_LEAVE_COMBAT
RegisterCreatureEvent(OOK_OOK_ENTRY, 4, OnDied)          -- CREATURE_EVENT_ON_DIED
RegisterCreatureEvent(OOK_OOK_ENTRY, 5, OnSpawn)         -- CREATURE_EVENT_ON_SPAWN
RegisterCreatureEvent(OOK_OOK_ENTRY, 9, OnDamageTaken)   -- CREATURE_EVENT_ON_DAMAGE_TAKEN

-- NOTE: Global proximity scanner DISABLED
-- GetPlayersInWorld() doesn't work in map states, GetPlayersInMap() doesn't exist in Eluna
-- The proximity trigger is handled via OnSpawn event above instead
-- If you need to trigger Ook-Ook manually, use: .npc add 56637 near player
if ENABLE_PROXIMITY_TRIGGER then
    PrintInfo("[Ook-Ook] Proximity trigger enabled (via OnSpawn event only - no global scanner)")
end

PrintInfo("[Stormstout Brewery] Ook-Ook boss script loaded (proximity trigger: " .. tostring(ENABLE_PROXIMITY_TRIGGER) .. ")")
