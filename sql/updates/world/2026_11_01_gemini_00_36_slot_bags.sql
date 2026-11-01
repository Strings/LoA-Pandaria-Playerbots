-- T-061: Custom 36-slot bags and vendor placement in Stormwind and Orgrimmar
-- Item range: 192000-192999 (docs/ID_RANGES.md)
-- Vendors: Alyssa Griffith (1321) in Stormwind, Gotri (3369) in Orgrimmar

START TRANSACTION;

DELETE FROM item_template WHERE entry IN (192000, 192001);
INSERT INTO item_template (
    entry, class, subclass, SoundOverrideSubclass, name, displayid, Quality,
    Flags, FlagsExtra, BuyCount, BuyPrice, SellPrice, InventoryType,
    AllowableClass, AllowableRace, ItemLevel, RequiredLevel, stackable,
    maxcount, ContainerSlots, bonding, Material, BagFamily, VerifiedBuild
) VALUES
(192000, 1, 0, -1, 'Bottomless Bag', 108865, 4, 0, 8192, 1, 50000000, 12500000, 18, -1, -1, 90, 0, 1, 0, 36, 0, 7, 0, 18414),
(192001, 1, 0, -1, 'Dimensional Satchel', 64151, 4, 0, 8192, 1, 50000000, 12500000, 18, -1, -1, 90, 0, 1, 0, 36, 0, 7, 0, 18414);

DELETE FROM hotfix_data WHERE type IN (1344507586, 2442913102) AND entry IN (192000, 192001);
INSERT INTO hotfix_data (entry, type, hotfixDate) VALUES
(192000, 1344507586, '2026-11-01 12:00:00'),
(192000, 2442913102, '2026-11-01 12:00:00'),
(192001, 1344507586, '2026-11-01 12:00:00'),
(192001, 2442913102, '2026-11-01 12:00:00');

DELETE FROM npc_vendor WHERE entry IN (1321, 3369) AND item IN (192000, 192001);
INSERT INTO npc_vendor (entry, slot, item, maxcount, incrtime, ExtendedCost, type) VALUES
(1321, 0, 192000, 0, 0, 0, 1),
(1321, 0, 192001, 0, 0, 0, 1),
(3369, 0, 192000, 0, 0, 0, 1),
(3369, 0, 192001, 0, 0, 0, 1);

COMMIT;
