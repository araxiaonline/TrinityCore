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

-- GM Toggle Button (top right)
local gmButton = CreateFrame("Button", nil, npcPanel, "UIPanelButtonTemplate, BackdropTemplate")
gmButton:SetSize(50, 22)
gmButton:SetPoint("TOPRIGHT", npcPanel, "TOPRIGHT", -10, -10)
gmButton:SetText("GM")

local function UpdateGMButton()
    -- Check if player is in GM mode
    local isGM = _G.AraxiaTrinityAdminGMMode or false
    if isGM then
        -- Hide button's normal textures so backdrop shows through
        if gmButton.Left then gmButton.Left:Hide() end
        if gmButton.Right then gmButton.Right:Hide() end
        if gmButton.Middle then gmButton.Middle:Hide() end
        if gmButton:GetNormalTexture() then gmButton:GetNormalTexture():SetAlpha(0) end
        if gmButton:GetPushedTexture() then gmButton:GetPushedTexture():SetAlpha(0) end
        
        -- Green background when GM mode is on
        gmButton:SetBackdrop({
            bgFile = "Interface/Buttons/WHITE8x8",
            edgeFile = "Interface/Tooltips/UI-Tooltip-Border",
            tile = true, tileSize = 8, edgeSize = 8,
            insets = { left = 2, right = 2, top = 2, bottom = 2 }
        })
        gmButton:SetBackdropColor(0, 0.6, 0, 0.8)  -- Green background
        gmButton:SetBackdropBorderColor(0, 1, 0, 1)  -- Green border
    else
        -- Restore button's normal textures
        if gmButton.Left then gmButton.Left:Show() end
        if gmButton.Right then gmButton.Right:Show() end
        if gmButton.Middle then gmButton.Middle:Show() end
        if gmButton:GetNormalTexture() then gmButton:GetNormalTexture():SetAlpha(1) end
        if gmButton:GetPushedTexture() then gmButton:GetPushedTexture():SetAlpha(1) end
        
        -- Default appearance when GM mode is off
        gmButton:SetBackdrop(nil)
    end
end

-- Function to sync GM state from server
local function SyncGMState()
    if not AMS then return end
    
    -- Request current GM state from server
    AMS.Send("GET_PLAYER_DATA", {})
end

-- Handler for player data response
if AMS then
    AMS.RegisterHandler("PLAYER_DATA_RESPONSE", function(data)
        if data and data.success and data.isGM ~= nil then
            _G.AraxiaTrinityAdminGMMode = data.isGM
            UpdateGMButton()
        end
    end)
end

gmButton:SetScript("OnClick", function()
    -- Toggle GM mode
    _G.AraxiaTrinityAdminGMMode = not _G.AraxiaTrinityAdminGMMode
    if _G.AraxiaTrinityAdminGMMode then
        SendChatMessage(".gm on", "GUILD")
        print("|cFF00FF00[ATA]|r GM mode enabled")
    else
        SendChatMessage(".gm off", "GUILD")
        print("|cFF00FF00[ATA]|r GM mode disabled")
    end
    UpdateGMButton()
end)

gmButton:SetScript("OnEnter", function(self)
    GameTooltip:SetOwner(self, "ANCHOR_BOTTOM")
    GameTooltip:AddLine("Toggle GM Mode", 1, 1, 1)
    GameTooltip:AddLine("Click to toggle .gm on/off", 0.7, 0.7, 0.7)
    GameTooltip:Show()
end)

gmButton:SetScript("OnLeave", function()
    GameTooltip:Hide()
end)

UpdateGMButton()

-- Sync GM state on panel show
npcPanel:HookScript("OnShow", function()
    SyncGMState()
end)

-- Initial sync
SyncGMState()

-- ============================================================================
-- Tab Buttons
-- ============================================================================

local tabContainer = CreateFrame("Frame", nil, npcPanel)
tabContainer:SetPoint("TOPLEFT", title, "BOTTOMLEFT", 0, -8)
tabContainer:SetPoint("RIGHT", npcPanel, "CENTER", -10, 0)
tabContainer:SetHeight(28)

local tabs = {}
local tabNames = {"Basic", "Stats", "AI", "Raw"}

local function CreateTab(name, index)
    local tab = CreateFrame("Button", nil, tabContainer, "UIPanelButtonTemplate, BackdropTemplate")
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

-- Helper function to style a tab as active (green) or inactive (normal)
local function StyleTabActive(tab, isActive)
    if isActive then
        -- Hide button's normal textures so backdrop shows through
        if tab.Left then tab.Left:Hide() end
        if tab.Right then tab.Right:Hide() end
        if tab.Middle then tab.Middle:Hide() end
        if tab:GetNormalTexture() then tab:GetNormalTexture():SetAlpha(0) end
        if tab:GetPushedTexture() then tab:GetPushedTexture():SetAlpha(0) end
        
        tab:SetBackdrop({
            bgFile = "Interface/Buttons/WHITE8x8",
            edgeFile = "Interface/Tooltips/UI-Tooltip-Border",
            tile = true, tileSize = 8, edgeSize = 8,
            insets = { left = 2, right = 2, top = 2, bottom = 2 }
        })
        tab:SetBackdropColor(0, 0.6, 0, 0.8)  -- Green background
        tab:SetBackdropBorderColor(0, 1, 0, 1)  -- Green border
    else
        -- Restore button's normal textures
        if tab.Left then tab.Left:Show() end
        if tab.Right then tab.Right:Show() end
        if tab.Middle then tab.Middle:Show() end
        if tab:GetNormalTexture() then tab:GetNormalTexture():SetAlpha(1) end
        if tab:GetPushedTexture() then tab:GetPushedTexture():SetAlpha(1) end
        
        tab:SetBackdrop(nil)
    end
end

for i, name in ipairs(tabNames) do
    CreateTab(name, i)
end

-- ============================================================================
-- Content Frames (one per tab)
-- ============================================================================

local function CreateScrollableContent(parent, useEditBox)
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
    
    local text
    if useEditBox then
        -- Use EditBox for large text (like Raw tab)
        text = CreateFrame("EditBox", nil, child)
        text:SetPoint("TOPLEFT", child, "TOPLEFT", 5, -5)
        text:SetPoint("RIGHT", child, "RIGHT", -5, 0)
        text:SetFontObject("GameFontHighlight")
        text:SetMultiLine(true)
        text:SetAutoFocus(false)
        text:EnableMouse(true)
        text:SetHyperlinksEnabled(false)
        text:SetTextInsets(0, 0, 0, 0)
        text:SetScript("OnEscapePressed", function(self) self:ClearFocus() end)
        -- Make it read-only looking but still scrollable/selectable
        text:SetScript("OnEditFocusGained", function(self) self:HighlightText(0, 0) end)
    else
        text = child:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
        text:SetPoint("TOPLEFT", child, "TOPLEFT", 5, -5)
        text:SetJustifyH("LEFT")
        text:SetJustifyV("TOP")
        text:SetWidth(280)
    end
    
    frame.scroll = scroll
    frame.child = child
    frame.text = text
    frame.useEditBox = useEditBox
    
    return frame
end

local contentFrames = {}
for _, name in ipairs(tabNames) do
    -- Use EditBox for Raw tab to handle large text
    local useEditBox = (name == "Raw")
    contentFrames[name] = CreateScrollableContent(npcPanel, useEditBox)
end

-- ============================================================================
-- Right side: Tabbed panel (3D Model / Waypoints / Paths)
-- ============================================================================

-- Current right tab
local currentRightTab = "3D Model"

-- Tracked paths storage: { [creatureGUID] = { name, entry, pathId, nodeCount } }
-- Initialized from SavedVariables below
local trackedPaths = {}

