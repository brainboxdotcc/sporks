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

DELIMITER $$
CREATE DEFINER=`admin`@`localhost` PROCEDURE `check_max` (IN `_guild_id` BIGINT(20) UNSIGNED)  READS SQL DATA
    SQL SECURITY INVOKER
    COMMENT 'Throws an error if the guild_id has more than 100 kv pairs'
BEGIN
DECLARE ERR_STATUS_NOT_VALID CONDITION FOR SQLSTATE '45000';
DECLARE matches INT(11) UNSIGNED;
SELECT COUNT(keyname) INTO matches FROM infobot_javascript_kv WHERE infobot_javascript_kv.guild_id = _guild_id;
IF matches > 100 THEN
    SET @text = CONCAT('No more KV storage allowed for guild id ', new.guild_id);
    SIGNAL ERR_STATUS_NOT_VALID SET MESSAGE_TEXT = @text;
END IF;
END$$

DELIMITER ;

CREATE TABLE `infobot_discord_javascript` (
  `id` bigint(20) UNSIGNED NOT NULL COMMENT 'Channel ID',
  `created` timestamp NOT NULL DEFAULT current_timestamp() COMMENT 'Creation date',
  `updated` timestamp NOT NULL DEFAULT current_timestamp() ON UPDATE current_timestamp() COMMENT 'Last modified date',
  `last_exec_ms` float NOT NULL DEFAULT 0 COMMENT 'Last execution time',
  `last_compile_ms` float NOT NULL DEFAULT 0 COMMENT 'Last compile time in ms',
  `script` longtext CHARACTER SET utf8mb4 DEFAULT NULL COMMENT 'Actual javascript content',
  `last_error` text CHARACTER SET utf8mb4 DEFAULT NULL COMMENT 'Last error message or empty/null',
  `last_memory_max` int(11) NOT NULL DEFAULT 0 COMMENT 'Last memory usage of script executed',
  `dirty` tinyint(1) UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Set to 1 if the bot is to reload this script'
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COMMENT='Information on which channels are using javascript replies';

CREATE TABLE `infobot_javascript_kv` (
  `guild_id` bigint(20) UNSIGNED NOT NULL COMMENT 'Guild ID',
  `keyname` varchar(100) NOT NULL COMMENT 'Key name',
  `value` text NOT NULL COMMENT 'Value content'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Key/Value store for javascript users';
DELIMITER $$
CREATE TRIGGER `insert_check_max` BEFORE INSERT ON `infobot_javascript_kv` FOR EACH ROW CALL check_max(new.guild_id)
$$
DELIMITER ;
DELIMITER $$
CREATE TRIGGER `update_check_max` BEFORE UPDATE ON `infobot_javascript_kv` FOR EACH ROW CALL check_max(new.guild_id)
$$
DELIMITER ;

ALTER TABLE `infobot_discord_javascript`
  ADD PRIMARY KEY (`id`),
  ADD KEY `dirty` (`dirty`),
  ADD KEY `created` (`created`),
  ADD KEY `updated` (`updated`);

ALTER TABLE `infobot_javascript_kv`
  ADD PRIMARY KEY (`guild_id`,`keyname`);

