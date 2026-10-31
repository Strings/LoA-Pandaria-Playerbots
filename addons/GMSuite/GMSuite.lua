-- GMSuite: a panel for the UltimateMoP GM test suite (T-048). It only sends GM commands; the server does the
-- work. The checkpoint and kit lists come from the server (.gmsuite sync answers with addon messages), so new
-- rows show up without changing the addon. Open with /gms.

local PREFIX = "GMSuite"
local BUTTON_W, BUTTON_H, COLUMNS = 150, 22, 3

local checkpoints, kits = {}, {}
local pendingCheckpoints, pendingKits = {}, {}

local function Command(text)
    SendChatMessage(text, "SAY")
end

local frame = CreateFrame("Frame", "GMSuiteFrame", UIParent, "BasicFrameTemplateWithInset")
frame:SetSize(COLUMNS * (BUTTON_W + 6) + 24, 420)
frame:SetPoint("CENTER")
frame:SetMovable(true)
frame:EnableMouse(true)
frame:RegisterForDrag("LeftButton")
frame:SetScript("OnDragStart", frame.StartMoving)
frame:SetScript("OnDragStop", frame.StopMovingOrSizing)
frame:Hide()
tinsert(UISpecialFrames, "GMSuiteFrame")

frame.title = frame:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
frame.title:SetPoint("TOP", 0, -5)
frame.title:SetText("GMSuite (applies to your target, or you)")

local content = CreateFrame("Frame", nil, frame)
content:SetPoint("TOPLEFT", 12, -30)
content:SetPoint("BOTTOMRIGHT", -12, 12)

local pool = {}
local function GetButton(index)
    local button = pool[index]
    if not button then
        button = CreateFrame("Button", nil, content, "UIPanelButtonTemplate")
        button:SetSize(BUTTON_W, BUTTON_H)
        button:SetScript("OnEnter", function(self)
            if self.tooltip then
                GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
                GameTooltip:SetText(self:GetText())
                GameTooltip:AddLine(self.tooltip, 1, 1, 1, true)
                GameTooltip:Show()
            end
        end)
        button:SetScript("OnLeave", GameTooltip_Hide)
        pool[index] = button
    end
    button:Show()
    return button
end

local headers = {}
local function Header(index, text, y)
    local header = headers[index]
    if not header then
        header = content:CreateFontString(nil, "OVERLAY", "GameFontNormal")
        headers[index] = header
    end
    header:SetPoint("TOPLEFT", 0, y)
    header:SetText(text)
    return y - 18
end

-- Lays out one section of buttons in a grid; returns the next free y.
local function Section(buttonIndex, entries, y)
    for i, entry in ipairs(entries) do
        local button = GetButton(buttonIndex + i)
        local column, row = (i - 1) % COLUMNS, math.floor((i - 1) / COLUMNS)
        button:ClearAllPoints()
        button:SetPoint("TOPLEFT", column * (BUTTON_W + 6), y - row * (BUTTON_H + 4))
        button:SetText(entry.label)
        button.tooltip = entry.tooltip
        button:SetScript("OnClick", entry.onClick)
    end
    return buttonIndex + #entries, y - math.ceil(#entries / COLUMNS) * (BUTTON_H + 4) - 6
end

StaticPopupDialogs["GMSUITE_APPLY"] = {
    text = "Apply %s to %s? Level, gear and Heartseed state change and cannot be undone.",
    button1 = YES,
    button2 = NO,
    OnAccept = function(_, data) Command(data) end,
    timeout = 0,
    whileDead = true,
    hideOnEscape = true,
}

local function TargetName()
    return UnitIsPlayer("target") and UnitName("target") or UnitName("player")
end

local levelBox, spellBox

