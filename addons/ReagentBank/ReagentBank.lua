-- ReagentBank: a panel for UltimateMoP's server-side reagent bank (T-043). It only sends .reagent commands; the
-- contents come from the server (.reagent sync answers with addon messages). Open with /rb or the button on the
-- profession window. Crafting draws from the bank on the server, so nothing here is needed to craft.

local PREFIX = "ReagentBank"
local COLUMNS, ROWS, SIZE = 8, 6, 37
local PER_PAGE = COLUMNS * ROWS

local contents, pending, counts = {}, {}, {}   -- counts: item id -> count, for tooltips
local pageIndex = 1

ReagentBankSeen = ReagentBankSeen   -- per character: the introduction was shown

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
        Command(".reagent sync")
    end
end)
local function SyncSoon(seconds)
    delay.left = seconds or 0.5
    delay:Show()
end

local frame = CreateFrame("Frame", "ReagentBankFrame", UIParent, "BasicFrameTemplateWithInset")
frame:SetSize(COLUMNS * SIZE + 24, ROWS * SIZE + 92)
frame:SetPoint("CENTER", 220, 0)
frame:SetMovable(true)
frame:EnableMouse(true)
frame:RegisterForDrag("LeftButton")
frame:SetScript("OnDragStart", frame.StartMoving)
frame:SetScript("OnDragStop", frame.StopMovingOrSizing)
frame:Hide()
tinsert(UISpecialFrames, "ReagentBankFrame")

frame.title = frame:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
frame.title:SetPoint("TOP", 0, -5)
frame.title:SetText("Reagent Bank")

frame.empty = frame:CreateFontString(nil, "OVERLAY", "GameFontDisable")
frame.empty:SetPoint("CENTER", 0, 10)
frame.empty:SetText("Empty. Deposit reagents from your bags.")

local buttons = {}
for index = 1, PER_PAGE do
    local button = CreateFrame("Button", nil, frame)
    button:SetSize(SIZE - 3, SIZE - 3)
    local column, row = (index - 1) % COLUMNS, math.floor((index - 1) / COLUMNS)
    button:SetPoint("TOPLEFT", 12 + column * SIZE, -30 - row * SIZE)
    button.icon = button:CreateTexture(nil, "ARTWORK")
    button.icon:SetAllPoints()
    button.count = button:CreateFontString(nil, "OVERLAY", "NumberFontNormal")
    button.count:SetPoint("BOTTOMRIGHT", -2, 2)
    button:SetHighlightTexture("Interface\\Buttons\\ButtonHilight-Square", "ADD")
    button:RegisterForClicks("LeftButtonUp", "RightButtonUp")
    button:SetScript("OnClick", function(self, mouse)
        if not self.id then return end
        if IsModifiedClick("CHATLINK") then
            local _, link = GetItemInfo(self.id)
            if link then ChatEdit_InsertLink(link) end
            return
        end
        -- Left: one stack. Right: all of it.
        Command(".reagent take " .. self.id .. (mouse == "RightButton" and (" " .. self.total) or ""))
        SyncSoon()
    end)
    button:SetScript("OnEnter", function(self)
        if not self.id then return end
        GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
        GameTooltip:SetHyperlink("item:" .. self.id)
        GameTooltip:AddLine("Click: take a stack. Right-click: take all.", 0.6, 0.6, 0.6)
        GameTooltip:Show()
    end)
    button:SetScript("OnLeave", function() GameTooltip:Hide() end)
    buttons[index] = button
end

local deposit = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
deposit:SetSize(150, 22)
deposit:SetPoint("BOTTOMLEFT", 10, 10)
deposit:SetText("Deposit all reagents")
deposit:SetScript("OnClick", function() Command(".reagent deposit"); SyncSoon() end)

local prev = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
prev:SetSize(26, 22)
prev:SetPoint("BOTTOMRIGHT", -40, 10)
prev:SetText("<")
local nextButton = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
nextButton:SetSize(26, 22)
nextButton:SetPoint("BOTTOMRIGHT", -10, 10)
nextButton:SetText(">")
local pageText = frame:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
pageText:SetPoint("RIGHT", prev, "LEFT", -6, 0)

