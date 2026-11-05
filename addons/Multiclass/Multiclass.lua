-- Multiclass: the second-class panel for UltimateMoP (T-094). The server holds every value; ".mc sync" answers with
-- addon messages (prefix "Multiclass"), ".mc switch <id>" changes the active second class. The panel lists the
-- second classes your class can take, the selected one's level and experience, and its spellbook: drag an ability
-- to an action bar or click it to cast. Open with Alt-N (Key Bindings > Multiclass) or /mc.

local PREFIX = "Multiclass"
local DEFAULT_KEY = "ALT-N"
local WIDTH, HEIGHT = 440, 520
local ROW = 44

BINDING_HEADER_MULTICLASS = "Multiclass"
BINDING_NAME_MULTICLASS_TOGGLE = "Toggle the second-class panel"

local data, incoming, selected

local function Command(text)
    SendChatMessage(text, "SAY")
end

local function Sync()
    incoming = { classes = {}, abilities = {} }
    Command(".mc sync")
end

-- 5.4.8 has no C_Timer: a short delay so the panel refreshes after the server has applied a change.
local delay = CreateFrame("Frame")
delay:Hide()
delay:SetScript("OnUpdate", function(self, elapsed)
    self.left = (self.left or 0) - elapsed
    if self.left <= 0 then
        self:Hide()
        Sync()
    end
end)
local function SyncSoon()
    delay.left = 0.4
    delay:Show()
end

-- Frame ----------------------------------------------------------------------------------------------------------

local frame = CreateFrame("Frame", "MulticlassFrame", UIParent)
frame:SetSize(WIDTH, HEIGHT)
frame:SetPoint("CENTER")
frame:SetFrameStrata("HIGH")
frame:SetMovable(true)
frame:EnableMouse(true)
frame:RegisterForDrag("LeftButton")
frame:SetScript("OnDragStart", frame.StartMoving)
frame:SetScript("OnDragStop", frame.StopMovingOrSizing)
frame:SetBackdrop({
    bgFile = "Interface\\DialogFrame\\UI-DialogBox-Background",
    edgeFile = "Interface\\DialogFrame\\UI-DialogBox-Border",
    tile = true, tileSize = 32, edgeSize = 32,
    insets = { left = 11, right = 12, top = 12, bottom = 11 },
})
frame:Hide()
tinsert(UISpecialFrames, "MulticlassFrame")

local title = frame:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
title:SetPoint("TOP", 0, -18)
title:SetText("Second Class")

local close = CreateFrame("Button", nil, frame, "UIPanelCloseButton")
close:SetPoint("TOPRIGHT", -6, -6)

local status = frame:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
status:SetPoint("TOP", title, "BOTTOM", 0, -6)
status:SetWidth(WIDTH - 40)

-- Class tabs along the top.
local tabs = {}
local function Tab(i)
    if tabs[i] then return tabs[i] end
    local b = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
    b:SetSize(130, 24)
    b:SetPoint("TOPLEFT", 20 + (i - 1) % 3 * 134, -60 - math.floor((i - 1) / 3) * 28)
    tabs[i] = b
    return b
end

local header = frame:CreateFontString(nil, "OVERLAY", "GameFontNormal")
header:SetPoint("TOPLEFT", 22, -126)
header:SetWidth(WIDTH - 44)
header:SetJustifyH("LEFT")

local blurb = frame:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
blurb:SetPoint("TOPLEFT", header, "BOTTOMLEFT", 0, -4)
blurb:SetWidth(WIDTH - 44)
blurb:SetJustifyH("LEFT")

local bar = CreateFrame("StatusBar", nil, frame)
bar:SetSize(WIDTH - 44, 14)
bar:SetPoint("TOPLEFT", 22, -178)
bar:SetStatusBarTexture("Interface\\TargetingFrame\\UI-StatusBar")
bar:SetStatusBarColor(0.58, 0.0, 0.55)
local barBg = bar:CreateTexture(nil, "BACKGROUND")
barBg:SetAllPoints()
barBg:SetTexture(0, 0, 0, 0.5)
local barText = bar:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
barText:SetPoint("CENTER")

