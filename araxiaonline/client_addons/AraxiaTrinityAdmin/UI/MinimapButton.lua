-- AraxiaTrinityAdmin Minimap Button
-- Minimap icon for quick access to the admin panel

local addonName = "AraxiaTrinityAdmin"

-- Wait for addon to load
local initFrame = CreateFrame("Frame")
initFrame:RegisterEvent("ADDON_LOADED")
initFrame:SetScript("OnEvent", function(self, event, loadedAddon)
    if loadedAddon ~= addonName then return end
    self:UnregisterEvent("ADDON_LOADED")
    
    local ATA = AraxiaTrinityAdmin
    if not ATA then return end

-- Create minimap button
local minimapButton = CreateFrame("Button", "AraxiaTrinityAdminMinimapButton", Minimap)
minimapButton:SetSize(32, 32)
minimapButton:SetFrameStrata("MEDIUM")
minimapButton:SetFrameLevel(8)
minimapButton:RegisterForClicks("LeftButtonUp", "RightButtonUp")
minimapButton:SetHighlightTexture("Interface\\Minimap\\UI-Minimap-ZoomButton-Highlight")

-- Icon texture
local icon = minimapButton:CreateTexture(nil, "BACKGROUND")
icon:SetSize(20, 20)
icon:SetPoint("CENTER", 0, 1)
icon:SetTexture("Interface\\Icons\\INV_Misc_Gear_01")  -- Gear icon for admin
minimapButton.icon = icon

-- Border texture
local border = minimapButton:CreateTexture(nil, "OVERLAY")
border:SetSize(52, 52)
border:SetPoint("TOPLEFT", 0, 0)
border:SetTexture("Interface\\Minimap\\MiniMap-TrackingBorder")
minimapButton.border = border

-- Tooltip
minimapButton:SetScript("OnEnter", function(self)
    GameTooltip:SetOwner(self, "ANCHOR_LEFT")
    GameTooltip:SetText("Araxia Trinity Admin", 1, 1, 1)
    GameTooltip:AddLine("Left-click to toggle admin window", 0, 1, 0)
    GameTooltip:AddLine("Right-click to lock/unlock position", 0.7, 0.7, 0.7)
    GameTooltip:Show()
end)

minimapButton:SetScript("OnLeave", function(self)
    GameTooltip:Hide()
end)

-- Click handler
minimapButton:SetScript("OnClick", function(self, button)
    if button == "LeftButton" then
        -- Toggle admin window
        if ATA.MainWindow then
            if ATA.MainWindow:IsShown() then
                ATA.MainWindow:Hide()
            else
                ATA.MainWindow:Show()
            end
        else
            print("|cFFFF0000[Araxia Trinity Admin]|r Main window not initialized.")
        end
    elseif button == "RightButton" then
        -- Toggle lock state
        AraxiaTrinityAdminDB.minimapLocked = not AraxiaTrinityAdminDB.minimapLocked
        if AraxiaTrinityAdminDB.minimapLocked then
            print("|cFF00FF00[Araxia Trinity Admin]|r Minimap button locked.")
        else
            print("|cFF00FF00[Araxia Trinity Admin]|r Minimap button unlocked. Drag to reposition.")
        end
    end
end)

-- Dragging functionality
minimapButton:SetMovable(true)
minimapButton:RegisterForDrag("LeftButton")

minimapButton:SetScript("OnDragStart", function(self)
    if not AraxiaTrinityAdminDB.minimapLocked then
        self:LockHighlight()
        self.isDragging = true
    end
end)

minimapButton:SetScript("OnDragStop", function(self)
    self:UnlockHighlight()
    self.isDragging = false
end)

-- Update position on frame update (for smooth dragging around the circle)
minimapButton:SetScript("OnUpdate", function(self, elapsed)
    if self.isDragging then
        local mx, my = Minimap:GetCenter()
        local px, py = GetCursorPosition()
        local scale = Minimap:GetEffectiveScale()
        px, py = px / scale, py / scale
        
        -- Calculate angle from minimap center to cursor
        local angle = math.atan2(py - my, px - mx)
        AraxiaTrinityAdminDB.minimapAngle = angle
        
        -- Update position immediately for smooth dragging
        self:UpdatePosition()
    end
end)

-- Function to update button position
function minimapButton:UpdatePosition()
    local angle = AraxiaTrinityAdminDB.minimapAngle or math.rad(225)  -- Default bottom-left
    local x, y
    local cos = math.cos(angle)
    local sin = math.sin(angle)
    local minimapShape = GetMinimapShape and GetMinimapShape() or "ROUND"
    
    -- Calculate position based on minimap shape
    -- Use larger radius to position outside the minimap border
    if minimapShape == "SQUARE" then
        local radius = 110  -- Position outside square minimap
        x = math.max(-82, math.min(82, cos * radius))
        y = math.max(-82, math.min(82, sin * radius))
    else  -- ROUND or other shapes
        local radius = 110  -- Position outside round minimap
        x = cos * radius
        y = sin * radius
    end
    
    self:ClearAllPoints()
    self:SetPoint("CENTER", Minimap, "CENTER", x, y)
end

-- Initialize position
minimapButton:UpdatePosition()

-- Hide/show based on saved settings
if AraxiaTrinityAdminDB.minimapHidden then
    minimapButton:Hide()
else
    minimapButton:Show()
end

-- Store reference in addon namespace
ATA.MinimapButton = minimapButton

print("|cFF00FF00[Araxia Trinity Admin]|r Minimap button loaded.")

end) -- End of ADDON_LOADED handler