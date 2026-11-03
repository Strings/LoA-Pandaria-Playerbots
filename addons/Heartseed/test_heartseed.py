"""Heartseed addon tests in a Lua VM with a stubbed client API (no game client needed)."""
from pathlib import Path
import unittest

from lupa import LuaRuntime

ADDON = Path(__file__).with_name("Heartseed.lua")

STUBS = r"""
sent, bindings, saved = {}, {}, 0
widgets = {}
local function widget()
    local w = { shown = false, text = "", scripts = {}, events = {}, height = 0, attributes = {}, playing = false }
    widgets[#widgets + 1] = w
    return setmetatable(w, { __index = function(self, key)
        if key == "Show" then return function(s) s.shown = true; if s.scripts.OnShow then s.scripts.OnShow(s) end end end
        if key == "Hide" then return function(s) local was = s.shown; s.shown = false; if was and s.scripts.OnHide then s.scripts.OnHide(s) end end end
        if key == "IsShown" then return function(s) return s.shown end end
        if key == "SetText" then return function(s, t) s.text = t end end
        if key == "SetHeight" then return function(s, h) s.height = h end end
        if key == "GetStringHeight" then return function() return 100 end end
        if key == "SetScript" then return function(s, name, fn) s.scripts[name] = fn end end
        if key == "SetAttribute" then return function(s, k, v) s.attributes[k] = v end end
        if key == "Play" then return function(s) s.playing = true end end
        if key == "Stop" then return function(s) s.playing = false end end
        if key == "RegisterEvent" then return function(s, e) s.events[e] = true end end
        if key == "CreateFontString" or key == "CreateTexture" or key == "CreateAnimationGroup"
            or key == "CreateAnimation" then return function() return widget() end end
        return function() end
    end })
end
frames = {}
function CreateFrame(kind, name)
    local w = widget()
    frames[#frames + 1] = w
    if name then _G[name] = w end
    return w
end
UIParent = widget()
UISpecialFrames = {}
function SendChatMessage(text, channel) sent[#sent + 1] = text end
function RegisterAddonMessagePrefix() end
function UnitName() return "Tester" end
function UnitLevel() return 25 end
sounds = {}
function PlaySound(name) sounds[#sounds + 1] = name end
function GetSpellInfo(id) if id == 123904 then return "Invoke Xuen, the White Tiger", nil, "icon_xuen" end end
function GetSpellCooldown() return 0, 0 end
function InCombatLockdown() return false end
function Ambiguate(name) return name end
function GetBindingKey(action) for k, a in pairs(bindings) do if a == action then return k end end end
function GetBindingAction(key) return bindings[key] or "" end
function SetBinding(key, action) bindings[key] = action end
function SaveBindings() saved = saved + 1 end
function GetCurrentBindingSet() return 1 end
SlashCmdList = {}
tinsert = table.insert
unpack = unpack or table.unpack
function strsplit(sep, text)
    local out, start = {}, 1
    while true do
        local i = string.find(text, sep, start, true)
        if not i then out[#out + 1] = string.sub(text, start); break end
        out[#out + 1] = string.sub(text, start, i - 1)
        start = i + 1
    end
    return unpack(out)
end
function fire(event, ...)
    for _, f in ipairs(frames) do
        if f.events[event] and f.scripts.OnEvent then f.scripts.OnEvent(f, event, ...) end
    end
end
function addon(msg) fire("CHAT_MSG_ADDON", "Heartseed", msg, "WHISPER", "Tester") end
now = 100
function GetTime() return now end
glows = {}
function ActionButton_ShowOverlayGlow(b) glows[b] = true end
function ActionButton_HideOverlayGlow(b) glows[b] = nil end
hooks = {}
function hooksecurefunc(name, fn) hooks[name] = fn end
actions = {}
function GetActionInfo(slot) if actions[slot] then return "spell", actions[slot] end end
function IsShiftKeyDown() return false end
"""


