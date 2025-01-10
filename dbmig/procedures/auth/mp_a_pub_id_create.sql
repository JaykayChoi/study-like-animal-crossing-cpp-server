CREATE PROCEDURE `mp_a_pub_id_create`(
  IN inAccountId VARCHAR(32) CHARSET ascii,
  IN inPubId VARCHAR(32) CHARSET ascii,
  IN inWorldId VARCHAR(32) CHARSET ascii
)
label_body:BEGIN

  INSERT INTO a_pub_ids (accountId, pubId, worldId) VALUES (inAccountId, inPubId, inWorldId);

  INSERT INTO a_world_users (pubId) VALUES(inPubId);

  -- Return the user id.
  SELECT LAST_INSERT_ID();
END