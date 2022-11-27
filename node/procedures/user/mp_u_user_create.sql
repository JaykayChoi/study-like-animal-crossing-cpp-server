CREATE PROCEDURE `mp_u_user_create`(
  IN inUserId INT,
  IN inPubId VARCHAR(32) CHARSET ascii,
  IN inLastLoginTimeUtc INT
)
label_body:BEGIN
  INSERT INTO u_users (id, pubId) VALUES(inUserId, inPubId);
  
  INSERT INTO u_states (userId, lastLoginTimeUtc) VALUES (inUserId, FROM_UNIXTIME(inLastLoginTimeUtc));

  INSERT INTO u_soft_data (userId) VALUES (inUserId);

END