-- Heartseed: a panel for UltimateMoP's Heartseed. The server holds every value; .seed sync answers with addon
-- messages, and "Choose paths" opens the server's own choice window (.seed). Open with /hs or the key binding.

local PREFIX = "Heartseed"
local DEFAULT_KEY = "SHIFT-H"
local WIDTH, HEIGHT = 400, 520
local ICONS = "Interface\\Icons\\"

-- Custom art goes in art/ (see art/README.md). Until it ships, built-in textures stand in: set USE_ART once the
-- files are there, since the client draws nothing (not a fallback) for a missing file.
local ART = "Interface\\AddOns\\Heartseed\\art\\"
local USE_ART = false
local function Art(name, fallback)
    return USE_ART and (ART .. name) or fallback
end

-- The four Celestials sit at their compass points: Xuen the White Tiger west, Niuzao the Black Ox north,
-- Yu'lon the Jade Serpent east, Chi-Ji the Red Crane south.
local PATHS = {
    { key = "Xuen", title = "Xuen, the White Tiger", blurb = "Strength and speed", x = -1, y = 0,
      color = { 0.85, 0.9, 1.0 }, icon = Art("xuen", ICONS .. "monk_stance_whitetiger") },
    { key = "Niuzao", title = "Niuzao, the Black Ox", blurb = "Endurance and defense", x = 0, y = 1,
      color = { 0.9, 0.65, 0.35 }, icon = Art("niuzao", ICONS .. "monk_ability_summonoxstatue") },
    { key = "Yu'lon", title = "Yu'lon, the Jade Serpent", blurb = "Wisdom and spell power", x = 1, y = 0,
      color = { 0.35, 1.0, 0.6 }, icon = Art("yulon", ICONS .. "inv_celestialserpentmount") },
    { key = "Chi-Ji", title = "Chi-Ji, the Red Crane", blurb = "Healing and hope", x = 0, y = -1,
      color = { 1.0, 0.4, 0.3 }, icon = Art("chiji", ICONS .. "inv_pet_cranegod") },
}
local PATH_BY_KEY = {}
for _, p in ipairs(PATHS) do PATH_BY_KEY[p.key] = p end
local SEED_COLOR = { 1.0, 0.85, 0.4 }

local SOUND = {
    open = "igSpellBookOpen",
    close = "igSpellBookClose",
    click = "igMainMenuOptionCheckBoxOn",
    gift = "LEVELUPSOUND",
}
local function Sound(name)
    if PlaySound then PlaySound(SOUND[name]) end
end

local function Hex(c)
    return string.format("|cff%02x%02x%02x", math.floor(c[1] * 255 + 0.5), math.floor(c[2] * 255 + 0.5), math.floor(c[3] * 255 + 0.5))
end

local data, incoming, lastGift

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

local function Tooltip(owner, lines)
    owner:SetScript("OnEnter", function(self)
        local l = type(lines) == "function" and lines(self) or lines
        if not l or not GameTooltip then return end
        GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
        for i, line in ipairs(l) do
            if i == 1 then
                GameTooltip:SetText(line, 1, 0.82, 0)
            else
                GameTooltip:AddLine(line, 1, 1, 1, true)
            end
        end
        GameTooltip:Show()
    end)
    owner:SetScript("OnLeave", function() if GameTooltip then GameTooltip:Hide() end end)
end

local function Pulse(texture, change, seconds)
    local group = texture:CreateAnimationGroup()
    local alpha = group:CreateAnimation("Alpha")
    alpha:SetChange(change)
    alpha:SetDuration(seconds)
    alpha:SetSmoothing("IN_OUT")
    group:SetLooping("BOUNCE")
    return group
end

-- Frame -----------------------------------------------------------------------------------------------------

local frame = CreateFrame("Frame", "HeartseedFrame", UIParent)
frame:SetSize(WIDTH, HEIGHT)
frame:SetPoint("CENTER", 240, 0)
frame:SetFrameStrata("HIGH")
frame:SetBackdrop({
    bgFile = Art("background", "Interface\\FrameGeneral\\UI-Background-Marble"),
    edgeFile = "Interface\\DialogFrame\\UI-DialogBox-Gold-Border",
    tile = false, edgeSize = 32,
    insets = { left = 11, right = 11, top = 11, bottom = 11 },
})
frame:SetMovable(true)
frame:EnableMouse(true)
frame:RegisterForDrag("LeftButton")
frame:SetScript("OnDragStart", frame.StartMoving)
frame:SetScript("OnDragStop", frame.StopMovingOrSizing)
frame:Hide()
tinsert(UISpecialFrames, "HeartseedFrame")

