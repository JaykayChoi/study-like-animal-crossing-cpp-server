CREATE TABLE a_pub_ids (
  accountId VARCHAR(32) NOT NULL,
  pubId VARCHAR(32) NOT NULL,
  worldId VARCHAR(32) NULL DEFAULT NULL,
  PRIMARY KEY (accountId, pubId),
  UNIQUE KEY UQ_a_pub_ids__accountId_worldId (accountId, worldId),
  INDEX IDX_a_pub_ids__pubId_worldId (pubId, worldId),
  INDEX IDX_a_pub_ids__accountId_worldId (accountId, worldId)
) ENGINE=InnoDB DEFAULT CHARSET=ascii;

CREATE TABLE a_world_users (
  userId INT NOT NULL AUTO_INCREMENT,
  pubId VARCHAR(32) NOT NULL,
  name VARCHAR(40) NULL DEFAULT NULL COLLATE 'utf8mb4_general_ci',
  PRIMARY KEY (userId),
  INDEX IDX_a_world_users__pubId (pubId),
	INDEX IDX_a_world_users__name (name)
) ENGINE=InnoDB DEFAULT CHARSET=ascii;
