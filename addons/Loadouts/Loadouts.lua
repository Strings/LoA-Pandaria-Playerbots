-- Loadouts: a panel for UltimateMoP's server-side spec loadouts (T-017). It only sends .loadout commands; the
-- list comes from the server (.loadout sync answers with addon messages). Open with /lo.

local PREFIX = "Loadouts"
local ROW_H = 24

local loadouts, pending = {}, {}

local function Command(text)
    SendChatMessage(text, "SAY")
end

-- 5.4.8 has no C_Timer: a tiny OnUpdate delay so the list refreshes after the server has applied a command.
local delay = CreateFrame("Frame")
delay:Hide()
delay:SetScript("OnUpdate", function(self, elapsed)
    self.left = self.left - elapsed
    if self.left <= 0 then
        self:Hide()
        Command(".loadout sync")
    end
end)
local function SyncSoon()
    delay.left = 0.5
    delay:Show()
end

local frame = CreateFrame("Frame", "LoadoutsFrame", UIParent, "BasicFrameTemplateWithInset")
frame:SetSize(360, 140)
frame:SetPoint("CENTER")
frame:SetMovable(true)
frame:EnableMouse(true)
frame:RegisterForDrag("LeftButton")
frame:SetScript("OnDragStart", frame.StartMoving)
frame:SetScript("OnDragStop", frame.StopMovingOrSizing)
frame:Hide()
tinsert(UISpecialFrames, "LoadoutsFrame")

frame.title = frame:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
frame.title:SetPoint("TOP", 0, -5)
frame.title:SetText("Loadouts")

StaticPopupDialogs["LOADOUTS_NEW"] = {
    text = "Save your current talents, glyphs and bars as a new loadout named:",
    button1 = ACCEPT, button2 = CANCEL, hasEditBox = true, maxLetters = 24,
    OnAccept = function(self) Command(".loadout new " .. self.editBox:GetText()); SyncSoon() end,
    EditBoxOnEnterPressed = function(self) Command(".loadout new " .. self:GetText()); SyncSoon(); self:GetParent():Hide() end,
    timeout = 0, whileDead = true, hideOnEscape = true,
}
StaticPopupDialogs["LOADOUTS_RENAME"] = {
    text = "New name for %s:",
    button1 = ACCEPT, button2 = CANCEL, hasEditBox = true, maxLetters = 24,
    OnAccept = function(self, id) Command(".loadout rename " .. id .. " " .. self.editBox:GetText()); SyncSoon() end,
    EditBoxOnEnterPressed = function(self, id) Command(".loadout rename " .. id .. " " .. self:GetText()); SyncSoon(); self:GetParent():Hide() end,
    timeout = 0, whileDead = true, hideOnEscape = true,
}
StaticPopupDialogs["LOADOUTS_DELETE"] = {
    text = "Delete loadout %s?",
    button1 = DELETE, button2 = CANCEL,
    OnAccept = function(_, id) Command(".loadout delete " .. id); SyncSoon() end,
    timeout = 0, whileDead = true, hideOnEscape = true,
}

local rows = {}
local function Button(parent, text, width, onClick)
    local button = CreateFrame("Button", nil, parent, "UIPanelButtonTemplate")
    button:SetSize(width, ROW_H - 2)
    button:SetText(text)
    button:SetScript("OnClick", onClick)
    return button
end

local function GetRow(index)
    local row = rows[index]
    if row then
        row:Show()
        return row
    end
    row = CreateFrame("Frame", nil, frame)
    row:SetSize(336, ROW_H)
    row:SetPoint("TOPLEFT", 12, -30 - (index - 1) * ROW_H)
    row.label = row:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    row.label:SetPoint("LEFT", 0, 0)
    row.label:SetWidth(180)
    row.label:SetJustifyH("LEFT")
    row.switch = Button(row, "Switch", 56, function() Command(".loadout switch " .. row.id); SyncSoon() end)
    row.switch:SetPoint("LEFT", 182, 0)
    row.rename = Button(row, "Rename", 56, function()
        local popup = StaticPopup_Show("LOADOUTS_RENAME", row.name)
        if popup then popup.data = row.id end
    end)
    row.rename:SetPoint("LEFT", row.switch, "RIGHT", 2, 0)
    row.delete = Button(row, "X", 24, function()
        local popup = StaticPopup_Show("LOADOUTS_DELETE", row.name)
        if popup then popup.data = row.id end
    end)
    row.delete:SetPoint("LEFT", row.rename, "RIGHT", 2, 0)
    rows[index] = row
    return row
end

local newButton = Button(frame, "Save current as new", 160, function() StaticPopup_Show("LOADOUTS_NEW") end)

local function Layout()
    for _, row in ipairs(rows) do
        row:Hide()
    end
    for index, loadout in ipairs(loadouts) do
        local row = GetRow(index)
        row.id, row.name = loadout.id, loadout.name
        local text = loadout.name .. " |cff999999(" .. loadout.spec .. ")|r"
        if loadout.flags:find("a") then
            text = "|cff00ff00" .. loadout.name .. "|r |cff999999(" .. loadout.spec .. ")|r"
        elseif loadout.flags:find("e") then
            text = text .. " |cff999999empty|r"
        end
        row.label:SetText(text)
        if loadout.flags:find("a") then row.switch:Disable() else row.switch:Enable() end
        -- A specialization's own loadout can't be deleted, nor one a talent group holds.
        if loadout.flags:find("d") or loadout.flags:find("a") or loadout.flags:find("g") then
            row.delete:Disable()
        else
            row.delete:Enable()
        end
    end
    newButton:ClearAllPoints()
    newButton:SetPoint("TOPLEFT", 12, -34 - #loadouts * ROW_H)
    frame:SetHeight(76 + #loadouts * ROW_H)
end

local events = CreateFrame("Frame")
events:RegisterEvent("ADDON_LOADED")
events:RegisterEvent("CHAT_MSG_ADDON")
events:RegisterEvent("ACTIVE_TALENT_GROUP_CHANGED")
events:RegisterEvent("PLAYER_SPECIALIZATION_CHANGED")
events:SetScript("OnEvent", function(_, event, ...)
    if event == "ADDON_LOADED" then
        if ... == "Loadouts" then
            RegisterAddonMessagePrefix(PREFIX)
        end
        return
    end
    if event ~= "CHAT_MSG_ADDON" then
        if frame:IsShown() then SyncSoon() end
        return
    end

    local prefix, message, _, sender = ...
    if prefix ~= PREFIX or (sender and Ambiguate(sender, "none") ~= UnitName("player")) then
        return
    end
    local kind, id, name, spec, flags = strsplit("\t", message)
    if kind == "L" then
        tinsert(pending, { id = id, name = name or "?", spec = spec or "?", flags = flags or "" })
    elseif kind == "E" then
        loadouts, pending = pending, {}
        Layout()
    end
end)

frame:SetScript("OnShow", function() Command(".loadout sync") end)

SLASH_LOADOUTS1 = "/lo"
SLASH_LOADOUTS2 = "/loadouts"
SlashCmdList.LOADOUTS = function()
    if frame:IsShown() then frame:Hide() else frame:Show() end
end