class HeartseedAddonTest(unittest.TestCase):
    def setUp(self):
        self.lua = LuaRuntime(unpack_returned_tuples=True)
        self.lua.execute(STUBS)
        self.lua.execute(ADDON.read_text(encoding="utf-8"))
        self.lua.execute('fire("ADDON_LOADED", "Heartseed")')
        self.g = self.lua.globals()

    def texts(self):
        return [w.text for w in self.g.widgets.values() if isinstance(w.text, str) and w.text]

    def test_toggle_syncs_and_renders(self):
        self.lua.execute("HeartseedPanel_Toggle()")
        self.assertTrue(self.g.HeartseedFrame.shown)
        self.assertEqual(self.g.sent[1], ".seed sync")
        self.assertEqual(self.g.sounds[1], "igSpellBookOpen")
        for line in ["S	3	2	40	450	Sprout	Xuen	2	1	",
                     "M	10	Call of Xuen	Xuen	123904	A spectral white tiger fights at your side.",
                     "M	25			0	", "M	40			0	",
                     "X	Essence of Storms	2	1", "G	1	Stamina	1	1	2	30", "G	3	Critical strike	0	0	1	10", "E"]:
            self.g.addon(line)
        body = "\n".join(self.texts())
        self.assertIn("Growth level 3", body)
        self.assertIn("2 points to spend", body)
        self.assertIn("+1%", body)
        self.assertIn("Call of Xuen", body)
        self.assertIn("A spectral white tiger fights at your side.", body)
        self.assertIn("Level 25", body)
        self.assertIn("Essence of Storms x2  slot 1", body)
        self.assertIn("Weapons fed: 2", body)
        self.assertEqual(self.g.HeartseedGiftButton.attributes["spell"], "Invoke Xuen, the White Tiger")

    def stat_buttons(self):
        found = self.lua.eval("function() local t = {} for _, w in pairs(widgets) do "
                              "if type(w) == 'table' and rawget(w, 'stat') then t[#t + 1] = w end end return t end")()
        return list(found.values())

    def test_stat_button_spends_only_with_points(self):
        self.lua.execute("HeartseedPanel_Toggle()")
        for line in ["S	2	1	0	400	Sprout	none	0	1	", "G	1	Stamina	1	1	2	30", "G	2	Main stat	0	0	1	20", "E"]:
            self.g.addon(line)
        crit = [b for b in self.stat_buttons() if b.stat.id == 2][0]
        crit.scripts.OnClick(crit)
        self.assertIn(".seed spend 2", list(self.g.sent.values()))
        sent = len(self.g.sent)
        self.lua.execute("HeartseedPanel_Toggle(); HeartseedPanel_Toggle()")
        for line in ["S	2	0	0	400	Sprout	none	0	1	", "G	1	Stamina	1	1	2	30", "G	2	Main stat	1	1	1	20", "E"]:
            self.g.addon(line)
        crit = [b for b in self.stat_buttons() if b.stat.id == 2][0]
        before = len(self.g.sent)
        crit.scripts.OnClick(crit)
        self.assertEqual(len(self.g.sent), before)                 # nothing left to spend

    def test_new_gift_plays_sound(self):
        self.lua.execute("HeartseedPanel_Toggle()")
        for line in ["S	1	0	0	350	Sprout	none	0	1	", "M	10			0	", "E"]:
            self.g.addon(line)
        self.assertNotIn("LEVELUPSOUND", list(self.g.sounds.values()))
        self.lua.execute("HeartseedPanel_Toggle(); HeartseedPanel_Toggle()")
        for line in ["S	1	0	0	350	Sprout	Xuen	0	1	", "M	10	Call of Xuen	Xuen	123904	", "E"]:
            self.g.addon(line)
        self.assertIn("LEVELUPSOUND", list(self.g.sounds.values()))

    def test_gift_waits_for_combat_to_end(self):
        self.lua.execute("InCombatLockdown = function() return true end; HeartseedPanel_Toggle()")
        for line in ["S	1	0	0	350	Sprout	Xuen	0	1	", "M	10	Call of Xuen	Xuen	123904	", "E"]:
            self.g.addon(line)
        self.assertIsNone(self.g.HeartseedGiftButton.attributes["spell"])
        self.lua.execute('InCombatLockdown = function() return false end; fire("PLAYER_REGEN_ENABLED")')
        self.assertEqual(self.g.HeartseedGiftButton.attributes["spell"], "Invoke Xuen, the White Tiger")

    def test_gift_drags_to_action_bar(self):
        self.lua.execute("picked = nil; PickupSpell = function(id) picked = id end; HeartseedPanel_Toggle()")
        for line in ["S	1	0	0	350	Sprout	Xuen	0	1	", "M	10	Call of Xuen	Xuen	123904	", "E"]:
            self.g.addon(line)
        self.lua.execute("InCombatLockdown = function() return true end")
        self.lua.execute("HeartseedGiftButton.scripts.OnDragStart(HeartseedGiftButton)")
        self.assertIsNone(self.g.picked)
        self.lua.execute("InCombatLockdown = function() return false end")
        self.lua.execute("HeartseedGiftButton.scripts.OnDragStart(HeartseedGiftButton)")
        self.assertEqual(self.g.picked, 123904)

    def test_gift_icon_retried_when_spells_load(self):
        self.lua.execute("realInfo = GetSpellInfo; GetSpellInfo = function() end; HeartseedPanel_Toggle()")
        for line in ["S	1	0	0	350	Sprout	Xuen	0	1	", "M	10	Call of Xuen	Xuen	123904	", "E"]:
            self.g.addon(line)
        self.assertIsNone(self.g.HeartseedGiftButton.attributes["spell"])
        self.lua.execute('GetSpellInfo = realInfo; fire("SPELLS_CHANGED")')
        self.assertEqual(self.g.HeartseedGiftButton.attributes["spell"], "Invoke Xuen, the White Tiger")

    def tick(self, seconds):
        self.lua.execute(f"now = now + {seconds}; HeartseedMeterFrame.scripts.OnUpdate(HeartseedMeterFrame, 0.3)")

    def test_meter_counts_glows_when_full_and_decays(self):
        self.lua.execute('local b = CreateFrame("Button", "ActionButton1"); b.action = 1; actions[1] = 123904')
        self.g.addon("P	3	5	0	15000	123904")                  # no sync needed for meter updates
        frame = self.g.HeartseedMeterFrame
        self.assertTrue(frame.shown)
        self.assertEqual(frame.count.text, 3)
        self.assertIsNone(self.g.glows[self.g.ActionButton1])
        self.g.addon("P	5	5	0	15000	123904")
        self.assertTrue(self.g.glows[self.g.ActionButton1])          # full: the Gift glows on the bar
        self.assertTrue(self.g.glows[self.g.HeartseedGiftButton])
        self.lua.execute("hooks.ActionButton_UpdateOverlayGlow(ActionButton1)")   # survives a bar update
        self.assertTrue(self.g.glows[self.g.ActionButton1])
        self.tick(16)                                                # past the 15 s decay
        self.assertEqual(frame.count.text, 0)
        self.assertIsNone(self.g.glows[self.g.ActionButton1])

    def test_lit_meter_shows_until_it_closes(self):
        self.g.addon("P	0	0	6000	0	116098")
        frame = self.g.HeartseedMeterFrame
        self.assertTrue(frame.shown)
        self.assertTrue(self.g.glows[frame])
        self.tick(5)
        self.assertTrue(frame.shown)
        self.tick(2)
        self.assertFalse(frame.shown)
        self.assertIsNone(self.g.glows[frame])

    def test_meter_ignores_other_senders(self):
        self.lua.execute('fire("CHAT_MSG_ADDON", "Heartseed", "P	5	5	0	0	123904", "WHISPER", "Someone")')
        self.assertFalse(self.g.HeartseedMeterFrame.shown)

    def test_no_heartseed(self):
        self.lua.execute('HeartseedPanel_Toggle(); addon("N"); addon("E")')
        self.assertIn("No Heartseed", self.texts())
        self.lua.execute("HeartseedPanel_Toggle()")
        self.assertEqual(self.g.sounds[2], "igSpellBookClose")

    def test_ignores_other_senders_and_prefixes(self):
        self.lua.execute('HeartseedPanel_Toggle()')
        self.lua.execute('fire("CHAT_MSG_ADDON", "Heartseed", "N", "WHISPER", "Someone")')
        self.lua.execute('fire("CHAT_MSG_ADDON", "Other", "N", "WHISPER", "Tester")')
        self.g.addon("S	1	0	0	350	Seed	none	0	0	")
        self.g.addon("E")
        texts = self.texts()
        self.assertNotIn("No Heartseed", texts)
        self.assertIn("Growth level 1", texts)

    def test_default_binding_only_when_free(self):
        self.lua.execute('fire("PLAYER_LOGIN")')
        self.assertEqual(self.g.bindings["SHIFT-H"], "HEARTSEED_TOGGLE")
        self.assertEqual(self.g.saved, 1)
        self.lua.execute('fire("PLAYER_LOGIN")')
        self.assertEqual(self.g.saved, 1)

    def test_default_binding_respects_existing_key(self):
        self.lua.execute('bindings["SHIFT-H"] = "TOGGLEWORLDMAP"; fire("PLAYER_LOGIN")')
        self.assertEqual(self.g.bindings["SHIFT-H"], "TOGGLEWORLDMAP")
        self.assertEqual(self.g.saved, 0)

    def test_slash_command(self):
        self.assertIsNotNone(self.g.SlashCmdList["HEARTSEED"])
        self.assertEqual(self.g.SLASH_HEARTSEED1, "/hs")


if __name__ == "__main__":
    unittest.main()
