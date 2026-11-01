# ReagentBank addon

Panel for the reagent bank (T-043). `/rb` or the **Reagent Bank** button on a profession window opens it. Copy this
folder to `Interface\AddOns\ReagentBank\` in a client copy (never into `mop-clean\`). No client patch needed.

- **Deposit all reagents** moves every crafting reagent in your backpack and bags into the bank.
- Click a reagent to take one stack back to your bags; right-click takes all of it; shift-click links it.
- Item tooltips show how many the bank holds, since the profession window counts only your bags.
- The first time a profession window opens, a popup introduces the bank.

Crafting needs nothing from the addon: the server takes reagents from your bags first, then from the bank. The addon
only sends `.reagent` commands; the contents come from the server (`.reagent sync`).
