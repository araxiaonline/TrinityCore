-- AraxiaTrinityAdmin Add NPC Panel
-- Search and add NPCs to the game

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
local addNPCPanel = CreateFrame("Frame", "AraxiaTrinityAdminAddNPCPanel", UIParent)
addNPCPanel:Hide()

-- Title
local title = addNPCPanel:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
title:SetPoint("TOPLEFT", addNPCPanel, "TOPLEFT", 10, -10)
title:SetText("Add NPC")

-- Search/Lookup Section
local searchLabel = addNPCPanel:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
searchLabel:SetPoint("TOPLEFT", title, "BOTTOMLEFT", 0, -10)
searchLabel:SetText("Search NPC:")

-- Search input box
local searchBox = CreateFrame("EditBox", nil, addNPCPanel, "InputBoxTemplate")
searchBox:SetSize(300, 25)
searchBox:SetPoint("TOPLEFT", searchLabel, "BOTTOMLEFT", 5, -5)
searchBox:SetAutoFocus(false)
searchBox:SetMaxLetters(50)

-- Search button
local searchButton = CreateFrame("Button", nil, addNPCPanel, "UIPanelButtonTemplate")
searchButton:SetSize(80, 25)
searchButton:SetPoint("LEFT", searchBox, "RIGHT", 5, 0)
searchButton:SetText("Lookup")

-- Clear button
local clearButton = CreateFrame("Button", nil, addNPCPanel, "UIPanelButtonTemplate")
clearButton:SetSize(60, 25)
clearButton:SetPoint("LEFT", searchButton, "RIGHT", 5, 0)
clearButton:SetText("Clear")

-- Results pane (left side - 1/4 width)
local resultsFrame = CreateFrame("Frame", nil, addNPCPanel, "BackdropTemplate")
resultsFrame:SetPoint("TOPLEFT", searchBox, "BOTTOMLEFT", -5, -15)
resultsFrame:SetPoint("BOTTOMLEFT", addNPCPanel, "BOTTOMLEFT", 10, 10)
resultsFrame:SetWidth(200)  -- Fixed width for left pane
resultsFrame:SetBackdrop({
    bgFile = "Interface/Tooltips/UI-Tooltip-Background",
    edgeFile = "Interface/Tooltips/UI-Tooltip-Border",
    tile = true,
    tileSize = 16,
    edgeSize = 16,
    insets = { left = 4, right = 4, top = 4, bottom = 4 }
})
resultsFrame:SetBackdropColor(0, 0, 0, 0.5)
resultsFrame:SetBackdropBorderColor(0.4, 0.4, 0.4, 1)

-- Results title
local resultsTitle = resultsFrame:CreateFontString(nil, "OVERLAY", "GameFontNormal")
resultsTitle:SetPoint("TOP", resultsFrame, "TOP", 0, -8)
resultsTitle:SetText("Results")

-- Scroll frame for results
local resultsScrollFrame = CreateFrame("ScrollFrame", nil, resultsFrame, "UIPanelScrollFrameTemplate")
resultsScrollFrame:SetPoint("TOPLEFT", resultsFrame, "TOPLEFT", 8, -30)
resultsScrollFrame:SetPoint("BOTTOMRIGHT", resultsFrame, "BOTTOMRIGHT", -28, 8)

local resultsScrollChild = CreateFrame("Frame", nil, resultsScrollFrame)
resultsScrollChild:SetSize(1, 1)
resultsScrollFrame:SetScrollChild(resultsScrollChild)

-- Portrait/Details pane (right side - 3/4 width)
local detailsFrame = CreateFrame("Frame", nil, addNPCPanel, "BackdropTemplate")
detailsFrame:SetPoint("TOPLEFT", resultsFrame, "TOPRIGHT", 10, 0)
detailsFrame:SetPoint("BOTTOMRIGHT", addNPCPanel, "BOTTOMRIGHT", -10, 10)
detailsFrame:SetBackdrop({
    bgFile = "Interface/Tooltips/UI-Tooltip-Background",
    edgeFile = "Interface/Tooltips/UI-Tooltip-Border",
    tile = true,
    tileSize = 16,
    edgeSize = 16,
    insets = { left = 4, right = 4, top = 4, bottom = 4 }
})
detailsFrame:SetBackdropColor(0, 0, 0, 0.5)
detailsFrame:SetBackdropBorderColor(0.4, 0.4, 0.4, 1)