local activate = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
activate:SetSize(150, 24)
activate:SetPoint("BOTTOM", 0, 18)
activate:SetScript("OnClick", function()
    if not selected then return end
    Command(".mc switch " .. (selected.active and "none" or selected.id))
    SyncSoon()
end)

-- Ability rows: a secure button (click to cast, drag to an action bar) with the name, level and description.
local rows = {}
local function Row(i)
    if rows[i] then return rows[i] end
    local r = {}
    r.button = CreateFrame("Button", "MulticlassAbility" .. i, frame, "SecureActionButtonTemplate")
    r.button:SetSize(36, 36)
    r.button:SetPoint("TOPLEFT", 22, -202 - (i - 1) * ROW)
    r.button:RegisterForClicks("AnyUp")
    r.button:RegisterForDrag("LeftButton")
    r.icon = r.button:CreateTexture(nil, "ARTWORK")
    r.icon:SetAllPoints()
    r.button:SetScript("OnDragStart", function(self)
        if self.spell and self.known and not InCombatLockdown() then PickupSpell(self.spell) end
    end)
    r.button:SetScript("OnEnter", function(self)
        if not self.spell then return end
        GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
        GameTooltip:SetSpellByID(self.spell)
        GameTooltip:Show()
    end)
    r.button:SetScript("OnLeave", function() GameTooltip:Hide() end)
    r.name = frame:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    r.name:SetPoint("TOPLEFT", r.button, "TOPRIGHT", 8, 0)
    r.name:SetWidth(WIDTH - 100)
    r.name:SetJustifyH("LEFT")
    r.text = frame:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    r.text:SetPoint("TOPLEFT", r.name, "BOTTOMLEFT", 0, -2)
    r.text:SetWidth(WIDTH - 100)
    r.text:SetJustifyH("LEFT")
    rows[i] = r
    return r
end

local pendingLayout

local function Layout()
    if InCombatLockdown() then
        pendingLayout = true
        return
    end
    pendingLayout = nil
    for _, t in ipairs(tabs) do t:Hide() end
    for _, r in ipairs(rows) do r.button:Hide(); r.name:SetText(""); r.text:SetText("") end
    header:SetText(""); blurb:SetText(""); bar:Hide(); activate:Hide()
    if not data then
        status:SetText("Loading...")
        return
    end
    local u = data.unlock or {}
    if #data.classes == 0 then
        status:SetText("Your class has no second classes yet.")
        return
    end
    if (u.level or 0) < (u.unlock or 0) then
        status:SetText(string.format("Second classes open at level %d.", u.unlock))
    else
        status:SetText("Choose a second class; it levels as you do. Drag an ability to your bars.")
    end

    if not selected or not data.byId[selected.id] then
        selected = nil
        for _, c in ipairs(data.classes) do
            if c.active then selected = c end
        end
        selected = selected or data.classes[1]
    else
        selected = data.byId[selected.id]
    end

    for i, c in ipairs(data.classes) do
        local t = Tab(i)
        t:SetText((c.active and "|cff00ff00" or "") .. c.name .. (c.learned and (" " .. c.level) or ""))
        t:SetScript("OnClick", function() selected = c; Layout() end)
        if c == selected then t:LockHighlight() else t:UnlockHighlight() end
        t:Show()
    end

    local c = selected
    header:SetText(string.format("%s  |cffffffff%s|r", c.name,
        c.learned and string.format("level %d / %d%s", c.level, c.max, c.active and " (active)" or "") or "not taken"))
    blurb:SetText(c.description)
    if c.learned and c.level < c.max then
        bar:SetMinMaxValues(0, math.max(u.need or 1, 1))
        bar:SetValue(c.xp)
        barText:SetText(string.format("%d / %d", c.xp, u.need or 0))
        bar:Show()
    elseif c.learned then
        bar:SetMinMaxValues(0, 1); bar:SetValue(1); barText:SetText("Mastered"); bar:Show()
    end
    activate:SetText(c.active and "Set aside" or "Make active")
    if (u.level or 0) >= (u.unlock or 0) or c.active then activate:Show() end

    local i = 0
    for _, a in ipairs(data.abilities) do
        if a.class == c.id then
            i = i + 1
            local r = Row(i)
            local known = c.active and c.level >= a.level and a.kind == 0
            local _, _, icon = GetSpellInfo(a.spell)
            r.icon:SetTexture(icon or "Interface\\Icons\\INV_Misc_QuestionMark")
            r.icon:SetDesaturated(not (c.learned and c.level >= a.level))
            r.button.spell, r.button.known = a.spell, known
            r.button:SetAttribute("type", known and "spell" or nil)
            r.button:SetAttribute("spell", known and a.spell or nil)
            r.name:SetText(string.format("%s |cff999999(level %d%s)|r", a.name, a.level, a.kind == 1 and ", passive" or ""))
            r.text:SetText(a.description)
            r.button:Show()
        end
    end
