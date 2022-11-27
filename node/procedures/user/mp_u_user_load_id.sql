CREATE PROCEDURE `mp_u_user_load_id`(
  IN inUserId INT
)
label_body:BEGIN

  SELECT id FROM u_users WHERE id = inUserId;

END