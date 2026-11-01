-- Heartseed: a panel for UltimateMoP's Heartseed. The server holds every value; .seed sync answers with addon
-- messages, and "Choose paths" opens the server's own choice window (.seed). Open with /hs or the key binding.

local PREFIX = "Heartseed"
local DEFAULT_KEY = "SHIFT-H"
local WIDTH = 320

BINDING_HEADER_HEARTSEED = "Heartseed"
BINDING_NAME_HEARTSEED_TOGGLE = "Toggle the Heartseed panel"

local data, incoming

local function Command(text)
    SendChatMessage(text, "SAY")
end

local function Sync()
    incoming = { majors = {}, essences = {} }
    Command(".seed sync")
end

-- 5.4.8 has no C_Timer: a short OnUpdate delay so the panel refreshes after the server has applied a change.
local delay = CreateFrame("Frame")
delay:Hide()
delay:SetScript("OnUpdate", function(self, elapsed)
    self.left = self.left - elapsed
    if self.left <= 0 then
        self:Hide()
        Sync()
    end
end)
local function SyncSoon()
    delay.left = 0.5
    delay:Show()
end

local frame = CreateFrame("Frame", "HeartseedFrame", UIParent, "BasicFrameTemplateWithInset")
frame:SetSize(WIDTH, 200)
frame:SetPoint("CENTER", 240, 0)
frame:SetMovable(true)
frame:EnableMouse(true)
frame:RegisterForDrag("LeftButton")
frame:SetScript("OnDragStart", frame.StartMoving)
frame:SetScript("OnDragStop", frame.StopMovingOrSizing)
frame:Hide()
tinsert(UISpecialFrames, "HeartseedFrame")

frame.title = frame:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
frame.title:SetPoint("TOP", 0, -5)
frame.title:SetText("Heartseed")

local function Text(font, y)
    local fs = frame:CreateFontString(nil, "OVERLAY", font or "GameFontHighlight")
    fs:SetPoint("TOPLEFT", 16, y)
    fs:SetWidth(WIDTH - 32)
    fs:SetJustifyH("LEFT")
    return fs
end

local header = Text("GameFontNormalLarge", -34)
local rankLine = Text(nil, -58)

local bar = CreateFrame("StatusBar", nil, frame)
bar:SetPoint("TOPLEFT", 16, -78)
bar:SetSize(WIDTH - 32, 14)
bar:SetStatusBarTexture("Interface\\TargetingFrame\\UI-StatusBar")
bar:SetStatusBarColor(0.3, 0.8, 0.4)
bar.bg = bar:CreateTexture(nil, "BACKGROUND")
bar.bg:SetAllPoints()
bar.bg:SetTexture(0, 0, 0, 0.5)
bar.text = bar:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
bar.text:SetPoint("CENTER")

local body = Text(nil, -100)
body:SetSpacing(2)

local choose = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
choose:SetSize(120, 22)
choose:SetPoint("BOTTOMLEFT", 12, 10)
choose:SetText("Choose paths")
choose:SetScript("OnClick", function() Command(".seed") end)

local refresh = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
refresh:SetSize(80, 22)
refresh:SetPoint("BOTTOMRIGHT", -12, 10)
refresh:SetText("Refresh")
refresh:SetScript("OnClick", function() Sync() end)