-- Details title
local detailsTitle = detailsFrame:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
detailsTitle:SetPoint("TOP", detailsFrame, "TOP", 0, -10)
detailsTitle:SetText("NPC Details")

-- Portrait model
local npcModel = CreateFrame("PlayerModel", nil, detailsFrame)
npcModel:SetPoint("TOP", detailsTitle, "BOTTOM", 0, -10)
npcModel:SetSize(300, 300)
npcModel:SetCamDistanceScale(1.5)
npcModel:SetRotation(0.61)
npcModel:SetPosition(0, 0, 0)

-- NPC info display
local npcInfoFrame = CreateFrame("Frame", nil, detailsFrame, "BackdropTemplate")
npcInfoFrame:SetPoint("TOPLEFT", npcModel, "BOTTOMLEFT", 0, -10)
npcInfoFrame:SetPoint("BOTTOMRIGHT", detailsFrame, "BOTTOMRIGHT", -10, 10)
npcInfoFrame:SetBackdrop({
    bgFile = "Interface/Tooltips/UI-Tooltip-Background",
    tile = true,
    tileSize = 16,
    insets = { left = 4, right = 4, top = 4, bottom = 4 }
})
npcInfoFrame:SetBackdropColor(0.1, 0.1, 0.1, 0.8)

-- Scroll frame for NPC info
local infoScrollFrame = CreateFrame("ScrollFrame", nil, npcInfoFrame, "UIPanelScrollFrameTemplate")
infoScrollFrame:SetPoint("TOPLEFT", npcInfoFrame, "TOPLEFT", 8, -8)
infoScrollFrame:SetPoint("BOTTOMRIGHT", npcInfoFrame, "BOTTOMRIGHT", -28, 8)

local infoScrollChild = CreateFrame("Frame", nil, infoScrollFrame)
infoScrollChild:SetSize(1, 1)
infoScrollFrame:SetScrollChild(infoScrollChild)

local infoText = infoScrollChild:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
infoText:SetPoint("TOPLEFT", infoScrollChild, "TOPLEFT", 5, -5)
infoText:SetJustifyH("LEFT")
infoText:SetJustifyV("TOP")
infoText:SetText("Select an NPC from the results to view details")
infoText:SetWidth(300)

-- Add NPC button
local addButton = CreateFrame("Button", nil, detailsFrame, "UIPanelButtonTemplate")
addButton:SetSize(120, 30)
addButton:SetPoint("TOP", npcModel, "BOTTOM", 0, 5)
addButton:SetText("Add NPC")
addButton:Hide()  -- Only show when an NPC is selected

-- Store search results
local searchResults = {}
local selectedNPC = nil
local resultButtons = {}
local isSearching = false
local searchStartTime = 0
local currentSearchQuery = ""
local isFetchingNPCInfo = false
local pendingNPCInfoID = nil

-- Status message label
local statusLabel = addNPCPanel:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
statusLabel:SetPoint("TOPLEFT", searchBox, "BOTTOMLEFT", 0, -5)
statusLabel:SetText("")
statusLabel:SetTextColor(1, 1, 0, 1)  -- Yellow

-- Forward declare functions that will be used by event handlers
local DisplayResults
local DisplayNPCDetails
local SearchNPCs

