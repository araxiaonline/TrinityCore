-- Asaad - Vortex Pinnacle Boss 3 (Final Boss)
-- Entry: 43875, BossId: 2

local BOSS_ENTRY = 43875
local BOSS_ID = 2
local MAP_ID = 657
local BOSS_NAME = "Asaad"

-- Spells (placeholder - add actual spell IDs as needed)
local SPELLS = {
    -- CHAIN_LIGHTNING = 87622,
    -- SUPREMACY_OF_THE_STORM = 86930,
    -- STATIC_CLING = 87618,
}

-- CREATURE_EVENT_ON_ENTER_COMBAT = 1
RegisterCreatureEvent(BOSS_ENTRY, 1, function(event, creature, target)
    print(string.format("[VP] %s (Final Boss) engaged in combat", BOSS_NAME))
end)

-- CREATURE_EVENT_ON_DIED = 4
RegisterCreatureEvent(BOSS_ENTRY, 4, function(event, creature, killer)
    print(string.format("[VP] %s defeated - Instance complete! BossId %d should transition to DONE", BOSS_NAME, BOSS_ID))
    
    if killer and killer:GetObjectType() == "Player" then
        killer:SendBroadcastMessage(BOSS_NAME .. " defeated! Vortex Pinnacle cleared!")
    end
end)

-- CREATURE_EVENT_ON_LEAVE_COMBAT = 2
RegisterCreatureEvent(BOSS_ENTRY, 2, function(event, creature)
    print(string.format("[VP] %s left combat (wipe or evade)", BOSS_NAME))
end)

print(string.format("[Eluna] Loaded boss script: %s (%d)", BOSS_NAME, BOSS_ENTRY))
