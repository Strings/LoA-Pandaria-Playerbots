-- The server lets any character ride from level 1, but the 5.4.8 journal greys out mounts below their spell level
-- and won't let them be dragged. Those rows get a summon macro on the cursor instead; usable rows are untouched.
local frame = CreateFrame("Frame", "MountBarFrame")
local hooked = false
local patched = setmetatable({}, { __mode = "k" })

local function PickupMountMacro(index)
    if InCombatLockdown() then
        print("MountBar: macros can't be made in combat.")
        return
    end
    local _, name, spellID, icon = GetCompanionInfo("MOUNT", index)
    if not spellID then
        return
    end
    local macroName = MountBarModel.MacroName(name)
    local body = MountBarModel.MacroBody(spellID)
    local macro = GetMacroIndexByName(macroName)
    if macro == 0 then
        local _, characterCount = GetNumMacros()
        local perCharacter = characterCount < MAX_CHARACTER_MACROS and 1 or nil
        macro = CreateMacro(macroName, MountBarModel.IconName(icon), body, perCharacter)
    elseif GetMacroBody(macro) ~= body then
        macro = EditMacro(macro, macroName, MountBarModel.IconName(icon), body)
    end
    if not macro or macro == 0 then
        print("MountBar: no free macro slot for " .. macroName .. ".")
        return
    end
    PickupMacro(macro)
end

local function OnDragStart(self)
    local index = self:GetParent().index
    if not index then
        return
    end
    if self.mountBarLocked then
        PickupMountMacro(index)
    else
        PickupCompanion("MOUNT", index)
    end
end

local function Patch()
    local scroll = MountJournal and MountJournal.ListScrollFrame
    if not scroll or not scroll.buttons then
        return
    end
    for _, button in ipairs(scroll.buttons) do
        local drag = button.DragButton
        if drag then
            -- The journal re-runs its enable state on every update; read it before overriding.
            drag.mountBarLocked = not drag:IsEnabled()
            if drag.mountBarLocked then
                drag:Enable()
            end
            if not patched[drag] then
                drag:RegisterForDrag("LeftButton")
                drag:SetScript("OnDragStart", OnDragStart)
                patched[drag] = true
            end
        end
    end
end

local function Hook()
    if hooked or not MountJournal_UpdateMountList then
        return
    end
    hooked = true
    hooksecurefunc("MountJournal_UpdateMountList", Patch)
    -- The scroll frame keeps its own reference to the update function.
    local scroll = MountJournal and MountJournal.ListScrollFrame
    if scroll and scroll.update then
        hooksecurefunc(scroll, "update", Patch)
    end
    Patch()
end

frame:RegisterEvent("ADDON_LOADED")
frame:RegisterEvent("PLAYER_LOGIN")
frame:SetScript("OnEvent", function(self, event, addon)
    if event == "PLAYER_LOGIN" or addon == "Blizzard_PetJournal" then
        Hook()
    end
end)
