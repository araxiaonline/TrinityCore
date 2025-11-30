# Araxia Trinity Admin - Development Documentation

## Project Overview
World of Warcraft addon for TrinityCore server administration and content creation. Provides in-game tools for viewing NPC information, managing content, and accessing server commands.

## Architecture

### File Structure
```
AraxiaTrinityAdmin/
├── AraxiaTrinityAdmin.toc    # Addon manifest (Interface 110205)
├── Core.lua                   # Main initialization, namespace, utilities
├── UI/
│   ├── MainWindow.lua         # Main window with tab navigation
│   └── Panels/
│       └── NPCInfoPanel.lua   # NPC information display panel
├── README.md                  # User documentation
└── CASCADE.md                 # This file
```

### Load Order
1. `Core.lua` - Creates `AraxiaTrinityAdmin` namespace, registers events
2. `UI/MainWindow.lua` - Creates main window (waits for ADDON_LOADED)
3. `UI/Panels/NPCInfoPanel.lua` - Creates and registers NPC panel (waits for ADDON_LOADED)

## Key Components

### Namespace: `AraxiaTrinityAdmin` (ATA)
Global table containing all addon functionality.

**Properties:**
- `version` - Addon version string
- `loaded` - Boolean, true after ADDON_LOADED
- `MainWindow` - Reference to main window frame
- `PanelContainer` - Reference to panel content area

**Methods:**
- `InitDatabase()` - Initialize SavedVariables with defaults
- `GetTargetNPCInfo()` - Returns table of NPC data from current target

### SavedVariables: `AraxiaTrinityAdminDB`
Persisted data stored in WTF folder.

**Structure:**
```lua
{
    windowPosition = {point, x, y},
    windowSize = {width, height},
    windowShown = boolean,
    selectedPanel = "PanelName"
}
```

### Main Window (`UI/MainWindow.lua`)
Multi-panel tabbed interface.

**Key Frames:**
- `mainWindow` - Main frame (800x600 default, resizable 600x400 to 1400x1000)
- `mainWindow.tabBar` - Horizontal tab bar at top (30px height)
- `mainWindow.panelContainer` - Content area below tabs
- `mainWindow.tabs[]` - Array of tab buttons

**Methods:**
- `RegisterPanel(name, displayName, panelFrame)` - Add new panel
- `ShowPanel(name)` - Switch to specified panel

**Important Notes:**
- Uses `BasicFrameTemplateWithInset` template
- `mainWindow.Inset` may be nil - fallback to mainWindow with margins
- All child frames must wait for ADDON_LOADED event
- Panels are reparented to `panelContainer` and must have points cleared before anchoring

### NPC Info Panel (`UI/Panels/NPCInfoPanel.lua`)
Split-panel display: text info on left, 3D model on right.

**Layout:**
- Left: Scrollable text info (50% width)
- Right: PlayerModel viewer (50% width)
- Both panels aligned to same height

**Data Displayed:**
- Basic: Name, NPC ID, GUID
- Stats: Level, Health, Power (if applicable), Classification, Reaction
- Additional: Creature Type, Faction, Tagged status
- Commands: TrinityCore console commands for this NPC

**Auto-update:** Listens to `PLAYER_TARGET_CHANGED` event

## WoW API Limitations

### Available for NPCs (Client-side)
- `UnitName()`, `UnitGUID()`, `UnitLevel()`
- `UnitHealth()`, `UnitHealthMax()`
- `UnitPower()`, `UnitPowerMax()`, `UnitPowerType()`
- `UnitClassification()` - normal/elite/rare/worldboss
- `UnitCreatureType()` - Humanoid/Beast/etc
- `UnitFactionGroup()` - Horde/Alliance/Neutral
- `UnitReaction()` - Hostile/Neutral/Friendly (1-8)
- `UnitIsTapDenied()` - Tagged by another player

### NOT Available for NPCs (Returns 0)
- `UnitArmor()` - Always 0 for NPCs
- `UnitAttackPower()` - Always 0 for NPCs
- `GetSpellBonusDamage()` - Player-only
- Detailed combat stats require server-side database queries

## Common Issues & Solutions