-- Function to save tracked paths to SavedVariables
local function SaveTrackedPaths()
    if AraxiaTrinityAdminDB then
        AraxiaTrinityAdminDB.trackedPaths = trackedPaths
    end
end

-- Function to load tracked paths from SavedVariables
local function LoadTrackedPaths()
    if AraxiaTrinityAdminDB and AraxiaTrinityAdminDB.trackedPaths then
        trackedPaths = AraxiaTrinityAdminDB.trackedPaths
    end
end

-- Right side tab container (positioned relative to the left tabs)
local rightTabContainer = CreateFrame("Frame", nil, npcPanel)
rightTabContainer:SetPoint("TOPLEFT", tabContainer, "TOPRIGHT", 15, 0)
rightTabContainer:SetPoint("RIGHT", npcPanel, "RIGHT", -10, 0)
rightTabContainer:SetHeight(28)

-- Right side tabs
local rightTabs = {}
local rightTabNames = {"3D Model", "Waypoints", "Paths"}

local function CreateRightTab(name, index)
    local tab = CreateFrame("Button", nil, rightTabContainer, "UIPanelButtonTemplate, BackdropTemplate")
    tab:SetSize(80, 24)
    tab:SetText(name)
    if index == 1 then
        tab:SetPoint("LEFT", rightTabContainer, "LEFT", 0, 0)
    else
        tab:SetPoint("LEFT", rightTabs[index-1], "RIGHT", 2, 0)
    end
    rightTabs[index] = tab
    return tab
end

for i, name in ipairs(rightTabNames) do
    CreateRightTab(name, i)
end

-- Right side content container (shared backdrop)
local rightContentFrame = CreateFrame("Frame", nil, npcPanel, "BackdropTemplate")
rightContentFrame:SetPoint("TOPLEFT", rightTabContainer, "BOTTOMLEFT", 0, -5)
rightContentFrame:SetPoint("BOTTOMRIGHT", npcPanel, "BOTTOMRIGHT", -10, 10)
rightContentFrame:SetBackdrop({
    bgFile = "Interface/Tooltips/UI-Tooltip-Background",
    edgeFile = "Interface/Tooltips/UI-Tooltip-Border",
    tile = true, tileSize = 16, edgeSize = 16,
    insets = { left = 4, right = 4, top = 4, bottom = 4 }
})
rightContentFrame:SetBackdropColor(0, 0, 0, 0.5)
rightContentFrame:SetBackdropBorderColor(0.4, 0.4, 0.4, 1)

-- ============================================================================
-- Right Tab Content: 3D Model
-- ============================================================================

local modelContent = CreateFrame("Frame", nil, rightContentFrame)
modelContent:SetAllPoints(rightContentFrame)

local modelTitle = modelContent:CreateFontString(nil, "OVERLAY", "GameFontNormal")
modelTitle:SetPoint("TOP", modelContent, "TOP", 0, -8)
modelTitle:SetText("3D Model")

local npcModel = CreateFrame("PlayerModel", nil, modelContent)
npcModel:SetPoint("TOPLEFT", modelContent, "TOPLEFT", 8, -30)
npcModel:SetPoint("BOTTOMRIGHT", modelContent, "BOTTOMRIGHT", -8, 8)
npcModel:SetCamDistanceScale(1.5)
npcModel:SetRotation(0.61)

-- Model zoom state
local modelZoom = 1.5
local modelRotation = 0.61

-- Scroll wheel to zoom the model
npcModel:EnableMouseWheel(true)
npcModel:SetScript("OnMouseWheel", function(self, delta)
    modelZoom = modelZoom - (delta * 0.2)
    modelZoom = math.max(0.3, math.min(5.0, modelZoom))  -- Clamp between 0.3 and 5.0
    self:SetCamDistanceScale(modelZoom)
end)

-- Click and drag to rotate the model (horizontal only)
local isDragging = false
local dragStartX = 0

npcModel:EnableMouse(true)
npcModel:SetScript("OnMouseDown", function(self, button)
    if button == "LeftButton" then
        isDragging = true
        dragStartX = GetCursorPosition()
    end
end)

npcModel:SetScript("OnMouseUp", function(self, button)
    if button == "LeftButton" then
        isDragging = false
    end
end)

npcModel:SetScript("OnUpdate", function(self, elapsed)
    if isDragging then
        local currentX = GetCursorPosition()
        local deltaX = (currentX - dragStartX) * 0.01
        modelRotation = modelRotation + deltaX
        self:SetRotation(modelRotation)
        dragStartX = currentX
    end
end)

-- Tooltip for model interaction hints
npcModel:SetScript("OnEnter", function(self)
    GameTooltip:SetOwner(self, "ANCHOR_TOPLEFT")
    GameTooltip:AddLine("3D Model", 1, 1, 1)
    GameTooltip:AddLine("Scroll to zoom", 0.7, 0.7, 0.7)
    GameTooltip:AddLine("Drag to rotate", 0.7, 0.7, 0.7)
    GameTooltip:Show()
end)

npcModel:SetScript("OnLeave", function(self)
    GameTooltip:Hide()
    isDragging = false  -- Stop dragging if mouse leaves
end)

-- ============================================================================
-- Right Tab Content: Waypoints
-- ============================================================================

local waypointContent = CreateFrame("Frame", nil, rightContentFrame)
waypointContent:SetAllPoints(rightContentFrame)
waypointContent:Hide()

-- Waypoint data storage
local currentWaypointPathId = nil
local currentWaypointNodes = {}
local selectedWaypointNodeId = nil
local panelLockedToNPC = false  -- Lock panel to NPC when showing waypoints

-- Waypoint list (top half - 60% of height)
local waypointListContainer = CreateFrame("Frame", nil, waypointContent)
waypointListContainer:SetPoint("TOPLEFT", waypointContent, "TOPLEFT", 8, -8)
waypointListContainer:SetPoint("RIGHT", waypointContent, "RIGHT", -8, 0)
waypointListContainer:SetHeight(200)

local waypointListScroll = CreateFrame("ScrollFrame", nil, waypointListContainer, "UIPanelScrollFrameTemplate")
waypointListScroll:SetPoint("TOPLEFT", waypointListContainer, "TOPLEFT", 0, 0)
waypointListScroll:SetPoint("BOTTOMRIGHT", waypointListContainer, "BOTTOMRIGHT", -22, 0)

local waypointListChild = CreateFrame("Frame", nil, waypointListScroll)
waypointListChild:SetWidth(280)
waypointListChild:SetHeight(100)
waypointListScroll:SetScrollChild(waypointListChild)

-- Divider line between list and detail
local waypointDivider = waypointContent:CreateTexture(nil, "ARTWORK")
waypointDivider:SetHeight(1)
waypointDivider:SetPoint("TOPLEFT", waypointListContainer, "BOTTOMLEFT", 0, -4)
waypointDivider:SetPoint("RIGHT", waypointContent, "RIGHT", -8, 0)
waypointDivider:SetColorTexture(0.4, 0.4, 0.4, 0.8)

-- Detail panel (bottom half - always visible when waypoint selected)
local waypointDetailPanel = CreateFrame("Frame", nil, waypointContent, "BackdropTemplate")
waypointDetailPanel:SetPoint("TOPLEFT", waypointDivider, "BOTTOMLEFT", 0, -4)
waypointDetailPanel:SetPoint("BOTTOMRIGHT", waypointContent, "BOTTOMRIGHT", -8, 8)
waypointDetailPanel:SetBackdrop({
    bgFile = "Interface/Tooltips/UI-Tooltip-Background",
    edgeFile = "Interface/Tooltips/UI-Tooltip-Border",
    tile = true, tileSize = 16, edgeSize = 12,
    insets = { left = 3, right = 3, top = 3, bottom = 3 }
})
waypointDetailPanel:SetBackdropColor(0.1, 0.1, 0.1, 0.8)
waypointDetailPanel:SetBackdropBorderColor(0.3, 0.3, 0.3, 1)

