"""Heartseed addon tests in a Lua VM with a stubbed client API (no game client needed)."""
from pathlib import Path
import unittest

from lupa import LuaRuntime

ADDON = Path(__file__).with_name("Heartseed.lua")

STUBS = r"""
sent, bindings, saved = {}, {}, 0
widgets = {}
local function widget()
    local w = { shown = false, text = "", scripts = {}, events = {}, height = 0 }
    widgets[#widgets + 1] = w
    return setmetatable(w, { __index = function(self, key)
        if key == "Show" then return function(s) s.shown = true; if s.scripts.OnShow then s.scripts.OnShow(s) end end end
        if key == "Hide" then return function(s) s.shown = false end end
        if key == "IsShown" then return function(s) return s.shown end end
        if key == "SetText" then return function(s, t) s.text = t end end
        if key == "SetHeight" then return function(s, h) s.height = h end end
        if key == "GetStringHeight" then return function() return 100 end end
        if key == "SetScript" then return function(s, name, fn) s.scripts[name] = fn end end
        if key == "RegisterEvent" then return function(s, e) s.events[e] = true end end
        if key == "CreateFontString" or key == "CreateTexture" then return function() return widget() end end
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
        for line in ["S\t3\t5\t7\t40\t160\tSprout\tXuen\t2\t1\t",
                     "M\t10\tTiger's Fury\tXuen", "M\t25\t\t", "M\t40\t\t",
                     "X\tEssence of Storms\t2\t1", "E"]:
            self.g.addon(line)
        body = "\n".join(self.texts())
        self.assertIn("Rank 3 of 7  (account 5)", body)
        self.assertIn("Level 10: Tiger's Fury", body)
        self.assertIn("Level 25: |cff40ff40choose one|r", body)
        self.assertIn("Level 40: |cff808080locked|r", body)
        self.assertIn("Essence of Storms x2  slot 1", body)
        self.assertIn("Weapons fed: 2", body)

    def test_no_heartseed(self):
        self.lua.execute('HeartseedPanel_Toggle(); addon("N"); addon("E")')
        self.assertIn("No Heartseed", self.texts())

    def test_ignores_other_senders_and_prefixes(self):
        self.lua.execute('HeartseedPanel_Toggle()')
        self.lua.execute('fire("CHAT_MSG_ADDON", "Heartseed", "N", "WHISPER", "Someone")')
        self.lua.execute('fire("CHAT_MSG_ADDON", "Other", "N", "WHISPER", "Tester")')
        self.g.addon("S	1	1	1	0	100	Seed	none	0	0	")
        self.g.addon("E")
        texts = self.texts()
        self.assertNotIn("No Heartseed", texts)
        self.assertIn("Rank 1 of 1", texts)

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
