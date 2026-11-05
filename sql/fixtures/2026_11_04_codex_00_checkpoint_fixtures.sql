-- T-007: test-only presets through T-048. Apply only to a private test world.
DELETE FROM `gmtools_checkpoint_step` WHERE `checkpoint` IN ('fixture-1','fixture-30','fixture-60','fixture-85','fixture-90');
DELETE FROM `gmtools_checkpoint` WHERE `name` IN ('fixture-1','fixture-30','fixture-60','fixture-85','fixture-90');
INSERT INTO `gmtools_checkpoint` (`name`,`category`,`sort_order`,`description`) VALUES
('fixture-1','fixtures',1,'Disposable level 1 fixture; native checkpoint gear and skills.'),
('fixture-30','fixtures',30,'Disposable level 30 fixture; native checkpoint gear and skills.'),
('fixture-60','fixtures',60,'Disposable level 60 fixture; native checkpoint gear and skills.'),
('fixture-85','fixtures',85,'Disposable level 85 fixture; native checkpoint gear and skills.'),
('fixture-90','fixtures',90,'Disposable level 90 fixture; native checkpoint gear and skills.');
INSERT INTO `gmtools_checkpoint_step` (`checkpoint`,`step`,`type`,`arg1`,`arg2`,`text`,`comment`) VALUES
('fixture-1',1,'level',1,0,'',''),
('fixture-1',2,'class_spells',0,0,'',''),
('fixture-1',3,'kit',0,0,'bags',''),
('fixture-1',4,'riding',0,0,'',''),
('fixture-1',5,'tele',0,0,'Stormwind|Orgrimmar',''),
('fixture-30',1,'level',30,0,'',''),
('fixture-30',2,'class_spells',0,0,'',''),
('fixture-30',3,'kit',0,0,'bags',''),
('fixture-30',4,'riding',0,0,'',''),
('fixture-30',5,'gear',2,0,'',''),
('fixture-30',6,'tele',0,0,'Stormwind|Orgrimmar',''),
('fixture-60',1,'level',60,0,'',''),
('fixture-60',2,'class_spells',0,0,'',''),
('fixture-60',3,'kit',0,0,'bags',''),
('fixture-60',4,'riding',0,0,'',''),
('fixture-60',5,'gear',3,0,'',''),
('fixture-60',6,'tele',0,0,'Stormwind|Orgrimmar',''),
('fixture-85',1,'level',85,0,'',''),
('fixture-85',2,'class_spells',0,0,'',''),
('fixture-85',3,'kit',0,0,'bags',''),
('fixture-85',4,'riding',0,0,'',''),
('fixture-85',5,'gear',3,0,'',''),
('fixture-85',6,'tele',0,0,'Stormwind|Orgrimmar',''),
('fixture-90',1,'level',90,0,'',''),
('fixture-90',2,'class_spells',0,0,'',''),
('fixture-90',3,'kit',0,0,'bags',''),
('fixture-90',4,'riding',0,0,'',''),
('fixture-90',5,'gear',4,0,'',''),
('fixture-90',6,'tele',0,0,'Stormwind|Orgrimmar','');
