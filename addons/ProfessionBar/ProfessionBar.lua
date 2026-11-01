local frame = CreateFrame("Frame", "ProfessionBarFrame", UIParent)
frame:SetPoint("CENTER", UIParent, "CENTER", 0, -200)
frame:SetSize(410, 150)
local title = frame:CreateFontString(nil, "OVERLAY", "GameFontNormal")
title:SetPoint("TOP", 0, -4)
title:SetText("Extra professions")
local buttons = {}

local function Update()
    -- Protected buttons and their parent only change outside combat; events retry it.
    if InCombatLockdown() then return end
    ProfessionBarDB = ProfessionBarDB or { enabled=true }
    local primary = {}
    if GetProfessions and GetProfessionInfo then
        local first, second = GetProfessions()
        for _, index in pairs({first, second}) do
            local _, _, _, _, _, _, skill = GetProfessionInfo(index)
            if skill then primary[skill] = true end
        end
    end
    local visible = ProfessionBarModel.Visible(IsSpellKnown, primary)
    for i, profession in ipairs(visible) do
        local button = buttons[i]
        if not button then
            button = CreateFrame("Button", "ProfessionBarButton"..i, frame, "SecureActionButtonTemplate")
            button:SetSize(130, 32)
            button:RegisterForClicks("AnyUp")
            button.icon = button:CreateTexture(nil, "ARTWORK")
            button.icon:SetSize(26, 26)
            button.icon:SetPoint("LEFT", 2, 0)
            button.label = button:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
            button.label:SetPoint("LEFT", button.icon, "RIGHT", 4, 0)
            button.label:SetWidth(96)
            button:SetScript("OnEnter", function(self)
                GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
                GameTooltip:SetText(self.professionName)
                if self.skill == 393 then GameTooltip:AddLine("Target a skinnable corpse first.", 1, 1, 1) end
                GameTooltip:Show()
            end)
            button:SetScript("OnLeave", function() GameTooltip:Hide() end)
            buttons[i] = button
        end
        local name, _, icon = GetSpellInfo(profession.ranks[1])
        local actionName = GetSpellInfo(profession.action or profession.ranks[1])
        button:SetPoint("TOPLEFT", 4 + ((i-1)%3)*134, -26 - math.floor((i-1)/3)*36)
        button:SetAttribute("type", "spell")
        button:SetAttribute("spell", actionName)
        button.icon:SetTexture(icon)
        button.label:SetText(name)
        button.professionName, button.skill = name, profession.skill
        button:Show()
    end
    for i=#visible+1, #buttons do buttons[i]:Hide() end
    if ProfessionBarDB.enabled and #visible > 0 then frame:Show() else frame:Hide() end
end

for _, event in ipairs({"PLAYER_LOGIN", "SPELLS_CHANGED", "SKILL_LINES_CHANGED", "PLAYER_REGEN_ENABLED"}) do
    frame:RegisterEvent(event)
end
frame:SetScript("OnEvent", Update)
SLASH_PROFESSIONBAR1 = "/profbar"
SlashCmdList.PROFESSIONBAR = function()
    ProfessionBarDB = ProfessionBarDB or { enabled=true }
    ProfessionBarDB.enabled = not ProfessionBarDB.enabled
    Update()
    if InCombatLockdown() then print("ProfessionBar will update after combat.") end
end
frame:Hide()