local function Layout()
    if not data then
        header:SetText("Asking the seed...")
        rankLine:SetText("")
        bar:Hide()
        body:SetText("")
        choose:Hide()
        frame:SetHeight(110)
        return
    end
    if data.none or not data.state then
        header:SetText("No Heartseed")
        rankLine:SetText("You do not carry a Heartseed yet.")
        bar:Hide()
        body:SetText("")
        choose:Hide()
        frame:SetHeight(120)
        return
    end

    local s = data.state
    header:SetText(s.stage .. (s.path ~= "none" and ("  |cffffd100" .. s.path .. "|r") or ""))
    local accountNote = s.rank < s.accountRank and ("  (account " .. s.accountRank .. ")") or ""
    rankLine:SetText("Rank " .. s.rank .. " of " .. s.cap .. accountNote)
    bar:Show()
    bar:SetMinMaxValues(0, math.max(s.need, 1))
    bar:SetValue(math.min(s.growth, s.need))
    bar.text:SetText("Growth " .. s.growth .. " / " .. s.need)

    local level = UnitLevel("player")
    local lines = { "|cffffd100Majors|r" }
    for _, m in ipairs(data.majors) do
        local value
        if m.name ~= "" then
            value = m.name .. " |cff999999(" .. m.path .. ")|r"
        elseif level >= m.level then
            value = "|cff40ff40choose one|r"
        else
            value = "|cff808080locked|r"
        end
        lines[#lines + 1] = "  Level " .. m.level .. ": " .. value
    end
    lines[#lines + 1] = " "
    lines[#lines + 1] = "|cffffd100Essences|r  |cff999999" .. s.slots .. " slots open|r"
    if #data.essences == 0 then
        lines[#lines + 1] = "  |cff808080none yet: feed old weapons to find them|r"
    end
    for _, e in ipairs(data.essences) do
        local slot = ""
        if e.slot > 0 then
            slot = e.slot <= s.slots and ("  slot " .. e.slot) or "  |cff808080locked slot|r"
        end
        lines[#lines + 1] = "  " .. e.name .. " x" .. e.count .. slot
    end
    lines[#lines + 1] = " "
    lines[#lines + 1] = "Weapons fed: " .. s.fed
    if s.shrine ~= "" then
        lines[#lines + 1] = "|cff40ff40At " .. s.shrine .. ": majors can change here.|r"
    end
    body:SetText(table.concat(lines, "\n"))
    choose:Show()
    frame:SetHeight(150 + body:GetStringHeight())
end

function HeartseedPanel_Toggle()
    if frame:IsShown() then
        frame:Hide()
    else
        frame:Show()
    end
end

frame:SetScript("OnShow", function()
    Layout()
    Sync()
end)

local events = CreateFrame("Frame")
events:RegisterEvent("ADDON_LOADED")
events:RegisterEvent("PLAYER_LOGIN")
events:RegisterEvent("CHAT_MSG_ADDON")
events:RegisterEvent("PLAYER_LEVEL_UP")
events:RegisterEvent("GOSSIP_CLOSED")
events:RegisterEvent("QUEST_FINISHED")
events:RegisterEvent("PLAYER_SPECIALIZATION_CHANGED")
events:RegisterEvent("ZONE_CHANGED")
events:RegisterEvent("ZONE_CHANGED_NEW_AREA")
events:SetScript("OnEvent", function(_, event, ...)
    if event == "ADDON_LOADED" then
        if ... == "Heartseed" then
            RegisterAddonMessagePrefix(PREFIX)
        end
        return
    end
    if event == "PLAYER_LOGIN" then
        -- Bind the default key once, only if nothing else uses it; Key Bindings > Heartseed changes it.
        if not GetBindingKey("HEARTSEED_TOGGLE") and (GetBindingAction(DEFAULT_KEY) or "") == "" then
            SetBinding(DEFAULT_KEY, "HEARTSEED_TOGGLE")
            SaveBindings(GetCurrentBindingSet())
        end
        return
    end
    if event ~= "CHAT_MSG_ADDON" then
        if frame:IsShown() then SyncSoon() end
        return
    end

    local prefix, message, _, sender = ...
    if prefix ~= PREFIX or not incoming or (sender and Ambiguate(sender, "none") ~= UnitName("player")) then
        return
    end
    local f = { strsplit("\t", message) }
    if f[1] == "N" then
        incoming.none = true
    elseif f[1] == "S" then
        incoming.state = {
            rank = tonumber(f[2]) or 0, accountRank = tonumber(f[3]) or 0, cap = tonumber(f[4]) or 0,
            growth = tonumber(f[5]) or 0, need = tonumber(f[6]) or 0, stage = f[7] or "", path = f[8] or "none",
            fed = tonumber(f[9]) or 0, slots = tonumber(f[10]) or 0, shrine = f[11] or "",
        }
    elseif f[1] == "M" then
        incoming.majors[#incoming.majors + 1] = { level = tonumber(f[2]) or 0, name = f[3] or "", path = f[4] or "" }
    elseif f[1] == "X" then
        incoming.essences[#incoming.essences + 1] = { name = f[2] or "?", count = tonumber(f[3]) or 0, slot = tonumber(f[4]) or 0 }
    elseif f[1] == "E" then
        data, incoming = incoming, nil
        if frame:IsShown() then Layout() end
    end
end)

SLASH_HEARTSEED1 = "/hs"
SLASH_HEARTSEED2 = "/heartseed"
SlashCmdList["HEARTSEED"] = HeartseedPanel_Toggle
