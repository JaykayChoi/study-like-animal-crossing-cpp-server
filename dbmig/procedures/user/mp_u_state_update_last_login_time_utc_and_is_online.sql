CREATE PROCEDURE `mp_u_state_update_last_login_time_utc_and_is_online`(
  IN inUserId INT,
  IN inLastLoginTimeUtc INT,
  IN inIsOnline TINYINT
)
label_body:BEGIN
  UPDATE u_states SET lastLoginTimeUtc = FROM_UNIXTIME(inLastLoginTimeUtc), isOnline = inIsOnline WHERE userId = inUserId;
END