local function Layout()
    for _, button in ipairs(pool) do button:Hide() end

    local entries = {}
    for _, checkpoint in ipairs(checkpoints) do
        tinsert(entries, {
            label = checkpoint.name,
            tooltip = "[" .. checkpoint.category .. "] " .. checkpoint.description,
            onClick = function()
                StaticPopup_Show("GMSUITE_APPLY", checkpoint.name, TargetName(), ".checkpoint apply " .. checkpoint.name)
            end,
        })
    end
    local index, y = 0, 0
    y = Header(1, "Checkpoints", y)
    index, y = Section(index, entries, y)

    entries = {}
    for _, kit in ipairs(kits) do
        tinsert(entries, {
            label = kit.name,
            tooltip = "Levels " .. kit.levels .. ". " .. kit.description,
            onClick = function() Command(".kit apply " .. kit.name) end,
        })
    end
    y = Header(2, "Kits", y)
    index, y = Section(index, entries, y)

    y = Header(3, "Ability tester", y)
    index, y = Section(index, {
        { label = "Refresh", tooltip = "Reset cooldowns, full health and power (revives too).", onClick = function() Command(".gmsuite refresh") end },
        { label = "No cooldowns", tooltip = "Toggle .cheat cooldown.", onClick = function() Command(".cheat cooldown") end },
        { label = "Free power", tooltip = "Toggle .cheat power.", onClick = function() Command(".cheat power") end },
        { label = "God mode", tooltip = "Toggle .cheat god.", onClick = function() Command(".cheat god") end },
        { label = "Learn class", tooltip = "Learn every class spell for your level (yourself only).", onClick = function() Command(".learn all my class") end },
        { label = "Heartseed info", tooltip = "Rank, Growth, majors, essences and story flags.", onClick = function() Command(".heartseed info") end },
        { label = "Choose majors", tooltip = "Open the Heartseed choice window (.seed).", onClick = function() Command(".seed") end },
        { label = "Reload lists", tooltip = "Ask the server for the checkpoint and kit lists again.", onClick = function() Command(".gmsuite sync") end },
    }, y)

    levelBox:ClearAllPoints()
    levelBox:SetPoint("TOPLEFT", 6, y)
    spellBox:ClearAllPoints()
    spellBox:SetPoint("TOPLEFT", BUTTON_W + 12, y)
    y = y - 40

    frame:SetHeight(-y + 50)
end

local function EditBox(label, onEnter)
    local box = CreateFrame("EditBox", nil, content, "InputBoxTemplate")
    box:SetSize(BUTTON_W - 10, 20)
    box:SetAutoFocus(false)
    box:SetNumeric(true)
    box:SetScript("OnEnterPressed", function(self)
        local value = tonumber(self:GetText())
        if value then onEnter(value) end
        self:SetText("")
        self:ClearFocus()
    end)
    box:SetScript("OnEscapePressed", box.ClearFocus)
    local text = box:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    text:SetPoint("BOTTOMLEFT", box, "TOPLEFT", -4, 2)
    text:SetText(label)
    return box
end

levelBox = EditBox("Set level (Enter)", function(level)
    if UnitIsPlayer("target") and not UnitIsUnit("target", "player") then
        Command(".character level " .. UnitName("target") .. " " .. level)
    else
        Command(".levelup " .. (level - UnitLevel("player")))
    end
end)
spellBox = EditBox("Learn spell id (Enter)", function(spell) Command(".learn " .. spell) end)

local events = CreateFrame("Frame")
events:RegisterEvent("ADDON_LOADED")
events:RegisterEvent("CHAT_MSG_ADDON")
events:SetScript("OnEvent", function(_, event, ...)
    if event == "ADDON_LOADED" then
        if ... == "GMSuite" then
            GMSuiteDB = GMSuiteDB or {}
            RegisterAddonMessagePrefix(PREFIX)
            Layout()
        end
        return
    end

    local prefix, message, _, sender = ...
    if prefix ~= PREFIX or (sender and Ambiguate(sender, "none") ~= UnitName("player")) then
        return
    end
    local kind, a, b, c = strsplit("\t", message)
    if kind == "C" then
        tinsert(pendingCheckpoints, { name = a, category = b or "", description = c or "" })
    elseif kind == "K" then
        tinsert(pendingKits, { name = a, levels = b or "", description = c or "" })
    elseif kind == "E" then
        checkpoints, kits = pendingCheckpoints, pendingKits
        pendingCheckpoints, pendingKits = {}, {}
        Layout()
    end
end)

frame:SetScript("OnShow", function()
    if #checkpoints == 0 then
        Command(".gmsuite sync")
    end
end)

SLASH_GMSUITE1 = "/gms"
SLASH_GMSUITE2 = "/gmsuite"
SlashCmdList.GMSUITE = function()
    if frame:IsShown() then frame:Hide() else frame:Show() end
end
