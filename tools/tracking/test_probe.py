from pathlib import Path
import unittest
from lupa import LuaRuntime

SCRIPT = Path(__file__).resolve().parents[2] / 'lua_scripts/Custom/Tracking/tracking_probe.lua'


class ProbeTests(unittest.TestCase):
    def setUp(self):
        self.lua = LuaRuntime(unpack_returned_tuples=True)
        self.lua.execute('''
            config = 'Tracking.Probe.Enable = 0'
            RegisterPlayerEvent = function(event, callback) registered = event end
            os.getenv = function() return nil end
            io.open = function()
                if config == nil then return nil end
                return { lines=function()
                    local done=false
                    return function() if not done then done=true; return config end end
                end, close=function() end }
            end
            player = {rank=2, auras={}, changed=0, messages={}}
            function player:GetGMRank() return self.rank end
            function player:HasAura(id) return self.auras[id] == true end
            function player:AddAura(id) self.auras[id]=true; self.changed=self.changed+1; return true end
            function player:RemoveAura(id) self.auras[id]=nil; self.changed=self.changed+1 end
            function player:GetUInt32Value() return 0 end
            function player:SendBroadcastMessage(text) table.insert(self.messages,text) end
        ''')
        self.probe = self.lua.execute(SCRIPT.read_text(encoding='utf-8'))
        self.player = self.lua.globals().player

    def command(self, text):
        return self.probe.Command(42, self.player, text)

    def test_disabled_and_missing_config_fail_closed(self):
        for value in ('Tracking.Probe.Enable = 0', 'Tracking.Probe.Enable = invalid', None):
            self.lua.globals().config = value
            self.assertFalse(self.probe.Enabled())
            self.assertFalse(self.command('trackingprobe all'))
            self.assertEqual(self.player.changed, 0)

    def test_non_gm_cannot_mutate(self):
        self.lua.globals().config = 'Tracking.Probe.Enable = 1'
        self.player.rank = 0
        self.command('trackingprobe all')
        self.assertEqual(self.player.changed, 0)

    def test_resource_profile_only_adds_resources(self):
        self.lua.globals().config = 'Tracking.Probe.Enable = true # test'
        self.assertTrue(self.probe.Enabled())
        self.command('.trackingprobe resources')
        self.assertTrue(self.player.HasAura(self.player, 2383))
        self.assertTrue(self.player.HasAura(self.player, 2580))
        self.assertFalse(self.player.HasAura(self.player, 1494))

    def test_status_and_invalid_input_do_not_change_auras(self):
        for text in ('trackingprobe', 'trackingprobe status', 'trackingprobe all extra', 'trackingprobe typo'):
            self.command(text)
        self.assertEqual(self.player.changed, 0)

    def test_unrelated_command_passes_through(self):
        self.assertIsNone(self.command('trackingprobeOther all'))
        self.assertEqual(self.player.changed, 0)

    def test_clear_allowed_after_disabling(self):
        self.lua.globals().config = 'Tracking.Probe.Enable = 1'
        self.command('trackingprobe all')
        self.lua.globals().config = 'Tracking.Probe.Enable = 0'
        self.command('trackingprobe clear')
        self.assertFalse(self.player.HasAura(self.player, 1494))
        self.assertFalse(self.player.HasAura(self.player, 2580))


if __name__ == '__main__':
    unittest.main()
