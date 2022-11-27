CREATE PROCEDURE `mp_a_account_create`(
  IN inId VARCHAR(32) CHARSET ascii,
  IN inTimeUtc INT
)
label_body:BEGIN

  INSERT INTO a_accounts 
    (id, createTimeUtc, lastLoginTimeUtc) VALUES 
    (inId, FROM_UNIXTIME(inTimeUtc), FROM_UNIXTIME(inTimeUtc));

END