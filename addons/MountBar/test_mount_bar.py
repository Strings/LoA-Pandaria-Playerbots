from pathlib import Path
import unittest
from lupa import LuaRuntime

ROOT = Path(__file__).parent
MOCK = r'''
combat = false
mounted = false
called = {}
picked = {}
macros = {}
characterMacros = 0
MAX_CHARACTER_MACROS = 18
mounts = {
    {name="Turquoise Raptor", spell=10796, icon="Interface\\Icons\\Ability_Mount_Raptor"},
    {name="Swift Spectral Gryphon", spell=107517, icon="Interface\\Icons\\Ability_Mount_Gryphon_01"},
}
local function widget()
    local t = {scripts={}, enabled=true}
    setmetatable(t, {__index=function() return function() end end})
    function t:SetScript(event, callback) self.scripts[event] = callback end
    function t:RegisterEvent() end
    function t:IsEnabled() return self.enabled end
    function t:Enable() self.enabled = true end
    function t:Disable() self.enabled = false end
    return t
end
function CreateFrame(kind, name) local t = widget(); _G[name] = t; return t end
function hooksecurefunc(a, b, c)
    if type(a) == "string" then
        local old = _G[a]
        _G[a] = function(...) old(...); b(...) end
    else
        local old = a[b]
        a[b] = function(...) old(...); c(...) end
    end
end
function InCombatLockdown() return combat end
function IsMounted() return mounted end
function Dismount() mounted = false end
function print() end
function GetNumCompanions() return #mounts end
function GetCompanionInfo(kind, i)
    local m = mounts[i]
    if not m then return nil end
    return 1000 + i, m.name, m.spell, m.icon, false, 0
end
function CallCompanion(kind, i) called[#called + 1] = i; mounted = true end
function PickupCompanion(kind, i) picked[#picked + 1] = "companion" .. i end
function GetMacroIndexByName(name)
    for i, m in pairs(macros) do if m.name == name then return i end end
    return 0
end
function GetNumMacros() return 0, characterMacros end
function GetMacroBody(i) return macros[i].body end
function CreateMacro(name, icon, body, perCharacter)
    local i = perCharacter and (121 + characterMacros) or 1
    if perCharacter then characterMacros = characterMacros + 1 end
    macros[i] = {name=name, icon=icon, body=body}
    return i
end
function EditMacro(i, name, icon, body) macros[i] = {name=name, icon=icon, body=body}; return i end
function PickupMacro(i) picked[#picked + 1] = "macro" .. i end

-- The journal: row 1 greyed out (below its level), row 2 usable.
local function row(index, usable)
    local button = widget()
    button.index = index
    button.DragButton = widget()
    button.DragButton.GetParent = function() return button end
    button.usable = usable
    return button
end
MountJournal = {ListScrollFrame={buttons={row(1, false), row(2, true)}}}
function MountJournal_UpdateMountList()
    for _, b in ipairs(MountJournal.ListScrollFrame.buttons) do
        if b.usable then b.DragButton:Enable() else b.DragButton:Disable() end
    end
end
MountJournal.ListScrollFrame.update = MountJournal_UpdateMountList
'''


class MountBarTests(unittest.TestCase):
    def setUp(self):
        self.lua = LuaRuntime(unpack_returned_tuples=True)
        self.lua.execute(MOCK)
        self.lua.execute((ROOT / 'Model.lua').read_text())
        self.lua.execute((ROOT / 'MountBar.lua').read_text())
        self.g = self.lua.globals()
        self.g.MountJournal_UpdateMountList()
        self.g.MountBarFrame.scripts['OnEvent'](self.g.MountBarFrame, 'ADDON_LOADED', 'Blizzard_PetJournal')
        self.g.MountJournal_UpdateMountList()

    def drag(self, row):
        button = self.g.MountJournal.ListScrollFrame.buttons[row].DragButton
        self.assertTrue(button.IsEnabled(button))
        button.scripts['OnDragStart'](button)

    def test_greyed_mount_drags_as_macro(self):
        self.drag(1)
        self.assertEqual(self.g.picked[1], 'macro121')
        macro = self.g.macros[121]
        self.assertEqual(macro.name, 'Turquoise Raptor')
        self.assertEqual(macro.icon, 'Ability_Mount_Raptor')

    def test_usable_mount_drags_as_companion(self):
        self.drag(2)
        self.assertEqual(self.g.picked[1], 'companion2')
        self.assertEqual(self.g.characterMacros, 0)

    def test_macro_reused_and_name_capped(self):
        self.g.MountJournal.ListScrollFrame.buttons[2].usable = False
        self.g.MountJournal.ListScrollFrame.update()
        self.drag(2)
        self.drag(2)
        self.assertEqual(self.g.characterMacros, 1)
        self.assertEqual(self.g.macros[121].name, 'Swift Spectral G')

    def test_macro_body_summons_by_spell_and_dismounts(self):
        self.drag(1)
        body = self.g.macros[121].body
        self.assertTrue(body.startswith('/run '))
        self.assertLessEqual(len(body), 255)
        run = self.lua.eval('function(code) return loadstring(code) end')(body[5:]) if self.lua.eval('loadstring') else self.lua.eval('function(code) return load(code) end')(body[5:])
        # Reorder the journal: the macro must still find the raptor by spell.
        self.lua.execute('table.insert(mounts, 1, {name="Other", spell=1, icon=""})')
        run()
        self.assertEqual(list(self.g.called.values()), [2])
        run()
        self.assertFalse(self.g.mounted)

    def test_no_macro_in_combat(self):
        self.g.combat = True
        self.drag(1)
        self.assertEqual(len(self.g.picked), 0)


if __name__ == '__main__':
    unittest.main()