end

function MulticlassPanel_Toggle()
    if frame:IsShown() then
        frame:Hide()
    else
        frame:Show()
    end
end

frame:SetScript("OnShow", function()
    if PlaySound then PlaySound("igSpellBookOpen") end
    Layout()
    Sync()
end)
frame:SetScript("OnHide", function()
    if PlaySound then PlaySound("igSpellBookClose") end
end)

-- Events ---------------------------------------------------------------------------------------------------------

local events = CreateFrame("Frame")
for _, e in ipairs({ "ADDON_LOADED", "PLAYER_LOGIN", "CHAT_MSG_ADDON", "PLAYER_LEVEL_UP", "PLAYER_REGEN_ENABLED",
    "LEARNED_SPELL_IN_TAB" }) do
    events:RegisterEvent(e)
end
events:SetScript("OnEvent", function(_, event, ...)
    if event == "ADDON_LOADED" then
        if ... == "Multiclass" then RegisterAddonMessagePrefix(PREFIX) end
        return
    end
    if event == "PLAYER_LOGIN" then
        -- Bind the default key once, only if nothing else uses it; Key Bindings > Multiclass changes it.
        if not GetBindingKey("MULTICLASS_TOGGLE") and (GetBindingAction(DEFAULT_KEY) or "") == "" then
            SetBinding(DEFAULT_KEY, "MULTICLASS_TOGGLE")
            SaveBindings(GetCurrentBindingSet())
        end
        return
    end
    if event == "PLAYER_REGEN_ENABLED" then
        if pendingLayout and frame:IsShown() then Layout() end
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
    local f = { strsplit("\t", message) }
    -- Level-ups and switches push a sync without a request.
    if not incoming then
        incoming = { classes = {}, abilities = {} }
    end
    if f[1] == "U" then
        incoming.unlock = { unlock = tonumber(f[2]) or 0, level = tonumber(f[3]) or 0, need = tonumber(f[4]) or 0 }
    elseif f[1] == "C" then
        incoming.classes[#incoming.classes + 1] = { id = tonumber(f[2]) or 0, name = f[3] or "?", level = tonumber(f[4]) or 1,
            xp = tonumber(f[5]) or 0, max = tonumber(f[6]) or 1, active = f[7] == "1", learned = f[8] == "1", description = f[9] or "" }
    elseif f[1] == "A" then
        incoming.abilities[#incoming.abilities + 1] = { class = tonumber(f[2]) or 0, level = tonumber(f[3]) or 1,
            spell = tonumber(f[4]) or 0, kind = tonumber(f[5]) or 0, cooldown = tonumber(f[6]) or 0, name = f[7] or "?",
            description = f[8] or "" }
    elseif f[1] == "E" then
        incoming.byId = {}
        for _, c in ipairs(incoming.classes) do incoming.byId[c.id] = c end
        data, incoming = incoming, nil
        if frame:IsShown() then Layout() end
    end
end)

SLASH_MULTICLASS1 = "/mc"
SLASH_MULTICLASS2 = "/multiclass"
SlashCmdList["MULTICLASS"] = MulticlassPanel_Toggle
