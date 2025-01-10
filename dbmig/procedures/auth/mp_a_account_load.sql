CREATE PROCEDURE `mp_a_account_load`(
  IN inId VARCHAR(32) CHARSET ascii
)
label_body:BEGIN
  SELECT 
    isOnline
  FROM 
    a_accounts 
  WHERE 
    id = inId;
END