-- Function to display search results
DisplayResults = function()
    -- Clear existing buttons
    for _, btn in ipairs(resultButtons) do
        btn:Hide()
        btn:SetParent(nil)
    end
    wipe(resultButtons)
    
    if #searchResults == 0 then
        local noResults = resultsScrollChild:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
        noResults:SetPoint("TOPLEFT", resultsScrollChild, "TOPLEFT", 5, -5)
        noResults:SetText("No results found")
        resultsScrollChild:SetHeight(30)
        return
    end
    
    -- Create result buttons
    local yOffset = -5
    for i, npc in ipairs(searchResults) do
        local btn = CreateFrame("Button", nil, resultsScrollChild, "BackdropTemplate")
        btn:SetSize(170, 30)
        btn:SetPoint("TOPLEFT", resultsScrollChild, "TOPLEFT", 5, yOffset)
        btn:SetBackdrop({
            bgFile = "Interface/Tooltips/UI-Tooltip-Background",
            edgeFile = "Interface/Tooltips/UI-Tooltip-Border",
            tile = true,
            tileSize = 16,
            edgeSize = 12,
            insets = { left = 2, right = 2, top = 2, bottom = 2 }
        })
        btn:SetBackdropColor(0.1, 0.1, 0.1, 0.8)
        btn:SetBackdropBorderColor(0.4, 0.4, 0.4, 1)
        
        local btnText = btn:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
        btnText:SetPoint("LEFT", btn, "LEFT", 5, 0)
        btnText:SetText(string.format("%s (ID: %d)", npc.name, npc.id))
        btnText:SetJustifyH("LEFT")
        btnText:SetWidth(160)
        
        -- Highlight on hover
        btn:SetScript("OnEnter", function(self)
            self:SetBackdropColor(0.2, 0.2, 0.3, 1)
        end)
        btn:SetScript("OnLeave", function(self)
            if selectedNPC ~= npc then
                self:SetBackdropColor(0.1, 0.1, 0.1, 0.8)
            end
        end)
        
        -- Click handler
        btn:SetScript("OnClick", function(self)
            -- Deselect previous
            for _, b in ipairs(resultButtons) do
                b:SetBackdropColor(0.1, 0.1, 0.1, 0.8)
            end
            
            -- Select this one
            self:SetBackdropColor(0.2, 0.2, 0.3, 1)
            selectedNPC = npc
            DisplayNPCDetails(npc)
        end)
        
        table.insert(resultButtons, btn)
        yOffset = yOffset - 35
    end
    resultsScrollChild:SetWidth(resultsScrollFrame:GetWidth())
    resultsScrollChild:SetHeight(math.abs(yOffset) + 5)
end