-- Detail panel content
local detailTitle = waypointDetailPanel:CreateFontString(nil, "OVERLAY", "GameFontNormal")
detailTitle:SetPoint("TOPLEFT", waypointDetailPanel, "TOPLEFT", 10, -8)
detailTitle:SetText("Select a waypoint above")

-- Detail info text
local detailInfoText = waypointDetailPanel:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
detailInfoText:SetPoint("TOPLEFT", waypointDetailPanel, "TOPLEFT", 10, -26)
detailInfoText:SetPoint("RIGHT", waypointDetailPanel, "RIGHT", -80, 0)
detailInfoText:SetJustifyH("LEFT")
detailInfoText:SetJustifyV("TOP")
detailInfoText:SetText("")

-- Action buttons container (right side of detail panel)
local detailActionsContainer = CreateFrame("Frame", nil, waypointDetailPanel)
detailActionsContainer:SetPoint("TOPRIGHT", waypointDetailPanel, "TOPRIGHT", -8, -8)
detailActionsContainer:SetSize(65, 100)

-- Teleport button
local teleportBtn = CreateFrame("Button", nil, detailActionsContainer, "UIPanelButtonTemplate")
teleportBtn:SetSize(60, 22)
teleportBtn:SetPoint("TOP", detailActionsContainer, "TOP", 0, 0)
teleportBtn:SetText("Teleport")
teleportBtn:Disable()

teleportBtn:SetScript("OnClick", function()
    if not selectedWaypointNodeId or not currentWaypointPathId then return end
    
    -- Find the selected node data
    local node = nil
    for _, n in ipairs(currentWaypointNodes) do
        if n.id == selectedWaypointNodeId then
            node = n
            break
        end
    end
    
    if node and AMS then
        print("|cFF00FF00[ATA]|r Teleporting to waypoint " .. node.id)
        AMS.Send("TELEPORT_TO_WAYPOINT", {
            x = node.x,
            y = node.y,
            z = node.z,
            orientation = node.orientation
        })
    end
end)

teleportBtn:SetScript("OnEnter", function(self)
    GameTooltip:SetOwner(self, "ANCHOR_LEFT")
    GameTooltip:AddLine("Teleport to Waypoint", 1, 1, 1)
    GameTooltip:AddLine("Teleports you to this waypoint location", 0.7, 0.7, 0.7)
    GameTooltip:Show()
end)

teleportBtn:SetScript("OnLeave", function()
    GameTooltip:Hide()
end)

-- Buttons to hold waypoint node entries
local waypointNodeButtons = {}

-- Forward declare functions
local UpdateWaypointList
local UpdateWaypointDetail

-- Function to update waypoint detail display
UpdateWaypointDetail = function()
    if not selectedWaypointNodeId then
        detailTitle:SetText("Select a waypoint above")
        detailInfoText:SetText("")
        teleportBtn:Disable()
        return
    end
    
    local node = nil
    for _, n in ipairs(currentWaypointNodes) do
        if n.id == selectedWaypointNodeId then
            node = n
            break
        end
    end
    
    if not node then 
        teleportBtn:Disable()
        return 
    end
    
    detailTitle:SetText("Node " .. node.id)
    
    local detailStr = string.format(
        "|cFFFFD700Position:|r %.1f, %.1f, %.1f\n" ..
        "|cFFFFD700Orientation:|r %.2f\n" ..
        "|cFFFFD700Delay:|r %dms\n" ..
        "|cFFFFD700Move Type:|r %d",
        node.x, node.y, node.z,
        node.orientation,
        node.delay,
        node.moveType
    )
    
    detailInfoText:SetText(detailStr)
    teleportBtn:Enable()
end