local shade = frame:CreateTexture(nil, "BACKGROUND", nil, 1)
shade:SetPoint("TOPLEFT", 11, -11)
shade:SetPoint("BOTTOMRIGHT", -11, 11)
shade:SetTexture(0, 0.05, 0.04, 0.55)

local close = CreateFrame("Button", nil, frame, "UIPanelCloseButton")
close:SetPoint("TOPRIGHT", -6, -6)

local crest = frame:CreateTexture(nil, "ARTWORK")
crest:SetSize(36, 36)
crest:SetPoint("TOPLEFT", 22, -20)
crest:SetTexture(ICONS .. "achievement_faction_celestials")

local title = frame:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
title:SetPoint("TOPLEFT", crest, "TOPRIGHT", 10, -2)
title:SetText("Heartseed")

local subtitle = frame:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
subtitle:SetPoint("TOPLEFT", title, "BOTTOMLEFT", 0, -3)

-- The mandala: the seed in the middle, the four Celestials around it -------------------------------------------

local CENTER_Y, RADIUS = -150, 86
local mandala = CreateFrame("Frame", nil, frame)
mandala:SetSize(1, 1)
mandala:SetPoint("TOP", 0, CENTER_Y)

local halo = mandala:CreateTexture(nil, "BACKGROUND", nil, 2)
halo:SetSize(230, 230)
halo:SetPoint("CENTER")
halo:SetTexture(Art("halo", "Interface\\Cooldown\\star4"))
halo:SetBlendMode("ADD")
halo:SetAlpha(0.35)

local glow = mandala:CreateTexture(nil, "ARTWORK", nil, 1)
glow:SetSize(150, 150)
glow:SetPoint("CENTER")
glow:SetTexture(Art("glow", "Interface\\Cooldown\\star4"))
glow:SetBlendMode("ADD")
local glowPulse = Pulse(glow, -0.6, 1.6)

local seed = CreateFrame("Button", nil, mandala)
seed:SetSize(64, 64)
seed:SetPoint("CENTER")
seed.icon = seed:CreateTexture(nil, "ARTWORK", nil, 2)
seed.icon:SetAllPoints()
seed.icon:SetTexture(Art("seed", ICONS .. "inv_farm_seedofharmony2"))
seed.icon:SetTexCoord(0.08, 0.92, 0.08, 0.92)
seed.ring = seed:CreateTexture(nil, "OVERLAY")
seed.ring:SetSize(96, 96)
seed.ring:SetPoint("CENTER")
seed.ring:SetTexture(Art("ring", "Interface\\Common\\GoldRing"))

