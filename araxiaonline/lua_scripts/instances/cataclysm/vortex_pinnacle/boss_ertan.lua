-- Grand Vizier Ertan - Vortex Pinnacle Boss 1
-- Entry: 43878, BossId: 0

local BOSS_ENTRY = 43878
local BOSS_ID = 0
local MAP_ID = 657
local BOSS_NAME = "Grand Vizier Ertan"

-- Spells (placeholder - add actual spell IDs as needed)
local SPELLS = {
    -- CYCLONE_SHIELD = 86267,
    -- STORMS_EDGE = 86295,
}

-- CREATURE_EVENT_ON_ENTER_COMBAT = 1
RegisterCreatureEvent(BOSS_ENTRY, 1, function(event, creature, target)
    print(string.format("[VP] %s engaged in combat", BOSS_NAME))
end)

-- CREATURE_EVENT_ON_DIED = 4
RegisterCreatureEvent(BOSS_ENTRY, 4, function(event, creature, killer)
    print(string.format("[VP] %s defeated - BossId %d should transition to DONE", BOSS_NAME, BOSS_ID))
    
    if killer and killer:GetObjectType() == "Player" then
        killer:SendBroadcastMessage(BOSS_NAME .. " defeated! Slipstreams to next platform activated.")
    end
end)

-- CREATURE_EVENT_ON_LEAVE_COMBAT = 2
RegisterCreatureEvent(BOSS_ENTRY, 2, function(event, creature)
    print(string.format("[VP] %s left combat (wipe or evade)", BOSS_NAME))
end)

print(string.format("[Eluna] Loaded boss script: %s (%d)", BOSS_NAME, BOSS_ENTRY))
