from pathlib import Path
import re
import unittest
from lupa import LuaRuntime

ROOT = Path(__file__).parent
MOCK = r'''
combat = false
known = {}
primaries = {}
frames = {}
SlashCmdList = {}
UIParent = {}
local widget
widget = function()
    local t = {attributes={}, scripts={}, shown=false}
    setmetatable(t, {__index=function() return function() end end})
    function t:SetAttribute(key, value)
        assert(not combat, "protected attributes changed in combat")
        self.attributes[key] = value
    end
    function t:SetScript(event, callback) self.scripts[event] = callback end
    function t:CreateFontString() return widget() end
    function t:CreateTexture() return widget() end
    function t:Show() assert(not self.secure or not combat); self.shown=true end
    function t:Hide() assert(not self.secure or not combat); self.shown=false end
    return t
end
function CreateFrame(kind, name, parent, template)
    local t = widget()
    if template == "SecureActionButtonTemplate" then
        t.secure=true
        parent.secure=true
    end
    frames[name]=t
    return t
end
function IsSpellKnown(id) return known[id] == true end
function InCombatLockdown() return combat end
function GetSpellInfo(id) return "spell"..id, "", id end
function GetProfessions() return 1, 2 end
function GetProfessionInfo(index) return "primary", nil, 0, 0, 0, 0, primaries[index] end
function print() end
GameTooltip = widget()
'''


class ProfessionBarTests(unittest.TestCase):
    def setUp(self):
        self.lua = LuaRuntime(unpack_returned_tuples=True)
        self.lua.execute(MOCK)
        self.lua.execute((ROOT / 'Model.lua').read_text())
        self.lua.execute((ROOT / 'ProfessionBar.lua').read_text())
        self.globals = self.lua.globals()

    def event(self, event='PLAYER_LOGIN'):
        self.globals.frames['ProfessionBarFrame'].scripts['OnEvent'](event)

    def test_only_extras_and_highest_ranks(self):
        rows = self.globals.ProfessionBarModel.professions
        for row in rows.values():
            self.globals.known[row.ranks[8]] = True
        self.globals.primaries[1], self.globals.primaries[2] = 171, 164
        self.event()
        self.assertTrue(self.globals.frames['ProfessionBarFrame'].shown)
        buttons = [self.globals.frames['ProfessionBarButton' + str(i)] for i in range(1, 10)]
        self.assertNotIn(171, [button.skill for button in buttons])
        self.assertNotIn(164, [button.skill for button in buttons])
        self.assertTrue(all(button.attributes['type'] == 'spell' for button in buttons))

    def test_gathering_actions(self):
        self.globals.known[102161] = True  # Zen Mining, not apprentice rank.
        self.event()
        button = self.globals.frames['ProfessionBarButton1']
        self.assertEqual(button.skill, 186)
        self.assertEqual(button.attributes['spell'], 'spell2580')

    def test_combat_defers_new_buttons_and_toggle(self):
        self.globals.known[2259] = True
        self.event()
        self.globals.combat = True
        self.globals.known[2018] = True
        self.event('SPELLS_CHANGED')
        self.assertIsNone(self.globals.frames['ProfessionBarButton2'])
        self.globals.SlashCmdList.PROFESSIONBAR()
        self.assertTrue(self.globals.frames['ProfessionBarFrame'].shown)
        self.globals.combat = False
        self.event('PLAYER_REGEN_ENABLED')
        self.assertIsNotNone(self.globals.frames['ProfessionBarButton2'])
        self.assertFalse(self.globals.frames['ProfessionBarFrame'].shown)

    def test_forgetting_skill_hides_stale_button(self):
        self.globals.known[2259] = True
        self.event()
        self.globals.known[2259] = False
        self.event('SKILL_LINES_CHANGED')
        self.assertFalse(self.globals.frames['ProfessionBarButton1'].shown)
        self.assertFalse(self.globals.frames['ProfessionBarFrame'].shown)

    def test_model_matches_server_primary_skills(self):
        header = (ROOT.parents[1] / 'src/server/scripts/Custom/Professions/ProfessionRules.h').read_text()
        server = {(int(a), int(b)) for a, b in re.findall(r'\{(\d+),\s*(\d+)\}', header)}
        rows = list(self.globals.ProfessionBarModel.professions.values())
        self.assertEqual(len(rows), 11)
        self.assertEqual(server, {(row.skill, row.ranks[1]) for row in rows})
        for row in rows:
            self.assertEqual(len(row.ranks), 8)


if __name__ == '__main__':
    unittest.main()
