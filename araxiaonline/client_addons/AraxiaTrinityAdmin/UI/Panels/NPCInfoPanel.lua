-- AraxiaTrinityAdmin NPC Info Panel
-- Display detailed information about selected NPCs with tabbed interface

local addonName = "AraxiaTrinityAdmin"

-- Wait for addon to load
local initFrame = CreateFrame("Frame")
initFrame:RegisterEvent("ADDON_LOADED")
initFrame:SetScript("OnEvent", function(self, event, loadedAddon)
    if loadedAddon ~= addonName then return end
    self:UnregisterEvent("ADDON_LOADED")
    
    local ATA = AraxiaTrinityAdmin
    if not ATA then return end

-- Create panel frame (will be reparented by MainWindow:RegisterPanel)
local npcPanel = CreateFrame("Frame", "AraxiaTrinityAdminNPCPanel", UIParent)
npcPanel:Hide()

-- Server data storage
local serverData = nil
local isLoadingServerData = false

-- Waypoint visualization state
local waypointsVisible = false
local currentTargetGUID = nil

-- Current tab
local currentTab = "Basic"

-- ============================================================================
-- Header: Title and Buttons
-- ============================================================================

local title = npcPanel:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
title:SetPoint("TOPLEFT", npcPanel, "TOPLEFT", 10, -10)
title:SetText("NPC Information")

local refreshButton = CreateFrame("Button", nil, npcPanel, "UIPanelButtonTemplate")
refreshButton:SetSize(80, 22)
refreshButton:SetPoint("LEFT", title, "RIGHT", 15, 0)
refreshButton:SetText("Refresh")

local deleteButton = CreateFrame("Button", nil, npcPanel, "UIPanelButtonTemplate")
deleteButton:SetSize(80, 22)
deleteButton:SetPoint("LEFT", refreshButton, "RIGHT", 5, 0)
deleteButton:SetText("Delete")

local waypointButton = CreateFrame("Button", nil, npcPanel, "UIPanelButtonTemplate")
waypointButton:SetSize(110, 22)
waypointButton:SetPoint("LEFT", deleteButton, "RIGHT", 5, 0)
waypointButton:SetText("Show Waypoints")
waypointButton:Disable()  -- Disabled until we have a creature with waypoints

-- ============================================================================
-- Tab Buttons
-- ============================================================================

local tabContainer = CreateFrame("Frame", nil, npcPanel)
tabContainer:SetPoint("TOPLEFT", title, "BOTTOMLEFT", 0, -8)
tabContainer:SetPoint("RIGHT", npcPanel, "CENTER", -10, 0)
tabContainer:SetHeight(28)

local tabs = {}
local tabNames = {"Basic", "Stats", "AI"}

local function CreateTab(name, index)
    local tab = CreateFrame("Button", nil, tabContainer, "UIPanelButtonTemplate")
    tab:SetSize(70, 24)
    tab:SetText(name)
    if index == 1 then
        tab:SetPoint("LEFT", tabContainer, "LEFT", 0, 0)
    else
        tab:SetPoint("LEFT", tabs[index-1], "RIGHT", 2, 0)
    end
    tabs[index] = tab
    return tab
end

for i, name in ipairs(tabNames) do
    CreateTab(name, i)
end

-- ============================================================================
-- Content Frames (one per tab)
-- ============================================================================

local function CreateScrollableContent(parent)
    local frame = CreateFrame("Frame", nil, parent, "BackdropTemplate")
    frame:SetPoint("TOPLEFT", tabContainer, "BOTTOMLEFT", 0, -5)
    frame:SetPoint("BOTTOMLEFT", npcPanel, "BOTTOMLEFT", 10, 10)
    frame:SetPoint("RIGHT", npcPanel, "CENTER", -10, 0)
    frame:SetBackdrop({
        bgFile = "Interface/Tooltips/UI-Tooltip-Background",
        edgeFile = "Interface/Tooltips/UI-Tooltip-Border",
        tile = true, tileSize = 16, edgeSize = 16,
        insets = { left = 4, right = 4, top = 4, bottom = 4 }
    })
    frame:SetBackdropColor(0, 0, 0, 0.5)
    frame:SetBackdropBorderColor(0.4, 0.4, 0.4, 1)
    frame:Hide()
    
    local scroll = CreateFrame("ScrollFrame", nil, frame, "UIPanelScrollFrameTemplate")
    scroll:SetPoint("TOPLEFT", frame, "TOPLEFT", 8, -8)
    scroll:SetPoint("BOTTOMRIGHT", frame, "BOTTOMRIGHT", -28, 8)
    
    local child = CreateFrame("Frame", nil, scroll)
    child:SetSize(1, 1)
    scroll:SetScrollChild(child)
    
    local text = child:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
    text:SetPoint("TOPLEFT", child, "TOPLEFT", 5, -5)
    text:SetJustifyH("LEFT")
    text:SetJustifyV("TOP")
    text:SetWidth(280)
    
    frame.scroll = scroll
    frame.child = child
    frame.text = text
    
    return frame
end

local contentFrames = {}
for _, name in ipairs(tabNames) do
    contentFrames[name] = CreateScrollableContent(npcPanel)
end

-- ============================================================================
-- Right side: 3D Model display
-- ============================================================================

local modelFrame = CreateFrame("Frame", nil, npcPanel, "BackdropTemplate")
modelFrame:SetPoint("TOPLEFT", tabContainer, "BOTTOMRIGHT", 15, -5)
modelFrame:SetPoint("BOTTOMRIGHT", npcPanel, "BOTTOMRIGHT", -10, 10)
modelFrame:SetBackdrop({
    bgFile = "Interface/Tooltips/UI-Tooltip-Background",
    edgeFile = "Interface/Tooltips/UI-Tooltip-Border",
    tile = true, tileSize = 16, edgeSize = 16,
    insets = { left = 4, right = 4, top = 4, bottom = 4 }
})
modelFrame:SetBackdropColor(0, 0, 0, 0.5)
modelFrame:SetBackdropBorderColor(0.4, 0.4, 0.4, 1)

local modelTitle = modelFrame:CreateFontString(nil, "OVERLAY", "GameFontNormal")
modelTitle:SetPoint("TOP", modelFrame, "TOP", 0, -8)
modelTitle:SetText("3D Model")

local npcModel = CreateFrame("PlayerModel", nil, modelFrame)
npcModel:SetPoint("TOPLEFT", modelFrame, "TOPLEFT", 8, -30)
npcModel:SetPoint("BOTTOMRIGHT", modelFrame, "BOTTOMRIGHT", -8, 8)
npcModel:SetCamDistanceScale(1.5)
npcModel:SetRotation(0.61)

-- ============================================================================
-- Tab Switching
-- ============================================================================

local function ShowTab(tabName)
    currentTab = tabName
    for name, frame in pairs(contentFrames) do
        if name == tabName then
            frame:Show()
        else
            frame:Hide()
        end
    end
    -- Update tab button appearance
    for i, tab in ipairs(tabs) do
        if tabNames[i] == tabName then
            tab:SetEnabled(false)
        else
            tab:SetEnabled(true)
        end
    end
end

for i, tab in ipairs(tabs) do
    tab:SetScript("OnClick", function()
        ShowTab(tabNames[i])
    end)
end

-- ============================================================================
-- Content Formatters
-- ============================================================================

local function FormatBasicTab(npcData)
    if not npcData then
        return "No valid NPC target found.\n\nPlease target a creature or NPC."
    end
    
    local lines = {}
    table.insert(lines, "|cFFFFD700Basic Information|r")
    table.insert(lines, string.format("  |cFF00FF00Name:|r %s", npcData.name or "Unknown"))
    table.insert(lines, string.format("  |cFF00FF00Entry ID:|r %s", npcData.npcID or "Unknown"))
    table.insert(lines, string.format("  |cFF00FF00GUID:|r %s", npcData.guid or "Unknown"))
    table.insert(lines, string.format("  |cFF00FF00Level:|r %s", npcData.level == -1 and "??" or npcData.level))
    table.insert(lines, "")
    
    -- Classification
    local classification = npcData.classification or "normal"
    local classColors = {
        elite = "|cFFFFFF00Elite|r",
        rareelite = "|cFFFF00FFRare Elite|r",
        rare = "|cFF0070DDRare|r",
        worldboss = "|cFFFF0000World Boss|r"
    }
    table.insert(lines, string.format("  |cFF00FF00Classification:|r %s", classColors[classification] or classification))
    
    -- Reaction
    if npcData.reactionColor then
        local reactions = {
            [1] = "|cFFFF0000Hostile|r", [2] = "|cFFFF0000Hostile|r",
            [3] = "|cFFFF0000Unfriendly|r", [4] = "|cFFFFFF00Neutral|r",
            [5] = "|cFF00FF00Friendly|r", [6] = "|cFF00FF00Friendly|r",
            [7] = "|cFF00FF00Friendly|r", [8] = "|cFF00FF00Friendly|r"
        }
        table.insert(lines, string.format("  |cFF00FF00Reaction:|r %s", reactions[npcData.reactionColor] or "Unknown"))
    end
    
    table.insert(lines, string.format("  |cFF00FF00Creature Type:|r %s", npcData.creatureType or "Unknown"))
    table.insert(lines, string.format("  |cFF00FF00Faction:|r %s", npcData.faction or "Unknown"))
    table.insert(lines, "")
    
    table.insert(lines, "|cFFFFD700GM Commands|r")
    table.insert(lines, string.format("  |cFFFFFF00.npc info|r"))
    table.insert(lines, string.format("  |cFFFFFF00.lookup creature %s|r", npcData.name or ""))
    table.insert(lines, string.format("  |cFFFFFF00.npc add %s|r", npcData.npcID or ""))
    
    return table.concat(lines, "\n")
end

local function FormatStatsTab(npcData, sData)
    local lines = {}
    
    -- Client-side vitals
    if npcData then
        table.insert(lines, "|cFFFFD700Vitals|r")
        table.insert(lines, string.format("  |cFF00FF00Health:|r %d / %d (%.1f%%)", 
            npcData.health or 0, npcData.maxHealth or 0,
            npcData.maxHealth > 0 and (npcData.health / npcData.maxHealth * 100) or 0))
        
        if npcData.maxPower and npcData.maxPower > 0 then
            local powerTypes = {"Mana", "Rage", "Focus", "Energy", "Combo Points", "Runes", "Runic Power", "Soul Shards", "Lunar Power", "Holy Power", "Maelstrom", "Chi", "Insanity"}
            local powerName = powerTypes[(npcData.powerType or 0) + 1] or "Power"
            table.insert(lines, string.format("  |cFF00FF00%s:|r %d / %d", powerName, npcData.power or 0, npcData.maxPower))
        end
        table.insert(lines, "")
    end
    
    -- Server data
    if isLoadingServerData then
        table.insert(lines, "|cFF888888Loading server data...|r")
    elseif sData and sData.success then
        -- Combat stats
        if sData.combat then
            table.insert(lines, "|cFFFFD700Combat|r")
            table.insert(lines, string.format("  |cFF00FF00Armor:|r %d", sData.combat.armor or 0))
            if sData.combat.baseAttackTime and sData.combat.baseAttackTime > 0 then
                table.insert(lines, string.format("  |cFF00FF00Attack Time:|r %d ms", sData.combat.baseAttackTime))
            end
            if sData.combat.rangedAttackTime and sData.combat.rangedAttackTime > 0 then
                table.insert(lines, string.format("  |cFF00FF00Ranged Time:|r %d ms", sData.combat.rangedAttackTime))
            end
            table.insert(lines, "")
        end
        
        -- Movement speeds
        if sData.speeds then
            table.insert(lines, "|cFFFFD700Movement|r")
            table.insert(lines, string.format("  |cFF00FF00Walk:|r %.2f", sData.speeds.walk or 0))
            table.insert(lines, string.format("  |cFF00FF00Run:|r %.2f", sData.speeds.run or 0))
            if sData.speeds.fly and sData.speeds.fly > 0 then
                table.insert(lines, string.format("  |cFF00FF00Fly:|r %.2f", sData.speeds.fly))
            end
            table.insert(lines, "")
        end
        
        -- Resistances
        if sData.resistances then
            local hasResist = false
            local resistLines = {}
            local schools = {"Holy", "Fire", "Nature", "Frost", "Shadow", "Arcane"}
            for _, school in ipairs(schools) do
                local val = sData.resistances[school]
                if val and val > 0 then
                    hasResist = true
                    table.insert(resistLines, string.format("  |cFF00FF00%s:|r %d", school, val))
                end
            end
            if hasResist then
                table.insert(lines, "|cFFFFD700Resistances|r")
                for _, line in ipairs(resistLines) do
                    table.insert(lines, line)
                end
                table.insert(lines, "")
            end
        end
    elseif not npcData then
        table.insert(lines, "|cFF888888Target an NPC to see stats|r")
    end
    
    return table.concat(lines, "\n")
end

local function FormatAITab(npcData, sData)
    local lines = {}
    
    if isLoadingServerData then
        table.insert(lines, "|cFF888888Loading server data...|r")
        return table.concat(lines, "\n")
    end
    
    if not sData or not sData.success then
        if npcData then
            table.insert(lines, "|cFF888888Click Refresh to load AI data|r")
        else
            table.insert(lines, "|cFF888888Target an NPC to see AI info|r")
        end
        return table.concat(lines, "\n")
    end
    
    -- Scripts & AI
    if sData.scripts then
        table.insert(lines, "|cFFFFD700Scripts & AI|r")
        if sData.scripts.aiName and sData.scripts.aiName ~= "" then
            table.insert(lines, string.format("  |cFF00FF00AI:|r %s", sData.scripts.aiName))
        else
            table.insert(lines, "  |cFF00FF00AI:|r None")
        end
        if sData.scripts.scriptName and sData.scripts.scriptName ~= "" then
            table.insert(lines, string.format("  |cFF00FF00Script:|r %s", sData.scripts.scriptName))
        end
        table.insert(lines, "")
    end
    
    -- Movement
    if sData.movement then
        table.insert(lines, "|cFFFFD700Movement|r")
        
        -- Decode movement types
        local moveTypes = {
            [0] = "Idle (stationary)",
            [1] = "Random",
            [2] = "Waypoint Path"
        }
        local defaultType = sData.movement.defaultType or 0
        local currentType = sData.movement.currentType or 0
        
        table.insert(lines, string.format("  |cFF00FF00Default:|r %s", moveTypes[defaultType] or ("Type " .. defaultType)))
        
        -- More specific current movement types
        local currentMoveTypes = {
            [0] = "Idle",
            [1] = "Random",
            [2] = "Waypoint",
            [4] = "Confused",
            [5] = "Chasing",
            [6] = "Fleeing",
            [7] = "Distracted",
            [8] = "Following",
            [9] = "Rotating"
        }
        table.insert(lines, string.format("  |cFF00FF00Current:|r %s", currentMoveTypes[currentType] or ("Type " .. currentType)))
        
        if sData.movement.currentWaypointId and sData.movement.currentWaypointId > 0 then
            table.insert(lines, string.format("  |cFF00FF00Current WP:|r #%d", sData.movement.currentWaypointId))
        end
        
        -- Waypoint path details
        if sData.movement.waypointPath then
            local wp = sData.movement.waypointPath
            table.insert(lines, string.format("  |cFF00FF00Path ID:|r %d (%d nodes)", wp.pathId or 0, wp.nodeCount or 0))
            
            local moveTypes = {[0] = "Walk", [1] = "Run", [2] = "Land", [3] = "Take Off"}
            table.insert(lines, string.format("  |cFF00FF00Move Type:|r %s", moveTypes[wp.moveType] or "Unknown"))
            
            -- Show first few waypoint coordinates
            if wp.nodes and #wp.nodes > 0 then
                table.insert(lines, "  |cFF888888Waypoints:|r")
                local maxShow = math.min(5, #wp.nodes)
                for i = 1, maxShow do
                    local node = wp.nodes[i]
                    local marker = (node.id == wp.currentNodeId) and " |cFF00FF00<--|r" or ""
                    table.insert(lines, string.format("    #%d: (%.0f, %.0f, %.0f)%s", 
                        node.id, node.x, node.y, node.z, marker))
                end
                if #wp.nodes > maxShow then
                    table.insert(lines, string.format("    |cFF888888... and %d more|r", #wp.nodes - maxShow))
                end
            end
        end
        table.insert(lines, "")
    end
    
    -- Behavior
    if sData.behavior then
        table.insert(lines, "|cFFFFD700Behavior|r")
        table.insert(lines, string.format("  |cFF00FF00Respawn:|r %d sec", sData.behavior.respawnDelay or 0))
        if sData.behavior.wanderRadius and sData.behavior.wanderRadius > 0 then
            table.insert(lines, string.format("  |cFF00FF00Wander Radius:|r %.1f yards", sData.behavior.wanderRadius))
        end
        if sData.behavior.isCivilian then
            table.insert(lines, "  |cFF888888Civilian (non-aggro)|r")
        end
        table.insert(lines, "")
    end
    
    -- Template (with decoded flags)
    if sData.template then
        table.insert(lines, "|cFFFFD700Template|r")
        if sData.template.unitClass then
            local classes = {[1] = "Warrior", [2] = "Paladin", [4] = "Rogue", [8] = "Mage"}
            table.insert(lines, string.format("  |cFF00FF00Class:|r %s", classes[sData.template.unitClass] or sData.template.unitClass))
        end
        table.insert(lines, "")
        
        -- Decoded flags (using ServerData module's decoder)
        if ATA.ServerData and ATA.ServerData.DecodeFlags then
            local services = ATA.ServerData:DecodeNPCFlags(sData.template.npcFlags)
            if services then
                table.insert(lines, "  |cFF00FF00Services:|r " .. services)
            end
            local behaviors = ATA.ServerData:DecodeUnitFlags(sData.template.unitFlags)
            if behaviors then
                table.insert(lines, "  |cFF00FF00Behaviors:|r " .. behaviors)
            end
            local props = ATA.ServerData:DecodeExtraFlags(sData.template.extraFlags)
            if props then
                table.insert(lines, "  |cFF00FF00Properties:|r " .. props)
            end
        else
            -- Fallback to hex display
            if sData.template.npcFlags and sData.template.npcFlags > 0 then
                table.insert(lines, string.format("  |cFF00FF00NPC Flags:|r 0x%X", sData.template.npcFlags))
            end
            if sData.template.unitFlags and sData.template.unitFlags > 0 then
                table.insert(lines, string.format("  |cFF00FF00Unit Flags:|r 0x%X", sData.template.unitFlags))
            end
            if sData.template.extraFlags and sData.template.extraFlags > 0 then
                table.insert(lines, string.format("  |cFF00FF00Extra Flags:|r 0x%X", sData.template.extraFlags))
            end
        end
    end
    
    return table.concat(lines, "\n")
end

-- ============================================================================
-- Update Functions
-- ============================================================================

local function UpdateContentSize(frame)
    local scrollWidth = frame.scroll:GetWidth()
    if scrollWidth > 0 then
        frame.text:SetWidth(scrollWidth - 40)
    end
    local textHeight = frame.text:GetStringHeight()
    frame.child:SetWidth(math.max(scrollWidth, 1))
    frame.child:SetHeight(math.max(textHeight + 10, frame.scroll:GetHeight()))
end

function npcPanel:Update(requestServerData)
    local npcData = ATA:GetTargetNPCInfo()
    
    -- Update Basic tab
    contentFrames["Basic"].text:SetText(FormatBasicTab(npcData))
    UpdateContentSize(contentFrames["Basic"])
    
    -- Update Stats tab
    contentFrames["Stats"].text:SetText(FormatStatsTab(npcData, serverData))
    UpdateContentSize(contentFrames["Stats"])
    
    -- Update AI tab
    contentFrames["AI"].text:SetText(FormatAITab(npcData, serverData))
    UpdateContentSize(contentFrames["AI"])
    
    -- Request server data if needed
    if requestServerData and npcData and npcData.guid and ATA.ServerData then
        isLoadingServerData = true
        serverData = nil
        
        -- Update displays to show loading
        contentFrames["Stats"].text:SetText(FormatStatsTab(npcData, nil))
        contentFrames["AI"].text:SetText(FormatAITab(npcData, nil))
        
        ATA.ServerData:RequestNPCData(npcData.guid, function(data, error)
            isLoadingServerData = false
            if error then
                serverData = { success = false, error = error }
            else
                serverData = data
            end
            npcPanel:Update(false)
            
            -- Enable waypoint button if creature has a waypoint path
            if data and data.movement and data.movement.waypointPath then
                waypointButton:Enable()
            else
                waypointButton:Disable()
            end
        end)
    end
    
    -- Update 3D model
    if npcData and UnitExists("target") then
        npcModel:SetUnit("target")
        npcModel:SetCamDistanceScale(1.5)
        npcModel:SetRotation(0.61)
    else
        npcModel:ClearModel()
        npcModel:SetModel("interface/buttons/talktomequestionmark.m2")
    end
end

-- ============================================================================
-- Event Handlers
-- ============================================================================

refreshButton:SetScript("OnClick", function()
    npcPanel:Update(true)
end)

deleteButton:SetScript("OnClick", function()
    if UnitExists("target") and not UnitIsPlayer("target") then
        local npcData = ATA:GetTargetNPCInfo()
        if npcData and npcData.npcID then
            SendChatMessage(".npc delete", "GUILD")
            print(string.format("|cFF00FF00[ATA]|r Deleted NPC: %s (ID: %s)", npcData.name or "Unknown", npcData.npcID))
            C_Timer.After(0.5, function() npcPanel:Update() end)
        end
    else
        print("|cFFFF0000[ATA]|r Target an NPC first.")
    end
end)

-- Waypoint visualization toggle
waypointButton:SetScript("OnClick", function()
    if not AMS then
        print("|cFFFF0000[ATA]|r AMS not available")
        return
    end
    
    local npcData = ATA:GetTargetNPCInfo()
    if not npcData or not npcData.guid then
        print("|cFFFF0000[ATA]|r Target an NPC first.")
        return
    end
    
    currentTargetGUID = npcData.guid
    
    if waypointsVisible then
        -- Hide waypoints
        print("|cFF00FF00[ATA]|r Hiding waypoint markers...")
        waypointButton:SetText("Working...")
        waypointButton:Disable()
        AMS.Send("HIDE_WAYPOINTS", { guid = npcData.guid })
    else
        -- Show waypoints
        print("|cFF00FF00[ATA]|r Spawning waypoint markers in 3D space...")
        waypointButton:SetText("Working...")
        waypointButton:Disable()
        AMS.Send("SHOW_WAYPOINTS", { guid = npcData.guid })
    end
end)

-- Handle waypoint response from server
local function InitWaypointHandler()
    if not AMS then
        C_Timer.After(0.5, InitWaypointHandler)
        return
    end
    
    AMS.RegisterHandler("WAYPOINTS_RESPONSE", function(data)
        if data.success then
            waypointsVisible = not waypointsVisible
            waypointButton:SetText(waypointsVisible and "Hide Waypoints" or "Show Waypoints")
            waypointButton:Enable()
            print("|cFF00FF00[ATA]|r " .. (data.message or "Done"))
        else
            waypointButton:SetText(waypointsVisible and "Hide Waypoints" or "Show Waypoints")
            waypointButton:Enable()
            print("|cFFFF0000[ATA]|r " .. (data.error or "Failed"))
        end
    end)
end
InitWaypointHandler()

local updateFrame = CreateFrame("Frame")
updateFrame:RegisterEvent("PLAYER_TARGET_CHANGED")
updateFrame:SetScript("OnEvent", function()
    if npcPanel:IsShown() then
        serverData = nil
        isLoadingServerData = false
        -- Reset waypoint state on target change
        waypointsVisible = false
        waypointButton:SetText("Show Waypoints")
        waypointButton:Disable()
        npcPanel:Update(true)
    end
end)

-- ============================================================================
-- Initialization
-- ============================================================================

ShowTab("Basic")  -- Default tab

local function InitPanel()
    if ATA.MainWindow then
        ATA.MainWindow:RegisterPanel("NPCInfo", "NPC Info", npcPanel)
        if AraxiaTrinityAdminDB and AraxiaTrinityAdminDB.selectedPanel then
            ATA.MainWindow:ShowPanel(AraxiaTrinityAdminDB.selectedPanel)
        else
            ATA.MainWindow:ShowPanel("NPCInfo")
        end
    else
        C_Timer.After(0.1, InitPanel)
    end
end

C_Timer.After(0.1, InitPanel)

end) -- End of ADDON_LOADED handler
