CREATE PROCEDURE `mp_a_world_user_load_user_id`(
  IN inPubId VARCHAR(32) CHARSET ascii
)
label_body:BEGIN
  SELECT userId FROM a_world_users WHERE pubId = inPubId;
END