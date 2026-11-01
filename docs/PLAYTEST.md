# Playtest checklist (human)

Things only a person at the client can judge: does it look right, work as intended, feel good. Do any of it at
any time, in any order. Agents keep this list current as systems land; tick nothing here, just report.

## Reporting

- **In game:** stand where the problem is and type `/bug <what happened>` or `/idea <what you want>` (with the Capture addon, or `.bug` / `.idea` directly). The ticket
  saves your position, target, nearby bots and what they were doing to `tickets/`. Agents pick tickets up from there.
- **Longer notes:** write them in chat to Claude Code, or as a file in `tickets/`.
- A short "felt slow", "ugly", "confusing" is enough; agents dig into the why.

## Setup

- Copy `addons/GMSuite`, `addons/Capture`, `addons/Loadouts` and `addons/ReagentBank` into the client's
  `Interface/AddOns`.
- `/gms` opens the GM panel. Checkpoints (`hs-10`, `hs-25`, ... and twink brackets) put a character into a state in
  one click, so you can test late systems without leveling.

## Heartseed

- [ ] A new character gets the granting quest window a few seconds after login. Text reads well.
- [ ] The weapon glow is visible, fits each path and stage, and isn't ugly. Candidates per path are in
      `docs/agents/heartseed-glows.md`; say which you like and Claude sets them.
- [ ] The path-choice window at level 10 (and `.seed`): readable, choices clear, the pick takes effect.
- [ ] Changing a major only at a shrine feels right, not like a chore.
- [ ] Upgrading a weapon prompts to feed the old one; the prompt is clear and not annoying.
- [ ] Equipping a twink weapon shows the one-time tutorial.
- [ ] The proc fires visibly in combat and the numbers feel neither useless nor silly (they're placeholders).

## Quick wins

- [ ] Mount speed (double) feels good at every riding level, and flying doesn't overshoot everything.
- [ ] Free respec and dual spec from 10 work from the normal talent UI.
- [ ] Solo settings: flight paths, guaranteed fishing skill-ups, no weekly currency caps, lighter death penalty.
- [ ] Twink gear tooltips show the buffed stats (client hotfixes), and a twink bracket feels strong but fair.
- [ ] Raid saves stay until finished or abandoned and reset every 2 days (after T-013 merges).
- [ ] Starting kit: Apprentice Riding, a mount and four 10-slot bags on a new character (after T-011 merges).

## Loadouts and gear pages

- [ ] `/lo` panel: readable, buttons work.
- [ ] Switching keeps each loadout's talents, glyphs and action bars exactly; nothing gets scrambled.
- [ ] Heartseed majors follow the loadout.
- [ ] A named extra loadout (e.g. PvP) for the same spec stays separate from the default one.
- [ ] Gear pages: **Gear** in `/lo` opens the page; dragging a piece from the bags onto a slot puts it there, clicking
  takes it back. Icons and tooltips (enchants, gems) look right.
- [ ] Switching loadouts swaps exactly the pieces on the page and nothing else; set bonuses, two-handers vs shields
  and the Heartseed weapon behave as if you had equipped by hand. Same through the talent frame's spec switch.

## Reagent bank

- [ ] **Most important:** with a recipe's reagents only in the bank (none in the bags), the profession window still
  lets you press Create and the craft works. If the client greys the button out or says "Missing reagent" without
  asking the server, report it: the bank then needs a craft button of its own.
- [ ] `/rb` panel: **Deposit all reagents**, click to take a stack, right-click to take all. Icons and counts look right.
- [ ] Item tooltips show "Reagent bank: N".
- [ ] The intro popup shows once, the first time a profession window opens.

## GM tools

- [ ] GMSuite panel layout is usable; the confirm before applying a checkpoint isn't annoying.
- [ ] `.clock speed 60` visibly speeds up day and night in the client; `.clock reset` brings it back.
- [ ] `.spectate <bot>` follows a bot smoothly; `.spectate stop` returns you.
- [ ] `.bot why` on a bot reads clearly enough to understand what it's doing.

## Bots and world

- [ ] Bots around you look and act alive enough at the levels you play; note anything that looks robotic.
- [ ] Server performance: any lag spikes, with roughly how many bots were nearby.