local function Layout()
    local pages = math.max(1, math.ceil(#contents / PER_PAGE))
    pageIndex = math.min(math.max(pageIndex, 1), pages)
    for index, button in ipairs(buttons) do
        local holding = contents[(pageIndex - 1) * PER_PAGE + index]
        if holding then
            button.id, button.total = holding.id, holding.count
            button.icon:SetTexture(GetItemIcon(holding.id) or "Interface\\Icons\\INV_Misc_QuestionMark")
            button.count:SetText(holding.count > 9999 and (math.floor(holding.count / 1000) .. "k") or holding.count)
            button:Show()
        else
            button.id = nil
            button:Hide()
        end
    end
    frame.empty:SetShown(#contents == 0)
    pageText:SetText(pageIndex .. "/" .. pages)
    prev:SetEnabled(pageIndex > 1)
    nextButton:SetEnabled(pageIndex < pages)
end
prev:SetScript("OnClick", function() pageIndex = pageIndex - 1; Layout() end)
nextButton:SetScript("OnClick", function() pageIndex = pageIndex + 1; Layout() end)

-- Reagent counts in item tooltips, so the profession window's "0/5" doesn't hide what the bank holds.
GameTooltip:HookScript("OnTooltipSetItem", function(tooltip)
    local _, link = tooltip:GetItem()
    local id = link and tonumber(link:match("item:(%d+)"))
    if id and counts[id] then
        tooltip:AddLine("Reagent bank: " .. counts[id], 0.4, 0.8, 1)
    end
end)

StaticPopupDialogs["REAGENTBANK_INTRO"] = {
    text = "Reagent bank\n\nYour reagents can live outside your bags. Crafting takes what it needs from your bags first, then from the reagent bank, anywhere you are.\n\nOpen it with the Reagent Bank button here or /rb.",
    button1 = OKAY,
    timeout = 0, whileDead = true, hideOnEscape = true,
}

local function AddProfessionButton()
    if not TradeSkillFrame or TradeSkillFrame.reagentBankButton then return end
    local button = CreateFrame("Button", nil, TradeSkillFrame, "UIPanelButtonTemplate")
    button:SetSize(100, 20)
    button:SetPoint("TOPRIGHT", TradeSkillFrame, "TOPRIGHT", -30, -28)
    button:SetText("Reagent Bank")
    button:SetScript("OnClick", function() if frame:IsShown() then frame:Hide() else frame:Show() end end)
    TradeSkillFrame.reagentBankButton = button
end

local events = CreateFrame("Frame")
events:RegisterEvent("ADDON_LOADED")
events:RegisterEvent("PLAYER_ENTERING_WORLD")
events:RegisterEvent("CHAT_MSG_ADDON")
events:RegisterEvent("TRADE_SKILL_SHOW")
events:RegisterEvent("BAG_UPDATE_DELAYED")
events:RegisterEvent("GET_ITEM_INFO_RECEIVED")
events:SetScript("OnEvent", function(_, event, ...)
    if event == "ADDON_LOADED" then
        local name = ...
        if name == "ReagentBank" then
            RegisterAddonMessagePrefix(PREFIX)
        elseif name == "Blizzard_TradeSkillUI" then
            AddProfessionButton()
        end
        return
    end
    if event == "PLAYER_ENTERING_WORLD" then
        events:UnregisterEvent("PLAYER_ENTERING_WORLD")
        SyncSoon(3)
        return
    end
    if event == "TRADE_SKILL_SHOW" then
        AddProfessionButton()
        if not ReagentBankSeen then
            ReagentBankSeen = true
            StaticPopup_Show("REAGENTBANK_INTRO")
        end
        SyncSoon()
        return
    end
    if event == "BAG_UPDATE_DELAYED" then
        -- Crafting from the bank changes the bags (the product), so this catches it too.
        if frame:IsShown() or (TradeSkillFrame and TradeSkillFrame:IsShown()) then SyncSoon(1) end
        return
    end
    if event == "GET_ITEM_INFO_RECEIVED" then
        if frame:IsShown() then Layout() end
        return
    end

    local prefix, message, _, sender = ...
    if prefix ~= PREFIX or (sender and Ambiguate(sender, "none") ~= UnitName("player")) then
        return
    end
    local kind, id, count = strsplit("\t", message)
    if kind == "R" then
        tinsert(pending, { id = tonumber(id), count = tonumber(count) or 0 })
    elseif kind == "E" then
        contents, pending = pending, {}
        counts = {}
        for _, holding in ipairs(contents) do counts[holding.id] = holding.count end
        table.sort(contents, function(a, b)
            local nameA, nameB = GetItemInfo(a.id) or "", GetItemInfo(b.id) or ""
            if nameA ~= nameB then return nameA < nameB end
            return a.id < b.id
        end)
        Layout()
    end
end)

frame:SetScript("OnShow", function() Command(".reagent sync") end)

SLASH_REAGENTBANK1 = "/rb"
SLASH_REAGENTBANK2 = "/reagentbank"
SlashCmdList.REAGENTBANK = function()
    if frame:IsShown() then frame:Hide() else frame:Show() end
end
