from pathlib import Path
import unittest
from lupa import LuaRuntime


class PrototypeTests(unittest.TestCase):
    def setUp(self):
        self.lua = LuaRuntime(unpack_returned_tuples=True)
        self.lua.execute('''
            handlers={}
            function RegisterCreatureGossipEvent(entry,event,fn) handlers[event]=fn end
            player={rank=3, menu={}, gold=100}
            function player:GetGMRank() return self.rank end
            function player:GossipClearMenu() self.menu={} end
            function player:GossipMenuAddItem(icon,label,sender,action) self.menu[#self.menu+1]=action end
            function player:GossipSendMenu(text,creature) self.text=text end
            function player:SetAtLoginFlag(flag) self.flag=(self.flag or 0) | flag end
            function player:SaveToDB() self.saved=true end
            function player:GossipComplete() self.closed=true end
            function player:SendBroadcastMessage(text) self.message=text end
        ''')
        self.lua.execute(Path(__file__).with_name('services_prototype.lua').read_text())
        self.g = self.lua.globals()

    def test_menu_and_all_free_services(self):
        self.assertTrue(self.g.handlers[1](1, self.g.player, None))
        self.assertEqual(len(self.g.player.menu), 4)
        for action, flag in [(1, 1), (2, 8), (3, 128), (4, 64)]:
            self.g.player.flag = 16  # Separate choices; native API adds bits and preserves unrelated flags.
            self.assertTrue(self.g.handlers[2](2, self.g.player, None, 0, action))
            self.assertEqual(self.g.player.flag, 16 | flag)
            self.assertTrue(self.g.player.saved)
            self.assertTrue(self.g.player.closed)
            self.assertEqual(self.g.player.gold, 100)

    def test_invalid_selection_and_prototype_gm_guard(self):
        self.assertFalse(self.g.handlers[2](2, self.g.player, None, 1, 1))
        self.assertFalse(self.g.handlers[2](2, self.g.player, None, 0, 999))
        self.assertIsNone(self.g.player.flag)
        self.g.player.rank = 0
        self.assertFalse(self.g.handlers[1](1, self.g.player, None))
        self.assertFalse(self.g.handlers[2](2, self.g.player, None, 0, 1))


if __name__ == '__main__':
    unittest.main()
