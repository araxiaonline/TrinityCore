-- AraxiaTrinityAdmin Main Window
-- Multi-panel window framework

local addonName = "AraxiaTrinityAdmin"

-- Wait for addon to load before initializing UI
local initFrame = CreateFrame("Frame")
initFrame:RegisterEvent("ADDON_LOADED")
initFrame:SetScript("OnEvent", function(self, event, loadedAddon)
    if loadedAddon ~= addonName then return end
    self:UnregisterEvent("ADDON_LOADED")
    
    local ATA = AraxiaTrinityAdmin
    if not ATA then
        print("Error: AraxiaTrinityAdmin namespace not found!")
        return
    end

-- Create main window frame
local mainWindow = CreateFrame("Frame", "AraxiaTrinityAdminMainWindow", UIParent, "BasicFrameTemplateWithInset")
mainWindow:SetSize(800, 600)
mainWindow:SetPoint("CENTER")
mainWindow:SetFrameStrata("HIGH")  -- Ensure window appears above action bars
mainWindow:SetMovable(true)
mainWindow:SetResizable(true)
mainWindow:EnableMouse(true)
mainWindow:RegisterForDrag("LeftButton")
mainWindow:SetClampedToScreen(true)
mainWindow:SetResizeBounds(600, 400, 1400, 1000)
mainWindow:Hide()

-- Allow Escape key to close the window
table.insert(UISpecialFrames, "AraxiaTrinityAdminMainWindow")

-- Set title
mainWindow.title = mainWindow:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
mainWindow.title:SetPoint("TOP", mainWindow, "TOP", 0, -5)
mainWindow.title:SetText("Araxia Trinity Admin")

-- Save position on drag
mainWindow:SetScript("OnDragStart", function(self)
    self:StartMoving()
end)

mainWindow:SetScript("OnDragStop", function(self)
    self:StopMovingOrSizing()
    local point, _, _, x, y = self:GetPoint()
    AraxiaTrinityAdminDB.windowPosition = {
        point = point,
        x = x,
        y = y
    }
    -- Save size too
    AraxiaTrinityAdminDB.windowSize = {
        width = self:GetWidth(),
        height = self:GetHeight()
    }
end)

-- Add resize handle
local resizeButton = CreateFrame("Button", nil, mainWindow)
resizeButton:SetSize(16, 16)
resizeButton:SetPoint("BOTTOMRIGHT", mainWindow, "BOTTOMRIGHT", -2, 2)
resizeButton:SetNormalTexture("Interface\\ChatFrame\\UI-ChatIM-SizeGrabber-Up")
resizeButton:SetHighlightTexture("Interface\\ChatFrame\\UI-ChatIM-SizeGrabber-Highlight")
resizeButton:SetPushedTexture("Interface\\ChatFrame\\UI-ChatIM-SizeGrabber-Down")
resizeButton:SetScript("OnMouseDown", function(self)
    mainWindow:StartSizing("BOTTOMRIGHT")
end)
resizeButton:SetScript("OnMouseUp", function(self)
    mainWindow:StopMovingOrSizing()
    -- Save size
    AraxiaTrinityAdminDB.windowSize = {
        width = mainWindow:GetWidth(),
        height = mainWindow:GetHeight()
    }
end)

-- Restore position and size
if AraxiaTrinityAdminDB and AraxiaTrinityAdminDB.windowPosition then
    local pos = AraxiaTrinityAdminDB.windowPosition
    mainWindow:ClearAllPoints()
    mainWindow:SetPoint(pos.point, UIParent, pos.point, pos.x, pos.y)
end

if AraxiaTrinityAdminDB and AraxiaTrinityAdminDB.windowSize then
    local size = AraxiaTrinityAdminDB.windowSize
    mainWindow:SetSize(size.width, size.height)
end

-- Track visibility
mainWindow:SetScript("OnShow", function(self)
    AraxiaTrinityAdminDB.windowShown = true
end)

mainWindow:SetScript("OnHide", function(self)
    AraxiaTrinityAdminDB.windowShown = false
end)

-- Determine content area (Inset or fallback to mainWindow with margins)
local contentArea = mainWindow.Inset or mainWindow
local topOffset = mainWindow.Inset and -4 or -30  -- Account for title bar if no Inset
local bottomOffset = mainWindow.Inset and 4 or 10
local leftOffset = mainWindow.Inset and 4 or 10
local rightOffset = mainWindow.Inset and -4 or -10

-- Tab bar for panel selection
local tabHeight = 30
mainWindow.tabBar = CreateFrame("Frame", nil, mainWindow, "BackdropTemplate")
mainWindow.tabBar:SetPoint("TOPLEFT", contentArea, "TOPLEFT", leftOffset, topOffset)
mainWindow.tabBar:SetPoint("TOPRIGHT", contentArea, "TOPRIGHT", rightOffset, topOffset)
mainWindow.tabBar:SetHeight(tabHeight)
mainWindow.tabBar:SetFrameLevel(mainWindow:GetFrameLevel() + 1)
mainWindow.tabBar:SetBackdrop({
    bgFile = "Interface/Tooltips/UI-Tooltip-Background",
    edgeFile = "Interface/Tooltips/UI-Tooltip-Border",
    tile = true,
    tileSize = 16,
    edgeSize = 16,
    insets = { left = 4, right = 4, top = 4, bottom = 4 }
})
mainWindow.tabBar:SetBackdropColor(0.1, 0.1, 0.1, 0.8)
mainWindow.tabBar:SetBackdropBorderColor(0.4, 0.4, 0.4, 1)

-- Panel container (below tabs)
mainWindow.panelContainer = CreateFrame("Frame", nil, mainWindow)
mainWindow.panelContainer:SetPoint("TOPLEFT", mainWindow.tabBar, "BOTTOMLEFT", 0, -4)
mainWindow.panelContainer:SetPoint("BOTTOMRIGHT", contentArea, "BOTTOMRIGHT", rightOffset, bottomOffset)
mainWindow.panelContainer:SetFrameLevel(mainWindow:GetFrameLevel() + 1)

-- Panel registry
mainWindow.panels = {}
mainWindow.tabs = {}
mainWindow.currentPanel = nil

-- Function to register a panel
function mainWindow:RegisterPanel(name, displayName, panelFrame)
    self.panels[name] = {
        displayName = displayName,
        frame = panelFrame
    }
    
    panelFrame:SetParent(self.panelContainer)
    panelFrame:SetScale(self.panelContainer:GetScale())
    panelFrame:ClearAllPoints()
    panelFrame:SetPoint("TOPLEFT", self.panelContainer, "TOPLEFT", 0, 0)
    panelFrame:SetPoint("BOTTOMRIGHT", self.panelContainer, "BOTTOMRIGHT", 0, 0)
    panelFrame:Hide()
    
    -- Create tab button
    local tab = CreateFrame("Button", nil, self.tabBar, "UIPanelButtonTemplate, BackdropTemplate")
    tab:SetSize(120, 24)
    tab:SetText(displayName)
    tab:SetScript("OnClick", function()
        self:ShowPanel(name)
    end)
    
    table.insert(self.tabs, tab)
    
    -- Position tabs horizontally
    for i, t in ipairs(self.tabs) do
        t:ClearAllPoints()
        if i == 1 then
            t:SetPoint("LEFT", self.tabBar, "LEFT", 8, 0)
        else
            t:SetPoint("LEFT", self.tabs[i-1], "RIGHT", 4, 0)
        end
    end
end

-- Function to show a specific panel
function mainWindow:ShowPanel(name)
    -- Hide current panel
    if self.currentPanel and self.panels[self.currentPanel] then
        self.panels[self.currentPanel].frame:Hide()
    end
    
    -- Show new panel
    if self.panels[name] then
        self.panels[name].frame:Show()
        self.currentPanel = name
        AraxiaTrinityAdminDB.selectedPanel = name
        
        -- Update panel if it has an Update function
        if self.panels[name].frame.Update then
            self.panels[name].frame:Update()
        end
        
        -- Update tab states
        for i, tab in ipairs(self.tabs) do
            local panelName = nil
            for pName, pData in pairs(self.panels) do
                if pData.displayName == tab:GetText() then
                    panelName = pName
                    break
                end
            end
            
            if panelName == name then
                -- Hide button's normal textures so backdrop shows through
                if tab.Left then tab.Left:Hide() end
                if tab.Right then tab.Right:Hide() end
                if tab.Middle then tab.Middle:Hide() end
                if tab:GetNormalTexture() then tab:GetNormalTexture():SetAlpha(0) end
                if tab:GetPushedTexture() then tab:GetPushedTexture():SetAlpha(0) end
                
                -- Green active style
                tab:SetBackdrop({
                    bgFile = "Interface/Buttons/WHITE8x8",
                    edgeFile = "Interface/Tooltips/UI-Tooltip-Border",
                    tile = true, tileSize = 8, edgeSize = 8,
                    insets = { left = 2, right = 2, top = 2, bottom = 2 }
                })
                tab:SetBackdropColor(0, 0.6, 0, 0.8)
                tab:SetBackdropBorderColor(0, 1, 0, 1)
            else
                -- Restore button's normal textures
                if tab.Left then tab.Left:Show() end
                if tab.Right then tab.Right:Show() end
                if tab.Middle then tab.Middle:Show() end
                if tab:GetNormalTexture() then tab:GetNormalTexture():SetAlpha(1) end
                if tab:GetPushedTexture() then tab:GetPushedTexture():SetAlpha(1) end
                
                -- Normal style
                tab:SetBackdrop(nil)
            end
        end
    end
end

-- Store reference
ATA.MainWindow = mainWindow
ATA.PanelContainer = mainWindow.panelContainer

end) -- End of ADDON_LOADED handler
