CREATE TABLE u_users (
  id INT NOT NULL,
  pubId VARCHAR(32) NOT NULL DEFAULT '',
  name VARCHAR(40) NULL DEFAULT NULL COLLATE 'utf8mb4_unicode_ci',
  PRIMARY KEY (id)
) ENGINE=InnoDB DEFAULT CHARSET=ascii;

CREATE TABLE u_soft_data (
  userId INT(11) NOT NULL,
  exp INT(11) NOT NULL DEFAULT 0,
	level INT(11) NOT NULL DEFAULT 1,
  PRIMARY KEY (userId)
) ENGINE=InnoDB DEFAULT CHARSET=ascii;