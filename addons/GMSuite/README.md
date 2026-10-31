# GMSuite addon

Panel for the GM test suite (T-048). `/gms` opens it. Copy this folder to `Interface\AddOns\GMSuite\` in a client
copy (never into `mop-clean\`). No client patch needed.

- **Checkpoints:** one button per server checkpoint; asks to confirm, then sends `.checkpoint apply <name>`.
- **Kits:** sends `.kit apply <name>`.
- **Ability tester:** Refresh (`.gmsuite refresh`), toggles for `.cheat cooldown`, `.cheat power`, `.cheat god`,
  learn class spells, Heartseed info and choice window, a level box and a learn-spell box.

Commands go to your target if it is a player, else to you. The lists come from the server (`.gmsuite sync`, sent
when the panel first opens and by Reload lists), so new checkpoints and kits appear without changing the addon.
The addon only sends commands; the server does the work.
