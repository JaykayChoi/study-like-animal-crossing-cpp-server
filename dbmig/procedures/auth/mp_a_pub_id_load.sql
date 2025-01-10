CREATE PROCEDURE `mp_a_pub_id_load`(
  IN inAccountId VARCHAR(32) CHARSET ascii,
  IN inWorldId VARCHAR(64) CHARSET ascii
)
label_body:BEGIN
  SELECT pubId FROM a_pub_ids WHERE accountId = inAccountId AND worldId = inWorldId;
END