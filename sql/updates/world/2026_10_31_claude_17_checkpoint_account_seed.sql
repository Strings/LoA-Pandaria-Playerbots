-- T-049: the Heartseed's Growth is the account's now, so checkpoints no longer reset it. Their heartseed_rank
-- steps only raise the shared rank. Idempotent.

DELETE FROM `gmtools_checkpoint_step` WHERE `type` = 'heartseed_growth';