### Issue: "Inset is nil" error
**Cause:** `BasicFrameTemplateWithInset` doesn't always create Inset frame  
**Solution:** Use fallback: `local contentArea = mainWindow.Inset or mainWindow`

### Issue: Panel shows fullscreen instead of in container
**Cause:** Panel created with UIParent, retains original anchor points  
**Solution:** `panelFrame:ClearAllPoints()` before `SetPoint()` in RegisterPanel

### Issue: SetBackdrop error
**Cause:** Backdrop API moved to mixin in WoW 9.0+  
**Solution:** Use `"BackdropTemplate"` in CreateFrame: `CreateFrame("Frame", nil, parent, "BackdropTemplate")`

### Issue: SetMinResize/SetMaxResize error
**Cause:** Deprecated in modern WoW  
**Solution:** Use `SetResizeBounds(minW, minH, maxW, maxH)` instead

### Issue: Main window not initialized when slash command runs
**Cause:** UI files loading before namespace ready  
**Solution:** Wrap all UI code in ADDON_LOADED event handler

### Issue: 3D model too zoomed in
**Cause:** Default camera distance too close  
**Solution:** `SetCamDistanceScale(1.5)` to zoom out, `SetPosition(0,0,0)` to center

## Adding New Panels

### Step 1: Create Panel File
Create `UI/Panels/YourPanel.lua`:

```lua
local addonName = "AraxiaTrinityAdmin"

local initFrame = CreateFrame("Frame")
initFrame:RegisterEvent("ADDON_LOADED")
initFrame:SetScript("OnEvent", function(self, event, loadedAddon)
    if loadedAddon ~= addonName then return end
    self:UnregisterEvent("ADDON_LOADED")
    
    local ATA = AraxiaTrinityAdmin
    if not ATA then return end

    -- Create panel frame
    local yourPanel = CreateFrame("Frame", "YourPanelName", UIParent)
    yourPanel:Hide()
    
    -- Add your UI elements here
    
    -- Update function
    function yourPanel:Update()
        -- Update panel content
    end
    
    -- Register with main window
    local function InitPanel()
        if ATA.MainWindow then
            ATA.MainWindow:RegisterPanel("YourPanel", "Display Name", yourPanel)
        else
            C_Timer.After(0.1, InitPanel)
        end
    end
    
    C_Timer.After(0.1, InitPanel)

end)
```

### Step 2: Add to TOC
Add line to `AraxiaTrinityAdmin.toc`:
```
UI/Panels/YourPanel.lua
```

### Step 3: Reload
`/reload` in-game to load new panel

## Slash Commands
- `/araxia admin` - Toggle main window
- `/araxia` - Show help

## TrinityCore Integration
Addon provides quick-copy commands for server console:
- `.npc info <id>` - Full NPC database info
- `.lookup creature <name>` - Search creatures
- `.npc set entry <id>` - Change targeted NPC

## Development Notes

### Code Style
- Use `local` for all variables unless global needed
- Prefix addon globals with `ATA`
- Use descriptive variable names
- Comment complex logic

### Event Handling
- Always unregister one-time events after firing
- Use `C_Timer.After()` for delayed initialization
- Check frame existence before accessing

### Frame Hierarchy
```
UIParent
└── mainWindow (AraxiaTrinityAdminMainWindow)
    ├── tabBar (tab buttons)
    └── panelContainer
        └── [panel frames reparented here]
```

### Color Codes
- `|cFFFFD700` - Gold (section headers)
- `|cFF00FF00` - Green (labels, friendly)
- `|cFFFFFF00` - Yellow (values, neutral)
- `|cFFFF0000` - Red (hostile, errors)
- `|cFF888888` - Gray (notes, disabled)
- `|cFF0070DD` - Blue (rare)
- `|cFFFF00FF` - Purple (rare elite)

## Future Expansion Ideas
- Item Info Panel - View item details, stats, sources
- Quest Info Panel - Quest chains, requirements, rewards
- Spawn Manager - View/edit creature spawns on map
- Teleport Panel - Quick teleport to locations
- GM Tools - Player management, server commands
- Database Search - Search creatures/items/quests
- Macro Builder - Generate TrinityCore command macros

## Version History
- v1.0.0 - Initial release with NPC Info panel, tab navigation, 3D model viewer
