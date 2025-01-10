CREATE PROCEDURE `mp_u_user_load`(
  IN inUserId INT
)
label_body:BEGIN
  SELECT
      u_users.name,
      u_soft_data.exp,
      u_soft_data.level
    FROM
      u_users, u_soft_data
    WHERE
      u_users.id = inUserId
      AND u_soft_data.userId = inUserId;
END