local celestials = {}
for _, path in ipairs(PATHS) do
    local b = CreateFrame("Button", nil, mandala)
    b:SetSize(44, 44)
    b:SetPoint("CENTER", path.x * RADIUS, path.y * RADIUS)
    b.icon = b:CreateTexture(nil, "ARTWORK")
    b.icon:SetAllPoints()
    b.icon:SetTexture(path.icon)
    b.icon:SetTexCoord(0.08, 0.92, 0.08, 0.92)
    b.ring = b:CreateTexture(nil, "OVERLAY")
    b.ring:SetSize(66, 66)
    b.ring:SetPoint("CENTER")
    b.ring:SetTexture(Art("ring", "Interface\\Common\\GoldRing"))
    b.name = b:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
    b.name:SetText(Hex(path.color) .. path.key .. "|r")
    if path.y == 0 then
        b.name:SetPoint("TOP", b, "BOTTOM", 0, -6)
    else
        b.name:SetPoint(path.y > 0 and "RIGHT" or "LEFT", b, path.y > 0 and "LEFT" or "RIGHT", path.y > 0 and -8 or 8, 0)
    end
    b.path = path
    Tooltip(b, function(self)
        local lines = { path.title, path.blurb }
        if data and data.state and data.state.path == path.key then
            lines[#lines + 1] = Hex(path.color) .. "Your seed leans toward " .. path.key .. ".|r"
        end
        return lines
    end)
    celestials[path.key] = b
end

local rankLine = frame:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
rankLine:SetPoint("TOP", 0, CENTER_Y - RADIUS - 44)

local bar = CreateFrame("StatusBar", nil, frame)
bar:SetPoint("TOP", rankLine, "BOTTOM", 0, -6)
bar:SetSize(WIDTH - 90, 12)
bar:SetStatusBarTexture("Interface\\TargetingFrame\\UI-StatusBar")
bar.bg = bar:CreateTexture(nil, "BACKGROUND")
bar.bg:SetAllPoints()
bar.bg:SetTexture(0, 0, 0, 0.6)
bar.text = bar:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
bar.text:SetPoint("CENTER", 0, 1)

-- The Celestial Gift: the level 10 major's active spell, castable from the panel ------------------------------

local giftBox = CreateFrame("Frame", nil, frame)
giftBox:SetPoint("TOPLEFT", 24, CENTER_Y - RADIUS - 92)
giftBox:SetPoint("TOPRIGHT", -24, CENTER_Y - RADIUS - 92)
giftBox:SetHeight(64)
giftBox.bg = giftBox:CreateTexture(nil, "BACKGROUND")
giftBox.bg:SetAllPoints()
giftBox.bg:SetTexture(1, 1, 1, 0.06)

local giftButton = CreateFrame("Button", "HeartseedGiftButton", giftBox, "SecureActionButtonTemplate")
giftButton:SetSize(48, 48)
giftButton:SetPoint("LEFT", 8, 0)
giftButton:SetAttribute("type", "spell")
giftButton:RegisterForClicks("AnyUp")
giftButton.icon = giftButton:CreateTexture(nil, "ARTWORK")
giftButton.icon:SetAllPoints()
giftButton.cooldown = CreateFrame("Cooldown", nil, giftButton, "CooldownFrameTemplate")
giftButton.cooldown:SetAllPoints()
giftButton:SetHighlightTexture("Interface\\Buttons\\ButtonHilight-Square", "ADD")
local giftFlash = giftButton:CreateTexture(nil, "OVERLAY")
giftFlash:SetSize(96, 96)
giftFlash:SetPoint("CENTER")
giftFlash:SetTexture("Interface\\Cooldown\\star4")
giftFlash:SetBlendMode("ADD")
giftFlash:SetAlpha(0)
local giftFlashAnim = giftFlash:CreateAnimationGroup()
do
    local up = giftFlashAnim:CreateAnimation("Alpha")
    up:SetChange(1)
    up:SetDuration(0.25)
    up:SetOrder(1)
    local down = giftFlashAnim:CreateAnimation("Alpha")
    down:SetChange(-1)
    down:SetDuration(1.2)
    down:SetOrder(2)
end

local giftName = giftBox:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
giftName:SetPoint("TOPLEFT", giftButton, "TOPRIGHT", 10, -2)
giftName:SetPoint("RIGHT", -8, 0)
giftName:SetJustifyH("LEFT")
local giftText = giftBox:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
giftText:SetPoint("TOPLEFT", giftName, "BOTTOMLEFT", 0, -3)
giftText:SetPoint("RIGHT", -8, 0)
giftText:SetJustifyH("LEFT")

local giftSpell, pendingGiftSpell
local function SetGiftSpell(spellId)
    local name, icon
    if spellId and spellId > 0 and GetSpellInfo then
        name, _, icon = GetSpellInfo(spellId)
    end
    giftButton.icon:SetTexture(icon or ICONS .. "INV_Misc_QuestionMark")
    giftButton.icon:SetDesaturated(not name)
    giftSpell = name and spellId or nil
    -- Secure attributes cannot change in combat; apply when it ends.
    if InCombatLockdown and InCombatLockdown() then
        pendingGiftSpell = spellId or 0
        return
    end
    pendingGiftSpell = nil
    giftButton:SetAttribute("spell", name)
end
local function UpdateGiftCooldown()
    if giftSpell and GetSpellCooldown then
        local start, duration = GetSpellCooldown(giftSpell)
        if start and duration and duration > 1.5 then
            giftButton.cooldown:SetCooldown(start, duration)
            return
        end
    end
    giftButton.cooldown:Hide()
end
Tooltip(giftButton, function()
    if giftSpell and GameTooltip then
        GameTooltip:SetOwner(giftButton, "ANCHOR_RIGHT")
        GameTooltip:SetSpellByID(giftSpell)
        GameTooltip:Show()
        return nil
    end
    return { "Celestial Gift", "Choose a Celestial at level 10 to wake your Gift." }
end)

-- Majors: one node per tier ------------------------------------------------------------------------------------

local nodes = {}
local nodesLabel = frame:CreateFontString(nil, "OVERLAY", "GameFontNormal")
nodesLabel:SetPoint("TOPLEFT", giftBox, "BOTTOMLEFT", 0, -12)
nodesLabel:SetText("Majors")
for i = 1, 6 do
    local n = CreateFrame("Button", nil, frame)
    n:SetSize(34, 34)
    n:SetPoint("TOPLEFT", nodesLabel, "BOTTOMLEFT", (i - 1) * 56 + 6, -6)
    n.icon = n:CreateTexture(nil, "ARTWORK")
    n.icon:SetAllPoints()
    n.icon:SetTexCoord(0.08, 0.92, 0.08, 0.92)
    n.ring = n:CreateTexture(nil, "OVERLAY")
    n.ring:SetSize(52, 52)
    n.ring:SetPoint("CENTER")
    n.ring:SetTexture(Art("ring", "Interface\\Common\\GoldRing"))
    n.glow = n:CreateTexture(nil, "BACKGROUND")
    n.glow:SetSize(64, 64)
    n.glow:SetPoint("CENTER")
    n.glow:SetTexture("Interface\\Cooldown\\star4")
    n.glow:SetBlendMode("ADD")
    n.pulse = Pulse(n.glow, -0.7, 0.9)
    n.level = n:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    n.level:SetPoint("TOP", n, "BOTTOM", 0, -6)
    Tooltip(n, function(self) return self.lines end)
    nodes[i] = n
end

local essenceLine = frame:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
essenceLine:SetPoint("TOPLEFT", nodesLabel, "BOTTOMLEFT", 0, -64)
essenceLine:SetPoint("RIGHT", -24, 0)
essenceLine:SetJustifyH("LEFT")
essenceLine:SetSpacing(2)

local footer = frame:CreateFontString(nil, "OVERLAY", "GameFontDisableSmall")
footer:SetPoint("BOTTOMLEFT", 24, 44)
footer:SetPoint("RIGHT", -24, 0)
footer:SetJustifyH("LEFT")

local choose = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
choose:SetSize(130, 24)
choose:SetPoint("BOTTOMLEFT", 20, 16)
choose:SetText("Choose paths")
choose:SetScript("OnClick", function() Sound("click"); Command(".seed") end)

local refresh = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
refresh:SetSize(90, 24)
refresh:SetPoint("BOTTOMRIGHT", -20, 16)
refresh:SetText("Refresh")
refresh:SetScript("OnClick", function() Sound("click"); Sync() end)

local content = { mandala, rankLine, bar, giftBox, nodesLabel, essenceLine, footer }
for _, n in ipairs(nodes) do content[#content + 1] = n end
local function ShowContent(shown)
    for _, w in ipairs(content) do
        if shown then w:Show() else w:Hide() end
    end
end

-- Layout -------------------------------------------------------------------------------------------------------

local function Layout()
    if not data or data.none or not data.state then
        ShowContent(false)
        choose:Hide()
        if not data then
            subtitle:SetText("Asking the seed...")
        else
            subtitle:SetText("You do not carry a Heartseed yet.")
        end
        rankLine:Show()
        rankLine:SetText(data and "No Heartseed" or "")
        glowPulse:Stop()
        return
    end
    ShowContent(true)
    choose:Show()

    local s = data.state
    local path = PATH_BY_KEY[s.path]
    local color = path and path.color or SEED_COLOR
    subtitle:SetText(s.stage .. (path and ("  " .. Hex(color) .. path.title .. "|r") or ""))

    glow:SetVertexColor(color[1], color[2], color[3])
    halo:SetVertexColor(color[1], color[2], color[3])
    glowPulse:Play()
    for key, b in pairs(celestials) do
        local lit = s.path == key
        b.icon:SetDesaturated(not lit and s.path ~= "none")
        b:SetAlpha((lit or s.path == "none") and 1 or 0.55)
    end

    local accountNote = s.rank < s.accountRank and ("  (account " .. s.accountRank .. ")") or ""
    rankLine:SetText("Rank " .. s.rank .. " of " .. s.cap .. accountNote)
    bar:SetStatusBarColor(color[1], color[2], color[3])
    bar:SetMinMaxValues(0, math.max(s.need, 1))
    bar:SetValue(math.min(s.growth, s.need))
    bar.text:SetText("Growth " .. s.growth .. " / " .. s.need)

    local level = UnitLevel("player")
    local gift = data.majors[1]
    if gift and gift.gift > 0 then
        local p = PATH_BY_KEY[gift.path]
        giftName:SetText((p and Hex(p.color) or "") .. gift.name .. "|r")
        giftText:SetText(gift.description ~= "" and gift.description or "Your Celestial Gift.")
        SetGiftSpell(gift.gift)
    else
        giftName:SetText("|cff808080Celestial Gift|r")
        giftText:SetText(level >= (gift and gift.level or 10) and "Choose a Celestial to wake your Gift."
            or "Wakes at level " .. (gift and gift.level or 10) .. ".")
        SetGiftSpell(nil)
    end
    UpdateGiftCooldown()
    if gift and gift.gift > 0 and lastGift ~= nil and lastGift ~= gift.gift then
        Sound("gift")
        giftFlashAnim:Play()
    end
    lastGift = gift and gift.gift or 0

    for i, n in ipairs(nodes) do
        local m = data.majors[i]
        if m then
            n:Show()
            n.level:SetText("Level " .. m.level)
            local p = PATH_BY_KEY[m.path]
            if m.name ~= "" then
                n.icon:SetTexture(p and p.icon or ICONS .. "INV_Misc_QuestionMark")
                n.icon:SetDesaturated(false)
                n:SetAlpha(1)
                n.pulse:Stop()
                n.glow:SetAlpha(0)
                n.state = "chosen"
                n.lines = { (p and Hex(p.color) or "") .. m.name .. "|r", m.description }
            elseif level >= m.level then
                n.icon:SetTexture(ICONS .. "INV_Misc_QuestionMark")
                n.icon:SetDesaturated(false)
                n:SetAlpha(1)
                n.glow:SetAlpha(1)
                n.pulse:Play()
                n.state = "open"
                n.lines = { "Level " .. m.level .. " major", "|cff40ff40Ready to choose.|r Visit a Celestial shrine and press Choose paths." }
            else
                n.icon:SetTexture(ICONS .. "INV_Misc_QuestionMark")
                n.icon:SetDesaturated(true)
                n:SetAlpha(0.45)
                n.pulse:Stop()
                n.glow:SetAlpha(0)
                n.state = "locked"
                n.lines = { "Level " .. m.level .. " major", "|cff808080Locked until level " .. m.level .. ".|r" }
            end
        else
            n:Hide()
        end
    end

    local lines = { "|cffffd100Essences|r  |cff999999" .. s.slots .. " slots open|r" }
    if #data.essences == 0 then
        lines[#lines + 1] = "  |cff808080None yet: feed old weapons to find them.|r"
    end
    for _, e in ipairs(data.essences) do
        local slot = ""
        if e.slot > 0 then
            slot = e.slot <= s.slots and ("  slot " .. e.slot) or "  |cff808080locked slot|r"
        end
        lines[#lines + 1] = "  " .. e.name .. " x" .. e.count .. slot
    end
    essenceLine:SetText(table.concat(lines, "\n"))

    local foot = "Weapons fed: " .. s.fed
    if s.shrine ~= "" then
        foot = foot .. "   |cff40ff40At " .. s.shrine .. ": majors can change here.|r"
    end
    footer:SetText(foot)
end

function HeartseedPanel_Toggle()
    if frame:IsShown() then
        frame:Hide()
    else
        frame:Show()
    end
end

frame:SetScript("OnShow", function()
    Sound("open")
    Layout()
    Sync()
end)
frame:SetScript("OnHide", function()
    Sound("close")
    glowPulse:Stop()
end)

-- Events -------------------------------------------------------------------------------------------------------

local events = CreateFrame("Frame")
for _, e in ipairs({ "ADDON_LOADED", "PLAYER_LOGIN", "CHAT_MSG_ADDON", "PLAYER_LEVEL_UP", "GOSSIP_CLOSED",
    "QUEST_FINISHED", "PLAYER_SPECIALIZATION_CHANGED", "ZONE_CHANGED", "ZONE_CHANGED_NEW_AREA", "LEARNED_SPELL_IN_TAB",
    "SPELL_UPDATE_COOLDOWN", "PLAYER_REGEN_ENABLED" }) do
    events:RegisterEvent(e)
end
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
    if event == "SPELL_UPDATE_COOLDOWN" then
        if frame:IsShown() then UpdateGiftCooldown() end
        return
    end
    if event == "PLAYER_REGEN_ENABLED" then
        if pendingGiftSpell then SetGiftSpell(pendingGiftSpell) end
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
        incoming.majors[#incoming.majors + 1] = { level = tonumber(f[2]) or 0, name = f[3] or "", path = f[4] or "",
            gift = tonumber(f[5]) or 0, description = f[6] or "" }
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
