DROP TABLE IF EXISTS u_states;
CREATE TABLE u_states (
  userId INT NOT NULL,
  lastLoginTimeUtc TIMESTAMP NULL DEFAULT NULL,
  isOnline TINYINT NOT NULL DEFAULT 1,
  PRIMARY KEY (userId),
  INDEX IDX_u_states__lastLoginTimeUtc (lastLoginTimeUtc)
) ENGINE=InnoDB DEFAULT CHARSET=ascii;
