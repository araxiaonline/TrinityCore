-- AraxiaTrinityAdmin Agent Chat Panel
-- Chat interface for communicating with AI agents

local addonName = "AraxiaTrinityAdmin"

-- Wait for addon to load
local initFrame = CreateFrame("Frame")
initFrame:RegisterEvent("ADDON_LOADED")
initFrame:SetScript("OnEvent", function(self, event, loadedAddon)
    if loadedAddon ~= addonName then return end
    self:UnregisterEvent("ADDON_LOADED")
    
    local ATA = AraxiaTrinityAdmin
    if not ATA then return end

-- Create panel frame
local chatPanel = CreateFrame("Frame", "AraxiaTrinityAdminAgentChatPanel", UIParent)
chatPanel:Hide()

-- Chat history storage (persisted via SavedVariables)
local chatHistory = {}  -- { {from="player"|"agent", agent_name, content, timestamp, message_id}, ... }
local MAX_HISTORY = 200

-- Message send status tracking
local pendingMessageId = nil

-- Current selected agent
local selectedAgent = nil
local availableAgents = {}

-- Polling state
local isSubscribed = false

-- Status indicator for message sending
local sendStatusText = nil  -- Will be created after chatPanel is set up

-- ============================================================================
-- Header Section
-- ============================================================================

local title = chatPanel:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
title:SetPoint("TOPLEFT", chatPanel, "TOPLEFT", 10, -10)
title:SetText("Agent Chat")

-- Agent selector dropdown
local agentDropdown = CreateFrame("Frame", "AgentChatAgentDropdown", chatPanel, "UIDropDownMenuTemplate")
agentDropdown:SetPoint("LEFT", title, "RIGHT", 10, -2)

local function GetAgentStatusText(agentName)
    for _, agent in ipairs(availableAgents) do
        if agent.name == agentName then
            return agentName .. (agent.online and " |cFF00FF00(online)|r" or " |cFF888888(offline)|r")
        end
    end
    return agentName or "Select Agent"
end

local function AgentDropdown_OnClick(self, arg1, arg2, checked)
    selectedAgent = arg1
    UIDropDownMenu_SetText(agentDropdown, GetAgentStatusText(arg1))
end

local function AgentDropdown_Initialize(self, level)
    local info = UIDropDownMenu_CreateInfo()
    
    if #availableAgents == 0 then
        info.text = "No agents available"
        info.disabled = true
        info.notCheckable = true
        UIDropDownMenu_AddButton(info)
        return
    end
    
    for _, agent in ipairs(availableAgents) do
        info.text = agent.name .. (agent.online and " |cFF00FF00(online)|r" or " |cFF888888(offline)|r")
        info.arg1 = agent.name
        info.func = AgentDropdown_OnClick
        info.checked = (selectedAgent == agent.name)
        info.notCheckable = false
        UIDropDownMenu_AddButton(info)
    end
end

UIDropDownMenu_SetWidth(agentDropdown, 150)
UIDropDownMenu_SetText(agentDropdown, "Select Agent")
UIDropDownMenu_Initialize(agentDropdown, AgentDropdown_Initialize)

-- Refresh agents button
local refreshAgentsBtn = CreateFrame("Button", nil, chatPanel, "UIPanelButtonTemplate")
refreshAgentsBtn:SetSize(60, 24)
refreshAgentsBtn:SetPoint("LEFT", agentDropdown, "RIGHT", -10, 2)
refreshAgentsBtn:SetText("Reload")
refreshAgentsBtn:SetScript("OnClick", function()
    if AMS then
        AMS.Send("AGENT_LIST_REQUEST", {})
    end
end)
refreshAgentsBtn:SetScript("OnEnter", function(self)
    GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
    GameTooltip:AddLine("Refresh agent list", 1, 1, 1)
    GameTooltip:Show()
end)
refreshAgentsBtn:SetScript("OnLeave", function() GameTooltip:Hide() end)

-- Status indicator (connection status)
local statusText = chatPanel:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
statusText:SetPoint("TOPRIGHT", chatPanel, "TOPRIGHT", -10, -14)
statusText:SetText("|cFF888888Disconnected|r")

-- Message send status indicator (shows Sending.../Delivered)
sendStatusText = chatPanel:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
sendStatusText:SetPoint("TOPRIGHT", chatPanel, "TOPRIGHT", -10, -28)
sendStatusText:SetText("")
sendStatusText:Hide()

-- ============================================================================
-- Chat History Display
-- ============================================================================

local chatContainer = CreateFrame("Frame", nil, chatPanel, "BackdropTemplate")
chatContainer:SetPoint("TOPLEFT", chatPanel, "TOPLEFT", 10, -45)
chatContainer:SetPoint("BOTTOMRIGHT", chatPanel, "BOTTOMRIGHT", -10, 50)
chatContainer:SetBackdrop({
    bgFile = "Interface/Tooltips/UI-Tooltip-Background",
    edgeFile = "Interface/Tooltips/UI-Tooltip-Border",
    tile = true, tileSize = 16, edgeSize = 16,
    insets = { left = 4, right = 4, top = 4, bottom = 4 }
})
chatContainer:SetBackdropColor(0.05, 0.05, 0.05, 0.9)
chatContainer:SetBackdropBorderColor(0.4, 0.4, 0.4, 1)

local chatScroll = CreateFrame("ScrollFrame", "AgentChatScrollFrame", chatContainer, "UIPanelScrollFrameTemplate")
chatScroll:SetPoint("TOPLEFT", chatContainer, "TOPLEFT", 5, -8)
chatScroll:SetPoint("BOTTOMRIGHT", chatContainer, "BOTTOMRIGHT", -28, 8)

local chatScrollChild = CreateFrame("Frame", nil, chatScroll)
chatScrollChild:SetWidth(chatScroll:GetWidth() - 10)
chatScrollChild:SetHeight(1)
chatScroll:SetScrollChild(chatScrollChild)

-- Message display elements
local messageFrames = {}

local function FormatTimestamp(ts)
    if not ts then return "" end
    return date("%H:%M", ts)
end

local function AddMessageToDisplay(from, agentName, content, timestamp, isFromPlayer)
    local frame = CreateFrame("Frame", nil, chatScrollChild)
    local frameWidth = chatScrollChild:GetWidth() - 20
    if frameWidth < 100 then frameWidth = 400 end  -- Fallback width
    frame:SetWidth(frameWidth)
    
    -- Header line (name + timestamp)
    local header = frame:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
    header:SetPoint("TOPLEFT", frame, "TOPLEFT", 15, -5)
    
    if isFromPlayer then
        header:SetText("|cFF00CCFF" .. UnitName("player") .. "|r |cFF888888" .. FormatTimestamp(timestamp) .. "|r")
    else
        header:SetText("|cFF00FF00" .. (agentName or "Agent") .. "|r |cFF888888" .. FormatTimestamp(timestamp) .. "|r")
    end
    
    -- Content - displayed below header
    local contentText = frame:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
    contentText:SetPoint("TOPLEFT", header, "BOTTOMLEFT", 0, -2)
    contentText:SetWidth(frameWidth - 20)
    contentText:SetJustifyH("LEFT")
    contentText:SetWordWrap(true)
    local displayContent = content or ""
    if displayContent == "" then displayContent = "[empty]" end
    contentText:SetText(displayContent)
    
    -- Calculate height
    local contentHeight = contentText:GetStringHeight()
    frame:SetHeight(header:GetStringHeight() + contentHeight + 15)
    
    -- Position frame
    local yOffset = 0
    for _, f in ipairs(messageFrames) do
        yOffset = yOffset + f:GetHeight() + 5
    end
    frame:SetPoint("TOPLEFT", chatScrollChild, "TOPLEFT", 0, -yOffset)
    
    table.insert(messageFrames, frame)
    frame:Show()
    
    -- Update scroll child height
    chatScrollChild:SetHeight(yOffset + frame:GetHeight() + 10)
    
    -- Scroll to bottom
    C_Timer.After(0.01, function()
        chatScroll:SetVerticalScroll(chatScroll:GetVerticalScrollRange())
    end)
    
    return frame
end

local function ClearMessageDisplay()
    for _, frame in ipairs(messageFrames) do
        frame:Hide()
        frame:SetParent(nil)
    end
    messageFrames = {}
    chatScrollChild:SetHeight(1)
end

local function RefreshChatDisplay()
    ClearMessageDisplay()
    
    for _, msg in ipairs(chatHistory) do
        local isFromPlayer = (msg.from == "player")
        AddMessageToDisplay(msg.from, msg.agent_name, msg.content, msg.timestamp, isFromPlayer)
    end
end

-- ============================================================================
-- Message Input
-- ============================================================================

local inputContainer = CreateFrame("Frame", nil, chatPanel, "BackdropTemplate")
inputContainer:SetPoint("BOTTOMLEFT", chatPanel, "BOTTOMLEFT", 10, 10)
inputContainer:SetPoint("BOTTOMRIGHT", chatPanel, "BOTTOMRIGHT", -80, 10)
inputContainer:SetHeight(30)
inputContainer:SetBackdrop({
    bgFile = "Interface/Tooltips/UI-Tooltip-Background",
    edgeFile = "Interface/Tooltips/UI-Tooltip-Border",
    tile = true, tileSize = 16, edgeSize = 12,
    insets = { left = 3, right = 3, top = 3, bottom = 3 }
})
inputContainer:SetBackdropColor(0.1, 0.1, 0.1, 0.8)
inputContainer:SetBackdropBorderColor(0.3, 0.3, 0.3, 1)

local inputBox = CreateFrame("EditBox", "AgentChatInputBox", inputContainer)
inputBox:SetPoint("TOPLEFT", inputContainer, "TOPLEFT", 8, -6)
inputBox:SetPoint("BOTTOMRIGHT", inputContainer, "BOTTOMRIGHT", -8, 6)
inputBox:SetFontObject("ChatFontNormal")
inputBox:SetAutoFocus(false)
inputBox:SetMaxLetters(1000)

local function SendMessage()
    local text = inputBox:GetText()
    if not text or text == "" then return end
    
    if not selectedAgent then
        print("|cFFFF0000[Agent Chat]|r Please select an agent first")
        return
    end
    
    if not AMS then
        print("|cFFFF0000[Agent Chat]|r AMS not available")
        return
    end
    
    -- Get context (current target info)
    local context = nil
    if UnitExists("target") then
        local guid = UnitGUID("target")
        context = {
            target_guid = guid,
            target_name = UnitName("target"),
            target_level = UnitLevel("target")
        }
    end
    
    -- Show sending status
    pendingMessageId = "local_" .. time()
    sendStatusText:SetText("|cFFFFFF00Sending...|r")
    sendStatusText:Show()
    
    -- Send message
    AMS.Send("AGENT_SEND_MESSAGE", {
        agent_name = selectedAgent,
        content = text,
        context = context
    })
    
    -- Add to local history immediately
    local msg = {
        from = "player",
        agent_name = selectedAgent,
        content = text,
        timestamp = time(),
        message_id = pendingMessageId
    }
    table.insert(chatHistory, msg)
    
    -- Cap history
    while #chatHistory > MAX_HISTORY do
        table.remove(chatHistory, 1)
    end
    
    -- Refresh display
    AddMessageToDisplay("player", selectedAgent, text, msg.timestamp, true)
    
    -- Clear input
    inputBox:SetText("")
end

inputBox:SetScript("OnEnterPressed", function()
    SendMessage()
end)

inputBox:SetScript("OnEscapePressed", function(self)
    self:ClearFocus()
end)

-- Send button
local sendBtn = CreateFrame("Button", nil, chatPanel, "UIPanelButtonTemplate")
sendBtn:SetSize(60, 30)
sendBtn:SetPoint("LEFT", inputContainer, "RIGHT", 5, 0)
sendBtn:SetText("Send")
sendBtn:SetScript("OnClick", SendMessage)

-- ============================================================================
-- AMS Response Handlers
-- ============================================================================

local function InitAMSHandlers()
    if not AMS then
        C_Timer.After(0.5, InitAMSHandlers)
        return
    end
    
    -- Handle agent list response
    AMS.RegisterHandler("AGENT_LIST_RESPONSE", function(data)
        if data.success then
            availableAgents = data.agents or {}
            UIDropDownMenu_Initialize(agentDropdown, AgentDropdown_Initialize)
            
            -- Auto-select first agent if none selected, or update status display
            if not selectedAgent and #availableAgents > 0 then
                selectedAgent = availableAgents[1].name
            end
            if selectedAgent then
                UIDropDownMenu_SetText(agentDropdown, GetAgentStatusText(selectedAgent))
            end
            
            print("|cFF00FF00[Agent Chat]|r Found " .. #availableAgents .. " agent(s)")
        end
    end)
    
    -- Handle send confirmation
    AMS.RegisterHandler("AGENT_SEND_MESSAGE_RESPONSE", function(data)
        if data.success then
            -- Show delivered status briefly
            sendStatusText:SetText("|cFF00FF00Delivered ✓|r")
            C_Timer.After(3, function()
                sendStatusText:SetText("")
                sendStatusText:Hide()
            end)
        else
            sendStatusText:SetText("|cFFFF0000Failed|r")
            print("|cFFFF0000[Agent Chat]|r Failed to send: " .. (data.error or "Unknown error"))
            C_Timer.After(5, function()
                sendStatusText:SetText("")
                sendStatusText:Hide()
            end)
        end
        pendingMessageId = nil
    end)
    
    -- Handle agent responses (push delivery)
    AMS.RegisterHandler("AGENT_MESSAGE_RESPONSE", function(data)
        if data.success and data.messages then
            for _, msg in ipairs(data.messages) do
                -- Add to history
                local historyEntry = {
                    from = "agent",
                    agent_name = msg.from_agent,
                    content = msg.content,
                    timestamp = msg.timestamp,
                    message_id = msg.message_id,
                    reply_to_id = msg.reply_to_id
                }
                table.insert(chatHistory, historyEntry)
                
                -- Add to display
                AddMessageToDisplay("agent", msg.from_agent, msg.content, msg.timestamp, false)
                
                -- Show notification if panel not visible
                if not chatPanel:IsVisible() then
                    print("|cFF00FF00[" .. msg.from_agent .. "]|r " .. msg.content:sub(1, 100) .. (msg.content:len() > 100 and "..." or ""))
                end
            end
            
            -- Cap history
            while #chatHistory > MAX_HISTORY do
                table.remove(chatHistory, 1)
            end
        end
    end)
    
    -- Handle poll responses (manual polling fallback)
    AMS.RegisterHandler("AGENT_POLL_RESPONSES_RESULT", function(data)
        if data.success and data.messages then
            for _, msg in ipairs(data.messages) do
                local historyEntry = {
                    from = "agent",
                    agent_name = msg.from_agent,
                    content = msg.content,
                    timestamp = msg.timestamp,
                    message_id = msg.message_id
                }
                table.insert(chatHistory, historyEntry)
                AddMessageToDisplay("agent", msg.from_agent, msg.content, msg.timestamp, false)
            end
        end
    end)
    
    print("[Agent Chat] AMS handlers registered")
end

-- ============================================================================
-- Panel Lifecycle
-- ============================================================================

-- Subscribe/unsubscribe when panel shows/hides
chatPanel:SetScript("OnShow", function()
    if AMS then
        AMS.Send("AGENT_CHAT_SUBSCRIBE", {})
        AMS.Send("AGENT_LIST_REQUEST", {})
        isSubscribed = true
        statusText:SetText("|cFF00FF00Connected|r")
    end
    inputBox:SetFocus()
end)

chatPanel:SetScript("OnHide", function()
    if AMS and isSubscribed then
        AMS.Send("AGENT_CHAT_UNSUBSCRIBE", {})
        isSubscribed = false
        statusText:SetText("|cFF888888Disconnected|r")
    end
    inputBox:ClearFocus()
end)

-- Manual poll button (hidden, for debugging)
local pollBtn = CreateFrame("Button", nil, chatPanel, "UIPanelButtonTemplate")
pollBtn:SetSize(60, 22)
pollBtn:SetPoint("BOTTOMRIGHT", chatPanel, "BOTTOMRIGHT", -10, 45)
pollBtn:SetText("Poll")
pollBtn:Hide()  -- Hidden by default
pollBtn:SetScript("OnClick", function()
    if AMS then
        AMS.Send("AGENT_POLL_RESPONSES", {})
    end
end)

-- Clear chat button (positioned left of status text)
local clearBtn = CreateFrame("Button", nil, chatPanel, "UIPanelButtonTemplate")
clearBtn:SetSize(50, 22)
clearBtn:SetPoint("RIGHT", statusText, "LEFT", -10, 0)
clearBtn:SetText("Clear")
clearBtn:SetScript("OnClick", function()
    chatHistory = {}
    ClearMessageDisplay()
end)

-- ============================================================================
-- SavedVariables Persistence
-- ============================================================================

-- Load chat history from SavedVariables on login
local function LoadChatHistory()
    if AraxiaTrinityAdminDB and AraxiaTrinityAdminDB.AgentChatHistory then
        chatHistory = AraxiaTrinityAdminDB.AgentChatHistory
        -- Refresh display with loaded history
        C_Timer.After(0.5, function()
            RefreshChatDisplay()
        end)
    end
end

-- Save chat history to SavedVariables on logout
local function SaveChatHistory()
    if not AraxiaTrinityAdminDB then
        AraxiaTrinityAdminDB = {}
    end
    AraxiaTrinityAdminDB.AgentChatHistory = chatHistory
end

-- Register for logout event to save history
local saveFrame = CreateFrame("Frame")
saveFrame:RegisterEvent("PLAYER_LOGOUT")
saveFrame:SetScript("OnEvent", function(self, event)
    if event == "PLAYER_LOGOUT" then
        SaveChatHistory()
    end
end)

-- ============================================================================
-- Register with MainWindow
-- ============================================================================

local function InitPanel()
    if ATA.MainWindow then
        ATA.MainWindow:RegisterPanel("AgentChat", "Agent Chat", chatPanel)
        InitAMSHandlers()
        LoadChatHistory()  -- Load saved history after init
    else
        C_Timer.After(0.1, InitPanel)
    end
end

C_Timer.After(0.1, InitPanel)

-- Make available globally for debugging
ATA.AgentChatPanel = chatPanel
ATA.AgentChatHistory = chatHistory
ATA.SaveAgentChatHistory = SaveChatHistory  -- Allow manual save

end)  -- End ADDON_LOADED handler
