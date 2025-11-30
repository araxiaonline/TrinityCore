-- Eluna Reload Helper
-- Provides an in-game command to reload Eluna scripts

local function ReloadElunaCommand(event, player, command)
    if command == "elunareload" then
        if player:GetGMRank() >= 3 then
            player:SendBroadcastMessage("Reloading Eluna scripts...")
            ReloadEluna()
            player:SendBroadcastMessage("Eluna scripts reloaded!")
        else
            player:SendBroadcastMessage("You don't have permission to reload Eluna.")
        end
        return false -- Prevent command from going to server
    end
end

RegisterPlayerEvent(42, ReloadElunaCommand) -- PLAYER_EVENT_ON_COMMAND

print("[Eluna] Reload helper loaded. Use '.elunareload' in-game to reload scripts.")
