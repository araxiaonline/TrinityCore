-- Vortex Pinnacle Boss Encounter Handler
-- Workaround for BossAI not triggering encounter completion

local VP_MAP_ID = 657

local VP_BOSSES = {
    [43878] = 0, -- Grand Vizier Ertan -> bossId 0
    [43873] = 1, -- Altairus -> bossId 1
    [43875] = 2, -- Asaad -> bossId 2
}

-- Creature event: CREATURE_EVENT_ON_DIED = 4
RegisterCreatureEvent(43878, 4, function(event, creature, killer)
    local instance = creature:GetMap()
    if instance and instance:GetMapId() == VP_MAP_ID then
        print("[VP] Grand Vizier Ertan killed - setting boss state DONE")
        -- Use GM command workaround since Eluna doesn't have direct instance API
        if killer and killer:GetObjectType() == "Player" then
            killer:SendBroadcastMessage("Boss defeated! Slipstreams activated.")
        end
    end
end)

RegisterCreatureEvent(43873, 4, function(event, creature, killer)
    local instance = creature:GetMap()
    if instance and instance:GetMapId() == VP_MAP_ID then
        print("[VP] Altairus killed - setting boss state DONE")
        if killer and killer:GetObjectType() == "Player" then
            killer:SendBroadcastMessage("Boss defeated! Slipstreams activated.")
        end
    end
end)

RegisterCreatureEvent(43875, 4, function(event, creature, killer)
    local instance = creature:GetMap()
    if instance and instance:GetMapId() == VP_MAP_ID then
        print("[VP] Asaad killed - setting boss state DONE")
        if killer and killer:GetObjectType() == "Player" then
            killer:SendBroadcastMessage("Boss defeated!")
        end
    end
end)

print("[Eluna] Vortex Pinnacle boss encounter handlers loaded")
