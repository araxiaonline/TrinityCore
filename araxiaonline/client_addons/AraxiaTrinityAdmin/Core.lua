-- AraxiaTrinityAdmin Core
-- Main addon initialization and namespace

local ADDON_NAME, ns = ...

-- Create addon namespace
AraxiaTrinityAdmin = AraxiaTrinityAdmin or {}
local ATA = AraxiaTrinityAdmin

-- Version info
ATA.version = "1.0.0"
ATA.loaded = false

-- Database defaults
local defaults = {
    windowPosition = {
        point = "CENTER",
        x = 0,
        y = 0
    },
    windowShown = false,
    selectedPanel = "NPCInfo",
    minimapAngle = math.rad(225),  -- Default position (bottom-left)
    minimapLocked = true,  -- Lock minimap button by default
    minimapHidden = false  -- Show minimap button by default
}

-- Initialize saved variables
function ATA:InitDatabase()
    if not AraxiaTrinityAdminDB then
        AraxiaTrinityAdminDB = {}
    end
    
    -- Apply defaults
    for key, value in pairs(defaults) do
        if AraxiaTrinityAdminDB[key] == nil then
            AraxiaTrinityAdminDB[key] = value
        end
    end
end

-- Event frame
local eventFrame = CreateFrame("Frame")
eventFrame:RegisterEvent("ADDON_LOADED")
eventFrame:RegisterEvent("PLAYER_LOGIN")

eventFrame:SetScript("OnEvent", function(self, event, arg1)
    if event == "ADDON_LOADED" and arg1 == "AraxiaTrinityAdmin" then
        ATA:InitDatabase()
        ATA.loaded = true
        print("|cFF00FF00Araxia Trinity Admin|r v" .. ATA.version .. " loaded. Type |cFFFFFF00/araxia admin|r to open.")
    elseif event == "PLAYER_LOGIN" then
        -- Restore window state if it was open
        if AraxiaTrinityAdminDB.windowShown and ATA.MainWindow then
            ATA.MainWindow:Show()
        end
    end
end)

-- Slash command handler
SLASH_ARAXIAADMIN1 = "/araxia"
SlashCmdList["ARAXIAADMIN"] = function(msg)
    local command, args = msg:match("^(%S*)%s*(.-)$")
    command = command:lower()
    
    if command == "admin" then
        if ATA.MainWindow then
            if ATA.MainWindow:IsShown() then
                ATA.MainWindow:Hide()
            else
                ATA.MainWindow:Show()
            end
        else
            print("|cFFFF0000Error:|r Main window not initialized.")
        end
    else
        print("|cFF00FF00Araxia Trinity Admin|r Commands:")
        print("  |cFFFFFF00/araxia admin|r - Toggle admin window")
    end
end

-- Utility function to get target NPC info
function ATA:GetTargetNPCInfo()
    if not UnitExists("target") then
        return nil
    end
    
    if not UnitIsPlayer("target") then
        local guid = UnitGUID("target")
        if guid then
            local unitType, _, _, _, _, npcID = strsplit("-", guid)
            if unitType == "Creature" or unitType == "Vehicle" then
                return {
                    name = UnitName("target"),
                    npcID = tonumber(npcID),
                    level = UnitLevel("target"),
                    health = UnitHealth("target"),
                    maxHealth = UnitHealthMax("target"),
                    power = UnitPower("target"),
                    maxPower = UnitPowerMax("target"),
                    powerType = UnitPowerType("target"),
                    classification = UnitClassification("target"),
                    creatureType = UnitCreatureType("target"),
                    faction = UnitFactionGroup("target"),
                    reactionColor = UnitReaction("target", "player"),  -- Hostile/Neutral/Friendly
                    isTapDenied = UnitIsTapDenied("target"),
                    guid = guid
                }
            end
        end
    end
    
    return nil
end
