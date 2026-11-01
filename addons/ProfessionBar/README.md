# ProfessionBar (MoP 5.4.8)

Install this folder as Interface/AddOns/ProfessionBar in a writable game-client
copy. The repo's mop-clean input is never modified by the agent.

Known professions beyond the client's first two get secure spell buttons.
Crafting opens the profession window; Herbalism/Mining toggle their trackers;
Skinning uses the normal targeted ability. All eight MoP ranks are recognized,
so replacing an apprentice spell with a higher rank keeps its button.

`/profbar` toggles the bar. Button/visibility updates wait until combat ends.
No polling/timers, arbitrary chat commands or server round trips are used.
This is the interim bar until the client profession tab can scroll.

Tests run the Lua model/UI in a Lua VM with mocked WoW frames, covering rank
recognition, native-slot filtering, secure action attributes and combat deferral:
`python -m unittest discover -s addons/ProfessionBar -p test_*.py` (requires lupa).