-- Function to display NPC details
DisplayNPCDetails = function(npc)
    if not npc then
        npcModel:ClearModel()
        npcModel:SetModel("interface/buttons/talktomequestionmark.m2")
        infoText:SetText("Select an NPC from the results to view details")
        addButton:Hide()
        return
    end
    
    -- Update title
    detailsTitle:SetText(npc.name)
    
    -- If we don't have displayID yet, spawn a temp NPC to get its info
    if not npc.displayID and not isFetchingNPCInfo then
        isFetchingNPCInfo = true
        pendingNPCInfoID = npc.id
        -- Spawn temporary NPC to get its model info
        SendChatMessage(".npc add temp " .. npc.id, "GUILD")
        -- Wait a moment then get info (will be handled by chat event)
    end
    
    -- Update model
    npcModel:ClearModel()
    if npc.displayID then
        npcModel:SetDisplayInfo(npc.displayID)
        npcModel:SetCamDistanceScale(1.5)
        npcModel:SetRotation(0.61)
        npcModel:SetPosition(0, 0, 0)
    else
        -- Show a generic placeholder model while fetching
        npcModel:SetModel("interface/buttons/talktomequestionmark.m2")
        npcModel:SetCamDistanceScale(1.0)
    end
    
    -- Update info text
    local info = {}
    table.insert(info, "|cFFFFD700=== NPC Information ===|r")
    table.insert(info, string.format("|cFF00FF00Name:|r %s", npc.name))
    table.insert(info, string.format("|cFF00FF00Entry ID:|r %d", npc.id))
    table.insert(info, "")
    
    -- Note about fetching model info
    if not npc.displayID then
        if isFetchingNPCInfo and pendingNPCInfoID == npc.id then
            table.insert(info, "|cFFFFFF00Fetching model info...|r")
            table.insert(info, "|cFF888888Spawning temporary NPC to get details|r")
        end
        table.insert(info, "")
    end
    
    -- Only show level/type if available (from .lookup they're not provided)
    if npc.level and npc.level > 0 then
        table.insert(info, string.format("|cFF00FF00Level:|r %d", npc.level))
    end
    if npc.type and npc.type ~= "Unknown" then
        table.insert(info, string.format("|cFF00FF00Type:|r %s", npc.type))
    end
    if npc.displayID then
        table.insert(info, string.format("|cFF00FF00Display ID:|r %d", npc.displayID))
    else
        table.insert(info, "|cFF888888Display ID: Not available from .lookup|r")
    end
    table.insert(info, "")
    table.insert(info, "|cFFFFD700=== TrinityCore Commands ===|r")
    table.insert(info, "|cFF888888Use these commands to spawn:|r")
    table.insert(info, string.format("|cFFFFFF00.npc add %d|r", npc.id))
    table.insert(info, string.format("|cFFFFFF00.npc add temp %d|r (temporary)", npc.id))
    
    infoText:SetText(table.concat(info, "\n"))
    
    -- Update text width
    local scrollWidth = infoScrollFrame:GetWidth()
    if scrollWidth > 0 then
        infoText:SetWidth(scrollWidth - 40)
    end
    
    -- Adjust scroll child size
    local textHeight = infoText:GetStringHeight()
    infoScrollChild:SetWidth(math.max(scrollWidth, 1))
    infoScrollChild:SetHeight(math.max(textHeight + 10, infoScrollFrame:GetHeight()))
    
    -- Show add button
    addButton:Show()
end

-- Function to search NPCs using TrinityCore command
SearchNPCs = function(query)
    if not query or query == "" then
        statusLabel:SetText("Please enter a search term")
        statusLabel:SetTextColor(1, 0.5, 0, 1)  -- Orange
        return
    end
    
    -- Clear previous results
    searchResults = {}
    selectedNPC = nil
    isSearching = true
    searchStartTime = GetTime()
    currentSearchQuery = query
    timeSinceLastResult = 0
    
    -- Update status
    statusLabel:SetText("Searching...")
    statusLabel:SetTextColor(1, 1, 0, 1)  -- Yellow
    
    -- Execute TrinityCore lookup command
    SendChatMessage(".lookup creature " .. query, "GUILD")
    
    -- Note: Results will be captured by the CHAT_MSG_SYSTEM event handler
end

-- Chat message event frame for parsing .lookup creature results and .npc info
local chatFrame = CreateFrame("Frame")
chatFrame:RegisterEvent("CHAT_MSG_SYSTEM")
chatFrame:SetScript("OnEvent", function(self, event, message)
    -- print("[ATA Debug] CHAT_MSG_SYSTEM event handler started")
    -- Debug: print all messages when fetching NPC info
    if isFetchingNPCInfo then
        print("[ATA Debug] CHAT_MSG_SYSTEM: " .. tostring(message))
    end
    
    -- Handle .lookup creature results
    if isSearching then
        -- Parse TrinityCore .lookup creature output
        -- Format: "Creatures found: |cffffffff|Hcreature_entry:123|h[Name]|h|r"
        -- Or: "ID: |cffffffff|Hcreature_entry:123|h[123]|h|r - Name"
        
        -- Pattern 1: |Hcreature_entry:ID|h[Name]|h
        local id, name = message:match("|Hcreature_entry:(%d+)|h%[(.-)%]|h")
        
        if id and name then
            -- Found a creature result
            table.insert(searchResults, {
                id = tonumber(id),
                name = name,
                level = 0,  -- Level not provided by lookup command
                type = "Unknown",  -- Type not provided by lookup command
                displayID = nil  -- Will be fetched via .npc info when selected
            })
        elseif message:find("Creatures found") or message:find("creatures found") then
            -- Start of results
            searchResults = {}
        elseif (message:find("No creatures found") or message:find("no creatures found")) and isSearching then
            -- No results found
            isSearching = false
            statusLabel:SetText("No creatures found")
            statusLabel:SetTextColor(1, 0.5, 0, 1)  -- Orange
            DisplayResults()
        end
        
        -- Check if we've been collecting results for a bit (timeout after 2 seconds)
        if isSearching and (GetTime() - searchStartTime) > 2 and #searchResults > 0 then
            isSearching = false
            statusLabel:SetText(string.format("Found %d creature(s)", #searchResults))
            statusLabel:SetTextColor(0, 1, 0, 1)  -- Green
            DisplayResults()
        end
    end
    
    -- Handle temp NPC spawn confirmation
    if isFetchingNPCInfo and selectedNPC and pendingNPCInfoID == selectedNPC.id then
        print("[ATA Debug] Checking message for spawn confirmation...")
        -- Check if NPC was spawned successfully
        if message:find("Spawned") or message:find("spawned") then
            print("[ATA Debug] NPC spawned detected! Message: " .. message)
            print("[ATA Debug] Waiting 1.5s then getting info...")
            -- NPC spawned, wait for it to fully appear, then get its info
            C_Timer.After(1.5, function()
                if isFetchingNPCInfo and selectedNPC and pendingNPCInfoID == selectedNPC.id then
                    print("[ATA Debug] Sending .npc info command")
                    SendChatMessage(".npc info", "GUILD")
                end
            end)
        end
    end
    
    -- Handle .npc info results
    if isFetchingNPCInfo and selectedNPC and pendingNPCInfoID == selectedNPC.id then
        -- Debug: print the message to see what we're getting
        if message:find("Model ID") or message:find("Entry ID") then
            print("[ATA Debug] Got message: " .. message)
        end
        
        -- Parse Model ID from .npc info output
        -- Format: "Model ID: 12345"
        local modelID = message:match("Model ID:%s*(%d+)")
        local entryID = message:match("Entry ID:%s*(%d+)")
        
        if modelID then
            print("[ATA Debug] Found Model ID: " .. modelID)
        end
        if entryID then
            print("[ATA Debug] Found Entry ID: " .. entryID)
        end
        
        -- Check if this info matches our selected NPC
        if modelID and entryID and tonumber(entryID) == selectedNPC.id then
            print("[ATA Debug] Match! Updating display with Model ID: " .. modelID)
            selectedNPC.displayID = tonumber(modelID)
            isFetchingNPCInfo = false
            pendingNPCInfoID = nil
            
            -- Delete the temporary NPC
            SendChatMessage(".npc delete", "GUILD")
            
            -- Refresh the display with the new info
            DisplayNPCDetails(selectedNPC)
        end
    end
end)

-- Delayed result display (in case chat messages come in batches)
local updateFrame = CreateFrame("Frame")
local timeSinceLastResult = 0
updateFrame:SetScript("OnUpdate", function(self, elapsed)
    if not isSearching then return end
    
    timeSinceLastResult = timeSinceLastResult + elapsed
    
    -- If we have results and haven't received any new ones for 0.5 seconds, display them
    if #searchResults > 0 and timeSinceLastResult > 0.5 then
        isSearching = false
        statusLabel:SetText(string.format("Found %d creature(s)", #searchResults))
        statusLabel:SetTextColor(0, 1, 0, 1)  -- Green
        DisplayResults()
        timeSinceLastResult = 0
    end
    
    -- Timeout after 3 seconds
    if (GetTime() - searchStartTime) > 3 then
        isSearching = false
        if #searchResults == 0 then
            statusLabel:SetText("Search timeout - no results")
            statusLabel:SetTextColor(1, 0, 0, 1)  -- Red
        else
            statusLabel:SetText(string.format("Found %d creature(s)", #searchResults))
            statusLabel:SetTextColor(0, 1, 0, 1)  -- Green
        end
        DisplayResults()
        timeSinceLastResult = 0
    end
end)

-- Reset timer when new results come in
local function ResetResultTimer()
    timeSinceLastResult = 0
end

-- Search button handler
searchButton:SetScript("OnClick", function()
    local query = searchBox:GetText()
    SearchNPCs(query)
    DisplayResults()
end)

-- Enter key in search box
searchBox:SetScript("OnEnterPressed", function(self)
    local query = self:GetText()
    SearchNPCs(query)
    DisplayResults()
    self:ClearFocus()
end)

-- Clear button handler
clearButton:SetScript("OnClick", function()
    searchBox:SetText("")
    searchResults = {}
    selectedNPC = nil
    isSearching = false
    statusLabel:SetText("")
    DisplayResults()
    DisplayNPCDetails(nil)
end)

-- Add button handler
addButton:SetScript("OnClick", function()
    if selectedNPC then
        -- In a real implementation, this would send a command to the server
        print(string.format("|cFF00FF00[Araxia Trinity Admin]|r Adding NPC: %s (ID: %d)", selectedNPC.name, selectedNPC.id))
        print(string.format("|cFFFFFF00Command:|r .npc add %d", selectedNPC.id))
        -- You could also copy the command to clipboard or execute it directly
    end
end)

-- Update function
function addNPCPanel:Update()
    -- Called when panel is shown
    -- Could refresh data here if needed
end

-- Register panel with main window
local function InitPanel()
    if ATA.MainWindow then
        ATA.MainWindow:RegisterPanel("AddNPC", "Add NPC", addNPCPanel)
    else
        -- Retry if main window not loaded yet
        C_Timer.After(0.1, InitPanel)
    end
end

-- Initialize after a short delay to ensure main window is loaded
C_Timer.After(0.1, InitPanel)

end) -- End of ADDON_LOADED handler