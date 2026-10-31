# Ultimate Single-Player MoP Server — Design & Roadmap

Oct 30, 2026 · @Antonio Chavez

## Vision

Build the ultimate single-player Mists of Pandaria server: a world that feels alive at all times, populated by bots with personality, with a persistent war and an artifact that grows with you from level 1. MoP is the target because Legion private servers are nowhere near feature complete.

**Players:** built for solo play, with room for up to 7 friends on the same server. Friends level and catch up normally; there is no level normalization or modern sync system.

**Base:** [LoA-Pandaria-Playerbots](https://github.com/1LineAtaTime/LoA-Pandaria-Playerbots), a 5.4.8 (build 18414) Legends of Azeroth Pandaria fork with a full port of mod-playerbots. The README reports a median world update of about 2 ms with roughly 1000 bots online. It is under active development (last commit 2026-10-30).

**Design pillars**

- **Simulate only what a player can see.** Bots far from the player cost nearly nothing; the world wakes up around you.
- **Bots are characters, not filler.** Personality, memory and recurring faces turn population into stories.
- **Progression with a destination.** The living jewel builds toward a morph at 90.
- **The world changes.** War fronts flip towns, vendors and rewards.
- **Tooling that lets AI work in the world.** Spatial grounding (navmesh + screenshots) removes the content-placement bottleneck.
- **Broken is the reward.** No power budget: systems stack freely, and the corruption dial is the counterweight when you want a challenge.
- **Modular where possible.** Build systems as reusable modules with clean interfaces so they can serve other projects: the consciousness system (an engine-agnostic core plus a WoW adapter), the placement tool and classifiers, the MCP servers, the headless client, the DBC and patch tool. Each lives in its own folder with its own README and config, and core-game changes stay behind config keys in their own module folders so the fork stays easy to rebase on upstream.

**Design structure (decided)**

- **Core loops at every time scale:** a minute (fights, procs, reactions), a session (a rare circuit, a war front, a dungeon, a secret hunt), a week (front cycles, legacy projects, featured rotations) and a life (reincarnation). Every scale always has something to chase; a new system names the scales it feeds.
- **A power model for the corruption dial:** a rough model of expected player power by life count, legacy count and jewel rank, produced with the balance sim. It sets the dial's steps and rewards, and the dial can suggest a level.
- **Server timeline (to decide once):** the intro is at Theramore (5.0), leveling runs through a war that ends at Siege (5.4), and the Vale is in its post-Siege state. Fix what time it is in the world and what each character's story progress shows through phasing, so the Heartseed story, the True Horde hierarchy and the Sha of Grief never contradict each other.
- **Cross-expansion content (guideline, not a hard rule):** new content (rares, events, secrets, Sha hunters, world quests, discovery) aims to appear across every expansion's zones where it fits thematically, themed by the local races and factions. This includes WoD and Legion content as it is ported: their races, factions and assets join the pool and appear wherever they fit. Where no race or faction theme fits, skip it rather than force it.

## Bot performance

The ceiling is the per-map update thread, not bot count: every bot on a continent shares one thread. Login cost and wasted work around idle bots are the other big levers.

| Bottleneck | Where in the code | Fix |
| --- | --- | --- |
| 51 DB queries per bot login | `PlayerbotLoginQueryHolder` reuses `LoginQueryHolder` (`CharacterHandler.cpp`) | Bot-only holder that skips mail, pet journal, achievement criteria, social, account data (about half the queries) |
| Single DB worker thread | `CharacterDatabase.WorkerThreads = 1` | Raise to 4–8; size `innodb_buffer_pool_size` to the whole characters DB; NVMe |
| Bot AI on the map thread | `OnAfterUpdate(Player*)` → `botAI->UpdateAI()` inside `Player::Update` | `MapUpdate.Threads` = core count (helps across maps only); later, compute decisions on a worker pool and apply on the map thread |
| Packets built for nobody | `WorldSession::SendPacket` discards at the `m_Socket` check, after callers serialized | Short-circuit `SendMessageToSet` and broadcast paths when recipients are bots |
| Idle bots keep grids active | Minimal-tick bots are still in the world | True dormant state (below) |
| Random login/logout churn | `Min/MaxRandomBotInWorldTime` = 600 / 28800 s | Raise toward `PermanentlyInWorldTime` for resident pools |

**Already in the port:** `AiPlayerbot.BotActiveAlone = 10` (minimal tick away from players), `BotActiveAloneForceWhenInRadius = 150`, `BotActiveAloneForceWhenInZone = 1`, `botActiveAloneSmartScale`, and `.server info` with mean, median, p95, p99 and max update times. Leave `Map.UpdatableObjectList = 0`; the conf says it costs more than it saves with many bots.

**Dormant/wake (the key new mechanic):** take a bot off the map (`RemoveFromMap`, keeping the session and `Player` object) when no player is near, and add it back when a player enters its zone or a neighboring zone. Waking needs no DB work and is near-instant. Hook point: the `ForceWhenInZone` check in `PlayerbotAI::AllowActivity`.

**Hub density cap (decided):** the 5.4.8 client gets expensive with many player models on screen, so capitals, the Shrines and other hubs get a server-side cap on awake bots in view, separate from zone retinues. Extra bots stay dormant or move on.

## Zone retinues

Each zone keeps a resident pool of level-appropriate bots that are logged in but dormant, and wakes them before the player arrives.

- **Pool size:** about 20–40 bots per zone, levels taken from `AiPlayerbot.ZoneBracket.<zoneId> = min,max` (already in the port).
- **Wake trigger:** player enters the zone or an adjacent zone, so bots are active before they come into view.
- **Questing loops:** precomputed per-zone routes (hubs, quest chains, travel paths) followed with randomness, instead of live planning. Cheaper and more believable.
- **World PvP:** a share of each pool patrols contested areas (Krasarang, Townlong, Kun-Lai). Raise `RpgStatusProbWeight.OutdoorPvp` for Pandaria zones.
- **Reset:** snapshot character state on zone entry; when the loop finishes, restore the snapshot (a few row copies) and stagger restarts. Never delete and regenerate.
- **Phasing:** Jade Forest and Krasarang phase by faction and quest progress. Keep bot phases aligned with what players see, or use phase-neutral areas.

## Personality system

Each bot gets its own copy of the RPG status weights (`RpgStatusProbWeight.*`, currently global) plus a reaction layer for when it notices someone.

**Three layers**

1. **Traits (fixed):** 5–8 values from 0 to 1 (friendliness, mischief, aggression, curiosity, diligence, chattiness, persistence), seeded from the bot's GUID so they need no storage. Optional class and race skew. Traits scale the RPG weights.
2. **Mood (drifting):** boredom, annoyance, sociability, confidence. Rise and decay with events (long questing → boredom, deaths → annoyance, grouping → lower boredom).
3. **Relationships (memory):** a (bot, player) table with score and last-seen time. Drives nemeses and buddies. Persist only the strongest few per bot.

**Reaction loop:** when a bot sees a player or bot (only at full activity), score each reaction as trait × mood × relationship × context, then roll against a threshold. Reactions break off the current task and return to it.

- Friendly + bored → buff, `/wave`, invite to group, share quest
- Curious → follow, `/inspect`, comment on gear
- Mischievous + big level gap → start a macro behavior
- Aggressive + weak target → attack
- Diligent → ignore and keep questing

**Macro behaviors:** small authored state machines defined as data (steps, conditions, exits, cooldowns), so new ones need no recompile. Expressions use emotes, `/say` and the existing `playerbots_speech` table.

- Sap rogue: stealth → sap → `/dance` → pickpocket → vanish → reappear → `/laugh` → repeat until bored or the target leaves
- Emote mirror, quest-giver sitter, inn drunk, mailbox AFK, corpse camper who leaves after two kills, duel spammer

**Rules:** cooldowns and per-zone caps keep behaviors rare and memorable. Give each bot a consistent "tell" (name style, outfit, catchphrase) so players recognize it.

**Code shape:** a `personality` strategy always on for random bots, a trait struct on `PlayerbotAI`, a multiplier where `NewRpgStrategy` picks statuses, and new playerbots-DB tables for relationships and behavior definitions.

**Bot dialogue (decided)**

Talking to a bot opens real dialogue choices, not just whisper commands.

- **Choices:** what are you up to, any rumors (pulled from the news board), tell me a joke, want to group, seen anything interesting, plus options unlocked by relationship (buddies share stories, nemeses trade insults).
- **Random and funny content:** jokes, idle chatter and in-world gossip, written as large line libraries in `playerbots_speech`, tagged by personality trait and mood so a mischievous bot and a diligent one answer differently.
- **Provisional: interface.** The client can't open a gossip window on a player, so add a "Talk" entry to the right-click unit menu in the client patch, opening a dialogue frame fed over the addon channel. Before the patch exists, a whisper keyword menu works.
- **Provisional:** generate the line libraries offline with an LLM, filtered for voice and lore, so there are thousands of lines without any model running live.

**LLM-driven "conscious" bots (decided, long-term)**

A local LLM controls some bots, with a budget per zone. Each bot's level of consciousness depends on its relationship to the player: bots in your party get the most, bots in conversation with you get more, and the rest stay scripted.

| Level | Who gets it | What the LLM does |
| --- | --- | --- |
| 0 Scripted | Most bots | Nothing: state machine and line libraries |
| 1 Flavor | Bots near the player with a relationship | Occasional generated lines that fit the moment |
| 2 Conversation | Bots you are talking to | Free dialogue with memory of past meetings |
| 3 Companion | Party members, companions, nemeses | Conversation plus choosing intents: what to do next, reacting to events |

- Needs better hardware; likely a quantized small "flash"-class model run locally.
- **Provisional: the state machine stays in charge.** The LLM picks from structured intents (follow, quest here, duel, leave, emote, say) that map to existing strategies. It never drives movement or combat directly, so a bad answer costs a line, not a broken bot.
- **Provisional: context per call:** the bot's traits, mood and relationship, short memory summaries per (bot, player) stored in the playerbots DB, and the current zone, quest and recent events.
- **Provisional: budget.** superseded by the attention scheduler below (any bot can be promoted; level sets how often it thinks); replies are asynchronous, and when the budget is full, bots fall back to the line libraries.
- See also the provisional mod-llm-chatter port, a possible starting point.
- **Decided: the LLM chooses from the state machine, in character.** On a conscious tick, the state machine offers its top candidate actions with scores (keep questing, follow, duel, emote, go to town, start a macro behavior). The model picks one based on the bot's personality, mood, relationships and memories, and gives a one-line reason that goes into the consciousness file. If no answer arrives in time, the top-scored action runs.
  - **Provisional: the slow layer.** Between ticks, the model can also adjust the bot's own weights in its file (more aggressive after a humiliating death, friendlier toward someone who helped). Scripted ticks then reflect those choices, so even a demoted bot keeps acting on what it decided.

**Any bot can become conscious (decided).** Consciousness is a scheduling priority, not a fixed slot. One local model serves every bot through batched inference; a bot's level sets how often it thinks, and any bot can be promoted when something about it becomes interesting.

- **Attention scheduler (provisional rates):** Companion every \~4 s, Conversation on each player line plus every \~10 s, Flavor every \~60 s, Scripted never. Promotion triggers: grouping, talking, a strong relationship, a nemesis event, being near the player in a story moment. Demotion when the moment passes.
- **A tick costs output, not input:** about 1–2k prompt tokens (mostly a cached shared prefix) and about 60 output tokens (an intent plus an optional line). Prefill is fast on GPU, so throughput is set by output tokens.

**Consciousness file (decided).** Every promoted bot gets a persistent file in the playerbots DB, created lazily on first promotion (permanent roster bots always have one). It survives demotion, so the scripted layer can read its mood and grudges and the bot stays consistent.

- **Identity:** name, class, race, traits from the GUID seed, voice and tell, a short backstory written once.
- **State:** mood, current goals, opinions.
- **Relationships:** per player and per bot, a score plus a short summary.
- **Episodic memories:** timestamped entries with an importance score, written cheaply from server events (no LLM needed to record).
- **Reflection (provisional):** when idle, the model compresses old memories into summaries and updates goals, in the style of the Generative Agents research. Files stay capped at roughly 2k tokens loaded per tick.
- **Retrieval (provisional):** recent, important and relevant memories, using a small embedding model such as EmbeddingGemma (308M).

**Hardware (estimates, to benchmark).** Model choices: Qwen3.5-4B (about 3 GB at 4-bit) as the default live model, Qwen3.5-9B (about 6 GB) or Gemma 4 12B for companions on bigger cards, and NVIDIA's Nemotron 3.5 Bolt-E4B (a Gemma 4 E4B fine-tune, about 4 GB, tuned for role-play and instruction following) as a candidate to test. About 2.5 GB is reserved for the game client and desktop; numbers assume batched serving and are rough.

| GPU | VRAM / bandwidth | Recommended | Est. single-stream speed | Est. bot ticks per second (batched) |
| --- | --- | --- | --- | --- |
| RTX 4060 | 8 GB / 272 GB/s | 4B | \~65 tok/s | \~5 |
| RTX 2070 Super | 8 GB / 448 GB/s | 4B | \~105 tok/s | \~6–9 |
| RTX 5060 | 8 GB / 448 GB/s | 4B | \~105 tok/s | \~9 |
| RTX 3060 12 GB | 12 GB / 360 GB/s | 4B, or 9B for companions | \~85 (4B), \~40 (9B) | \~7 (4B), \~3 (9B) |
| RTX 3080 10 GB | 10 GB / 760 GB/s | 4B with large batches | \~180 tok/s | \~15 |
| RTX 4080 | 16 GB / 717 GB/s | 9B or Gemma 4 12B; 4B alongside | \~75 (9B), \~170 (4B) | \~6 (9B), \~14 (4B) |
| RTX 5080 | 16 GB / 960 GB/s | 9B or 12B; 4B alongside | \~100 (9B), \~230 (4B) | \~9 (9B), \~19 (4B) |

At about 5 ticks per second (the weakest card), the scheduler can run roughly 8 companions, 10 conversations and 100 flavor-level bots at once. On 16 GB cards, larger mixture-of-experts models (Gemma 4 26B-A4B, Qwen3.6-35B-A3B) are possible with experts offloaded to system RAM; worth testing for companions. The model shares GPU bandwidth with the game, so cap its batch size if frame rate drops.

**Same-PC budget (estimates, to benchmark).** Game client, worldserver, MySQL and the model on one machine. The table above covers VRAM only; while playing, the other limits matter too.

| Resource | Who competes | Plan |
| --- | --- | --- |
| GPU memory bandwidth | Game rendering and model decoding both need it | Expect roughly half to two-thirds of the table's throughput while the game renders. Cap the client's frame rate (e.g. 60 fps) and give the model a lower priority and a capped batch size. |
| VRAM (8 GB cards) | Game and desktop (\~2.5 GB), 4B weights (\~3 GB), KV cache (1–1.5 GB), CUDA overhead (\~0.5 GB) | Fits, but tight: fewer parallel slots and shorter contexts on 8 GB cards. |
| CPU | Client main thread, worldserver map threads, MySQL | The model adds little when fully on the GPU; offloading MoE experts to system RAM moves load onto the CPU, so do that only on 8+ core machines. |
| System RAM | Worldserver with resident bots, MySQL buffer pool sized to the characters DB, embedding model, any offloaded experts | 32 GB recommended; 16 GB works with the 4B and a smaller bot pool. |

- The world design models and render pipeline are development tools and do not run during normal play.
- When the player is idle or AFK, the scheduler can spend spare GPU time on reflection and memory compression, so the model works hardest when the game is not rendering much.
- **Provisional: remote GPU for the model.** Bot thoughts are asynchronous, so the model can run on a rented GPU while the game server stays on the host's PC; network latency doesn't matter. Prices checked Oct 2026: marketplace RTX 3060 12 GB about $0.05–0.17/hr and RTX 4080 16 GB about $0.30/hr on Vast.ai. Rented only during play sessions, about $20/month buys roughly 200 hours on a 3060 or 65 hours on a 4080. Running 24/7 costs about $70+/month (3060-class) to $200+ (dedicated GPU servers). Renting the game server itself is the expensive part after 2026 price rises (Hetzner's cheapest dedicated server is about €97/month), so the host's PC stays the default.

**Two-phase design (decided).** Every LLM feature ships first as a scripted approximation (Phase A), then gains an LLM provider (Phase B). The scripted version is the real system, so nothing breaks when the model is offline.

1. **One interface per call site, two providers.** Choose an intent, produce a line, adjust weights, summarize memories: each is a narrow interface. Phase A builds the scripted provider; Phase B adds an LLM provider behind the same interface.
2. **Decisions read structured data only.** The consciousness file keeps structured fields (scores, mood values, tagged memories with counts) that both providers write. LLM text (summaries, reasons) is optional on top; no system decides from free text.
3. **The LLM stays inside the scripted ranges.** Weight changes are clamped to the scripted ranges, and intents come only from the offered candidates. Grammar- or JSON-schema-constrained decoding guarantees valid intent IDs and cuts output tokens.
4. **Circuit breaker.** After N timeouts or errors the provider is marked offline, everything routes to scripted, and it retries in the background. No queue builds up and the game never waits.
5. **The attention scheduler is built once.** In Phase A, promotion means more candidates evaluated, richer memories and dialogue options; Phase B changes only who thinks at each level.
6. **Offline generation counts as Phase A.** LLM-written line libraries, backstories and lore objects ship as data. The rule is no live model required.

| Feature | Phase A (scripted) | Phase B (LLM) |
| --- | --- | --- |
| Companion intent choice | Utility scores × traits × mood × relationship, sampled by softmax with trait-based temperature | Picks from the same candidates, in character |
| Reasons | Templates filled from event tags | One generated line |
| Conversation | Dialogue choices plus trait- and mood-tagged libraries with memory slots ("last time, you {event} in {zone}") | Free dialogue with memory |
| Flavor lines | Context-tagged library lines, skipping recent ones | Generated for the moment |
| Slow-layer weight changes | Event → delta table | Judged adjustments, clamped |
| Memory compression | Repeats merged into counted records, importance decay | Narrative summaries |
| Retrieval | Recency × importance × tag match | Adds embedding similarity |
| Backstories, rumors | Offline-generated pools; structured rumors | Live rewording and distortion |
| Jewel voice, Brawler's announcer | Event-triggered line libraries | Generated, history-aware |
| Sha champions that learn | Counter-pick table from telemetry stats | Reasoned adaptations from their file |
| War front generals | Utility AI over objectives, speech library | Judged plans and speeches |

- **Voice drift:** the LLM prompt includes a few lines from the bot's own tagged library as voice examples, so a bot sounds the same on either provider.
- **Config:** `Consciousness.Provider.<Level> = scripted | llm`, so weaker GPUs can run companions on the LLM and flavor scripted. The test profile and regression suite always run scripted.
- **Roadmap:** Phase A lands in v0.2 with personalities; Phase 15 shrinks to adding providers and benchmarking.

## PvP coordination and plays

Bots coordinate through a shared call board, use their whole kit according to skill and personality, read terrain, and run class-specific plays. Warsong Gulch and Arathi Basin get plays first, then Alterac Valley.

### Call board

Each team has a list of open calls that bots read at their normal decision ticks. Calls also post a line to battleground chat from the speech library ("going flag, heals?", "inc farm 3").

- **A call:** type (heal me, going for the flag, incoming, tower clear and need backup, help, regroup), location, urgency, sender, wanted responders, expiry, current responders.
- **Who calls:** confident, chatty bots announce plays, and overconfident ones call when they shouldn't. Defenders call incoming. Anyone jumped calls for help, more often if chatty or panicky.
- **Who answers:** a utility score from role fit, personality (loyal bots answer more, diligent bots keep their job, some healers stay with the biggest group), relationship with the caller, distance, current commitment and saturation. Saturation lowers the chance as responders approach the wanted count, e.g. (1 − responders ÷ wanted)².
- **Learned trust:** bots remember how each caller's calls turned out. A caller who dies before help arrives gets weighted down; one who keeps capping flags gets followed. Uses the relationship table and episodic memories.
- **The player is on the board:** battleground chat is parsed for the same calls ("inc farm", "need heals", "going flag"), weighted by relationship and fame.
- **Beyond battlegrounds:** war fronts (plus generals posting objectives), world PvP and Sha ambushes (help calls bring nearby buddies), and dungeons (tank calls, with trust applied to reckless tanks).
- **Cost:** a short list per team, evaluated at existing decision ticks. Layers onto the port's battleground tactics, replacing fixed role assignment.

### Skill

Skill is a per-bot value separate from personality, seeded from the GUID, with roster bots higher. Personality decides what a bot wants; skill decides how well it does it.

- **Map awareness:** how many calls a bot notices and from how far. Low-skill bots tunnel.
- **Judgment:** high-skill healers prioritize the flag carrier; low-skill healers heal whoever is closest.
- **Accuracy of calls:** a low-skill defender calls "inc 2" when 5 are coming.
- **Execution:** reaction delays and cooldown use, mapped onto the telemetry distributions and scaled per bot.
- **Opening roles:** at match start each bot picks a plan from personality and skill. In Alterac Valley, greedy bots take mines, brawlers fight on roads, objective-minded bots take towers, a few rush the general; the call board reshapes it from there.

### Technique library

Off-rotation plays per class and spec, as data rows: trigger, action (a spell or short sequence), minimum skill, personality weights, cooldown and diminishing-returns tag. New techniques need no recompile.

- **Roll:** at each combat tick, chance = base rate × skill factor × trait multiplier × situation. The skill factor is zero below the minimum.
- **Low skill:** rotation plus obvious defensives; CC into diminishing returns or immunity; misses teammates being focused.
- **Mid skill:** CC on cooldown, sometimes interrupts heals, peels when it notices.
- **High skill:** a resto druid roots melee off itself, Cyclones the enemy healer during the team's burst, shifts to break roots and Wild Charges away, uses line of sight and respects diminishing returns.
- **Personality:** aggressive healers DPS when calm; protective bots peel and off-heal buddies first; selfish bots save CC and defensives for themselves; mischievous bots CC for fun.
- **Examples:** warriors Intervene and Intimidating Shout off a healer, rogues Kidney Shot whoever is on their healer, hunters trap the enemy healer, shamans and paladins off-heal, death knights grip the enemy carrier, mages Frost Nova and blink.
- **On the call board:** high-skill bots post "CC healer", "peel me", "swap to the shaman", so good teams chain CC and bad teams don't.
- **Later:** Phase 13 combat lookup tables tune these rates from the player's own play.

### Terrain-aware plays

Knocking someone off a cliff is a terrain problem, so the map is annotated offline and checked cheaply at runtime.

1. **Dangerous edges, offline:** walkable navmesh edges with a large drop, lava or deep water, or the edge of an objective platform, stored as segments with drop height. Battlegrounds first: the Eye of the Storm center, the Lumber Mill, Alterac Valley's bridges; then Sha ambush cliffs.
2. **Runtime check:** target near an edge, bot on the far side, and a predicted landing past the edge. Knockback spells have fixed horizontal and vertical speeds, so a parabola traced against the heightmap predicts the landing and fall height.
3. **Positioning:** high-skill bots move a few yards to get the angle instead of waiting for it.

- **In the technique library:** Typhoon and Thunderstorm as knockbacks, Death Grip in reverse to pull someone off a ledge. High minimum skill, boosted by mischief, long cooldown. The victim records a humiliating death, so it creates nemeses.
- **The same annotations:** pillars and walls for line-of-sight kiting, escape drops to shake melee, chokepoints for holding and saving AoE CC (tactical point annotation, as in Unreal's environment queries).

### Profession gadgets and items

Professions and items give each bot a personal toolkit, added to the technique library as item-based rows.

- **Kit source:** professions seeded from the GUID decide what can be in bags and on gear. Mischievous bots and a tinkerer type load up on gadgets; diligent bots carry potions and healthstones; showy bots and twinks carry the flashiest items.
- **Engineering:** Nitro Boosts to run down a carrier, Goblin Glider off a ledge (using the edge annotations), bombs into a group on a tower.
- **Tailoring nets** (Frostweave, Embersilk) on the flag carrier or a melee chaser.
- **Racials with intent:** Shadowmeld to drop combat, Will of the Forsaken out of a fear, War Stomp on a group capping a flag.
- **Consumables:** potions and healthstones in a pinch, drums for a push; low-skill bots sometimes drink the wrong thing.
- **Rules:** each bot has a signature gadget as its tell; engineering malfunctions are welcome comedy; bots use only what is in their bags, refilled from what they loot and craft, which feeds the bot-driven auction house idea.

### Class plays

Plays are multi-step plans with a place and a timing, run by the macro behavior engine with map anchors from the terrain annotations. A play row: class and spec (plus talents), map and anchor, start trigger (gates open, our flag taken, enemy carrier spotted), steps, exits, minimum skill, personality weights.

| Class | Plays |
| --- | --- |
| Druid | Prowl to the midfield and Mass Entanglement the clash; Stampeding Roar on the flag run; carry the flag in bear and travel form; Cyclone the healer chasing your carrier |
| Rogue | Sap the healer chasing your carrier and walk away without attacking; ninja-cap Arathi Basin bases from stealth; Smoke Bomb their healer during a kill; wait in stealth on the enemy carrier's route |
| Hunter | Camp the flag room from a ledge; trap the flag spawn; Flare the flag room for stealthers; Disengage off ledges |
| Priest | Mind Control an enemy off a cliff; Leap of Faith your carrier out of a group; Mass Dispel an Ice Block or Divine Shield |
| Paladin | Hand of Freedom and Hand of Protection on the carrier; the occasional bubble hearth |
| Warlock | Demonic Gateway on the team's flag escape route; Demonic Circle to escape; fear chains on cappers |
| Death Knight | Death Grip the enemy carrier off the roof or out of the flag room; Chains of Ice the carrier; Gorefiend's Grasp a group into AoE |
| Shaman | Thunderstorm off ledges; Grounding Totem against CC; Tremor Totem against fears; Earthbind on the carrier's path |
| Warrior | Intervene the carrier; Heroic Leap into the flag room; Intimidating Shout off a healer |
| Mage | Frost Nova a group at a cap; Ice Block through a cap; Ring of Frost at a choke |
| Monk | Roll and Flying Serpent Kick after the carrier; Ring of Peace around a healer; Paralysis on the chaser |

- **Priority (decided):** Warsong Gulch and Arathi Basin first, as the most strategic and highest-value maps; Alterac Valley next; other battlegrounds after.
- **Who runs what:** patient hunters camp the flag room and impatient ones wander off; protective, mischievous rogues sap to peel; cowardly but skilled paladins bubble hearth. Skill sets timing.
- **On the call board:** "Gateway up at our base", "sapped their healer, go".
- **Counterplay from memory:** high-skill defenders learn a bot's habits, spreading out against a known entangle or Flaring a flag room that is often camped. Nemeses learn the player's favorite plays too.
- **Effort:** mostly data: anchors per battleground from the annotation pass and about a dozen plays per class.

## Bot pools and lifecycle

Four pools with defined movement between them, sized 3–4x above need for variety. Dormant bots are only DB rows, so pool size costs disk, not CPU.

| Pool | Purpose | Sizing | Lifecycle |
| --- | --- | --- | --- |
| Zone retinues | Make zones feel alive | 20–40 per zone | Snapshot reset per loop |
| Instance pool | Dungeons and BGs, by level bracket | Dungeons: 4 × peak groups × 4 per bracket (≈48 for 3 groups), \~20% tanks, \~20% healers. BGs: largest slot count × 4 per bracket (≈80 for 10v10). AV ≈300 at 4x or cap per team | Graduates to leveling pool after X runs; rename or new character on move |
| Twink sub-pool | Rare surprise in dungeons and BGs | Small per x9 bracket | Never levels or graduates |
| Leveling pool | Natural levelers in the world | Remainder | At 90 too long → reset to low level |
| Permanent roster | Recognizable level-90 community | \~100 | Never resets; guilds, ranks, MOTD, persistent relationships |

**Existing levers (off by default):** `AiPlayerbot.ResetBotLevel.*`, `AiPlayerbot.LevelBrackets.*`, `DowngradeMaxLevelBot`. Use the `ExcludeNames` lists and `IgnoreGuildBotsWithRealPlayers` to protect the permanent roster and twinks.

**Twinks:** max level of their bracket or a gear breakpoint (the first level where a big gear jump unlocks, e.g. 78 for Hyjal and Vashj'ir quest gear), XP off, heirlooms, best blues, fixed enchant table, min-maxed spec. Roll per side per match: about 10% for one twink, 2% for a 2–3 twink premade, capped per match. Lower chance in dungeons. Arrogant, showy personality profile.

**Player twinking (decided):** an XP on/off toggle in the settings panel, free and usable anywhere, replacing the paid Experience Eliminator NPCs. No separate battleground queue for XP-off characters: twinks queue with everyone else, players and bots alike. If the core segregates XP-off players into their own queue, as retail did, disable it behind a config key. Reincarnation echoes are the one power that carries into low brackets beyond level scaling, and that is intended: a reincarnated twink is the ultimate twink, and the bot side has echo twinks too.

- **Twink gear buffs (decided, priority):** common twink gear in each bracket gets slightly better stats than heirlooms through item hotfixes, so chasing bracket gear beats leveling in heirlooms. Agents compile the current best non-heirloom items per bracket and breakpoint.
- **Twink templates (decided):** a Twinks tab in Collections stores up to three named gear templates per level bracket or breakpoint for each class (a config key; one extra key column, so it costs almost nothing), for different builds such as a healer, a flag carrier and a burst DPS set. Each template can link to a spec loadout, so loading it also sets spec, talents, glyphs and bars. Placing an item into a template keeps it there, and any character on the account in that bracket can load the template as a gear page. Loading creates account-bound copies (an exception to no binding, so templates can't duplicate gear for trading); unloading removes the copies, and taking the original out of the template removes every copy. The tab opens on the current character's class, with a dropdown for other classes, and by default shows only templates with at least one item slotted.

**Renames:** on pool moves, not on a timer, from a large name list avoiding recent names. The client caches GUID → name per session, so rename only while dormant, or recycle into a new character (new GUID). Rotation resets relationship memory; continuity lives in the permanent roster.

**Your own guild (decided):** the player can found and lead a guild. Bots apply based on the player's fame and stay out of loyalty, giving the solo endgame a raid team, officers and guild drama. It absorbs the hiring-and-loyalty idea; guild members draw from the roster and leveling pools and keep their relationship memory.

## The Heartseed (living jewel)

The artifact is a living jewel bound to the character that rides on whatever weapon is equipped, then morphs into a class-specific weapon at 90. The morph gives leveling the destination the Heart of Azeroth lacked.

**Top priority (decided).** The Heartseed is the first feature built: it makes development fun from day one and could almost anchor a server on its own.

**Name (decided): the Heartseed.** It may tell you its own personal name at its first awakening; everyone else calls it the Heartseed.

**Lore (decided)**

- Each of the four August Celestials put a sliver of themselves into one seed: Xuen's strength, Chi-Ji's hope, Niuzao's endurance, Yu'lon's wisdom. Sensing that the outsiders' war would wake the Sha, they sent it out through the mists to find someone to grow with.
- The mana bomb at Theramore was the largest outpouring of hatred and grief in years. The seed was drawn to it the way the Sha are, for the opposite reason.
- It needs a heart that isn't fully formed, so the two can grow together. A hero's heart is already set, and at Theramore it is full of rage or ambition that would turn the seed into a Sha. So it veers from the hero to the nobody beside them.
- Payoff in Pandaria, from a Celestial: "It did not choose the strongest. It chose the one who had not yet decided who to be."

**Intro scenario (decided; Horde figure provisional).** About 10 minutes on the existing Fall of Theramore scenario maps.

| Step | Alliance | Horde |
| --- | --- | --- |
| 1. Tutorial | Footman on the walls: carry supplies, put out fires, help the wounded | Grunt in the siege camp: haul ammunition, fight a few scouts |
| 2. The bomb | Outside the city on an errand when the bomb falls; the aftermath | Watching from the ridge as it falls |
| 3. The seed | A light streaks toward a raging Jaina, burns her outstretched hand and drops into yours; a vision of mist and a jade serpent | It streaks toward a Kor'kron champion, recoils from his hatred and finds you; same vision |
| 4. Sent away | "Really? You?" Kalecgos notes it isn't arcane: it chose. Jaina sighs and sends you through his portal | A Horde leader who opposes Garrosh's methods (provisional: Baine) tells you to hide it from Garrosh and sends you through a portal |
| 5. Portal | Any starting zone | Any starting zone |

Later threads: Jaina's reconciliation when she sees the seed bloom in Pandaria; Garrosh learning the Heartseed exists, ending at the Sha of Pride.

**Progression (decided)**

- **Growth** is the leveling currency: mostly from feeding weapons early, plus quests (small), rares and events (large) and first dungeon clears. Sha energy stays separate for the endgame and reincarnation.
- **Rank is capped by character level**; awakening beats (story flags) raise the cap.

| Stage | Levels | Glow | Unlocks |
| --- | --- | --- | --- |
| Seed | 1–9 | Faint | Core passive: weapon-scaled proc |
| Sprout | 10–29 | Soft | First major choice (10), first essence slot, major at 25 |
| Sapling | 30–59 | Clear | Majors at 40 and 55, second essence slot, Seedsense |
| Bloom | 60–84 | Bright | Majors at 70 and 85, third essence slot |
| Full Bloom | 85–89 | Radiant, flickering | Morph quest unlocks |
| Celestial Weapon | 90 | The morph | Class weapon, horizontal choices, raid-weapon fuel |

**What it grants**

1. **Core passive:** bonus weapon damage plus one proc, both scaling with weapon item level.
2. **Four Celestial paths** with minor traits that fill in automatically and a major choice at each major tier (10, 25, 40, 55, 70, 85, between MoP's talent tiers at 15, 30, 45, 60, 75, 90): Xuen the Tiger (offense and speed), Chi-Ji the Crane (healing and hope), Niuzao the Ox (endurance and defense), Yu'lon the Serpent (wisdom and spell power).
3. **Essence slots** (1 at 10, 2 at 40, 3 at 70): choose which collected essences are active.
4. **Utility perks at awakenings**, e.g. Seedsense: nearby rares and treasures on the minimap.
5. **Visuals:** glow and color by stage and dominant Celestial path.

**Specs (decided, answers the open question):** one Heartseed with a saved path loadout per spec; swapping specs swaps choices. Loadouts change at any Celestial shrine. The morph at 90 follows the dominant path plus absorbed essences.

**One seed per account (decided).** The seed chose a soul, not a body, and reincarnation already says the soul persists. Do this before Phase 0b hardens and real play builds up state: cheap now, painful later.

- **Shared on the account:** the "has a Heartseed" marker, rank and Growth (one pool), the essence collection and the morph gallery. Leveling a new class unlocks that class's Celestial Weapon variant as a collectible form, so alts become Heartseed content, not a regrind.
- **Per character:** path loadouts (per spec, so they travel with the character's own spec switching), essence slotting, and the story flags for this body's own events (first dungeon, reached Pandaria, first world boss).
- **The rank cap stays per character:** character level plus the awakenings this character has done itself. A character uses min(account rank, its own cap), so a fresh alt or a twink at 19 has exactly the balance it would have alone. Soul-level events (Theramore, later Celestial beats) live on the account and unlock lore, whispers and cosmetics, never cap. Low-level and twink balance is unchanged.
- **Who earns:** the character you are playing earns Growth (feeding, quests, rares). Your own alts logged in as playerbots earn none; they show the shared seed and use it at their own cap. Feeding a twink alt's old weapons grows the shared seed, and that is intended.
- **Fresh-seed run:** a character-creation toggle in the character challenges (Side fun), not a Heartseed feature. The character starts with no Heartseed, or a fresh one at rank 1 kept apart from the account's, for a seedless or fresh-start run; title and cosmetic reward on completion like the other challenges.
- **Bots recognize the seed:** a bot with a relationship to another character on your account (a nemesis, a rival, a friend) recognizes the shared Heartseed on sight: a flag check plus the glow. A nemesis says "I know that light. You." The world tracks the soul, not the save slot. It is a high-priority context trigger in the reaction loop with its own line-library entries; at LLM levels, consciousness-file memories can name the other character.

**MVP that leaves room for the full story (decided)**

1. **One canonical marker:** a single quest grants the Heartseed; its completion is the "has a Heartseed" flag everything else checks. The full Theramore scenario later becomes a better way to complete the same quest.
2. **Story beats are flags** in the Heartseed state table (first dungeon, reached Pandaria, first world boss). The MVP sets them from simple hooks; later quests set them instead.
3. **Traits are data rows** (path, required rank, spell, major or minor), so content adds rows, not code.
4. **Reserved ID ranges** for Heartseed quests, creatures, objects, items, spells and gossip (`docs/ID_RANGES.md`).

MVP scope: a short granting quest at level 1 (gossip prototyped in Lua), ranks with the level cap, Growth from feeding, the core passive, the first major choice at 10, two essence types, glow tiers and the GM rank command.

**Leveling (1–89)**

- **Scales with the weapon:** procs and traits read weapon item level or DPS, so new weapons also strengthen the jewel.
- **Feeding:** replaced weapons are consumed for artifact power and sometimes an essence (minor trait by origin: Sha-touched → shadow proc, Mogu → lightning).
- **Attunement:** a new weapon starts partly attuned; the last tier unlocks with use.
- **Appearance collection:** consumed weapons become transmog options.
- **Pacing:** dim at level 1 with one trait; wakes at story beats (first dungeon, first world boss, reaching Pandaria); major traits every 10–15 levels, offset from talent tiers; horizontal choices at 90.
- **Alive:** glow and color shift at milestones; rare whispers at key moments.

**The morph (90)**

- A quest chain where the jewel cracks and becomes a class weapon with spec variants, shaped by absorbed essences and playstyle (PvP vs PvE, damage schools).
- MoP lore anchors: monk staff tied to the August Celestials, Sha-fed warlock blade, Mogu-armory hunter bow.
- **Endgame (preferred):** raid weapons become fuel that raises the artifact's item level through MoP's item upgrade system. Alternatives: artifact as appearance and effect on any weapon, or one re-awakening quest per raid tier.

**Implementation**

- Precedent: MoP's Sha-Touched weapon socket and Eye of the Black Prince.
- Jewel state lives in account tables (rank, Growth, essences, morph forms) and character tables (loadouts, slotting, this body's story flags); see One seed per account. On equip, a script applies a hidden enchantment to a free enchant slot for the glow and trait links.
- Traits are passive auras learned and unlearned by rank and choice; procs are spell scripts reading weapon damage.
- UI: gossip menu first, then an addon panel over the addon channel.
- Item stats and new entries can be pushed by server hotfix (5.4.8 supports item data hotfixes). New models need a client patch; reuse existing models with new glows and names first.

## Sha hunters

Opt-in Sha-aligned bots hunt jewel carriers in the open world. Opting in is the fastest way to grow the jewel; opting out means a catch-up grind at 90.

- **Hunters are playerbots:** real classes, rotations and PvP strategies, with reused MoP Sha visuals and a faction hostile to both sides.
- **Corruption from the pools:** leveling-pool bots are sometimes consumed by the Sha, hunt for a while, then are cleansed and return.
- **Nemeses:** hunters who kill you or escape remember you, gain rank and a title, and return.
- **Director:** a hidden heat meter grows with time in the world and jewel rank. Ambushes only out of combat and away from quest givers and flight masters. Escalation: scouts → packs → named Sha champion. Scales with group size.
- **Stakes:** kills drop Sha essence (fastest jewel fuel); deaths let hunters steal a little essence.
- **Toggle:** a Shado-Pan watcher NPC in each major hub, with a cooldown.
- **Tuning target:** opt-in maxes the jewel around 82–85; opt-out catches up in a few hours at 90.
- **Twink balance:** no normalization and no separate twink bracket (decided); jewel rank is capped by level and traits scale with weapon item level, so low-level jewels are already level-sized.
- **Stretch:** real level-90 players invade opted-in worlds as Sha hunters.

**The True Horde hierarchy (Siege of Orgrimmar).** The Sha hunters are Garrosh's champions and Alliance defectors, giving them a leader, a motive and an endpoint.

- **Theme:** the Heartseed's mirror. The seed chooses people who haven't decided who to be; Garrosh recruits people who have, in rage or ambition. Kor'kron loyalists and Alliance soldiers who never got over Theramore end up on the same side for the same reason.
- **Motive:** Garrosh holds the Heart of Y'Shaarj and wants the seed, a living piece of all four Celestials, to oppose or corrupt. This pays off the planned thread of Garrosh learning the Heartseed exists.
- **Ranks:** Garrosh (reachable only in Siege); the Kor'kron champion from the intro, whom the seed rejected, as his right hand and the player's apex nemesis (absorbing the "rival" idea); a handful of named warlords, Kor'kron and defectors; captains and hunters from the bot pools.
- **Internal politics:** the Kor'kron despise the defectors and the defectors must prove themselves. Both compete for Garrosh's favor, and the currency is the player's jewel. A defector who wounds you is promoted over a Kor'kron captain; rivals sabotage each other or leave each other to die.
- **Succession after each Siege kill:** with 2-day resets Garrosh dies often. Each kill fractures the hierarchy, and warlords fight for leadership in the open world until the reset; the winner leads the hunt until then.
- **Betrayal:** corruption from the leveling pool is weighted by personality (high aggression, low friendliness) and bad experiences in the bot's consciousness file. A buddy bot can defect, return as a captain hunting you with its memories intact, and come back cleansed. Horde bots become loyalists, Alliance bots defectors.
- **Cost:** Kor'kron armor and display IDs exist in 5.4.8; Sha-touched visuals are already planned.

**Mini-expansion: the Sha of Grief (long-term, provisional).** After Siege, the Sha thread MoP left open becomes a personal story built from each player's own nemeses. Needs the world design pipeline and the LLM bots.

- **The villain is emergent:** whichever nemesis has the most interactions with the player and the most power becomes the Sha of Grief. The raid encounter is authored; the nemesis supplies its name, appearance, voice and history.
- **The son is real:** a Kor'kron bot the player actually killed, chosen from the kill log and bot memories (a captain ganked at a war front, a hunter cut down in an ambush), is revealed as the nemesis's son. Players can find him in their own history.
- **The plan:** with Kor'kron deserters, the nemesis raids titan and mogu relics (the Spoils of Pandaria, the Thunder Forge, the Engine of Nalak'sha) and perverts them with Sha energy. "Finish what Garrosh started" is what the deserters follow; the real goal is to bring the son back.
- **Why the Heartseed:** it is the one thing that grows a heart. The nemesis wants it to give their son a heart again, which makes the finale a question: does the seed belong with someone who hasn't decided who to be, or someone who decided on grief?
- **Other nemeses** become Sha lords matching their personality and memories (arrogant twinks to Pride, cowards to Fear, aggressive bots to Anger) and fill out the encounters.
- **Ground:** Sha-corrupted versions of places that matter (the Theramore crater, war front towns, zones where nemeses were made), phased in as the threat escalates, built with the world tools.
- **Ending:** defeat, or a Celestial offer to let the nemesis grieve, which gives the Heartseed's last awakening a real choice.

## War fronts

Scheduled, announced battles over contested towns, where the winner changes the town until the next battle.

**Core machinery already present**

- `game_event` tables for scheduled spawns and despawns; calendar entries and world broadcasts for announcements.
- Outdoor PvP script framework (Wintergrasp, Tol Barad style) for objectives, world-state UI and ownership flips.
- Phasing for town states: the winner's version appears, with vendors, quest hubs and flight masters changing too.

**Design**

- **Rotating fronts:** one front per continent (see Fronts by continent below).
- **Losing side:** occupied town with resistance quests and a sabotage track that grants an edge next battle.
- **Anti-snowball:** underdog buff and bot fill on the outnumbered side.
- **Build-up:** bots chat about upcoming battles, armies gather at staging points, permanent-roster guilds arrive together.
- **Sha tie-in:** long battles spawn a Sha manifestation as a third party; hunters target jewel carriers on the field.
- **Rewards:** tabards, transmog sets, titles, mounts, war currency, and a winner-only vendor in the captured town.

**Schedule (decided)**

Real time, frequent: each front fights about every 3 hours. If no player is there, the battle still runs for about an hour and resolves through dice rolls behind the scenes. Wintergrasp's default timers (30-minute battle, 150 minutes between) already match this cadence.

**Fronts by continent**

| Continent | Front | Status in the core |
| --- | --- | --- |
| Eastern Kingdoms | Hillsbrad (Southshore vs Tarren Mill), decided | Must be built: new geometry |
| Northrend | Wintergrasp, decided | Working and on by default |
| Cataclysm zones | Tol Barad, decided | Ported from TrinityCore about a week ago; off by default (`TolBarad.Enable = 0`), SQL still marked draft |
| Pandaria | Krasarang (Lion's Landing vs Domination Point), provisional | Must be built |
| Outland | Halaa (Nagrand), provisional | Outdoor PvP town flip exists |
| Kalimdor | Ashenvale or Stonetalon, provisional | Must be built |

Also in the core: outdoor PvP scripts for the Hellfire towers, Terokkar spirit towers, Zangarmarsh and Silithus. Venture Bay and the Eastern Plaguelands towers are not scripted. Vault of Archavon and Baradin Hold are tied to Wintergrasp and Tol Barad, so the faction holding the front gets the raid.

**Bots in battlefields (already in the port):** random bots in the zone accept the war invite automatically. With `AiPlayerbot.RandomBotJoinBattlefield = 1` (default 0), up to `RandomBotJoinBattlefieldCount` bots per faction queue, teleport to their team's graveyard, fight, and return to roaming.

**Provisional: battle simulation and churn**

- Use the Wintergrasp battlefield framework (`BattlefieldMgr` timers, queue, graveyards, workshops, building destruction) as the base for new fronts; prove the offline sim on Wintergrasp first.
- Stagger fronts about 25–30 minutes apart so a battle is always running or about to start.
- Simulate in ticks, not one roll. Inputs: army size from pools, roster guilds present, sabotage track, underdog buff, momentum, and a small weight for past player contribution.
- A player arriving mid-battle converts sim state into a live battle (armies sized to remaining strength, objectives as held). Leaving hands back to the sim.
- Server downtime: resolve missed battles quickly on startup, but only the most recent result per front counts.
- Hold thresholds (narrow wins contest, decisive wins flip) and occupation decay (fortifications grow) so flips feel like events.
- Signals: a 15-minute broadcast and calendar entry, town crier results, bot chatter, and a recruiter who ports you to the staging point.
- A shared war score across continents that unlocks a faction-wide offensive when one side holds four fronts.

**Asset conversion**

- WoD assets are closest to MoP formats and convert most easily.
- Legion and BfA models (chunked M2, newer textures and animation data) need downporting tools and manual cleanup of animations and particles.
- Ship everything as a client patch MPQ through a launcher, planned from the start.
- Convert buildings and props first; mounts and creatures last.

**Enhanced graphics (decided, optional).** The launcher offers an "enhanced graphics" toggle; no client source needed.

- **DXVK:** drop-in DLL that translates the client's DirectX 9 and 11 calls to Vulkan; often smoother frame times, especially on Linux and newer AMD cards.
- **ReShade with a tuned preset:** post-processing with depth access: ambient occlusion, color grading, sharpening, depth of field, screen-space global illumination.
- **HD textures:** AI-upscaled texture overrides in the patch MPQ, run through the existing texture conversion pipeline.
- **Out of reach without source:** new lighting models, real ray tracing, longer draw distance, modern shadows.

**Extraction difficulty by era (assessment)**

Reading files is solved for every version (wow.export handles current retail). The hard part is converting down to what the 5.4.8 client renders, and asset type matters more than era.

| Era | Difficulty | Main problems |
| --- | --- | --- |
| WoD | Easy | Same model generation as MoP; mostly CASC to MPQ repackaging |
| Legion | Moderate | The real format break: chunked M2, file IDs instead of paths, separate .anim and .skel files |
| BfA | Moderate | Like Legion, plus worse listfile coverage and more shared skeletons |
| Shadowlands to now | Moderate (static), hard (animated) | Newer shader and texture-combiner modes, higher bone counts, newer particles, DB-driven character customization |

- Static props and buildings from any era are feasible; at worst some materials render flat.
- Animated creatures and mounts get harder with each expansion after Legion. Anything tied to character models is probably not worth it.
- The Legion break is a one-time cost; later eras mostly add entries to a shader-mode remapping table.
- **Provisional:** check retail's player-housing decor as a large catalogue of placeable static props for the world design pipeline. Confirm what has shipped first.

## Open leveling and zone scaling

Level through any expansion, with per-zone scaling caps. The scaling layer is shared with the equal-item-level raids, so it is built once.

- **Decided: per-zone caps.** Creatures in a zone scale with the player up to that zone's cap, then stop. Once you pass the cap, you outlevel the zone naturally (e.g. Borean Tundra scales up to its cap and no further).
- **Decided: creature-based scaling.** No player-relative modern scaling for multiplayer; friends catch up normally.
- **Provisional: structure.** 1–60 unchanged (Cata-revamped old world, starting zones). 60–89 pick any expansion and switch freely. Everything converges at 90.
- **Provisional: full ladder per expansion.** Stretch each expansion's zone caps across 60–89 so any expansion is a complete journey; original caps would force the old order.
- **Provisional: one scaling table** (zone ID, minimum, cap) drives creatures, quest rewards, dungeons, and bot retinue brackets, replacing fixed `ZoneBracket` values.
- **Provisional: multiple players in one zone.** Scale each creature when first engaged, to whoever pulls it (or the group leader), capped by the zone. Unengaged creatures sit at the zone's base level.
- **Provisional:** Sha hunters appear in every expansion (the Sha feed on negative emotion; Hellfire and Icecrown have plenty).

| Outland zone | Provisional cap | Northrend zone | Provisional cap |
| --- | --- | --- | --- |
| Hellfire | 66 | Borean / Howling Fjord | 68 |
| Zangarmarsh | 70 | Dragonblight | 72 |
| Terokkar / Nagrand | 76 | Grizzly Hills / Zul'Drak | 78 |
| Blade's Edge / Netherstorm | 84 | Sholazar / Storm Peaks | 85 |
| Shadowmoon | 89 | Icecrown | 89 |

## Raids

Every raid from every expansion is viable endgame at the same item level, for sideways progression (decided). All 31 raids from Classic through MoP have every boss scripted; the bot side has no raid strategies yet.

**Lockouts (decided):** no weekly raid lockouts. Raids reset every 2 days. Provisional reading: an instance you are partway through is kept until you finish or abandon it, so a reset never wipes a run in progress. No weekly vault.

**Boss script audit (LoA repo, Oct 30, 2026)**

| Expansion | Raids | Status |
| --- | --- | --- |
| Classic | Molten Core, Blackwing Lair, Onyxia (WotLK level-80 version), AQ20, AQ40 | All bosses scripted; scripts are short (100–200 lines) and lean on original spell data |
| TBC | Karazhan, Gruul, Magtheridon, Serpentshrine, Tempest Keep, Hyjal, Black Temple, Sunwell | All bosses scripted, including Opera, chess and Hyjal waves. Kil'jaeden's Armageddon flagged for rewrite |
| WotLK | Naxxramas, Eye of Eternity, Obsidian Sanctum, Vault of Archavon, Ulduar, Trial of the Crusader, ICC, Ruby Sanctum | All scripted; Ulduar and ICC are the most thorough in the core. TODOs are timers |
| Cata | Blackwing Descent, Bastion of Twilight, Throne of the Four Winds, Baradin Hold, Firelands, Dragon Soul | All scripted. Cho'gall is missing absorb phases, Fester Blood and entry order |
| MoP | Mogu'shan Vaults, Heart of Fear, Terrace of Endless Spring, Throne of Thunder, Siege of Orgrimmar | 43/43 bosses, 1,300–5,000 lines each. Minor gaps: Garalon intro, Mel'jarak and Shek'zeer add aggro, Garrosh berserk timer |

Zul'Gurub and Zul'Aman are the Cata 5-player heroic versions, not raids.

**Bot support:** `mod_playerbots/src/strategy/dungeons/` covers all nine MoP 5-player dungeons plus Cata and WotLK dungeons, but there are no raid strategies (upstream WotLK raid strategies were not ported). The dungeon strategies (about 12 files per instance) are the template.

**Provisional**

- The corruption dial sets item level (Mythic+ style), not the raid. Raid choice is about flavor and unique drops.
- Each raid's reason to run: tier sets with translated bonuses (wearing gives the bonus; the legacy project makes it permanent), signature trinkets and weapons with translated effects, legendaries, jewel fuel and morph appearances.
- Enemy scaling: creature level 90 with per-raid health and melee scaling, a damage multiplier on flat-damage boss spells, and small tweaks for resistance checks (Molten Core fire, Sapphiron frost).
- Item scaling by script: keep stat ratios, recompute the budget at the target item level from the game's stat-budget tables, map obsolete stats (spell power, MP5, armor penetration, defense) to MoP stats, push as item hotfixes.
- Bot raiding starts as an overgeared carry; per-encounter strategies come later for higher corruption, one boss at a time. Terrace of Endless Spring first (four bosses, no TODOs). Smaller raid sizes reduce AI work.
- Zul'Gurub rebuilt as a 20-player raid (decided, pending a map check): check whether the 5.4.8 client still ships the original raid's map files; if not, host it on the Cata map. Port the original boss scripts from WotLK-era cores (AzerothCore, TrinityCore 3.3.5), reuse Hakkar's model from the Jin'do fight, and put the Swift Zulian Tiger and Swift Razzashi Raptor back on drops. Zul'Aman stays a Timewalking-style dungeon.

## Multiplayer (up to 8 players)

Solo-first, but viable for up to 7 friends (decided). Friends level and catch up normally; no level normalization.

**Group corruption (decided):** if anyone in the group has corruption 0, the group fights at 0. Otherwise use the average of members' dials, rounded down (40 and 1 gives 20).

**Provisional**

- Instances: average over members inside, fixed at the first pull. Open world: average over group members near the creature at the pull; ungrouped players use the puller's dial.
- Rewards (Sha energy, item levels) come from the effective dial, so the group gets the same.
- Dormant/wake: the awake set is the union of areas around each player. Budget awake bots per map thread, not per player; all players on one continent is the worst case.
- Retinue snapshot resets only when no player is in the zone. Sha heat is per player, with existing group scaling.
- Relationships are already per (bot, player), so each friend gets their own nemeses and buddies.
- The launcher and patch pipeline move earlier, before the first custom item or spell hotfix.
- Hosting on a small VPS or one machine with port forwarding. SOAP and placement tool endpoints stay private to the host; friends get normal accounts.
- Hardware: strong single-thread performance and enough cores for per-map threads.

## Travel

All mount speeds are doubled, and every character starts with a regular mount (decided). This gives Dragonflight-like travel speed without implementing dragonriding mechanics.

| Riding | MoP speed | New speed |
| --- | --- | --- |
| Apprentice (ground) | 60% | 120% |
| Journeyman (ground) | 100% | 200% |
| Expert (flying) | 150% | 300% |
| Artisan (flying) | 280% | 560% |
| Master (flying) | 310% | 620% |

- **Starter mount:** learn Apprentice Riding and a basic mount at character creation (`playercreateinfo` spell and skill rows).
- **Implementation:** a multiplier on mounted movement speed in the core's speed update. The server sends speeds to the client, so no client patch is needed.
- **Provisional:** double druid travel and flight forms and ghost wolf to match; double taxi flight speed; flavor the starter mount as a gift at the end of the jewel intro.
- **To test:** terrain streaming at 620% flying, and whether any anticheat speed check needs adjusting.

**Professions and tracking**

- **All professions (decided):** raise `MaxPrimaryTradeSkill` (default 2; confirm the key in the LoA fork) so a character can learn all eleven. The spellbook's Professions tab shows only two primaries, so extras open through an addon button bar until the client patch makes the tab a scrolling list. Profession perks stack, per the broken-is-the-reward pillar, and every legacy crafting step becomes self-sufficient.
- **Multi-tracking (provisional, needs a client test):** tracking auras set bits in the resource and creature tracking fields, which are bitmasks. Remove the tracker exclusivity rule and make the aura handler add and remove bits instead of overwriting, behind a config key. Test whether the minimap draws several types at once and whether the tracking dropdown shows several checkmarks; if not, an addon or patch tweak provides the checkbox list.

**Item binding (decided).** Everything trades: no soulbound gear.

- **How:** a core hook behind a config key skips binding when items are picked up or equipped, so it survives future imports and can be switched off. A one-time SQL pass clears the soulbound flag on existing items.
- **Tooltips:** the client still shows "Binds when picked up" from its own item data; fix with item hotfixes or the item data file in the patch MPQ.
- **Kept bound:** quest items (trading breaks quest logic) and custom progression items: the Heartseed, legacy project and dormant items, and reincarnation vault items.
- **Payoff:** gear and BoP mats move freely between alts and friends; bots that loot raid epics can sell them on the bot-driven auction house; twinks gear up from a main's bank.
- **To check:** stock MoP transmog binds the source item, so collection-based transmog must not rely on that.

**Bot economy (decided).** With no binding and bots that loot and sell, the auction house needs a pricing model. Prices follow supply and demand from what bots actually hold and craft, and gold sinks (36-slot bags, expensive vendors) keep it from inflating. Without a model it either floods or sits empty.

**Queue for outleveled dungeons (decided).** The dungeon finder works for dungeons you have outleveled, doubling as a round-trip teleport for transmog runs, mount farming, legacy components and raid secrets.

- **20+ levels over:** solo teleport, no group.
- **Fewer than 20 levels over:** a group of level-appropriate bots from the instance pool.
- **Measured against:** the dungeon's level cap in the scaling table, so a dungeon that scales up stays a group run until you are 20 past its cap. The 20 is a config key.
- **Server:** relax the "too high level" lock behind a config key. **Client:** the list shows server-sent locks, so this may be enough; if the client also filters by its own dungeon level data, fix it in the patch MPQ. Test first.
- **Difficulty:** outleveled runs stay easy; the corruption dial and the group corruption rule apply as everywhere else.
- **Provisional stretch:** add old raids to Raid Finder through client dungeon data, so raid secrets and legacy infusions are a click away.

**Free respecs and a loadout per spec (decided).**

- **Free changes:** no gold cost for talent resets, and no reagents (Tome of the Clear Mind, Dust of Disappearance) for changing talents or glyphs. A config-keyed hook in the reset cost and the reagent check.
- **Unlocked from the start:** spec switching available as soon as specializations unlock at level 10, free, instead of buying dual spec at 30.
- **Every spec, not just two:** the 5.4.8 client and its packets handle exactly two talent groups, so keep those two client-side and store any number of loadouts on the server. A loadout holds spec, talents, glyphs, action bars and the Heartseed path loadout. Switching writes the chosen loadout into the inactive group, then runs the normal spec switch. An addon lists loadouts; no client patch needed.
- **Default:** one loadout per spec created automatically (four for druids), plus room for extra named loadouts (e.g. a PvP build).
- **Fits the Heartseed:** the jewel's saved path loadout per spec rides along, so swapping specs swaps everything in one action.

**Gear pages (decided).** Each loadout gets its own page of equipment slots, so off-spec gear lives on its page instead of in your bags.

- **Storage:** items on inactive pages are held outside the inventory, the way void storage and mail already keep items detached from bags. They persist normally and don't take bag space.
- **Swap:** switching loadouts detaches the equipped set onto its page and equips the new page through the normal equip path, so unique-equipped checks, two-hand and dual-wield rules, set bonuses and the Heartseed's equip script all run as usual. Out of combat only.
- **Viewing and editing:** the client can't show detached items, so an addon panel shows each page from item links (with enchants and gems) and lets you move pieces between bags and pages.
- **Replaces the equipment manager** for spec swaps; transmog can follow per page.

**Talent rebalance (decided).** MoP's structure stays (six tiers of three class-wide choices); the fixes target rows with one obvious pick and PvP-only talents.

- **Measure first:** the balance sim runs every spec and every throughput row against the dummy room; agents flag rows where one choice wins by more than a few percent. Utility rows are judged by hand.
- **Buff, don't nerf:** weaker choices are raised to match the best, per the broken-is-the-reward pillar.
- **Situational rows are fine:** free respecs and per-loadout talents let players swap an AoE pick for a single-target one.
- **Tooltips:** the client reads talent numbers from its own spell data, so changes ship in the patch MPQ's spell data.
- **Reincarnation tie-in:** see the second-talent unlock under Reincarnation.

**Modern stats and itemization (decided).** Versatility, speed, leech and avoidance as real stats, shown on the character sheet.

- **Effects:** server-side auras from the item. Versatility raises damage and healing done and lowers damage taken; speed adds movement speed; leech heals from damage dealt; avoidance reduces area damage. Versatility rating gets a per-level conversion table like MoP's other ratings.
- **Character sheet (patched):** the stat sheet is FrameXML, so the patch adds the new stats to it, under the interface signature patch. Values come from the server over the addon channel, or a native Lua call through the client extension DLL.
- **Item tooltips:** test whether the client's stat-type table can show a new stat natively; otherwise the stat's tooltip line comes from a new enchantment entry in the item's spare property slots (the slots MoP uses for random suffixes).
- **Random upgrades:** extend MoP's Thunderforged and Warforged rolls to all gear at every level, scaled to item level: any drop can roll higher item level, a socket or a tertiary stat. Twinks can roll them too, while bot twinks' generated gear rarely has them, giving player twinks a new edge to chase.
- **Upgrade track:** at MoP's item upgrade NPC, next to reforging, a track that infuses a tertiary stat or a socket instead of item level, paid in Sha energy.
- **Bots:** generated bot gear rolls these too, so inspect shows them like anyone else's.

**More solo quality of life (decided).** Mostly existing config keys; confirm each survived in the LoA fork.

- **Cross-faction play with friends:** the `AllowTwoSide.*` keys for groups, chat, guilds, mail and auction house.
- **All flight paths known:** `AllFlightPaths`.
- **Guaranteed profession skill-ups:** the `SkillChance.*` keys, so orange, yellow and green recipes always raise skill.
- **No weekly currency caps:** valor and conquest caps removed, matching no lockouts.
- **Lighter death penalties:** reduced or no durability loss and resurrection sickness, shorter corpse reclaim delays.
- **Guild perks for everyone:** MoP's guild perks (Mobile Banking, Have Group Will Travel, Mass Resurrection, mount speed, bonus XP) granted as passive auras or spells on every character.
- **Unrestricted transmog:** mixed armor and weapon types in the transmog module, alongside collection-based transmog.
- **Bigger stacks:** larger material stack sizes, matched on the client by item hotfixes or the patch MPQ.
- **Free character services:** an NPC that runs customize, race change and faction change for free.
- **Shorter hearthstone cooldown:** changed on the server and in the patch's spell data so the client timer matches.
- **36-slot bags as progression, not a giveaway:** sold for a lot of gold as a sink, and planted as drops in specific content such as rares. Bags stay something you earn. Every new character starts with a 10-slot bag in all four bag slots (character creation item rows).

## Scenarios and rare drops

**New story scenarios (decided).** Turn memorable old moments (the Wrathgate, the Battle for Undercity) and new stories into replayable scenarios at 90, built with the world placement tool. The core already has MoP's scenario framework and 17 scripted scenarios to copy from. New scenarios probably also need scenario and step rows in the client data, which ship in the patch MPQ.

**Provisional: scenario rewards.** Not the best gear source; mostly collection and legacy fuel.

- A themed cosmetic per scenario (appearance, toy or title) for Collections
- A guaranteed legacy component for a related project (e.g. the Wrathgate drops Lich King-era materials)
- Themed jewel essences and Sha energy
- "Jewel memories": a first clear adds the moment to the jewel, unlocking a morph visual variant
- Gear at the effective corruption dial, like every other endgame source
- A weekly featured scenario with a bonus chest

**Rare mounts at 10% baseline (decided).** Rare mounts are a grind, not a subscription hook: even Invincible, Ashes of Al'ar and Mimiron's Head drop 10% of the time.

- **Provisional:** chance rises further with corruption.
- **Provisional:** each player rolls separately; bots always pass on mounts.
- **Provisional:** rare-spawn mounts (e.g. Time-Lost Proto-Drake) get shorter respawn timers to match.
- **Provisional:** the same treatment for rare pets and toys.

## Rares and discovery

Discovery is the weakest of the eight kinds of fun in this plan, so the world should reward wandering. Rares are the main tool (decided): many more of them, and each one worth checking for.

**Rares (decided)**

- **Big XP:** roughly 25–100% of a level, scaled to your level, so checking known spawns while leveling becomes a habit.
- **A signature drop per rare:** a mount, pet, toy or appearance at a good rate (provisional: 10–25%, with bad-luck protection). Rares are how collectibles get distributed across the world.
- **Jewel essences** themed by the rare's origin.
- **Minimap stars** through the core's existing vignette system (`src/server/game/Vignette/`); bots call out rares in general chat and sometimes beat you to them.

**Provisional: modifiers.** Each spawn rolls 0–3 Diablo-style modifiers; each modifier adds loot rolls, collectible chance and XP.

| Modifier | Effect | Built from |
| --- | --- | --- |
| Molten | Fire trails, explodes on death | Existing fire AoE spells |
| Frozen | Orbs that burst into frost novas | Existing frost spells |
| Vampiric | Heals from its damage | Leech aura |
| Shielded | Periodic absorb shield | Absorb aura |
| Teleporting | Blinks behind you | Existing blink spells |
| Sha-touched | Fears, leaves a Sha pool | MoP Sha abilities |
| Juggernaut | Huge health, slow, unstoppable | Scale plus root immunity |
| Summoner | Calls adds | Summon script |
| Borrowed boss ability | A signature move from a dungeon boss | That boss's spell script |

- **Legendary rares** (rarest tier) get 2–3 boss abilities and a short phase.
- **One shared affix module** serves rare modifiers, corruption dial affixes and Brawler mutators.
- **Modifiers show as buff icons** plus a spawn emote; the client caches names per creature entry, so per-spawn names aren't possible.

**Provisional: other discovery wins**

| Difficulty | Idea |
| --- | --- |
| Easy | Treasures in nooks across every zone, with era-matched loot (legacy components, appearances); placed with the population grammar |
| Easy | A traveling merchant at a random spot each week with rare stock |
| Easy | More frequent, dramatic weather; Sha-corrupted weather in contested zones |
| Easy | Shrines, cooking fires and fountains with short buffs or a little jewel power |
| Easy | Rare gathering-node variants and denser nodes in neglected zones |
| Moderate | Lore objects (books, journals, notes) collected in a Lore tab; LLM-written, checked against the lore bible |
| Moderate | Ambient life: street performers, duels at city gates, crowded inns at night, fishermen at dawn |
| Hard | Dynamic events (caravans attacked, Sha eruptions, bandit raids) with bots joining and news board results |
| Hard | Sunsong Farm as a cozy loop: crops that grow legacy materials, visits from your relationships |
| Hard | A server story in acts, with the Sha threat escalating as milestones pass |

**Provisional: discovery through existing systems.** Ideas drawn from Elden Ring, Breath of the Wild, Outer Wilds, Hades and retail WoW secrets. Most need no terrain tricks, so they survive 620% flying.

- **The Heartseed as a dowsing rod.** It pulses warmer near hidden things, and the hint sharpens as it grows: a faint pulse at Seed, direction at Sapling, Seedsense on the minimap at Bloom.
- **Conditional spawns.** Some rares and treasures appear only when conditions line up across systems: war front ownership, Sha weather, time of day, corruption level, past-life count. The combinations produce discoveries nobody authored.
- **Rumors as clues.** Bots who wander off route find things and gossip about them; some rumors are wrong, and buddy bots give better tips through "seen anything interesting."
- **Echoes found in the world.** Some ability mutations come from a shrine at the bottom of a cave or a scroll from a rare, not the reincarnation draft, so exploring grows the echo pool across lives.
- **Combination secrets.** Hidden morph forms for specific essence mixes, and recipes found by experimenting.
- **Visions at historic sites.** At places like the Theramore crater or old Southshore, the jewel shows short scenes from the past.

**Flying rule for terrain secrets.** Anything visible from the air can be landed on, so terrain secrets follow three rules:

1. **Default: hide them where flying can't go.** Caves, interiors (no mounting indoors), under overhangs, underwater. The placement tool raycasts upward from each candidate and rejects or scores down spots with open sky.
2. **Default: a barrier other than geography.** Objects phased in only while the Heartseed senses them, doors that need an item, or spots that only matter under the right conditions.
3. **Sparingly: local no-fly zones with flavor.** An area aura such as Strong Winds or Sha Turbulence dismounts you with slow fall and a parachute visual, on a few memorable peaks and secret areas. Precedent: Timeless Isle and Isle of Thunder were no-fly.

**Raid secrets.** When something in a raid has changed, the Heartseed says only "something has changed here." Players search the raid for interactables that unlock rare cosmetics. Two-day resets make this a reason to rerun familiar raids.

- **Rotating changes:** each reset, a different raid or wing gets the change.
- **Chains across raids:** an object in Karazhan gives an item that only works at one spot in Black Temple, Lucid Nightmare style.
- **Encounter conditions:** kill a boss a specific way or leave an add alive, and the room changes afterwards. Boss scripts already have the hooks; the corruption dial can gate the harder ones.
- **Bots as witnesses:** roster bots in your raid sometimes remark that something looks different.
- **Example:** keep Qin-Xi and Jan-Xi from killing each other in Mogu'shan Vaults, then interact with the throne, and the mogu emperor becomes a mount.

**Creature mounts and the triage pipeline.** A mount is an aura with a creature display ID, so any existing creature model can be mounted, and `.mod mount <displayId>` tests one in seconds. Goofy rewards are the point: the sillier the mount, the more obscure the secret it comes from.

| Tier | What the model needs | Work |
| --- | --- | --- |
| 1 | Nothing: shares a skeleton with an existing mount | Display scale row only |
| 2 | A rider attachment point | Small model edit (one attachment on a bone), shipped in the patch MPQ; the creature is unaffected |
| 3 | Missing jump or flying animations | Animation work; ground-only until flying animations exist |
| 4 | Humanoids and odd skeletons | Shoulder attachment; awkward on purpose, kept for goofy rewards |

- **Triage:** script `.mod mount` through the live server MCP, screenshot each display standing, running and jumping, and have the AI sort them into tiers. The output is a ranked list of mountable boss and rare displays for signature drops and raid secrets.

**Sealed secrets (decided).** The player writes the idea; agents own the implementation (placement, conditions, triggers) and keep it sealed. Automated checks (reachability, conditions firing, rewards granted) validate each secret, so it never needs to be looked at to be known to work.

- **Volume:** most secrets are agent-designed from briefs, so there are too many to remember even for the designer.
- **Secrets log, so nothing is unrecoverable:** every secret is recorded in a sealed log stored outside the game data. In game, a hint ladder per secret reveals in steps on request: a vague hint, then the zone, then the location, then the full solution. The settings panel can also unseal everything at once.

## Emergent events

Small procedural events in the open world, each with a boss and real rewards (decided as a concept; details provisional). A Kolkar invasion sets up camp outside a Barrens town, raids the road, and ends with a warband chief.

**Generate offline, assemble at runtime.** The procedural work happens offline where the tools can check it; runtime only picks from approved pieces.

1. **Event sites (offline):** per zone, the pipeline generates and validates candidate sites (flat, near a town or road, reachable, away from quest givers). You approve them in the rating loop.
2. **Camp layouts (offline):** several layouts per scene recipe ("Kolkar warband, grassland, staging") per site, from culture kits mined from Blizzard's placements. Approved layouts are saved.
3. **Assembly (runtime):** the event director picks a site, layout, faction, boss archetype and modifiers, and spawns 30–60 validated objects and creatures.

**Event arc**

| Stage | What happens |
| --- | --- |
| Arrival | Scouts appear, the camp goes up over a minute or two, bots in town shout about it, minimap star and map pin |
| Escalation | Raids on the town or road; world-state UI objectives (burn tents, free captives, kill war-shamans) |
| Climax | The boss: borrowed boss abilities plus rolled modifiers from the shared affix module |
| Resolution | Win: camp burns, town rewards you. Ignored: the town is briefly occupied (reuses the war front occupation mechanic) |

**Event types** come from the scene taxonomy: invasions, rituals, caravan ambushes, migrations, excavations and Sha eruptions, crossed with every culture that has a kit.

**Rewards (provisional)**

- The boss's signature drop: an event-themed collectible (e.g. a Kolkar war-kodo), with bad-luck protection
- Event currency for a vendor in the saved town
- Big XP and local reputation, scaled to contribution
- Jewel essences themed by culture
- Saved towns remember: NPC thanks, news board and bard mentions, fame

**Director (provisional):** shared with the Sha hunter heat meter. Places events near players but never on top of them or on quest hubs and flight masters; scales with group size and zone level caps; cooldowns keep events special; bots join in.

**Early version:** hand-placed sites (placement tool, Phase 2), a few hand-built layouts, and bosses from the rare framework and affix module (Phase 4d). The full generated version needs the world design pipeline (Phase 12).

## Battle pets as a combat passive (provisional)

Your summoned companion grants a passive from its battle pet data, so pet battling feeds combat power. The core already has the full battle pet system (`src/server/game/BattlePet/`). Server-side only; tooltips show existing spell names until the client patch adds custom ones.

- **Family buff (easy):** a small passive per family, scaling with pet level and quality (e.g. Critter: movement speed; Mechanical: once-per-fight cheat death; Aquatic: swim speed and water breathing; Beast: damage to wounded targets; Undead: small leech).
- **Ability procs (main work):** the companion occasionally uses one of its three chosen battle abilities in real combat. Battle abilities aren't real spells, so each needs a translation to a real spell with a similar visual (found with content search). Scales with pet level and player level.
- **The companion casts it:** companions are summoned creatures, so a small AI lets the bolt come from the pet's position.

## Onboarding and the quest log

- **Tutorials (decided):** every new player-facing system ships with a brief introduction, ideally an introductory quest offered at the level the system unlocks, with a short popup as a fallback.
- **Quest log cap: client limit.** The 25-quest cap is fixed in the client: the player's quest log is a fixed 750-field block (25 quests × 30 fields) in the client's update-field layout, mirrored by `MAX_QUEST_LOG_SIZE 25` on the server. It can't be raised without rewriting the client executable.
- **Provisional: quest backlog.** Keep extra quests in a server-side backlog shown in an addon or patched UI panel, and swap them in and out of the 25 real slots automatically (by zone, or by what you're tracking). World quests already use area auto-accept and auto-remove for the same reason.

**Player-facing UI (decided)**

- **Settings panel:** one addon panel for every player-facing option: corruption dial, Sha hunter opt-in, storyteller profile, solo dungeon threshold, LLM provider levels, multi-tracking and the rest.
- **Progression journal:** one window for where you stand in everything: jewel ranks and paths, echoes, legacy projects, nemeses and their ranks, discovered secrets, collections. It shows the next goal in each.
- **Custom achievements and titles:** for Heartseed awakenings, nemesis kills, discovery secrets, legacy sets, reincarnation counts and Brawler ranks; titles such as "Thrice-Reborn". Needs client achievement data in the patch MPQ.

**Chronicle instead of interruptions (decided).** Almost nothing interrupts play.

- **Chronicle panel:** a checkable log grouped by event type with unread counts: rumors, war front results, nemesis activity, rare sightings, ambush reports, auction house sales, achievements, guild news. Entries link to a waypoint, joining a front, or the nemesis's journal page. It is the personal counterpart of the in-world news board.
- **Live in game:** whispers from bots, the jewel speaking, a buddy calling for help, since those come from someone in the world.
- **True interrupts, each switchable in the settings panel:** an ambush incoming, a war front starting in 15 minutes, a legendary rare nearby.

**Audio cues (decided).** Heartseed awakenings, "something has changed here", Sha ambush warnings and nemesis arrivals get sounds, mostly reused from existing sound kits.

## Navmesh for dynamic geometry

The navmesh is server-side only, so new buildings need no client decompile. Without a fix, bots walk through spawned buildings because the mesh is baked from static geometry.

**What the core already has:** runtime tile load and unload (`MMapManager` `addTile`/`removeTile`), dynamic gameobject collision for line of sight and height (`DynamicMapTree`), and `dtNavMesh::setPolyFlags` in the bundled Detour.

| Technique | Use for | Runtime cost | Work |
| --- | --- | --- | --- |
| Pre-baked tile variants per town state | War front town flips | None (swap 1–4 tiles of 533 yd) | Generator run per state |
| Poly flags on baked barricade areas | Gates, doors, barricades | Instant | Bake areas as their own type |
| `DetourTileCache` obstacles + smaller tiles | Player-built or random structures | Milliseconds per tile, worker thread | Add library, regenerate mmaps |
| Steering fallback (raycast + inflated convex footprint + visibility graph) | Safety net for anything missed | Tiny | \~100 lines of geometry; cannot add floors or bridges |

**Plan:** tile variants + poly flags + steering fallback now; tile cache only if player-built structures arrive.

## World placement tool (MCP + viewer)

A tool that gives AI spatial grounding in the game world: it reads geometry, renders views, and maps screenshot pixels to world coordinates. It removes the content-placement bottleneck for everything else in this plan.

**MCP tools:** `slice_region`, `render_view`, `pixel_to_world`, `world_to_pixel`, `validate_spawns`, `export_sql`. The viewer is the human-facing view of the same tools.

1. **Slice:** read navmesh tiles (`.mmtile`), terrain heightmaps, static collision and DB spawns for a bounding box; export mesh + JSON.
2. **Viewer:** single-page three.js. Navmesh colored by area, terrain draped with client minimap tiles (exactly grid-aligned), existing spawns, draggable candidates colored by validation, SQL export. Renderable headlessly so the AI can inspect its own work.
3. **Validation:** a custom GM command over SOAP checks ground height, indoor/outdoor, liquid, slope, line of sight, navmesh reachability (same code as `.mmap path`) and spacing; bad candidates are nudged and rechecked.
4. **Screenshot registration:** an addon fixes first-person capture and logs position, facing and pitch per screenshot. FOV calibrated once with Perspective-n-Point against known landmarks. Then pixel → ray → mesh hit gives world coordinates, and spawns or wireframes project back onto screenshots for previews.
5. **In-game spot checks:** with the PC linked, `.go` to spawns and screenshot for final QA.

**Other uses:** debugging stuck bots, route authoring (questing loops, ambush points, patrols, staging), quest spawn density, previewing converted buildings before baking tiles.

**Precedent:** an existing [WoW Server MCP](https://glama.ai/mcp/servers/rga4yror3i) already lets AI clients manage Cata/MoP repack servers; this tool adds the spatial layer none have.

## Reincarnation (infinite progression)

One character, infinite progression: reincarnating purifies collected Sha energy into permanent Jade echoes (new and mutated abilities), balanced by an infinite Sha Corruption difficulty dial.

**Lore and loop:** the jewel absorbs Sha energy from endgame activities. A ritual with the August Celestials purifies it into echoes carried into the next life; the jewel restarts as a seed and can morph into a different class-weapon variant each life (collectible forms).

|  | Full rebirth | Re-awakening |
| --- | --- | --- |
| Resets | Level, jewel, quest state, optionally reputations | Jewel and morph only (stay 90) |
| Echo payout | Large | Small |
| Carries over | Echoes, mounts, transmog, achievements, jewel appearances | Same |
| Gear | Vault that unlocks at 90, or heirloom-style scaling | Kept |

**Echoes**

- Each reincarnation offers a draft: pick 1 of 3.
- Types: ability mutations (Mortal Strike leaves a Sha pool, Roll leaves a healing trail), borrowed cross-class spells, new mechanics (extra charge, resource refund), small passives (the only purely numeric kind).
- Echo slots grow slowly (one every few lives) while the echo pool keeps growing, so power grows mostly sideways and builds become the endgame.
- Past lives: bots in the relationship table remember your previous incarnation.
- **Mortal life (decided, optional):** a hardcore toggle per life. Dying forces an immediate reincarnation, and the risk pays extra echo draft choices or rarer echoes.
- **Second talent per tier (decided):** every few lives, unlock one more talent tier where you can take two choices. Slow horizontal power across lives, like echo slots, and it makes obvious-pick rows matter less.

**Cross-class rules**

1. **Borrowed utility is weaker:** about 1.5–2x the original cooldown or a reduced effect (warrior Blink on a 1-minute cooldown, borrowed Lay on Hands at 30%). No source-class talents or glyphs apply.
2. **Borrowed damage is translated:** costs the host's resource, scales off the host's main stat, and hooks the host's signature mechanic.

| Borrowed spell | Host | Translation |
| --- | --- | --- |
| Fireball | Rogue | Energy cost, attack power scaling; each combo point cuts cast time (instant at 5); ignites on crit |
| Chain Lightning | Warrior | Rage cost; more jumps at higher rage; crits refund rage |
| Holy Light | Death Knight | Runic power cost; stronger in Blood Presence; overheal becomes Blood Shield |
| Pyroblast | Monk | Consumes all chi; damage scales per chi spent |

**Infinite challenge:** a Sha Corruption dial on raids, dungeons and the open world scales enemy health and damage and adds affixes per level, with Sha energy gains scaling too. Higher levels unlock each life. Echoes keep the player ahead only with good picks.

**Bots with echoes**

- Bots get plausible loadouts, not tracked histories: prestige drawn around the player's reincarnation count with noise, echo count matching slots, class-compatible combos weighted toward synergy with a small chance of odd picks. Generated from the GUID seed; a few integers per bot.
- Every echo definition carries a bot usage hint (trigger, target, resource condition such as "cast at 5 combo points"), read by one generic strategy.
- Readability: glow color by prestige tier, titles such as "Thrice-Reborn", echoes visible on inspect via the addon.
- Permanent roster bots keep evolving loadouts and occasionally reincarnate (announced in guild chat). Named Sha champions roll counter-picks against the player. Echo twinks are a rare BG event.

**Implementation:** mutations reuse glyph-style spell modification and aura-based spell overrides plus spell scripts. Cross-class translation is a per-host-class table (resource mapping, scaling stat, mechanic hook) read at cast by a spell script that applies spell modifiers. Truly new spells need client `Spell.dbc` entries for icons and tooltips, so they ship in the war front client patch. The difficulty dial is a creature scaling aura plus an affix system.

## World quests

Legion-style world quests are a moderate port: on a single-player server the content spawns globally with no phasing, and MoP's rotating daily pools (`pool_quest`) already do most of the server work. The real work is the client UI, built as an addon.

**Server side**

- Rotation scheduler: active world quests per zone from a pool, 2–24 hour timers, a spawn set per quest (spawn pools and game events).
- Area auto-accept on entry and auto-remove on exit, with progress kept server-side.
- Progress bars (not in the MoP client) faked as a count-to-100 objective credited by kills and interactions.
- Emissaries ("complete 4 world quests in Kun-Lai") credited by script, plus reputation and paragon-style caches.

**Client side (addon over the addon channel)**

- World map pins with timers and reward icons (MoP's API supports custom pins, as HandyNotes shows), an objective tracker and an emissary board.
- World quests take quest log slots (cap 25); area auto-accept and auto-remove keep only a couple in the log at once.

**Integration:** zone bots get a world-quest RPG status and compete for rares and objectives; Sha hunters favor world quest areas; war front winners unlock front-specific world quests; the placement tool handles spawn layout.

**Scope:** about 15–25 per zone for six Pandaria zones plus Timeless Isle, mostly templates (kill rare, collect, defend point, pet battle, PvP objective) with different spawns.

## Legacy: old sets and legendaries

Old tier set bonuses and old legendaries become permanent, level-scaled powers the jewel absorbs, earned through crafting projects with guaranteed drops instead of lockouts and luck. Legacy powers survive reincarnation; echoes are what change each life.

**Crafting projects (the shared pattern)**

1. **Start a project** at Lorewalker Cho. Each project is a quest; up to about 3 active.
2. **Gather bulk era materials:** e.g. 200 Fel Iron Ore, 60 Felweed, 40 Netherweave Cloth, 10 Primal Fire for a TBC set.
3. **Collect rare components:** guaranteed drops from their source while the project is active (quest-only loot at 100%), ignoring lockouts.
4. **Sacrifice a base item** for top-tier projects: an old epic or legendary such as a T2 piece or a Windseeker binding.
5. **Craft a dormant item**, then **infuse it** by killing the signature boss with it in your bags.
6. **The jewel consumes it:** permanent power, level-scaled server-side, plus the appearance added to the weapon's morph forms.

**Legacy set bonuses**

- Every iconic tier (about 13 tiers × 11 classes × 2–4 bonuses). The 2-piece unlocks first, then 4-piece and Classic 6/8-piece at steeper cost.
- Infusion bosses by tier: Ragnaros (T1), Nefarian (T2), Illidan (T6), the Lich King (T10), and so on.
- Example (Judgement 8-piece): Thorium, Arcanite, Runecloth, Black Lotus; Sulfuron Ingot (Golemagg) and Essence of the Red (Vaelastrasz) guaranteed; infuse on Nefarian.
- Most old set bonus spells and tooltips should still exist in 5.4.8 data, so granting one means learning a passive spell. Bonuses referencing removed abilities get translation scripts.
- Stacking: everything stacks, with diminishing returns per tier band.

**Legendary powers**

| Legendary | Source | Power (translated to the host class) |
| --- | --- | --- |
| Thunderfury | Bindings + Prince Thunderaan | Chain lightning proc that slows |
| Atiesh | Naxx 40 splinters | Party-wide class aura + Karazhan portal |
| Warglaives of Azzinoth | Illidan | Fel cleave; dual-wield appearance |
| Val'anyr | Ulduar fragments | Healing leaves absorb shields |
| Shadowmourne | ICC chain | Soul fragments stack into a Chaos Bane burst |
| Dragonwrath | Firelands | Direct spells sometimes repeat |
| Fangs of the Father | Dragon Soul chain | Shadow-step burst on a cooldown |
| Frostmourne (custom) | Authored Icecrown chain ending at the Lich King | Killed enemies briefly rise as ghouls |
| Ashbringer (custom) | Purify the Corrupted Ashbringer from Naxx | Bonus damage to undead and Sha, cleansing burst |
| Doomhammer, Gorehowl (custom) | Faction-flavored chains | To design |

- Translation rule as for echoes: a priest consuming Warglaives gets a shadow cleave, not dual wield.
- Each consumed legendary adds a visible facet to the jewel.
- Original legendary questlines mostly still exist in 5.4.8 data; custom chains are authored quests. Powers are spell scripts on the echo translation table.
- Addon tracker shows each project's checklist and drop sources.

**Capstone: later-expansion sets (decided)**

Set bonuses from WoD and later become the largest legacy projects. They need materials from every expansion and unlock only after a number of other projects are done.

- **Tier I:** Classic and TBC set projects.
- **Tier II:** WotLK, Cata and MoP sets, unlocked after X Tier I projects.
- **Capstone:** WoD and later sets, needing materials from every expansion plus N completed projects.
- **Ported by script:** set names, bonus text and values come from DB2 exports and generate items, project quests and tooltips. Each bonus's mechanics still needs a hand-written spell script, translated to MoP abilities as with echoes.
- Pick iconic sets per class rather than every set; Legion bonuses are per spec, which multiplies the count. Demon hunter and evoker sets are skipped (no such classes).
- **Appearances:** helms, shoulders and weapons port fine. Chest, legs, gloves and bracers are textures mapped to WoD's newer character models and will probably look wrong on MoP's models, so body pieces are a late stretch goal.
- **Provisional:** infusion bosses for the capstone, since those expansions' raids are not in the game. Use an iconic substitute boss or a war front champion.

**Legacy consumables (decided).** Legacy projects and secrets end; consumables get used up, so they give the old world a recurring draw on its materials.

- **Consumable modification:** a base consumable (MoP flask, potion, food or weapon oil) is infused with one or two era reagents, each adding a modifier themed by its origin: Felweed adds a fel damage proc, Frost Lotus frost damage and a slow on hit, Black Lotus a burst of every stat at the start of combat, volatile elements a shield that discharges when broken. The base sets strength; modifiers add flavor. Some combinations are secret and found by experimenting.
- **Classic-style world buffs at 90:** Songflower Serenade from Felwood songflowers; Rallying Cry of the Dragonslayer when anyone, bots included, turns in Onyxia's or Nefarian's head in a capital (announced in the chronicle); Spirit of Zandalar, Warchief's Blessing, and new buffs from each era's zones. Powerful and kept through death.
- **Purpose:** high corruption dial levels are where stacked modifiers and a full set of world buffs matter.
- **Bots:** raid-ready roster bots buy and craft consumables, keeping material prices moving on the auction house.
- **Implementation:** modifiers are auras and procs on the shared affix module; infusion is a crafting script. No client patch.

## Collections

A modern Collections window (Mounts, Pets, Toys, Heirlooms, Appearances) delivered by client patch, on modern default keys (decided). The 5.4.8 client only has the Mount and Pet journals; the Toy Box and Heirlooms came in WoD and the appearance wardrobe in Legion.

**Interface signature check (needs a small exe patch).** The 3.3.5 client rejects modified FrameXML and GlueXML ("interface files are corrupt"), and the standard fix changes 2 bytes per check in Wow.exe. 5.4.8 almost certainly has the same check (not yet tested). The repo already ships the 5.4.8 Arctium connection patcher (`contrib/SkyFire Patcher/548`), which finds and replaces byte patterns in Wow.exe and Wow-64.exe. It has no interface patch yet.

1. Find the check in the 5.4.8 executables (trace references to the "interface files are corrupt" string).
2. Add the pattern and replacement to the patcher for x86 and x64 (and Mac if needed).
3. Test with one modified FrameXML file in a patch MPQ; the launcher applies the patch automatically.

**Client patch work**

- Turn `Blizzard_PetJournal` (already a tabbed window on Shift+P) into Collections with Toys, Heirlooms and Appearances tabs
- `Bindings.xml` changes for modern defaults
- WoD collection UI art ported as textures
- "Already known" and "Collected" tooltip lines

**Server work**

- Account-wide tables for toys, heirlooms and appearances (mounts and pets are already native)
- Unlock hooks: using a toy item, getting an heirloom, and appearances from loot, equipping and consumed jewel weapons
- Toy use: cast the item's spell with per-character cooldowns
- Heirlooms: create the item on request, with upgrade levels to 90
- Transmog: change `CustomTransmogrification` to apply appearances from the collection instead of owned items
- Data over the addon channel (5.4.8 has no native messages for these), with paging for large appearance lists

## Porting later-expansion content

WoD and Legion content (mounts, toys, pets, heirlooms, props, set pieces) is ported in order of difficulty: easy ports move early, hard ones become late projects (decided).

**Pipeline:** retail IDs in, client patch and SQL out.

1. **Models:** convert M2, skins and textures. WoD is easy; Legion needs the chunked M2 and .skel/.anim converter.
2. **Client data:** DBC rows for display info, model data, spells (5.4.8 splits a spell across several DBC files), sounds and item display info, packed into the patch MPQ.
3. **Server data:** the same modified DBC files go into the server's dbc folder; items go to `item_template` and hotfix tables.

| Wave | What | Why it sits here | Roadmap |
| --- | --- | --- | --- |
| 1 (easy) | WoD mounts, heirlooms, transform toys, pets reusing existing ability sets, WoD props and buildings | One model plus simple data; same model generation as MoP | Phase 1b |
| 2 (moderate) | Legion and BfA mounts and pets, static props from any era, helms, shoulders and weapons, capstone set bonus data | Needs the Legion converter and per-bonus spell scripts | Phase 8b |
| 3 (hard) | Toys with visual effects, animated Legion+ creatures, body armor appearances, anything tied to character models | Legion reworked spell visuals; newer body textures don't fit MoP character models | Phase 14 |

## Brawler's Guild: the Stupid Ladder

An endless ladder past rank 8 for fully maxed characters, built on the Brawler's Guild already scripted in this core (`scripts/World/BrawlersGuild/`).

- **Endless ranks** tied to the corruption dial, one more mutator per rank.
- **Mutators:** inverted echoes, no healing, legendary powers fight for the boss, Sha-pool floor, shrinking arena, periodic 3-second mind control.
- **Remixed old bosses:** Ragnaros in a 40-yard ring, Illidan and the Lich King together, Hogger at corruption 200.
- **Mirror match:** a bot snapshot of the player's own echoes, legacy bonuses and legendary powers.
- **Gauntlet nights:** permanent-roster bots back to back with full loadouts, including the player's nemesis.
- **Bot leaderboard:** roster bots post plausible, climbing ranks and taunt when passed.
- **Rewards:** titles, cosmetics, extra echo draft choices, rare legacy gem variants. Prestige over power.
- **Implementation:** mutators as auras and arena scripts; mirror match as a bot snapshot; remixed bosses reuse creature scripts with corruption scaling. No client patch.

## Side fun (decided)

Fun that isn't the main progression: games, minigames, old favorites and remix ideas. Cheapest and most likely to land first: the always-on Darkmoon Faire with bot leaderboards and betting, wild gems, fishing tournaments with bots, and bot city raids.

**Darkmoon Faire, always open**

- **Always on:** tickets feed Collections through the existing prize vendors plus new cosmetics, pets and toys.
- **Bot leaderboards on every game:** Whack-a-Gnoll, Tonk Commander, the shooting gallery, the cannon. Roster bots post scores and taunt when passed.
- **Betting booth:** wager gold on bot duels, Brawler fights and bot pet battles; a gold sink.
- **Darkmoon Deathmatch** as a scheduled bot free-for-all with a chest for the last one standing.

**Wintergrasp and Tol Barad**

- **Vehicle progression:** cosmetic and upgrade paths for siege engines, earned across battles.
- **Tol Barad prison break:** escaped Baradin Hold prisoners roam the peninsula as rares until recaptured.
- **Wintergrasp at 90:** the front scales up, with corruption-scaled Vault of Archavon bosses for the holder.

**Fishing**

- **Tournaments** (Stranglethorn, Kalu'ak) weekly, with bots competing on a leaderboard.
- **Aquarium:** rare fish as collectibles in their own Collections tab.
- **Fish that fight back:** rare catches become a mini-boss fight in the water.
- **Treasure maps** fished up that lead into the secrets system.

**Archaeology**

- **Faster surveys and bad-luck protection** on rare artifacts.
- **Digsite guardians:** digging sometimes wakes a rare.
- **Artifacts as Heartseed memories:** some finds unlock the jewel's historical visions or rare echoes.
- **Lore and pristine artifacts as a collection,** feeding the Lore tab.

**Remix-style**

- **Wild gems:** prismatic sockets with chaotic procs (chain lightning, meteor strikes, summoned allies), MoP Remix style; built on the shared affix module.
- **Seasonal remix mode** (promoted from the borrowed list): fast characters with stacking stats whose cosmetics return to your main.

**Community favorites**

- **Character challenges at creation,** Turtle WoW style: hardcore, no grouping, slow XP, no auction house, fresh seed (no shared Heartseed, see One seed per account), each with a title and cosmetic reward. Pairs with mortal life.
- **Bot city raids:** the opposite faction raids your capital, leaders and all; defend or counter-raid. Driven by war front momentum.
- **Gurubashi Arena chest:** the free-for-all chest fight revived with bots.
- **Prop hunt:** bots disguised as objects in a town via transformation spells; find them before time runs out.
- **Pet battle PvP against bots** with real teams, and roster bots known as tamers.

## Borrowed from retail (decided)

Retail features adopted beyond the provisional borrow list. Profession knowledge trees were considered and rejected: their modifiers add complexity that is hard to track, and the existing crafting systems cover the same ground.

- **Reagent bank with crafting from it:** reagents stored outside bags (detached storage, like gear pages), and crafting draws from it directly through a hook in the reagent check. Matters with all eleven professions and legacy consumables.
- **Class campaigns and class halls:** a campaign per class tied to its Heartseed morph, with halls in existing places: the Peak of Serenity (monks), Acherus (death knights), Moonglade, Light's Hope, the Violet Citadel, Ravenholdt and others. Class mounts later through the port waves.
- **Keystone runs:** MoP's challenge mode timers plus the corruption dial's scaling and affixes, with a keystone that rises when you beat the timer and drops when you don't. Mythic+ built mostly from existing parts.
- **Bonus objectives:** WoD-style area quests that activate on entry with no quest giver, using the world quest auto-accept.
- **Guild mission table:** send guild members and buddy bots on missions while offline. Requests include asking the guild to farm a raid for you or gather materials, with a limit per raid reset and per week. Results depend on the bots' skill and gear; their memories record what happened, and they return with stories.
- **Solo Shuffle against bots** in rated arena rotations, and **pet battle dungeons** modeled on WoD and Legion ones, with MoP's Celestial Tournament as the template.
- **Plunderstorm with bots:** a battle royale in a closed zone with a shrinking storm, loot that grants abilities, and bots filling the lobby. Pairs with the survivors arena.

## Sunsong housing (decided)

Sunsong Ranch becomes player housing with retail-style decor placement, built from the world tools. It doubles as a gold and resource sink. Moderate effort: the server side is easy, the addon UI is the main work.

- **Plot:** Sunsong is already phased per player, so placements are personal. Plot expansions (more land, more farm plots, the house interior) cost gold and materials.
- **Decor library:** the asset catalogue, filtered to placeable props, browsed in an addon with 3D previews (the client's model frames can display models). Decor comes from vendors, profession crafting, rares, secrets and the Darkmoon Faire, and is collected in Collections.
- **Click to place:** a ground-targeted placement spell uses the client's targeting circle, so clicking the ground gives the exact point. The server spawns the object there.
- **Adjust:** addon nudge buttons for move, raise, lower, rotate and scale, with optional grid and angle snapping done on the server. Each nudge respawns the object, which is acceptable at nudge speed; there is no smooth drag preview.
- **Rules:** the placement tool's validation (ground height, overlap, indoor and outdoor) as snapping and collision rules. A per-plot object budget keeps the client fast; extra budget is another sink.
- **Persistence:** placements in a per-account table, spawned on entering the phase.
- **Life:** companions, buddies and guild members visit; consumed legendaries and trophies go on display.

## More player types (decided)

- **Roleplayers:** character profiles like the TRP3 addon, readable by bots; bots respond to in-character emotes and `/say` (scripted in Phase A, conversation in Phase B); inn nights where conscious bots stay in character.
- **Theorycrafters:** the balance sim's dummy room open to players; a planner for echoes, legacy powers and Heartseed paths that shows the math behind each choice.
- **Gold makers:** market events on the bot auction house (a raid farm floods epics, a war front drives up potions) and price history in the chronicle, so flipping and crafting for profit are a real playstyle.

## World design pipeline

A one-time setup turns world design from the hardest bottleneck into a review step: an asset catalogue tells the AI what exists, a design grammar learned from Blizzard's own zones tells it how to compose, and a rating loop trains models on your taste. It builds on the placement tool.

**1. Asset catalogue**

- Extract WoD and Legion assets from CASC (CascLib, wow.export, community listfile). Paths already encode culture, zone and expansion (`orc_`, `draenei_`, `mogu_`, `expansion06/.../highmountain`).
- Render a thumbnail per model headlessly; AI captions them visually.
- Computed fields: texture palette (color clustering), bounding-box scale, polygon count, animations, particles, conversion difficulty.

| Field | Values |
| --- | --- |
| Usage scope | Universal · biome · culture · landmark (never reuse) |
| Placement | Ground, wall, hanging, cluster filler, centerpiece |
| Palette | e.g. warm brown/red, desaturated stone, fel green |
| Scale | Prop, building, landmark |
| Conversion | Easy (WoD), moderate, hard |

- SQLite with semantic search, exposed as an MCP tool (`search_assets`). Filter by path first (skip creatures, spell effects, UI).

**2. Design grammar**

- Mine Blizzard's placements from the map files (model, position, rotation, scale) for co-occurrence (kits such as a smithy kit), density and spacing, rotation and scale jitter, and hub/road/landmark composition.
- Add explicit principles: landmarks that pull players forward, sightlines, gradual reveals, hub spacing, vista framing.
- **Population grammar (decided):** mine NPC placement from existing dungeons, raids and zones in the world DB, not just model placement: pack sizes and composition, spacing, patrol paths, trash-to-boss pacing, and density per area type. This guides spawn layout for empty spaces such as ported Forever zones, with quests and spawns written as LLM work on top of the placement tool and classifiers.

**3. Scene taxonomy**

Labels combine four axes, so new combinations come free:

| Axis | Examples |
| --- | --- |
| Culture | Orc, Troll, Twilight's Hammer, Mogu, Pandaren, Scourge |
| Function | Camp, warband, outpost, ritual site, mine, village, migration camp |
| State | Thriving, abandoned, ruined, under siege, freshly destroyed |
| Biome | Grassland, jungle, snow, desert, Sha-corrupted |

- Each label is a recipe: required anchors, optional pieces, forbidden pieces, story signals (a troll migration camp has half-packed carts, temporary huts, livestock pens, a trail leading off), and population (creatures, patrols, idle emotes).
- Bootstrap examples by clustering existing placements into spatial groups (every camp, outpost and ruin in the game) and labeling the clusters.

**4. Models (only where volume demands)**

- **Asset tagger:** small image classifier or fine-tuned CLIP-style embeddings trained on a few thousand careful captions; tags the rest.
- **Placement plausibility scorer:** self-supervised. Blizzard placements are positives; corrupted copies (random rotation, floating, wrong biome, wrong density, culture mismatch) are negatives. Scores layouts in milliseconds.
- **Preference model:** after a few hundred ratings, predicts your verdict and becomes the main filter.
- **Generator:** after a few thousand approved layouts, conditioned on the brief, with rules as hard constraints (walkability, collision, culture).
- Not worth it: an end-to-end layout generator early on. All models fit a single consumer GPU.

**5. Rating loop**

1. Generate a patch from a brief ("128×128 yd orc camp, Nagrand grass, near a road"); the scorer filters junk.
2. Inspect in the viewer: 3D orbit, top-down, navmesh overlay, projections onto reference screenshots.
3. Rate: thumbs up/down or A-vs-B, with optional reason chips (too cluttered, too empty, wrong culture, floating, boring, love it).
4. Save brief, terrain sample, placements, generator version, seed, scorer score, rating and reasons.

- Show uncertain patches plus a few deliberately varied ones.
- Mobile swipe-rating page.
- About 50 ratings tune rule weights directly; a few hundred train the preference model; a few thousand train the generator. Approved patches go straight into the world.

**6. AI as labeler**

- Once the classifier is decent, the loop runs unattended: generator → scorer → render → AI labels and rates from screenshots → train → repeat. The classifier learns spatial statistics; the AI judges meaning (does it read as a troll migration camp, is there a focal point).
- Guardrails: a held-out calibration set rated by you measures agreement and updates the AI's written rubric; periodic spot checks and varied samples prevent self-reinforcing drift.
- Small clipping and floating are caught mechanically by navmesh and collision checks.

**End state:** brief → generated candidates → rated → placed and validated, for world quest areas, war front staging, Sha hunter ambush sites, Brawler's arenas and new zones.

**Models as MCP tools (decided).** The trained models join the placement MCP, so an agent can generate, score and revise inside one loop without a human in it.

| Tool | Model behind it | Returns |
| --- | --- | --- |
| `search_assets` | Asset tagger and embeddings | Matching assets with tags |
| `score_layout` | Placement plausibility scorer | A score plus per-object contributions (which pieces are floating, off-culture or too dense) |
| `classify_scene` | Scene taxonomy classifier | Culture, function, state and biome, with confidence |
| `predict_rating` | Preference model | Predicted thumbs up or down, with the likely reason chips |
| `score_population` | Population grammar | Pack, patrol and density checks against similar areas |

- Models run as a local GPU service behind the MCP, batch requests, and report a model version with every score.
- Per-object explanations matter more than the raw score: they tell the agent what to fix.
- Guard against the agent overfitting the scorer: final approval stays with the held-out calibration set and your own spot checks, as in the AI-as-labeler loop.
- **Fast rendering (provisional):** geometry checks (floating, clipping, reachability) are mesh queries with no render; render only for visual judgment and only for candidates near the score threshold; 256–512 px views, which vision models downscale to anyway and which cost fewer LLM tokens; a warm renderer that keeps the scene loaded and updates only changed objects; several cameras tiled into one contact sheet per call; cheap modes such as top-down orthographic, flat-shaded or category-colored renders; run on the GPU, not a software renderer.

## Human-like bots from player telemetry

Learn how you play from server-side telemetry, then compile it offline into parameters and tables the existing state machine samples. No model runs live; runtime cost is a lookup and a random number per decision.

**Telemetry**

- The server already sees every movement packet, cast, target change, emote, loot choice and chat line; log them with a server-side hook.
- An addon adds what the server cannot see (UI panels opened).
- Turn logging on early (Phase 1–2); every session before then is lost data. Distributions are usable after a few hours of play; a movement library wants tens of hours.

**Decision context (decided)**

Splitting data into general, class-specific and situation-specific behavior happens offline, at training time. Logging only has to make every split possible, because context that was not recorded can never be recovered.

- **Decision events** (cast, target change, pull, flee, mount, consumable use, loot choice) carry a context snapshot:
  - Self: class, spec, level, health, power and secondary resource (%), active buffs and debuffs, cooldowns ready, item level, mounted, swimming, flying, in combat, speed.
  - Target: creature entry or player class, level gap, classification (normal, elite, rare, boss), creature type, health %, distance, angle, casting (spell), who it is attacking.
  - Surroundings: hostiles within 5, 15 and 40 yards with their level gaps, attackers on self, enemies near death, friendly players and bots nearby, group size and roles.
  - Place and time: map, zone, subzone, instance type, indoors, server time, minutes into the session.
- **Relative values, not raw ones:** level gap rather than level, health % rather than hit points, distance and angle rather than coordinates, so one player's data carries over to other characters and zones. Raw positions stay on movement events only.
- **Stable IDs:** spell, creature entry, item entry, map and zone IDs, plus a player and session ID, so one person's style can be told apart from human behavior in general.
- **Outcome events** label the decisions: kill, death, damage taken per fight, fight duration, wipe, flee, resurrection.
- **Activity episodes:** each stretch is tagged (fight, travel, questing, gathering, vendor or bank, idle, dungeon, PvP) from server state, so data can be cut by situation without guessing afterward.
- **Schema version** on every row, so old logs stay readable as fields are added. Movement is sampled at a config rate. Chat logging has its own config key. Logs stay local.
- **Keeping test play out:** `.telemetry pause | resume | status` (macro-friendly; an addon button later), and `.telemetry discard <minutes>` drops the last stretch after the fact. Logging also pauses on its own while GM mode is on, after any GM command until the character next moves or casts, while AFK, after an idle timeout (config key), and around checkpoint loads and GM teleports. Paused stretches are logged as gaps so episodes are not stitched across them.

**Layers (offline)**

- An **ability role table** maps spell IDs to roles (gap closer, interrupt, defensive cooldown, crowd control, burst, heal, movement), so Charge, Death Grip and Blink all count as one behavior. It lives in data and can be fixed at any time without touching logs.
- Training produces layered profiles: **general** (movement, travel, idle habits, from every player), **role** (tank, healer, melee, ranged), **class and spec** (rotation, cooldown use) and **situation** (riding, swimming, PvP, dungeon, low health).
- At runtime a bot samples the most specific layer with enough data and falls back to the more general ones, so a rare class or situation still behaves plausibly.

**What to learn (texture, not decisions):** movement (curvature, strafing, jumps, look-around stops, mount timing), timing (reaction delays, uneven cast gaps, clipped globals, occasional misplays), habits (hub idling, vendor and bank trips, herb and rare detours, eating), targeting (pull order, disengage).

**Compiling into the state machine**

| Method | Output | Plugs into | Priority |
| --- | --- | --- | --- |
| Fitted distributions | A few parameters per context (e.g. delay after a proc: lognormal, mean 340 ms) | Trait struct / config | First |
| Activity transitions | Context-dependent transition table between activities | Replaces flat `RpgStatusProbWeight` in `NewRpg` statuses | First |
| Movement snippet library | A few hundred parameterized curves (curved approach, strafe-jump, stop-and-look) chosen by context and fitted to the navmesh path | Movement layer | Biggest visual win |
| Combat lookup tables | Action biases per state bucket (health, enemy count, cooldowns) | Trigger and action priority multipliers | Later |
| Policy distillation | Shallow decision tree or rule list from an offline model | Readable, editable strategy triggers | Later |

**Variety and iteration**

- Perturb compiled parameters per bot (GUID seed + personality traits) to get a population from one player's data; interpolate and exaggerate traits.
- Retrain offline (nightly if wanted), regenerate tables, hot-reload. A bad table is a config rollback.
- Every sampled behavior is loggable in plain terms, so it is debuggable.
- Turing test with the rating loop: shuffled clips of bots and the player, rated for which look human.
- The Brawler's Guild mirror match uses the player's own compiled profile.

## Provisional ideas

Suggested but not yet adopted.

**Living world**

- Offline progress: dormant bots advance along their questing routes by elapsed time when they wake, so the world never looks frozen.
- Town crier and news board reporting realm-firsts, nemesis ranks, war front results and twink sightings.
- Bot population by time of day: busy evenings, quiet 4 a.m.
- Bot mail and whispers: thank-yous after dungeons, nemesis taunts, gifts from guildies.
- Friends list: bots you grouped well with add you and say hi when they log on.
- World boss rallies: bots spread the word and form groups for Sha of Anger, Galleon and Nalak.
- Bot-to-bot relationships, so feuding roster guilds create stories without the player.
- LLM chat via a port of [mod-llm-chatter](https://github.com/Hokken/mod-llm-chatter) (WotLK 3.3.5a today), or offline-generated line libraries in `playerbots_speech`.

**Play**

- Companions: 2–3 named bots who travel with you, with loyalty, banter, small jewels and personal quests.
- Hiring and loyalty for dungeon and raid bots.
- Bounty boards to hunt your nemeses and named Sha champions.
- Home base at Sunsong Ranch with trophies, consumed legendaries on display and visiting companions.
- Secrets and mysteries: Legion-style hidden chains placed with the placement tool.
- Scenarios with bots, challenge modes against roster times, pet battle tamers, rated arena against named bot teams.
- Holiday events with bots taking part; Timewalking dungeons.
- Defecting or fighting as a mercenary on war fronts.
- Joining a roster guild with ranks, MOTD drama and calendar raids.
- A bot-driven auction house that lists what bots actually loot and craft.
- **Tower defense: Millhouse's Time Rift (idea; design notes agreed).** A fun side mode, not endgame content. Millhouse Manastorm opens a time rift, and enemies from across the timeline pour through to steal his mana cakes. You fight with your own character while building towers.
  - Fixed lanes and fixed build nodes. Each node is a wisp: talk to it to turn it into the tower of your choice for X points, and talk again to upgrade along one of 3 paths.
  - Towers are interactable stationary creatures (turret AI, like Wintergrasp's cannons); enemies follow waypoint lanes; leaks cost lives shown in world-state UI.
  - Provisional: waves themed by expansion (Scourge, demons, Mogu, Sha), co-op with friends and bot helpers, cosmetic rewards, and a siege-defense variant for the losing side of a war front.
- **Survivors-style arena:** move-only play in a vehicle seat, auto-firing weapons as periodic auras reusing existing spell visuals, XP pickups, and a 1-of-3 upgrade draft in the patched UI. Realistic scale is 150–300 enemies on screen. Weapons themed on echoes and legendaries; co-op; bot leaderboard.

**Engineering**

- Nightly soak test with `.botscenario` and the headless client, tracking p95/p99 regressions.
- Hot-reloadable data for behaviors, traits, echoes and world quest pools.
- Player save states before reincarnation or experiments.
- Session record and replay: log the packet stream and random seeds so a bug replays exactly on the test profile, for agents to debug against.
- Client extension DLL loaded by the launcher next to the exe patch: hooks client functions and adds native Lua API calls (bot relationships, jewel state, collections) instead of encoding everything over the addon channel. 5.4.8 already has a 64-bit Windows client; the quest log cap likely stays out of reach.

**Systems from game AI research**

- **Perception and salience:** each bot scans a cheap radius and scores what it notices (a node, a rare, a well-geared player, a fight, a mailbox) by traits and current goal. Anything over a threshold becomes a short side goal and bends the path, so deviations have a cause. Telemetry tunes the weights later; the reaction loop only reacts to what was noticed.
- **Crowd avoidance and standing spots:** DetourCrowd (part of Recast/Detour; may need adding if the core bundles only Detour) for local avoidance, plus free standing slots on a ring around quest givers and mailboxes, so bots stop stacking on one pixel.
- **One storyteller:** merge the Sha heat meter, event director, war front signals, rares and town crier into one pacing model with a tension curve (build-up, peak, forced relief), as in the Left 4 Dead AI Director and RimWorld's storytellers. Selectable profiles: steady, chaotic, and brutal tied to corruption.
- **Rumor propagation:** events create rumors held by bots who witnessed them; rumors spread when bots meet, with a hop count and distortion. Fame spreads geographically, and "any rumors?" gets local, slightly wrong answers.
- **Lanchester's laws for the war sim:** attrition equations as the core of the offline battle sim, with momentum, sabotage and underdog buffs as effectiveness multipliers. Remaining strength maps directly to bots spawned on live handoff.

**Keeping every expansion relevant (beyond legacy projects and war fronts)**

- **Featured expansion rotation:** each day or week one expansion gets bonus Sha energy, reputation and drop rates, so the world's focus moves around.
- **Returning invasions:** rerun removed pre-patch events (the Scourge invasion, Cata's elemental invasion, a Legion-style demon invasion of Outland) as scaled world events with bot defenders.
- **Scaled daily hubs:** Isle of Quel'Danas, the Argent Tournament, Molten Front and Tol Barad Peninsula at level 90, with their progression unlocks and rewards intact.
- **World boss rotation:** old world bosses (Doom Lord Kazzak, Doomwalker and others) scaled and Sha-touched, announced on the news board, with bots rallying.
- **Mage Tower-style class challenges:** solo spec challenges set in old raids (e.g. Karazhan, Ulduar), rewarding unique appearances and jewel morph forms.
- **Old reputations matter:** paragon-style caches for old factions, with legacy materials, mounts and appearances.
- **Archaeology and fishing:** digsites and fishing pools everywhere drop legacy project components; rerun fishing contests.
- **Cross-expansion achievement metas** with mount rewards, like Glory of the Raider for every era.
- **Hub cities:** Shattrath, Dalaran and Stormwind or Orgrimmar linked by a portal network, with the news board and recruiters in each.

**Borrowed from modern retail and WoW Forever, by difficulty**

| Difficulty | Idea | Source | What it takes here |
| --- | --- | --- | --- |
| Easy | Brawls: weekly battlegrounds with modified rules | Retail | Config and small scripts; bots fill both sides |
| Easy | Warband features: account-wide reputation, shared bank, shared progress | Retail | Server-side tables and hooks |
| Easy | Trading Post: monthly activity list and rotating cosmetic vendor | Retail | Achievement-style criteria, currency, vendor rotation; feeds Collections |
| Easy | Mount races: ring time trials at the new flying speeds | Retail | Checkpoint game objects and a timer |
| Moderate | Delves: short tiered solo instances with a leveling companion | Retail | Small scenarios plus one companion bot; uses the placement tool and corruption dial |
| Moderate | Island Expeditions: race an AI team for resources | Retail | A scripted island with playerbots as the rival team |
| Moderate | Warfront build-up: gather resources together to unlock the assault | Retail | Opening phase for the existing war fronts |
| Moderate | Remix seasons: fast seasonal characters whose rewards return to your main | Retail | Mostly reuses reincarnation and echoes |
| Moderate | Familiar zones with new stories (e.g. Sha-corrupted versions phased in as the war intensifies) | Forever | Phasing plus new quests |
| Moderate | New legendary questlines in old content | Forever | Fits the legacy legendary chains |
| Hard | Horrific Visions: corrupted capitals with a draining Sha-influence meter | Retail | Phased or instanced capital copies, sanity mechanic, run rewards for the jewel |
| Hard | Roguelite tower runs with temporary powers (Torghast-style) | Retail | Floor layouts, power drafts; doubles as an echo test bed |
| Hard | New dungeons in old-world places that never had one (Forever's examples: Hall of Thanes, Ruins of Lordaeron, Drowned City) | Forever | World design pipeline, new geometry, navmesh |
| Hard | New battleground on existing land (Forever's Darkspear Island, 15v15) | Forever | New map work; bots fill teams |
| Hard | Porting Forever's maps once the community extracts them | Forever | Same converter as modern assets (Forever uses modern file formats); quests and spawns must be rebuilt, since that server data is not in the client |

**Making the world react to you**

- **The jewel talks.** Give the jewel its own consciousness file: it comments on your choices, reacts to what you feed it, grumbles when you skip story beats and remembers past lives. Reuses the LLM bot system.
- **A rival.** One named bot who levels alongside you (a jewel-carrier on the other faction, or the hero the jewel nearly chose). Races you to realm-firsts, turns up in your dungeons, taunts you after war fronts; has a consciousness file.
- **Fame and infamy.** A server-wide reputation built from your deeds via the news board. Bots recognize you, bards sing about you in inns; ganking lowbies brings posses and rude guards.
- **Bots who need you.** Crafting orders from bots (professions matter), LFM calls in chat you can answer, guild officers asking for attunement help, lowbies asking for runs.
- **Trade chat culture.** Bots selling, recruiting, arguing about specs and complaining about lost war fronts, from line libraries plus flavor-level LLM lines.
- **Pranks both ways.** Players can prank bots back (dance on a mailbox camper, sap the sapper); bots remember and retaliate later.
- **Ghosts of past lives.** On reincarnation, your old character becomes a bot that replays your recorded telemetry: your routes, timing and habits. You might find your previous self questing in Kun-Lai, playing the way you did. Reuses Phase 13 telemetry and the reincarnation snapshot.
- **Bots that read the news board.** Bots who hear of your deeds come to you: asking to group because you cleared something, challenging you because you beat a guildmate, or steering clear because of your infamy. Fame becomes something that happens to you, not just a number.

**Expansions of existing systems**

- **War front generals:** a conscious bot general per side picks objectives, shifts troops and gives a speech before battle; the offline sim gets a personality.
- **Sha hunters that learn:** named champions read your telemetry and their own files and adapt (e.g. carry stealth detection after losing to your opener).
- **Brawler's announcer:** an LLM ringside announcer who knows your history.
- **Eavesdrop toy:** an in-game way to read the reasons bots log ("Left the party because the warrior kept pulling"); a debug view that doubles as a toy.

## Tooling

Developer tools and MCP servers that make the rest cheaper to build (decided). Each is a standalone, reusable module per the modularity pillar, so it can serve other projects.

| Priority | Tool | What it does | Cost | Needed from |
| --- | --- | --- | --- | --- |
| 1 | Live server MCP | Thin wrapper over SOAP and logs: bots near a position, a bot's state and recent reasons, run `.botscenario`, read `.server info`, tail logs, run GM commands. Agents debug against a live server from day one | Cheap | v0.1 |
| 2 | Crash triage | Read a crash dump or core log, map it to source, suggest the likely commit | Cheap | v0.1 (two agents committing) |
| 3 | SQL validator | Lint new SQL against the schema and references (quest givers exist, loot items exist, IDs don't collide) before it reaches a database | Cheap | v0.1 (jewel intro, quick wins) |
| 4 | Balance sim | A `.botscenario` dummy room: a bot or headless player with a given loadout reports DPS against target dummies | Cheap | v0.1 (jewel tuning) |
| 5 | Content search | Semantic index over spells, spell visuals, items, creatures and display IDs; built on a shared DBC reader library | Moderate | v0.1 (jewel visuals) |
| 6 | Quest and content author | Spec in, validated SQL out (uses the SQL validator); givers placed with the placement tool; text checked against a lore bible | Moderate | v0.2 (rares, events) |
| 7 | DBC and patch tool | Adds DBC/DB2 writing and MPQ building to the shared DBC reader; diffs client data against server data | Moderate | v0.3 |

The placement tool, model MCP tools and headless client (described above) follow the same module rules.

## Modeling and texturing pipeline (long-term)

Create new WoW-themed assets as needed (e.g. the Kolkar war-kodo), mostly by modifying existing assets rather than modeling from scratch (decided as a long-term goal). Blender is driven through MCP: the Blender developers ship an official Blender Lab MCP server (Blender 5.1+), and a community server also exists.

**Pipeline:** base asset from the catalogue → import to Blender (wow.export's glTF export and Blender add-on) → modify through Blender MCP → texture → export M2 (community M2 tools target 3.3.5; convert to MoP's M2 version with the porting converter) → BLP textures → DBC rows and patch → preview in the viewer and in game → rating loop → ship.

| Tier | What | Difficulty | Why |
| --- | --- | --- | --- |
| 1 | Texture variants on existing models (an armored war-kodo skin) | Easy | No mesh export: a new display entry points at the same model with new textures, which WoW supports natively |
| 2 | Kitbash on an existing skeleton (armor plates, banners, saddlebags attached to bones) | Moderate | Keeps the skeleton and every animation |
| 3 | New low-poly static props (crates, totems, tents, banners) | Moderate | Small poly budgets suit LLM-scripted Blender modeling; checked by the style classifier and your ratings |
| 4 | Generated meshes from image-to-3D models, retopologized and repainted to WoW budgets | Moderate to hard | Useful for props; quality varies |
| 5 | New creatures with new skeletons and animations | Hard | Rigging and animation are the expensive part |

**Texturing (provisional):** recolor and repaint existing BLPs first; later, a local image model for hand-painted-style textures conditioned on the model's UV layout and the asset catalogue's palettes. BLP conversion is scripted.

**Reuse:** the asset tagger, style classifier, rating loop and viewer from the world design pipeline judge new assets the same way they judge layouts. New assets join the asset catalogue, so the layout generator can place them.

## Testing and iteration speed

Fast restarts, instant test state and tests that need no server, built first under the priority rule (decided).

- **Eluna for prototyping.** The core already compiles the Eluna Lua engine (`src/server/game/LuaEngine/`), off by default (`Eluna.Enabled = 0`). Lua scripts reload while the server runs, so rares, events, gossip and jewel dialogue get prototyped in Lua and ported to C++ only when they need speed or deeper core access. There is no C++ script hot-reload in this core.
- **Test profile config:** bots off or minimal, smaller logs, grid preloading off, short timers.
- **Startup profiling first:** rank the server's own loader timings from the log before optimizing anything.
- **Spawn filter (if profiling shows spawns matter):** a test-mode key that loads creature and object spawns only for listed maps.
- **Pre-generated bots:** create the bot characters once, snapshot the characters and playerbots databases, restore instead of regenerating.
- **Fixture characters:** SQL presets at levels 1, 30, 60, 85 and 90 with fitting gear.
- **Fast resets:** restore database snapshots by file copy rather than replaying dumps.
- **State commands:** GM commands for jewel rank, corruption, pet state, relationship scores and war front state.
- **Every timer is a config key (rule):** war front cycles, rare respawns, raid resets, event cooldowns, Sha heat. The test profile shortens them all.
- **Bot load profiles:** none (0), local (20–50 in one zone), scale (500–2,000, performance work only).
- **Unit tests for pure logic:** a test framework in CMake; modules like corruption averaging, jewel math, the consciousness scheduler, affix rolls and director placement rules are tested without a server.
- **Regression suite:** scripted headless-client scenarios with assertions, run against the test profile before every PR.
- **Builds:** worldserver target only, Ninja, precompiled headers (already on), sccache shared across worktrees; sanitizer builds (`WITH_SANITIZER`) nightly only.

**More iteration speed (decided)**

- **Hot reload for every custom table:** `.reload` commands for techniques, plays, traits, behaviors, echoes, affixes, rare modifiers, storyteller and director parameters, world quest pools. Tuning never needs a restart.
- **Parallel servers per worktree:** each agent worktree gets its own worldserver, ports and database snapshot, so several agents build and test at once without stepping on each other.
- **Faster links:** lld or mold for worldserver, alongside Ninja, precompiled headers and sccache.
- **Settable global RNG seed** for tests, so bot decisions, rolls and events replay identically; pairs with session record and replay.
- **Client changes without client restarts:** prefer addons (`/reload`) and item hotfixes over FrameXML and DBC edits in the patch MPQ, which need a restart. Patch the client only for what can't be done otherwise.
- **Headless client fleet:** several headless clients run scenario tests in parallel.

**GM tools (decided).** An addon GM panel with buttons over the same commands, plus commands an agent can call through the live server MCP.

- **Time:** speed up or jump the server clock to test war front cycles, respawns, raid resets, day and night, and director cooldowns.
- **Force any system:** spawn a rare with chosen modifiers, start an event at a site, trigger a Sha ambush at a given heat, start or resolve a war front, post a call on the call board, run a play or technique, promote a bot to a consciousness level, make a bot defect, spawn a twink, open a reincarnation draft with chosen echoes, grant echoes and legacy powers.
- **Inspect bots:** `.bot why` (the last decision with each candidate's utility score), `.bot traits`, `.bot memory`, `.bot skill`, `.nemesis list`; set traits, skill and relationship scores directly.
- **In-world debug overlays:** temporary markers or spell visuals that draw navmesh paths, edge, pillar and chokepoint annotations, standing spots, salience targets and call board calls in the game itself.
- **Scenario loader:** `.scenario load <name>` sets up a full situation in one command (e.g. Warsong Gulch, your flag taken, bots at chosen skill), built on `.botscenario`.
- **Spectate:** follow any bot invisibly to watch its behavior.

**Save safety (decided).** Characters, bot memories and nemesis histories become irreplaceable, so:

- **Nightly snapshots** of all databases, using the file-copy restore, with rotation.
- **Versioned migrations:** every custom table change ships as a numbered migration, tested against a copy of the real save before it runs, so a year-two refactor can't break a year-one character.

**Capture while playing (decided)**

- **`/idea` and `/bug`:** save a note with a snapshot: position, target, nearby bots and their last decisions, active systems, a screenshot. Each becomes a ticket agents can pick up, so play sessions feed the next build.
- **Milestone dashboard:** time spent per system, what gets skipped, where sessions end, from the telemetry already collected, to answer each milestone's test with data.

## Roadmap and task list

Order follows dependencies, with one exception: the living jewel comes right after the foundation, because it needs no bot work and is the most fun to playtest. Bot performance runs alongside it. Then tooling, the features that most change how the world feels, and the big content systems.

**Priority rule (decided):** anything cheap that disproportionately speeds up the early phases, or the phases right after them, gets built early, whatever phase it nominally belongs to. This applies to testing infrastructure and tools first.

**Build for the end state (decided):** systems are built in their final shape, even when that leaves them incomplete until later phases land. A locally sufficient version that the long-term plan would break is not built, unless replacing it later is an easy fix (a config value, one lookup). Interim imbalance is accepted.

**Accepted interim states**

- **Heartseed pacing:** Growth comes slowly until rares and essences (4d) and Sha hunters (7) add their sources. Jewel pacing is tuned in v0.5, once every source exists; v0.1 only checks that it feels good, not that the numbers are final.
- **One director:** the first director built (emergent events, 4e) is built as the storyteller, so Sha heat, war front signals, rares and the town crier plug into it rather than being merged later.
- **Outleveled dungeon queue:** solo teleport only until the instance pool (5) provides bot groups; measured against the dungeon's original level until the scaling table (6) exists. Both are easy fixes.
- **Transmog:** the quick-win mixed-type options work on owned items until collection-based transmog (1b).
- **Retinue brackets:** `ZoneBracket` until the scaling table replaces it (one lookup).
- **Rumors:** dialogue and the news board read rumors through one interface from Phase 3, so propagation drops in later.
- **Shared modules built once:** the affix module (4d) serves rare modifiers, the corruption dial, Brawler mutators, wild gems and consumable modifiers; the call board (5b) serves battlegrounds, Sha ambushes, war fronts and dungeons; LLM provider interfaces (Phase A) come with personalities (3).
- **Achievements, journal entries and chronicle events ship with each system,** like tutorials, once the framework exists (1b).

**Milestones.** Each milestone is a playable build with one test it has to pass. v0.1 is decided; the later groupings are provisional.

| Milestone | Phases | Test |
| --- | --- | --- |
| v0.1 The jewel run | 0, 0a, 0b (fast iteration tools, GM tools, save safety, timeline decision, jewel MVP, intro, all quick wins: travel, professions, binding, respecs, loadouts, gear pages, solo quality of life) | Is leveling a fresh character more fun than stock MoP? |
| v0.2 A living world | 1, 3, 4, 4d, 5, plus the minimal part of 2 (bot performance, hub density cap, personalities and dialogue with LLM Phase A providers, zone retinues, rares and the affix module, bot pools, instance pool, roster, your own guild; slice exporter and validation) | Do zones feel populated, and do you remember individual bots? |
| v0.3 Collections | 1b (client patch, launcher, collections, chronicle, settings panel, journal, achievement framework, enhanced graphics, wave 1 ports) | Do friends get the patch with one click, and is collecting fun? |
| v0.4 Any expansion | 6, 6b (scaling, open leveling, all raids at one item level, Zul'Gurub raid, battle pet passive) | Does leveling in Outland or Northrend at 75 feel as good as Pandaria? |
| v0.5 War | 5b, 4e, 7, 8 (call board, skill, techniques and plays; the storyteller with emergent events; Sha hunters and the True Horde; war fronts; Heartseed pacing tuned) | Do you plan your evening around a front or an ambush? |
| v1.0 Endless | 9, 10, 11, 11b (reincarnation and mortal life, legacy and consumables, Brawler's ladder, side fun) | Do you want to reincarnate a maxed character? |
| Beyond | rest of 2, 4b, 4c, 8b, 12–17 (viewer and screenshot tools, world quests, scenarios, ports, world design, human-like and LLM-driven bots, modeling, the Sha of Grief) | Long-term projects; ship whenever each is ready |

### Phase 0: Foundation

- [ ] Fork LoA-Pandaria-Playerbots; build; extract maps, vmaps and mmaps from the 5.4.8 client
- [ ] Import the playerbots DB by hand (not covered by the SQL updater yet)
- [ ] Enable SOAP; set up the headless client and `.botscenario` as a test harness
- [ ] Baseline `.server info` (median, p95, p99) at 500, 1000 and 2000 bots
- [ ] Turn on player telemetry logging (feeds Phase 13 later)
- [ ] Quick win: double all mount speeds (120 / 200 / 300 / 560 / 620%)
- [ ] Quick win: Apprentice Riding and a starter mount at character creation
- [ ] Quick win: rare mount drop chances raised to 10% in loot tables
- [ ] Quick win: raid resets every 2 days instead of weekly, keeping in-progress instances
- [ ] Quick win: all professions (`MaxPrimaryTradeSkill`) plus an addon button bar for professions past the second
- [ ] Quick win (test first): multi-type tracking via combined tracking bitmasks
- [ ] Quick win: no item binding (config-keyed hook, soulbound-flag cleanup, tooltip hotfix), keeping quest and progression items bound
- [ ] Quick win: dungeon finder for outleveled dungeons (solo teleport at 20+ levels over the scaling cap, bot group otherwise)
- [ ] Quick win: free talent and glyph changes, spec switching from level 10
- [ ] Server-side spec loadouts (spec, talents, glyphs, action bars, Heartseed paths) with an addon picker
- [ ] Gear pages per loadout: detached storage, page swap through the normal equip path, addon page panel
- [ ] Quick wins (config): cross-faction play, all flight paths, guaranteed skill-ups, no weekly currency caps, lighter death penalties
- [ ] Quick wins (scripts and data): guild perks for everyone, unrestricted transmog, bigger stacks, free character services NPC, shorter hearthstone cooldown
- [ ] Starting 10-slot bags in all four bag slots; 36-slot bags as an expensive vendor gold sink plus drops planted in rares and other content
- [ ] Reagent bank in detached storage, with crafting drawing from it
- [ ] Quick win: free XP toggle in the settings panel; no separate battleground queue for XP-off characters
- [ ] Priority quick win: twink gear buffs above heirlooms via item hotfixes, per bracket and breakpoint

### Phase 0a: Fast iteration (priority rule)

- [ ] Test profile config and bot load profiles
- [ ] Startup profiling script; spawn filter only if spawns dominate
- [ ] Fixture characters, pre-generated bot snapshot, file-copy database restore
- [ ] Enable Eluna in dev profiles with a `lua_scripts` layout for prototypes
- [ ] Unit test framework in CMake
- [ ] Headless-client regression suite runner
- [ ] Live server MCP, crash triage and SQL validator (from the tooling track)
- [ ] Nightly database snapshots and versioned migrations for custom tables
- [ ] Hot reload for custom tables; parallel worldserver per worktree; lld or mold; settable RNG seed
- [ ] GM panel addon and commands: time control, force-any-system, bot inspection, debug overlays, scenario loader, spectate
- [ ] `/idea` and `/bug` capture with state snapshot and screenshot; milestone dashboard from telemetry
- [ ] Decide the server timeline; write the core loops per time scale

### Phase 0b: The Heartseed (first playable, top priority)

- [ ] Top priority. Reserve Heartseed ID ranges (`docs/ID_RANGES.md`)
- [ ] Heartseed state table (rank, Growth, path loadouts per spec, essences, story flags); equip script with hidden enchant and stage glow
- [ ] Trait table as data rows; core weapon-scaled passive; first major choice at 10
- [ ] Growth from feeding replaced weapons; two essence types; rank cap by level
- [ ] Granting quest at level 1 as the canonical marker (gossip prototyped in Lua)
- [ ] Story flags set by simple hooks (first dungeon, reached Pandaria, first world boss)
- [ ] GM commands: rank, Growth, story flags
- [ ] Later: full Theramore intro scenario completing the same quest; remaining majors and essence slots; Seedsense; attunement and appearances; morph chain; raid-weapon fuel
- [ ] Provisional: intro variants for Pandaren and death knights; a skip for alts and reincarnation

### Phase 1: Bot performance

- [ ] Config pass: `CharacterDatabase.WorkerThreads`, `MapUpdate.Threads`, MySQL buffer pool, in-world times
- [ ] Bot-only login query holder (skip unneeded queries)
- [ ] Short-circuit packet building for bot-only recipients
- [ ] Dormant/wake state (off-map hibernation, wake on zone or neighbor entry)
- [ ] Re-baseline and record gains
- [ ] Hub density cap on awake bots in view

### Phase 1b: Client patch, collections and easy ports

- [ ] Find the 5.4.8 interface signature check and add it to the bundled patcher (x86, x64, Mac)
- [ ] Patch MPQ build pipeline and launcher (applies the exe patch and the MPQ)
- [ ] Collections window from the Pet Journal: Toys, Heirlooms, Appearances tabs; modern keybinds
- [ ] Server collection tables, unlock hooks, toy use, heirloom creation, collection-based transmog
- [ ] Porting pipeline: retail IDs to M2, BLP, DBC rows, SQL and hotfixes
- [ ] Wave 1 ports: WoD mounts, heirlooms, transform toys, pets on existing ability sets, WoD props and buildings
- [ ] Settings panel and progression journal addons
- [ ] Custom achievements and titles in client data
- [ ] Chronicle panel with event groups, unread counts and true interrupts
- [ ] Audio cues from existing sound kits
- [ ] Patched character sheet with the new stats; item tooltip display test
- [ ] Twinks tab in Collections: up to three named templates per bracket per class, empty ones hidden by default, linked to spec loadouts, loaded as gear pages with account-bound copies
- [ ] Launcher "enhanced graphics" toggle: DXVK, tuned ReShade preset, optional HD texture pack

### Phase 2: World placement tool

- [ ] Slice exporter (mmtile, terrain, collision, DB spawns → mesh + JSON)
- [ ] Validation GM command over SOAP
- [ ] three.js viewer with minimap draping, spawn editing, SQL export
- [ ] Headless rendering for AI self-review
- [ ] Screenshot addon, FOV calibration, pixel ↔ world mapping
- [ ] Wrap as an MCP server

### Phase 3: Personalities

- [ ] Trait generation from GUID; per-bot RPG weight multipliers
- [ ] Mood variables and relationship table
- [ ] Reaction loop (buff, invite, share quest, follow, attack)
- [ ] Data-driven macro behavior engine; sap rogue as the first behavior
- [ ] Behavior library, cooldowns and per-zone caps
- [ ] Bot dialogue choices (rumors, jokes, group, relationship options) via whisper menu first, then the patched "Talk" frame
- [ ] Large line libraries tagged by trait and mood (provisional: generated offline by an LLM)

### Phase 4: Zone retinues

- [ ] Per-zone resident pools using `ZoneBracket`
- [ ] Questing routes per Pandaria zone (built with the placement tool)
- [ ] Snapshot and restore reset loop
- [ ] Phasing alignment for Jade Forest and Krasarang
- [ ] World PvP patrols in contested zones

### Phase 4b: World quests

- [ ] Rotation scheduler, timers and spawn sets per world quest
- [ ] Area auto-accept and auto-remove with server-side progress
- [ ] Faked progress-bar objectives and emissary quests
- [ ] Addon: map pins, tracker, emissary board
- [ ] Bot world-quest RPG status
- [ ] First 15–25 world quests per zone from templates
- [ ] Bonus objectives in leveling zones on the auto-accept system

### Phase 4c: Story scenarios

- [ ] Scenario template from the existing scripted scenarios, plus client scenario data in the patch MPQ
- [ ] First scenarios (e.g. the Wrathgate) built with the placement tool
- [ ] Reward model (provisional: cosmetics, legacy components, jewel memories)

### Phase 4d: Rares and discovery

- [ ] Rare framework: level-scaled XP, signature collectible with bad-luck protection, jewel essences, vignette stars
- [ ] Bots calling out rares in general chat
- [ ] Sealed secrets workflow: briefs in, agent implementation, automated validation, sealed log and in-game hint ladder
- [ ] Rare spawns across zones, placed with the placement tool and population grammar
- [ ] Provisional: shared affix module and rare modifiers; legendary rares with borrowed boss abilities
- [ ] Provisional: treasures, traveling merchant, weather, shrines, gathering variants

### Phase 4e: Emergent events (early version)

- [ ] Event framework: arc stages, world-state objectives, boss, resolution and occupation fallback
- [ ] Event director shared with Sha heat: placement rules, scaling, cooldowns, bot participation
- [ ] Hand-placed sites and hand-built camp layouts for a few zones
- [ ] Rewards: signature drop, event currency vendor, XP and reputation by contribution
- [ ] Later (Phase 12): generated sites and layouts from scene recipes and culture kits

### Phase 5: Pools and lifecycle

- [ ] Instance pool per bracket with role ratios, dormant until queue pop
- [ ] BG pool per bracket, AV cap
- [ ] Twink sub-pool, gear profiles, appearance rolls and caps
- [ ] Graduation, leveling pool, level reset wiring (`ResetBotLevel`, `LevelBrackets`)
- [ ] Rename or recycle-to-new-GUID on pool moves
- [ ] Permanent level-90 roster with guilds
- [ ] Bot auction house with supply-and-demand pricing from bot inventories
- [ ] Player-led guild: founding, fame-based bot applications, loyalty
- [ ] Guild mission table: offline missions, raid farm and gathering requests with per-reset and weekly limits

### Phase 5b: PvP coordination and plays

- [ ] Per-team call board, battleground chat lines, player chat parsing
- [ ] Per-bot skill value and its effects (awareness, judgment, call accuracy, execution)
- [ ] Learned trust per caller from relationships and memories
- [ ] Technique library as data rows; first techniques per class
- [ ] Edge, pillar and chokepoint annotations for battlegrounds; knockback landing prediction
- [ ] Profession and item kits; gadget techniques
- [ ] Class plays: Warsong Gulch and Arathi Basin first, then Alterac Valley
- [ ] Counterplay memory against recurring plays

### Phase 6: Scaling, open leveling and legacy raids

- [ ] Zone scaling table (zone ID, minimum, cap) and creature scaling up to each zone's cap
- [ ] Open leveling 60–89 through any expansion (provisional: full cap ladder per expansion)
- [ ] Bot retinue brackets read from the scaling table
- [ ] Raid scaling to a common level for all 31 raids: creature level, health, melee and spell damage
- [ ] Item scaling script and item hotfixes for old raid and quest loot
- [ ] Provisional: corruption dial sets item level; Cho'gall and Kil'jaeden script gaps
- [ ] Zul'Gurub raid rebuild: check for the original map files, port the original boss scripts, restore the old mounts
- [ ] Talent rebalance: balance sim matrix per spec and row, buffs to weaker choices, spell data in the patch for tooltips
- [ ] Modern stats: versatility, speed, leech, avoidance as auras with rating tables; random upgrade rolls on all gear at every level, rare on bot twinks; tertiary and socket upgrade track
- [ ] Provisional: bot raid strategies, starting with Terrace of Endless Spring
- [ ] Group corruption rule (zero opts out, otherwise floor of the average)

### Phase 6b: Battle pet passive (provisional)

- [ ] Family buffs scaled by pet level and quality
- [ ] Battle ability to real spell translation table
- [ ] Companion AI that casts procs from the pet's position

### Phase 7: Sha hunters

- [ ] Sha faction, visuals and hunter strategies
- [ ] Director with heat meter and escalation
- [ ] Essence drops and theft; nemesis ranks
- [ ] Shado-Pan toggle NPC and tuning to the 82–85 target
- [ ] Check that jewel power in low-level BGs stays level-sized (no normalization planned)

### Phase 8: War fronts

- [ ] Navmesh tile variants per town state; poly flags for gates; steering fallback
- [ ] Outdoor PvP script for Krasarang as the first front
- [ ] Scheduling, announcements, bot build-up and army fill
- [ ] Town state phasing, vendors and rewards
- [ ] Front assets through the patch pipeline (launcher built in Phase 1b)
- [ ] WoD asset conversion for buildings and props, then Hillsbrad
- [ ] Real-time schedule: each front about every 3 hours, with a roughly 1-hour dice-roll resolution when no player is present
- [ ] Turn on bot battlefield joining for Wintergrasp; test and enable Tol Barad
- [ ] Hillsbrad front (decided); provisional: Halaa, Krasarang, Ashenvale or Stonetalon
- [ ] Provisional: tick-based sim, live handoff when a player arrives, downtime catch-up, hold thresholds

### Phase 8b: Moderate ports

- [ ] Legion chunked M2 and .skel/.anim converter
- [ ] Wave 2 ports: Legion and BfA mounts and pets, static props from any era, helms, shoulders and weapons
- [ ] Capstone set bonus data by script (items, project quests, tooltips)

### Phase 9: Reincarnation

- [ ] Sha energy collection from endgame activities
- [ ] Reincarnation ritual: full rebirth and re-awakening, gear vault
- [ ] Echo data model, draft UI (addon) and slot progression
- [ ] Ability mutations via spell overrides and scripts
- [ ] Cross-class translation table and per-spell definitions
- [ ] Sha Corruption difficulty dial: scaling aura and affixes
- [ ] Power model from the balance sim; dial steps, rewards and suggested level
- [ ] Keystone runs: challenge mode timers with the corruption dial and a rising or falling keystone
- [ ] Second-talent-per-tier unlocks across lives
- [ ] Bot echo loadout generation, usage hints and generic strategy
- [ ] Prestige glow, titles and inspect display
- [ ] Morph variants per life
- [ ] Mortal life toggle: forced reincarnation on death, extra or rarer echo choices

### Phase 10: Legacy

- [ ] Project system as quests with quest-only guaranteed loot; lockout bypass
- [ ] Lorewalker Cho project NPC and addon project tracker
- [ ] Dormant item crafting and boss-kill infusion hook
- [ ] Jewel consumption: permanent level-scaled powers and appearance unlocks
- [ ] Legacy set bonuses: audit old spells in 5.4.8 data, translation scripts, diminishing returns
- [ ] Legendary projects for existing legendaries
- [ ] Custom Frostmourne and Ashbringer chains
- [ ] Bot legacy glows and loadouts
- [ ] Project tiers and unlock counts (Tier I, Tier II, capstone)
- [ ] Capstone set bonus spell scripts, translated per host class
- [ ] Consumable modification: base consumables, era reagent modifiers, secret combinations
- [ ] Classic-style world buffs at 90, including bot head turn-ins
- [ ] Class campaigns and class halls tied to each class's Heartseed morph

### Phase 11: Brawler's Guild Stupid Ladder

- [ ] Audit the existing Brawler's Guild scripts
- [ ] Endless rank system tied to the corruption dial
- [ ] Mutator library
- [ ] Remixed old boss encounters
- [ ] Mirror match from player snapshot
- [ ] Gauntlet nights and bot leaderboard
- [ ] Rewards

### Phase 11b: Side fun

- [ ] Always-on Darkmoon Faire, bot leaderboards, betting booth, Deathmatch free-for-all
- [ ] Wild gems on the shared affix module
- [ ] Fishing tournaments with bots, aquarium collection, fighting fish, treasure maps
- [ ] Bot city raids driven by war front momentum
- [ ] Archaeology: faster surveys, bad-luck protection, guardians, Heartseed memories, artifact collection
- [ ] Wintergrasp and Tol Barad: vehicle progression, prison break, Wintergrasp at 90
- [ ] Character challenges at creation; Gurubashi Arena chest; prop hunt; pet battle PvP against bots
- [ ] Seasonal remix mode
- [ ] Solo Shuffle against bots; pet battle dungeons
- [ ] Plunderstorm with bots

### Phase 11c: Sunsong housing and player types

- [ ] Plot expansions, per-account placement table, phase spawning
- [ ] Decor library addon with 3D previews; decor sources and Collections entries
- [ ] Ground-targeted click placement, nudge controls, server-side snapping, validation rules, object budget
- [ ] Roleplay profiles and bot responses to in-character emotes
- [ ] Player dummy room and build planner
- [ ] Auction house market events and price history

### Phase 12: World design pipeline

- [ ] CASC extraction and path-based filtering for WoD and Legion
- [ ] Headless thumbnails, computed fields, AI captioning
- [ ] Asset catalogue DB and `search_assets` MCP tool
- [ ] Mine Blizzard placements: kits, density, jitter, composition rules
- [ ] Scene taxonomy and recipes; cluster and label existing camps, outposts and ruins
- [ ] Rule-based patch generator from briefs
- [ ] Placement plausibility scorer (self-supervised)
- [ ] Rating loop: viewer review, A-vs-B, reason chips, dataset schema
- [ ] Mobile swipe-rating page
- [ ] Asset tagger once manual captioning bottlenecks
- [ ] Calibration set and AI-as-labeler loop
- [ ] Preference model, then conditioned generator

### Phase 13: Human-like bots

- [ ] Server-side telemetry hook and storage (enable early, alongside Phase 1–2)
- [ ] Telemetry addon for client-only signals
- [ ] Fitted timing and habit distributions per context
- [ ] Activity transition table replacing flat RPG weights
- [ ] Movement snippet extraction, clustering and path fitting
- [ ] Per-bot perturbation from GUID seed and traits
- [ ] Offline retrain and hot-reload pipeline
- [ ] Clip-based Turing test in the rating loop
- [ ] Combat lookup tables and policy distillation

### Phase 14: Hard ports

- [ ] Toys with visual effects, rebuilt in MoP's spell visual format
- [ ] Animated Legion+ creatures
- [ ] Body armor appearances for later sets (needs a fix for newer texture layouts)

### Phase 15: LLM-driven bots (long-term)

- [ ] Pick and benchmark a quantized local model on the target hardware
- [ ] Consciousness levels 0–3 and the per-zone budget, highest level first
- [ ] Structured intent output mapped to existing strategies
- [ ] Per (bot, player) memory summaries in the playerbots DB
- [ ] Asynchronous replies with fallback to line libraries
- [ ] Consciousness file schema (identity, state, relationships, episodic memories) created on first promotion
- [ ] Attention scheduler with promotion and demotion triggers, on a batched local server
- [ ] Idle reflection and memory retrieval with a small embedding model
- [ ] Benchmark Qwen3.5-4B, Qwen3.5-9B, Gemma 4 12B and Nemotron 3.5 Bolt-E4B for persona consistency on a 4060 and a 4080
- [ ] Phase A (with Phase 3): provider interfaces per call site and scripted providers for every LLM feature
- [ ] Circuit breaker and per-level provider config; test profile pinned to scripted
- [ ] Grammar- or schema-constrained decoding for intents; voice examples from the bot's own library

### Phase 16: Modeling and texturing (long-term)

- [ ] Blender MCP setup and the round trip: catalogue asset to Blender to MoP M2 to patch
- [ ] Tier 1: texture variants as new display entries (first target: an event boss mount)
- [ ] Tier 2: kitbash attachments on existing skeletons
- [ ] Tier 3: LLM-scripted low-poly props, judged by the style classifier and rating loop
- [ ] Later: image-to-3D props, a hand-painted texture model, new creatures

### Phase 17: The Sha of Grief (long-term)

- [ ] Nemesis ranking by interactions and power; selection of the Sha of Grief
- [ ] Son selection from the kill log and bot memories, with the reveal
- [ ] Sha lord assignment by personality for the other nemeses
- [ ] Authored raid encounter that takes the nemesis's name, appearance and voice
- [ ] Sha-corrupted phases of significant places, built with the world design pipeline
- [ ] Questline from the deserters' relic raids to the finale, with both endings

### Tooling track (runs alongside the phases)

- [ ] Live server MCP over SOAP and logs (v0.1)
- [ ] Crash triage (v0.1)
- [ ] SQL validator (v0.1)
- [ ] Balance sim dummy room (v0.1)
- [ ] Shared DBC reader library, then content search on top of it (v0.1)
- [ ] Quest and content author (v0.2)
- [ ] DBC writing and MPQ building (v0.3)

### Stretch

- [ ] Bot AI decisions on a worker pool
- [ ] `DetourTileCache` for player-built structures
- [ ] Player invasions as Sha hunters

## Open questions

- [x] Jewel trees: decided, one Heartseed with a saved path loadout per spec
- [ ] Endgame artifact model: raid weapons as fuel (preferred), appearance carrier, or per-tier re-awakening?
- [ ] Instance pool rotation: rename on move, or recycle into new characters?
- [ ] Twink appearance rates and per-match caps
- [x] Jewel and twinks in low-level BGs: decided, no normalization and no separate bracket. Jewel rank is capped by character level and its traits scale with weapon item level, so a twink's jewel is already sized to its level.
- [x] War front cadence: decided, real time about every 3 hours. Still open: the Kalimdor, Outland and Pandaria fronts
- [ ] Client patch distribution: custom launcher or manual MPQ?
- [ ] Hardware target for the bot count goal
- [ ] Intro: which Horde figure reacts to the jewel (Baine or Garrosh)?
- [ ] Should war front results feed a shared cross-continent war score?
- [ ] Reincarnation: what happens to reputations and gear by default?
- [ ] Echo slot growth rate and the size of the starting echo pool
- [x] Old raid lockouts: decided, raids reset every 2 days with in-progress instances kept
- [ ] Legacy stacking: diminishing returns curve per tier band

## Sources

- [LoA-Pandaria-Playerbots](https://github.com/1LineAtaTime/LoA-Pandaria-Playerbots) (code reviewed 2026-10-30)
- [WoW Server MCP](https://glama.ai/mcp/servers/rga4yror3i)
