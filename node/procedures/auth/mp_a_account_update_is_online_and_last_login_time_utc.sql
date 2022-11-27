CREATE PROCEDURE `mp_a_account_update_is_online_and_last_login_time_utc`(
  IN inId VARCHAR(32) CHARSET ascii,
  IN inIsOnline TINYINT,
  IN inLastLoginTimeUtc INT
)
label_body:BEGIN
  UPDATE a_accounts SET isOnline = inIsOnline, lastLoginTimeUtc = FROM_UNIXTIME(inLastLoginTimeUtc) WHERE id = inId;
END