-- Function to update waypoint list display
UpdateWaypointList = function()
    print("|cFF00FF00[ATA]|r UpdateWaypointList called: pathId=" .. (currentWaypointPathId or 0) .. ", nodes=" .. #currentWaypointNodes)
    
    -- Hide all existing buttons
    for _, btn in ipairs(waypointNodeButtons) do
        btn:Hide()
    end
    
    if not currentWaypointPathId or #currentWaypointNodes == 0 then
        -- Show empty message
        print("|cFFFF0000[ATA]|r No waypoint data to display")
        waypointListChild:SetHeight(40)
        return
    end
    
    -- Create/update buttons for each node
    for i, node in ipairs(currentWaypointNodes) do
        local btn = waypointNodeButtons[i]
        if not btn then
            btn = CreateFrame("Button", nil, waypointListChild)
            btn:SetHeight(24)
            btn:EnableMouse(true)
            btn:RegisterForClicks("AnyUp")
            btn:SetHighlightTexture("Interface/QuestFrame/UI-QuestTitleHighlight", "ADD")
            
            btn.bg = btn:CreateTexture(nil, "BACKGROUND")
            btn.bg:SetAllPoints()
            btn.bg:SetColorTexture(0.2, 0.2, 0.2, 0.5)
            
            btn.text = btn:CreateFontString(nil, "OVERLAY", "GameFontNormal")
            btn.text:SetPoint("LEFT", btn, "LEFT", 8, 0)
            btn.text:SetJustifyH("LEFT")
            
            btn:SetScript("OnClick", function(self)
                local clickedNode = self.nodeData
                if not clickedNode then return end
                
                selectedWaypointNodeId = clickedNode.id
                UpdateWaypointDetail()
                UpdateWaypointList()  -- Refresh to show selection highlight
                
                -- Send SELECT_WAYPOINT to server to highlight in world
                if AMS and currentWaypointPathId then
                    print("|cFF00FF00[ATA]|r Sending SELECT_WAYPOINT: path=" .. currentWaypointPathId .. " node=" .. clickedNode.id)
                    AMS.Send("SELECT_WAYPOINT", { pathId = currentWaypointPathId, nodeId = clickedNode.id })
                end
            end)
            
            waypointNodeButtons[i] = btn
        end
        
        btn.nodeData = node
        btn:ClearAllPoints()
        btn:SetPoint("TOPLEFT", waypointListChild, "TOPLEFT", 0, -((i-1) * 26))
        btn:SetPoint("RIGHT", waypointListChild, "RIGHT", 0, 0)
        
        -- Highlight selected node
        if node.id == selectedWaypointNodeId then
            btn.bg:SetColorTexture(0.3, 0.5, 0.3, 0.7)
            btn.text:SetTextColor(1, 1, 1, 1)
        else
            btn.bg:SetColorTexture(0.2, 0.2, 0.2, 0.5)
            btn.text:SetTextColor(1, 0.82, 0, 1)
        end
        
        btn.text:SetText("Node " .. node.id)
        btn:Show()
    end
    
    -- Update scroll child size to fit all buttons
    local totalHeight = #currentWaypointNodes * 26
    waypointListChild:SetHeight(totalHeight)
    print("|cFF00FF00[ATA]|r Set scroll child height to " .. totalHeight)
end

-- Deselect button no longer needed in split view layout
-- (kept for code compatibility but hidden)

-- ============================================================================
-- Wander Settings Content (shown for Random movement type instead of waypoints)
-- ============================================================================

local wanderSettingsContainer = CreateFrame("Frame", nil, waypointContent, "BackdropTemplate")
wanderSettingsContainer:SetAllPoints(waypointContent)
wanderSettingsContainer:SetBackdrop({
    bgFile = "Interface/Tooltips/UI-Tooltip-Background",
    tile = true, tileSize = 16,
    insets = { left = 4, right = 4, top = 4, bottom = 4 }
})
wanderSettingsContainer:SetBackdropColor(0.05, 0.05, 0.05, 0.5)
wanderSettingsContainer:Hide()

local wanderTitle = wanderSettingsContainer:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
wanderTitle:SetPoint("TOP", wanderSettingsContainer, "TOP", 0, -15)
wanderTitle:SetText("|cFFFFD700Random Movement Settings|r")

local wanderDesc = wanderSettingsContainer:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
wanderDesc:SetPoint("TOP", wanderTitle, "BOTTOM", 0, -10)
wanderDesc:SetWidth(280)
wanderDesc:SetJustifyH("CENTER")
wanderDesc:SetText("This creature uses Random movement.\nAdjust the wander distance below:")

-- Current wander distance display
local wanderCurrentLabel = wanderSettingsContainer:CreateFontString(nil, "OVERLAY", "GameFontNormal")
wanderCurrentLabel:SetPoint("TOPLEFT", wanderSettingsContainer, "TOPLEFT", 20, -90)
wanderCurrentLabel:SetText("|cFF00FF00Current Wander Distance:|r")

local wanderCurrentValue = wanderSettingsContainer:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
wanderCurrentValue:SetPoint("LEFT", wanderCurrentLabel, "RIGHT", 8, 0)
wanderCurrentValue:SetText("0 yards")

-- Edit section
local wanderEditLabel = wanderSettingsContainer:CreateFontString(nil, "OVERLAY", "GameFontNormal")
wanderEditLabel:SetPoint("TOPLEFT", wanderCurrentLabel, "BOTTOMLEFT", 0, -20)
wanderEditLabel:SetText("|cFF00FF00New Distance (yards):|r")

local wanderEditBox = CreateFrame("EditBox", nil, wanderSettingsContainer, "InputBoxTemplate")
wanderEditBox:SetSize(60, 24)
wanderEditBox:SetPoint("LEFT", wanderEditLabel, "RIGHT", 10, 0)
wanderEditBox:SetAutoFocus(false)
wanderEditBox:SetNumeric(false)  -- Allow decimal input
wanderEditBox:SetMaxLetters(6)
wanderEditBox:SetText("5")

-- Save button
local wanderSaveBtn = CreateFrame("Button", nil, wanderSettingsContainer, "UIPanelButtonTemplate")
wanderSaveBtn:SetSize(100, 26)
wanderSaveBtn:SetPoint("TOPLEFT", wanderEditLabel, "BOTTOMLEFT", 0, -20)
wanderSaveBtn:SetText("Save")
-- Helper function to disable all wander setting buttons while waiting for server
local function DisableWanderButtons()
    wanderSaveBtn:Disable()
    -- Other buttons will be defined below, we'll call this after they exist
end

-- Helper function to re-enable all wander setting buttons
local function EnableWanderButtons()
    wanderSaveBtn:Enable()
    if spawnMarkerBtn then spawnMarkerBtn:Enable() end
    if wanderRadiusBtn then wanderRadiusBtn:Enable() end
    if clearAllBtn then clearAllBtn:Enable() end
    if clearNearbyBtn then clearNearbyBtn:Enable() end
end

-- Forward declare buttons so DisableWanderButtons can reference them
local spawnMarkerBtn, wanderRadiusBtn, clearAllBtn, clearNearbyBtn

-- Update DisableWanderButtons to use the forward declarations
DisableWanderButtons = function()
    wanderSaveBtn:Disable()
    if spawnMarkerBtn then spawnMarkerBtn:Disable() end
    if wanderRadiusBtn then wanderRadiusBtn:Disable() end
    if clearAllBtn then clearAllBtn:Disable() end
    if clearNearbyBtn then clearNearbyBtn:Disable() end
end

-- Update EnableWanderButtons to use the forward declarations
EnableWanderButtons = function()
    wanderSaveBtn:Enable()
    if spawnMarkerBtn then spawnMarkerBtn:Enable() end
    if wanderRadiusBtn then wanderRadiusBtn:Enable() end
    if clearAllBtn then clearAllBtn:Enable() end
    if clearNearbyBtn then clearNearbyBtn:Enable() end
end

wanderSaveBtn:SetScript("OnClick", function()
    local newDistance = tonumber(wanderEditBox:GetText())
    if not newDistance then
        print("|cFFFF0000[ATA]|r Invalid distance value")
        return
    end
    
    if newDistance < 0 or newDistance > 100 then
        print("|cFFFF0000[ATA]|r Distance must be between 0 and 100 yards")
        return
    end
    
    if AMS then
        DisableWanderButtons()
        AMS.Send("SET_WANDER_DISTANCE", { distance = newDistance })
        print("|cFF00FF00[ATA]|r Saving wander distance: " .. newDistance)
    end
end)

-- Spawn marker toggle button
local spawnMarkerVisible = false
spawnMarkerBtn = CreateFrame("Button", nil, wanderSettingsContainer, "UIPanelButtonTemplate")
spawnMarkerBtn:SetSize(140, 26)
spawnMarkerBtn:SetPoint("LEFT", wanderSaveBtn, "RIGHT", 10, 0)
spawnMarkerBtn:SetText("Show Spawn Point")
spawnMarkerBtn:SetScript("OnClick", function()
    if AMS then
        if spawnMarkerVisible then
            AMS.Send("HIDE_SPAWN_MARKER", {})
            spawnMarkerVisible = false
            spawnMarkerBtn:SetText("Show Spawn Point")
        else
            AMS.Send("SHOW_SPAWN_MARKER", {})
            spawnMarkerVisible = true
            spawnMarkerBtn:SetText("Hide Spawn Point")
        end
    end
end)
spawnMarkerBtn:SetScript("OnEnter", function(self)
    GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
    GameTooltip:AddLine("Spawn Point Marker", 1, 1, 1)
    GameTooltip:AddLine("Shows a marker at this creature's", 0.7, 0.7, 0.7)
    GameTooltip:AddLine("spawn/home position.", 0.7, 0.7, 0.7)
    GameTooltip:Show()
end)
spawnMarkerBtn:SetScript("OnLeave", function()
    GameTooltip:Hide()
end)

-- Wander radius visualization button
local wanderRadiusVisible = false
wanderRadiusBtn = CreateFrame("Button", nil, wanderSettingsContainer, "UIPanelButtonTemplate")
wanderRadiusBtn:SetSize(140, 26)
wanderRadiusBtn:SetPoint("TOPLEFT", spawnMarkerBtn, "BOTTOMLEFT", 0, -5)
wanderRadiusBtn:SetText("Show Radius")
wanderRadiusBtn:SetScript("OnClick", function()
    if AMS then
        if wanderRadiusVisible then
            AMS.Send("HIDE_WANDER_RADIUS", {})
            wanderRadiusVisible = false
            wanderRadiusBtn:SetText("Show Radius")
        else
            AMS.Send("SHOW_WANDER_RADIUS", { segments = 16 })
            wanderRadiusVisible = true
            wanderRadiusBtn:SetText("Hide Radius")
        end
    end
end)
wanderRadiusBtn:SetScript("OnEnter", function(self)
    GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
    GameTooltip:AddLine("Wander Radius Visualization", 1, 1, 1)
    GameTooltip:AddLine("Shows markers in a circle around", 0.7, 0.7, 0.7)
    GameTooltip:AddLine("the spawn point at the wander", 0.7, 0.7, 0.7)
    GameTooltip:AddLine("distance to visualize the area.", 0.7, 0.7, 0.7)
    GameTooltip:Show()
end)
wanderRadiusBtn:SetScript("OnLeave", function()
    GameTooltip:Hide()
end)

-- Clear All button - removes spawn marker and wander radius markers
clearAllBtn = CreateFrame("Button", nil, wanderSettingsContainer, "UIPanelButtonTemplate")
clearAllBtn:SetSize(140, 26)
clearAllBtn:SetPoint("TOPLEFT", wanderRadiusBtn, "BOTTOMLEFT", 0, -5)
clearAllBtn:SetText("Clear All Markers")
clearAllBtn:SetScript("OnClick", function()
    if AMS then
        AMS.Send("CLEAR_WANDER_MARKERS", {})
        -- Reset button states
        spawnMarkerVisible = false
        spawnMarkerBtn:SetText("Show Spawn Point")
        wanderRadiusVisible = false
        wanderRadiusBtn:SetText("Show Radius")
    end
end)
clearAllBtn:SetScript("OnEnter", function(self)
    GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
    GameTooltip:AddLine("Clear All Markers", 1, 1, 1)
    GameTooltip:AddLine("Removes both spawn point marker", 0.7, 0.7, 0.7)
    GameTooltip:AddLine("and wander radius markers.", 0.7, 0.7, 0.7)
    GameTooltip:Show()
end)
clearAllBtn:SetScript("OnLeave", function()
    GameTooltip:Hide()
end)

-- Clear Nearby (orphaned) markers button
clearNearbyBtn = CreateFrame("Button", nil, wanderSettingsContainer, "UIPanelButtonTemplate")
clearNearbyBtn:SetSize(140, 26)
clearNearbyBtn:SetPoint("TOPLEFT", clearAllBtn, "BOTTOMLEFT", 0, -5)
clearNearbyBtn:SetText("Clear Orphaned")
clearNearbyBtn:SetScript("OnClick", function()
    if AMS then
        AMS.Send("CLEAR_NEARBY_MARKERS", { range = 100 })
    end
end)
clearNearbyBtn:SetScript("OnEnter", function(self)
    GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
    GameTooltip:AddLine("Clear Orphaned Markers", 1, 0.5, 0)
    GameTooltip:AddLine("Removes ALL waypoint markers within", 0.7, 0.7, 0.7)
    GameTooltip:AddLine("100 yards of your character.", 0.7, 0.7, 0.7)
    GameTooltip:AddLine(" ", 0.7, 0.7, 0.7)
    GameTooltip:AddLine("Use this to clean up markers that", 1, 1, 0)
    GameTooltip:AddLine("were lost after a script reload.", 1, 1, 0)
    GameTooltip:Show()
end)
clearNearbyBtn:SetScript("OnLeave", function()
    GameTooltip:Hide()
end)

-- Help text
local wanderHelp = wanderSettingsContainer:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
wanderHelp:SetPoint("BOTTOMLEFT", wanderSettingsContainer, "BOTTOMLEFT", 20, 20)
wanderHelp:SetPoint("RIGHT", wanderSettingsContainer, "RIGHT", -20, 0)
wanderHelp:SetJustifyH("LEFT")
wanderHelp:SetText("|cFF888888Wander distance is the radius (in yards) the creature will\nrandomly move within from its spawn point.\nSet to 0 for no movement (idle).|r")

-- Handler for spawn marker response
if AMS then
    AMS.RegisterHandler("SPAWN_MARKER_RESPONSE", function(data)
        if data and data.success then
            if data.message and data.message:find("shown") then
                spawnMarkerVisible = true
                spawnMarkerBtn:SetText("Hide Spawn Point")
                print("|cFF00FF00[ATA]|r Spawn marker shown at " .. string.format("%.1f, %.1f, %.1f", data.x or 0, data.y or 0, data.z or 0))
            else
                spawnMarkerVisible = false
                spawnMarkerBtn:SetText("Show Spawn Point")
                print("|cFF00FF00[ATA]|r Spawn marker hidden")
            end
        else
            print("|cFFFF0000[ATA]|r Spawn marker error: " .. (data.error or "Unknown error"))
        end
    end)
end

-- Handler for wander distance save response
if AMS then
    AMS.RegisterHandler("SET_WANDER_DISTANCE_RESPONSE", function(data)
        EnableWanderButtons()
        if data and data.success then
            print("|cFF00FF00[ATA]|r Wander distance updated to " .. (data.newDistance or "?") .. " yards")
            wanderCurrentValue:SetText(string.format("%.1f yards", data.newDistance or 0))
            
            -- Remember what markers were visible BEFORE we cleared them
            local wasSpawnVisible = spawnMarkerVisible
            local wasRadiusVisible = wanderRadiusVisible
            
            -- Reset button states since markers were hidden by the save
            spawnMarkerVisible = false
            spawnMarkerBtn:SetText("Show Spawn Point")
            wanderRadiusVisible = false
            wanderRadiusBtn:SetText("Show Radius")
            
            -- Schedule re-showing markers after creature respawns (2 seconds)
            -- Only if they were visible before AND we still have same target
            if wasSpawnVisible or wasRadiusVisible then
                C_Timer.After(2, function()
                    if AMS and UnitExists("target") then
                        if wasSpawnVisible then
                            AMS.Send("SHOW_SPAWN_MARKER", {})
                            spawnMarkerVisible = true
                            spawnMarkerBtn:SetText("Hide Spawn Point")
                        end
                        if wasRadiusVisible then
                            AMS.Send("SHOW_WANDER_RADIUS", { segments = 16 })
                            wanderRadiusVisible = true
                            wanderRadiusBtn:SetText("Hide Radius")
                        end
                        print("|cFF00FF00[ATA]|r Markers refreshed at new location")
                    else
                        print("|cFFFFFF00[ATA]|r Target lost - markers not refreshed")
                    end
                end)
            end
        else
            print("|cFFFF0000[ATA]|r Failed to update wander distance: " .. (data.message or "Unknown error"))
        end
    end)
end

-- Handler for wander radius visualization response
if AMS then
    AMS.RegisterHandler("WANDER_RADIUS_RESPONSE", function(data)
        if data and data.success then
            if data.message and data.message:find("shown") then
                wanderRadiusVisible = true
                wanderRadiusBtn:SetText("Hide Radius")
                print("|cFF00FF00[ATA]|r " .. data.message .. " (" .. (data.markerCount or "?") .. " markers)")
            else
                wanderRadiusVisible = false
                wanderRadiusBtn:SetText("Show Radius")
                print("|cFF00FF00[ATA]|r Wander radius hidden")
            end
        else
            print("|cFFFF0000[ATA]|r Wander radius error: " .. (data.error or "Unknown error"))
        end
    end)
end

-- Handler for clear all markers response
if AMS then
    AMS.RegisterHandler("CLEAR_WANDER_MARKERS_RESPONSE", function(data)
        if data and data.success then
            if data.cleared and data.cleared > 0 then
                print("|cFF00FF00[ATA]|r " .. (data.message or "Markers cleared"))
            else
                print("|cFFFFFF00[ATA]|r " .. (data.message or "No markers found to clear"))
            end
        else
            print("|cFFFF0000[ATA]|r Clear markers error: " .. (data.error or "Unknown error"))
        end
    end)
end

-- Handler for clear nearby orphaned markers response
if AMS then
    AMS.RegisterHandler("CLEAR_NEARBY_MARKERS_RESPONSE", function(data)
        if data and data.success then
            if data.cleared and data.cleared > 0 then
                print("|cFF00FF00[ATA]|r " .. data.message)
            else
                print("|cFFFFFF00[ATA]|r No orphaned markers found nearby")
            end
        else
            print("|cFFFF0000[ATA]|r Clear orphaned error: " .. (data.error or "Unknown error"))
        end
    end)
end

-- Function to update wander settings display
local function UpdateWanderSettings(wanderRadius)
    wanderCurrentValue:SetText(string.format("%.1f yards", wanderRadius or 0))
    wanderEditBox:SetText(string.format("%.1f", wanderRadius or 5))
end

-- Function to show waypoint or wander content based on movement type
local function UpdateMovementContent(movementType, wanderRadius, hasWaypointPath)
    if movementType == 1 and not hasWaypointPath then
        -- Random movement without waypoints - show wander settings
        waypointListContainer:Hide()
        waypointDivider:Hide()
        waypointDetailPanel:Hide()
        wanderSettingsContainer:Show()
        UpdateWanderSettings(wanderRadius)
        waypointButton:Disable()
    else
        -- Waypoint movement or has waypoint path - show waypoint list
        wanderSettingsContainer:Hide()
        waypointListContainer:Show()
        waypointDivider:Show()
        waypointDetailPanel:Show()
        if hasWaypointPath then
            waypointButton:Enable()
        else
            waypointButton:Disable()
        end
    end
    
    -- Reset button states when switching targets
    spawnMarkerVisible = false
    spawnMarkerBtn:SetText("Show Spawn Point")
    wanderRadiusVisible = false
    wanderRadiusBtn:SetText("Show Radius")
    
    -- Re-enable all buttons (in case they were disabled waiting for a response)
    EnableWanderButtons()
end

-- Request waypoint details from server
local function RequestWaypointDetails(pathId)
    if not AMS then return end
    
    currentWaypointPathId = pathId
    currentWaypointNodes = {}
    selectedWaypointNodeId = nil
    
    AMS.Send("GET_WAYPOINT_DETAILS", { pathId = pathId })
end

-- Handler for waypoint details response
if AMS then
    AMS.RegisterHandler("WAYPOINT_DETAILS_RESPONSE", function(data)
        if not data or not data.success then
            print("|cFFFF0000[ATA]|r Failed to get waypoint details: " .. (data.error or "Unknown error"))
            return
        end
        
        print("|cFF00FF00[ATA]|r Received waypoint details: pathId=" .. (data.pathId or 0) .. ", nodes=" .. #(data.nodes or {}))
        
        currentWaypointPathId = data.pathId
        currentWaypointNodes = data.nodes or {}
        selectedWaypointNodeId = nil
        
        print("|cFF00FF00[ATA]|r Updating waypoint list with " .. #currentWaypointNodes .. " nodes")
        UpdateWaypointList()
    end)
end

-- Waypoint action buttons container
local waypointButtonContainer = CreateFrame("Frame", nil, waypointContent)
waypointButtonContainer:SetPoint("BOTTOMLEFT", waypointContent, "BOTTOMLEFT", 8, 8)
waypointButtonContainer:SetPoint("BOTTOMRIGHT", waypointContent, "BOTTOMRIGHT", -8, 8)
waypointButtonContainer:SetHeight(100)

-- ============================================================================
-- Right Tab Content: Paths (tracked waypoint visualizations)
-- ============================================================================

local pathsContent = CreateFrame("Frame", nil, rightContentFrame)
pathsContent:SetAllPoints(rightContentFrame)
pathsContent:Hide()

local pathsTitle = pathsContent:CreateFontString(nil, "OVERLAY", "GameFontNormal")
pathsTitle:SetPoint("TOP", pathsContent, "TOP", 0, -8)
pathsTitle:SetText("Tracked Paths")

local pathsHelpText = pathsContent:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
pathsHelpText:SetPoint("TOP", pathsTitle, "BOTTOM", 0, -4)
pathsHelpText:SetText("|cFF888888Shift+Click to remove|r")

-- Forward declaration for UpdatePathsList (defined below)
local UpdatePathsList

-- Clear All button (top right)
local clearAllButton = CreateFrame("Button", nil, pathsContent, "UIPanelButtonTemplate")
clearAllButton:SetSize(70, 20)
clearAllButton:SetPoint("TOPRIGHT", pathsContent, "TOPRIGHT", -8, -8)
clearAllButton:SetText("Clear All")
clearAllButton:SetScript("OnClick", function()
    -- Send request to clear all waypoint markers (uses C++ ClearAllWaypointVisualizations)
    if AMS then
        AMS.Send("CLEAR_ALL_WAYPOINT_MARKERS", {})
    end
    -- Clear local tracking
    trackedPaths = {}
    SaveTrackedPaths()
    UpdatePathsList()
    -- Reset waypoint button state
    waypointsVisible = false
    waypointButton:SetText("Show Waypoints")
    print("|cFF00FF00[ATA]|r Cleared all tracked paths")
end)
clearAllButton:SetScript("OnEnter", function(self)
    GameTooltip:SetOwner(self, "ANCHOR_LEFT")
    GameTooltip:AddLine("Clear All Paths", 1, 1, 1)
    GameTooltip:AddLine("Removes all paths from tracker", 0.7, 0.7, 0.7)
    GameTooltip:AddLine("and hides markers in the world.", 0.7, 0.7, 0.7)
    GameTooltip:Show()
end)
clearAllButton:SetScript("OnLeave", function()
    GameTooltip:Hide()
end)

-- Scrollable container for path entries
local pathsScrollFrame = CreateFrame("ScrollFrame", nil, pathsContent, "UIPanelScrollFrameTemplate")
pathsScrollFrame:SetPoint("TOPLEFT", pathsContent, "TOPLEFT", 8, -45)
pathsScrollFrame:SetPoint("BOTTOMRIGHT", pathsContent, "BOTTOMRIGHT", -28, 8)

local pathsScrollChild = CreateFrame("Frame", nil, pathsScrollFrame)
pathsScrollChild:SetSize(1, 1)
pathsScrollFrame:SetScrollChild(pathsScrollChild)

-- Table to hold path entry buttons
local pathEntryButtons = {}

-- Function to update the paths list display
UpdatePathsList = function()
    -- Hide all existing buttons
    for _, button in ipairs(pathEntryButtons) do
        button:Hide()
    end
    
    -- Count tracked paths
    local count = 0
    for _ in pairs(trackedPaths) do
        count = count + 1
    end
    
    if count == 0 then
        -- Show empty message (parent to pathsContent for proper centering)
        if not pathsContent.emptyText then
            pathsContent.emptyText = pathsContent:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
            pathsContent.emptyText:SetPoint("TOP", pathsContent, "TOP", 0, -60)
            pathsContent.emptyText:SetPoint("LEFT", pathsContent, "LEFT", 20, 0)
            pathsContent.emptyText:SetPoint("RIGHT", pathsContent, "RIGHT", -20, 0)
            pathsContent.emptyText:SetJustifyH("CENTER")
        end
        pathsContent.emptyText:SetText("|cFF888888No paths being tracked.|r\n\nUse 'Show Waypoints' on creatures\nto track their paths here.")
        pathsContent.emptyText:Show()
        pathsScrollChild:SetHeight(100)
        return
    end
    
    -- Hide empty text if we have paths
    if pathsContent.emptyText then
        pathsContent.emptyText:Hide()
    end
    
    -- Create/update buttons for each tracked path
    local index = 0
    for guid, pathData in pairs(trackedPaths) do
        index = index + 1
        
        -- Create button if needed
        if not pathEntryButtons[index] then
            local btn = CreateFrame("Button", nil, pathsScrollChild)
            btn:SetSize(200, 40)
            btn:EnableMouse(true)
            btn:RegisterForClicks("AnyUp")
            btn:SetHighlightTexture("Interface/QuestFrame/UI-QuestTitleHighlight", "ADD")
            
            btn.bg = btn:CreateTexture(nil, "BACKGROUND")
            btn.bg:SetAllPoints()
            btn.bg:SetColorTexture(0.2, 0.2, 0.2, 0.5)
            
            btn.name = btn:CreateFontString(nil, "OVERLAY", "GameFontNormal")
            btn.name:SetPoint("TOPLEFT", btn, "TOPLEFT", 8, -6)
            btn.name:SetJustifyH("LEFT")
            
            btn.info = btn:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
            btn.info:SetPoint("TOPLEFT", btn.name, "BOTTOMLEFT", 0, -2)
            btn.info:SetJustifyH("LEFT")
            
            pathEntryButtons[index] = btn
        end
        
        local btn = pathEntryButtons[index]
        btn.guid = guid
        btn.pathData = pathData
        
        -- Position
        btn:ClearAllPoints()
        btn:SetPoint("TOPLEFT", pathsScrollChild, "TOPLEFT", 0, -((index-1) * 42))
        btn:SetPoint("RIGHT", pathsScrollChild, "RIGHT", 0, 0)
        
        -- Update text
        btn.name:SetText("|cFFFFD700" .. (pathData.name or "Unknown") .. "|r")
        btn.info:SetText(string.format("Entry: %s | Path: %d | Nodes: %d", 
            pathData.entry or "?", 
            pathData.pathId or 0, 
            pathData.nodeCount or 0))
        
        -- Click handler (TODO: Shift+Click not working - buttons need to be proper clickable buttons)
        btn:SetScript("OnClick", function(self, button)
            if IsShiftKeyDown() then
                -- Remove from tracking AND hide the markers in the world
                local creatureGUID = self.guid
                if trackedPaths[creatureGUID] then
                    -- Send hide request to server
                    if AMS then
                        AMS.Send("HIDE_WAYPOINTS_BY_GUID", { guid = creatureGUID })
                    end
                    trackedPaths[creatureGUID] = nil
                    print("|cFF00FF00[ATA]|r Removed path: " .. (self.pathData.name or "Unknown"))
                    UpdatePathsList()
                    SaveTrackedPaths()
                    
                    -- If this was the current target, reset waypoint button state
                    if creatureGUID == currentTargetGUID then
                        waypointsVisible = false
                        waypointButton:SetText("Show Waypoints")
                    end
                end
            end
        end)
        
        btn:SetScript("OnEnter", function(self)
            GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
            GameTooltip:AddLine(self.pathData.name or "Unknown", 1, 0.82, 0)
            GameTooltip:AddLine("Path ID: " .. (self.pathData.pathId or 0), 1, 1, 1)
            GameTooltip:AddLine("Nodes: " .. (self.pathData.nodeCount or 0), 1, 1, 1)
            GameTooltip:AddLine(" ")
            GameTooltip:AddLine("|cFFFF8800Shift+Click|r to remove path", 0.5, 0.5, 0.5)
            GameTooltip:AddLine("(hides markers in-game)", 0.4, 0.4, 0.4)
            GameTooltip:Show()
        end)
        
        btn:SetScript("OnLeave", function()
            GameTooltip:Hide()
        end)
        
        btn:Show()
    end
    
    -- Update scroll child height
    pathsScrollChild:SetHeight(math.max(index * 42, pathsScrollFrame:GetHeight()))
end

-- Store right content frames for tab switching
local rightContentFrames = {
    ["3D Model"] = modelContent,
    ["Waypoints"] = waypointContent,
    ["Paths"] = pathsContent
}

-- ============================================================================
-- Right Tab Switching
-- ============================================================================

local function ShowRightTab(tabName)
    currentRightTab = tabName
    for name, frame in pairs(rightContentFrames) do
        if name == tabName then
            frame:Show()
        else
            frame:Hide()
        end
    end
    -- Update tab button appearance with green active style
    for i, tab in ipairs(rightTabs) do
        StyleTabActive(tab, rightTabNames[i] == tabName)
    end
end

for i, tab in ipairs(rightTabs) do
    tab:SetScript("OnClick", function()
        ShowRightTab(rightTabNames[i])
    end)
end

-- Initialize right tabs
ShowRightTab("3D Model")

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
    -- Update tab button appearance with green active style
    for i, tab in ipairs(tabs) do
        StyleTabActive(tab, tabNames[i] == tabName)
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

-- Simple JSON-like pretty printer for Lua tables
local function PrettyPrint(obj, indent)
    indent = indent or 0
    local indentStr = string.rep("  ", indent)
    local nextIndent = string.rep("  ", indent + 1)
    
    if type(obj) == "nil" then
        return "|cFF888888null|r"
    elseif type(obj) == "boolean" then
        return obj and "|cFF00FF00true|r" or "|cFFFF0000false|r"
    elseif type(obj) == "number" then
        return "|cFF00FFFF" .. tostring(obj) .. "|r"
    elseif type(obj) == "string" then
        return "|cFFFFFF00\"" .. obj .. "\"|r"
    elseif type(obj) == "table" then
        local lines = {}
        local isArray = #obj > 0
        
        if isArray then
            table.insert(lines, "[")
            for i, v in ipairs(obj) do
                local comma = (i < #obj) and "," or ""
                table.insert(lines, nextIndent .. PrettyPrint(v, indent + 1) .. comma)
            end
            table.insert(lines, indentStr .. "]")
        else
            table.insert(lines, "{")
            local keys = {}
            for k in pairs(obj) do
                table.insert(keys, k)
            end
            table.sort(keys, function(a, b) return tostring(a) < tostring(b) end)
            
            for i, k in ipairs(keys) do
                local comma = (i < #keys) and "," or ""
                local keyStr = "|cFF00FF00\"" .. tostring(k) .. "\"|r"
                table.insert(lines, nextIndent .. keyStr .. ": " .. PrettyPrint(obj[k], indent + 1) .. comma)
            end
            table.insert(lines, indentStr .. "}")
        end
        return table.concat(lines, "\n")
    else
        return "|cFF888888<" .. type(obj) .. ">|r"
    end
end

local function FormatRawTab(npcData, sData)
    local lines = {}
    
    if isLoadingServerData then
        return "|cFF888888Loading raw server data...|r"
    end
    
    if not sData then
        return "|cFF888888No server data available.|r\n\n|cFF888888Click Refresh to load data from server.|r"
    end
    
    if sData.error then
        return "|cFFFF0000Error:|r " .. tostring(sData.error)
    end
    
    table.insert(lines, "|cFFFFD700Raw Server Data|r")
    table.insert(lines, "|cFF888888(Scroll to see all data)|r")
    table.insert(lines, "")
    table.insert(lines, PrettyPrint(sData, 0))
    
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
    
    local textHeight
    if frame.useEditBox then
        -- EditBox needs to be sized first, then we get its height
        frame.text:SetWidth(scrollWidth - 40)
        -- Force layout update
        textHeight = frame.text:GetHeight()
        if textHeight < 100 then
            -- EditBox might not have calculated height yet, estimate based on text
            local text = frame.text:GetText() or ""
            local lineCount = select(2, text:gsub("\n", "\n")) + 1
            textHeight = lineCount * 14  -- Approximate line height
        end
    else
        textHeight = frame.text:GetStringHeight()
    end
    
    frame.child:SetWidth(math.max(scrollWidth, 1))
    frame.child:SetHeight(math.max(textHeight + 20, frame.scroll:GetHeight()))
end

function npcPanel:Update(requestServerData, forceRefresh)
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
    
    -- Update Raw tab
    contentFrames["Raw"].text:SetText(FormatRawTab(npcData, serverData))
    UpdateContentSize(contentFrames["Raw"])
    
    -- Request server data if needed
    if requestServerData and npcData and npcData.guid and ATA.ServerData then
        isLoadingServerData = true
        serverData = nil
        
        -- Update displays to show loading
        contentFrames["Stats"].text:SetText(FormatStatsTab(npcData, nil))
        contentFrames["AI"].text:SetText(FormatAITab(npcData, nil))
        contentFrames["Raw"].text:SetText(FormatRawTab(npcData, nil))
        
        ATA.ServerData:RequestNPCData(npcData.guid, function(data, error)
            isLoadingServerData = false
            if error then
                serverData = { success = false, error = error }
            else
                serverData = data
            end
            npcPanel:Update(false, false)
            
            -- Enable waypoint button if creature has a waypoint path
            if data and data.movement and data.movement.waypointPath then
                waypointButton:Enable()
                
                -- Auto-load waypoint details for the Waypoints tab
                local pathId = data.movement.waypointPath.pathId
                if pathId and pathId > 0 then
                    RequestWaypointDetails(pathId)
                end
            else
                waypointButton:Disable()
            end
            
            -- Update Waypoints tab to show either waypoint list or wander settings
            if data and data.movement then
                local movementType = data.movement.defaultType or 0
                local wanderRadius = data.behavior and data.behavior.wanderRadius or 0
                local hasWaypointPath = data.movement.waypointPath ~= nil
                UpdateMovementContent(movementType, wanderRadius, hasWaypointPath)
            end
        end, forceRefresh)  -- Pass forceRefresh to bypass cache
    end
    
    -- Update 3D model
    if npcData and UnitExists("target") then
        npcModel:SetUnit("target")
        -- Reset zoom and rotation to defaults for new target
        modelZoom = 1.5
        modelRotation = 0.61
        npcModel:SetCamDistanceScale(modelZoom)
        npcModel:SetRotation(modelRotation)
    else
        npcModel:ClearModel()
        npcModel:SetModel("interface/buttons/talktomequestionmark.m2")
    end
    
    -- Waypoint tab is now handled by the new list/detail UI
    -- No additional update needed here
end

-- ============================================================================
-- Event Handlers
-- ============================================================================

refreshButton:SetScript("OnClick", function()
    -- Re-enable buttons in case they got stuck disabled from a failed request
    pcall(function()
        if EnableWanderButtons then EnableWanderButtons() end
    end)
    npcPanel:Update(true, true)  -- Always force refresh from server, bypass cache
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

-- Pending waypoint request data (stored while waiting for response)
local pendingWaypointRequest = nil

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
    
    -- Store data for tracking when response arrives
    local wp = serverData and serverData.movement and serverData.movement.waypointPath
    pendingWaypointRequest = {
        guid = npcData.guid,
        name = npcData.name or "Unknown",
        entry = npcData.npcID or "?",
        pathId = wp and wp.pathId or 0,
        nodeCount = wp and wp.nodeCount or 0,
        action = waypointsVisible and "hide" or "show"
    }
    
    if waypointsVisible then
        -- Hide waypoints
        print("|cFF00FF00[ATA]|r Hiding waypoint markers...")
        waypointButton:SetText("Working...")
        waypointButton:Disable()
        panelLockedToNPC = false
        AMS.Send("HIDE_WAYPOINTS", { guid = npcData.guid })
    else
        -- Show waypoints and lock panel to this NPC
        print("|cFF00FF00[ATA]|r Spawning waypoint markers in 3D space...")
        waypointButton:SetText("Working...")
        waypointButton:Disable()
        panelLockedToNPC = true  -- Lock panel to prevent target changes from unloading NPC data
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
            
            -- Track/untrack the path
            if pendingWaypointRequest then
                if pendingWaypointRequest.action == "show" then
                    -- Add to tracked paths
                    trackedPaths[pendingWaypointRequest.guid] = {
                        name = pendingWaypointRequest.name,
                        entry = pendingWaypointRequest.entry,
                        pathId = data.pathId or pendingWaypointRequest.pathId,
                        nodeCount = pendingWaypointRequest.nodeCount
                    }
                    print("|cFF00FF00[ATA]|r Tracking path for: " .. pendingWaypointRequest.name)
                    
                    -- Request waypoint details for the Waypoints tab
                    if data.pathId then
                        RequestWaypointDetails(data.pathId)
                    end
                elseif pendingWaypointRequest.action == "hide" then
                    -- Remove from tracked paths
                    if trackedPaths[pendingWaypointRequest.guid] then
                        trackedPaths[pendingWaypointRequest.guid] = nil
                        print("|cFF00FF00[ATA]|r Untracked path for: " .. pendingWaypointRequest.name)
                    end
                end
                pendingWaypointRequest = nil
                UpdatePathsList()
                SaveTrackedPaths()
            end
        else
            waypointButton:SetText(waypointsVisible and "Hide Waypoints" or "Show Waypoints")
            waypointButton:Enable()
            print("|cFFFF0000[ATA]|r " .. (data.error or "Failed"))
            pendingWaypointRequest = nil
        end
    end)
end
InitWaypointHandler()

local updateFrame = CreateFrame("Frame")
updateFrame:RegisterEvent("PLAYER_TARGET_CHANGED")
updateFrame:SetScript("OnEvent", function()
    if npcPanel:IsShown() then
        -- Check if targeting a waypoint marker (visual waypoint creature)
        local targetGUID = UnitGUID("target")
        if targetGUID and panelLockedToNPC then
            -- Panel is locked to an NPC, check if we targeted a waypoint marker
            local guidLow = tonumber(string.match(targetGUID, "Creature%-0%-0%-0%-(%d+)")) or 0
            if guidLow > 0 and AMS then
                -- Request waypoint info for this GUID
                AMS.Send("GET_WAYPOINT_FOR_GUID", { guid = guidLow })
                return  -- Don't update NPC panel
            end
        end
        
        -- Normal NPC targeting behavior
        serverData = nil
        isLoadingServerData = false
        
        -- Check if the new target has waypoints already visible
        local npcData = ATA:GetTargetNPCInfo()
        if npcData and npcData.guid and trackedPaths[npcData.guid] then
            -- This NPC's path is already being tracked/visible
            waypointsVisible = true
            waypointButton:SetText("Hide Waypoints")
            currentTargetGUID = npcData.guid
        else
            -- Reset waypoint state for new target
            waypointsVisible = false
            waypointButton:SetText("Show Waypoints")
            currentTargetGUID = npcData and npcData.guid or nil
        end
        waypointButton:Disable()  -- Will be enabled by Update() if creature has waypoints
        npcPanel:Update(true, true)  -- Request server data and force refresh from cache
    end
end)

-- Handler for waypoint selection response (with visual highlighting)
if AMS then
    AMS.RegisterHandler("WAYPOINT_SELECTED_RESPONSE", function(data)
        if data and data.success then
            print("|cFF00FF00[ATA]|r Waypoint selected: node " .. data.nodeId .. " at " .. data.x .. ", " .. data.y .. ", " .. data.z)
            
            -- The waypoint is already visualized as a creature marker in the world
            -- The highlighting happens through the UI selection (green background on the button)
            -- Future: Could add a spell effect or other visual indicator here
        end
    end)
    
    AMS.RegisterHandler("WAYPOINT_FOR_GUID_RESPONSE", function(data)
        if data and data.success then
            -- Select this waypoint in the Waypoints tab
            selectedWaypointNodeId = data.nodeId
            currentWaypointPathId = data.pathId
            
            -- Switch to Waypoints tab to show the detail
            ShowRightTab("Waypoints")
            
            -- Update the display
            UpdateWaypointList()
            UpdateWaypointDetail()
            
            print("|cFF00FF00[ATA]|r Selected waypoint node " .. data.nodeId)
        end
    end)
end

-- ============================================================================
-- Initialization
-- ============================================================================

ShowTab("Basic")  -- Default tab

-- Load tracked paths from SavedVariables
LoadTrackedPaths()
UpdatePathsList()

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
