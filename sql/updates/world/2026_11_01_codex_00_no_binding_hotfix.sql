-- NoBinding item-sparse invalidation. Original item_template binding stays intact.
-- Existing hotfix history (including stack updates) is preserved; rerunning adds no duplicates.
INSERT INTO `hotfix_data` (`entry`,`type`,`hotfixDate`)
SELECT i.`entry`,2442913102,'2026-11-01 00:00:00'
FROM `item_template` i
WHERE (i.`bonding`<>0 OR (i.`flags` & 134217728)<>0)
  AND NOT EXISTS (SELECT 1 FROM `hotfix_data` h WHERE h.`entry`=i.`entry`
    AND h.`type`=2442913102 AND h.`hotfixDate`='2026-11-01 00:00:00');
