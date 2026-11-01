-- Loadouts: a panel for UltimateMoP's server-side spec loadouts (T-017) and their gear pages (T-018). It only sends
-- .loadout commands; the list comes from the server (.loadout sync answers with addon messages). Open with /lo.

local PREFIX = "Loadouts"
local ROW_H = 24

local loadouts, pending = {}, {}
local pages, pendingPages = {}, {}   -- loadout id -> { [page slot] = item string }
local shownPage                      -- loadout id whose gear page is open

-- The client can't tell where a dragged item came from, so remember the last bag slot picked up.
local pickedBag, pickedSlot
hooksecurefunc("PickupContainerItem", function(bag, slot) pickedBag, pickedSlot = bag, slot end)

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
frame:SetSize(404, 140)
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
local ShowPage
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
    row:SetSize(380, ROW_H)
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
    row.gear = Button(row, "Gear", 44, function() ShowPage(row.id) end)
    row.gear:SetPoint("LEFT", row.delete, "RIGHT", 2, 0)
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

-- Gear page panel: the loadout's stored pieces by equipment slot. Drop a piece from your bags on a slot to put it on
-- the page; click a piece to take it back to your bags. The worn loadout's page is what you have on.
local SLOT_NAMES = { "Head", "Neck", "Shoulders", "Shirt", "Chest", "Waist", "Legs", "Feet", "Wrists", "Hands",
    "Finger 1", "Finger 2", "Trinket 1", "Trinket 2", "Back", "Main hand", "Off hand", "Ranged", "Tabard" }
local SLOT_SIZE, PER_ROW = 36, 7

local page = CreateFrame("Frame", "LoadoutsPageFrame", frame, "BasicFrameTemplateWithInset")
page:SetSize(PER_ROW * (SLOT_SIZE + 4) + 20, 200)
page:SetPoint("TOPLEFT", frame, "TOPRIGHT", 2, 0)
page:Hide()
page.title = page:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
page.title:SetPoint("TOP", 0, -5)
page.note = page:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
page.note:SetPoint("BOTTOMLEFT", 10, 8)
page.note:SetPoint("BOTTOMRIGHT", -10, 8)
page.note:SetJustifyH("LEFT")

local slotButtons = {}
local function SlotButton(index)
    local button = slotButtons[index]
    if button then return button end
    button = CreateFrame("Button", nil, page, "ItemButtonTemplate")
    button:SetSize(SLOT_SIZE, SLOT_SIZE)
    button:RegisterForClicks("LeftButtonUp")
    button:SetScript("OnEnter", function(self)
        GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
        if self.item then
            GameTooltip:SetHyperlink(self.item)
            GameTooltip:AddLine("Click: take back to your bags", 0.6, 0.6, 0.6)
        else
            GameTooltip:SetText(self.label)
            GameTooltip:AddLine("Drop a piece from your bags here", 0.6, 0.6, 0.6)
        end
        GameTooltip:Show()
    end)
    button:SetScript("OnLeave", function() GameTooltip:Hide() end)
    local function Drop(self)
        if GetCursorInfo() ~= "item" or not pickedBag or not shownPage then return end
        ClearCursor()
        local target = self.slot < 100 and (" " .. self.slot) or ""
        Command(".loadout page put " .. shownPage .. " " .. pickedBag .. " " .. pickedSlot .. target)
        SyncSoon()
    end
    button:SetScript("OnReceiveDrag", Drop)
    button:SetScript("OnClick", function(self)
        if GetCursorInfo() then Drop(self) return end
        if self.item and shownPage then
            Command(".loadout page take " .. shownPage .. " " .. self.slot)
            SyncSoon()
        end
    end)
    slotButtons[index] = button
    return button
end

local function LayoutPage()
    if not shownPage then page:Hide() return end
    local loadout
    for _, entry in ipairs(loadouts) do
        if entry.id == shownPage then loadout = entry end
    end
    if not loadout then page:Hide() return end
    local items = pages[shownPage] or {}
    page.title:SetText("Gear page: " .. loadout.name)

    -- Equipment slots first, then any spares (pieces that found no room on the way back).
    local order = {}
    for slot = 0, 18 do tinsert(order, slot) end
    local spares = {}
    for slot in pairs(items) do
        if slot >= 100 then tinsert(spares, slot) end
    end
    table.sort(spares)
    for _, slot in ipairs(spares) do tinsert(order, slot) end

    for _, button in ipairs(slotButtons) do button:Hide() end
    for index, slot in ipairs(order) do
        local button = SlotButton(index)
        button.slot = slot
        button.label = SLOT_NAMES[slot + 1] or ("Spare " .. (slot - 99))
        button.item = items[slot]
        local texture = button.item and select(10, GetItemInfo(button.item))
        SetItemButtonTexture(button, texture or "Interface\\PaperDoll\\UI-Backpack-EmptySlot")
        SetItemButtonDesaturated(button, not button.item)
        button:ClearAllPoints()
        button:SetPoint("TOPLEFT", 10 + ((index - 1) % PER_ROW) * (SLOT_SIZE + 4), -28 - math.floor((index - 1) / PER_ROW) * (SLOT_SIZE + 4))
        button:Show()
    end
    if loadout.flags:find("a") then
        page.note:SetText("You are wearing this loadout: equip pieces normally.")
    else
        page.note:SetText("Worn when you switch to it. Empty slots keep what you have on.")
    end
    page:SetHeight(64 + math.ceil(#order / PER_ROW) * (SLOT_SIZE + 4))
    page:Show()
end

ShowPage = function(id)
    if shownPage == id and page:IsShown() then
        page:Hide()
        return
    end
    shownPage = id
    LayoutPage()
end
page:SetScript("OnHide", function() shownPage = nil end)

local events = CreateFrame("Frame")
events:RegisterEvent("ADDON_LOADED")
events:RegisterEvent("CHAT_MSG_ADDON")
events:RegisterEvent("ACTIVE_TALENT_GROUP_CHANGED")
events:RegisterEvent("PLAYER_SPECIALIZATION_CHANGED")
events:RegisterEvent("GET_ITEM_INFO_RECEIVED")
events:SetScript("OnEvent", function(_, event, ...)
    if event == "ADDON_LOADED" then
        if ... == "Loadouts" then
            RegisterAddonMessagePrefix(PREFIX)
        end
        return
    end
    if event == "GET_ITEM_INFO_RECEIVED" then
        if page:IsShown() then LayoutPage() end
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
    elseif kind == "G" then
        -- "G", loadout id, page slot, item string
        pendingPages[id] = pendingPages[id] or {}
        pendingPages[id][tonumber(name) or 0] = spec
    elseif kind == "E" then
        loadouts, pending = pending, {}
        pages, pendingPages = pendingPages, {}
        Layout()
        LayoutPage()
    end
end)

frame:SetScript("OnShow", function() Command(".loadout sync") end)

SLASH_LOADOUTS1 = "/lo"
SLASH_LOADOUTS2 = "/loadouts"
SlashCmdList.LOADOUTS = function()
    if frame:IsShown() then frame:Hide() else frame:Show() end
end
