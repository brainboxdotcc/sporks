SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";

CREATE TABLE IF NOT EXISTS `infobot_discord_counts` (
  `shard_id` bigint(20) unsigned NOT NULL COMMENT 'Shard ID',
  `dev` tinyint(1) unsigned NOT NULL DEFAULT '0' COMMENT 'true if development data',
  `user_count` bigint(20) NOT NULL,
  `server_count` bigint(20) NOT NULL,
  `last_updated` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COMMENT='Counts of users/servers on a per-shard basis';

CREATE TABLE IF NOT EXISTS `infobot_discord_list_sites` (
`id` bigint(20) unsigned NOT NULL,
  `name` varchar(255) NOT NULL,
  `url` varchar(255) NOT NULL,
  `server_count_field` varchar(255) NOT NULL,
  `user_count_field` varchar(255) DEFAULT NULL,
  `authorization` varchar(255) NOT NULL
) ENGINE=InnoDB AUTO_INCREMENT=5 DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS `infobot_discord_settings` (
  `id` bigint(20) unsigned NOT NULL COMMENT 'discord channel id',
  `parent_id` bigint(20) unsigned DEFAULT NULL COMMENT 'Parent ID',
  `guild_id` bigint(20) unsigned NOT NULL COMMENT 'Guild ID',
  `name` text CHARACTER SET utf8mb4 COMMENT 'Channel name',
  `settings` longtext CHARACTER SET utf8mb4 NOT NULL,
  `tombstone` tinyint(1) unsigned NOT NULL DEFAULT '0'
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COMMENT='infobot settings for discord servers';

CREATE TABLE IF NOT EXISTS `infobot_discord_user_cache` (
  `id` bigint(20) unsigned NOT NULL COMMENT 'Discord user ID',
  `username` varchar(750) NOT NULL COMMENT 'Username',
  `discriminator` char(4) CHARACTER SET latin1 COLLATE latin1_general_ci NOT NULL,
  `avatar` varchar(256) CHARACTER SET latin1 COLLATE latin1_general_ci DEFAULT NULL,
  `bot` tinyint(1) unsigned NOT NULL DEFAULT '0' COMMENT 'User is a bot',
  `modified` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last modified timestamp'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='A database backed cache of discord users';


ALTER TABLE `infobot_discord_counts`
 ADD PRIMARY KEY (`shard_id`,`dev`), ADD KEY `last_updated` (`last_updated`);

ALTER TABLE `infobot_discord_list_sites`
 ADD PRIMARY KEY (`id`), ADD KEY `name` (`name`);

ALTER TABLE `infobot_discord_settings`
 ADD PRIMARY KEY (`id`), ADD KEY `guild_id` (`guild_id`), ADD KEY `parent_id` (`parent_id`);

ALTER TABLE `infobot_discord_user_cache`
 ADD PRIMARY KEY (`id`), ADD UNIQUE KEY `id` (`id`), ADD KEY `discriminator` (`discriminator`), ADD KEY `username` (`username`), ADD KEY `bot` (`bot`), ADD KEY `modified` (`modified`);


ALTER TABLE `infobot_discord_list_sites`
MODIFY `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,AUTO_INCREMENT=5;
