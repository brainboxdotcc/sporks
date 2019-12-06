SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";

DELIMITER $$
CREATE DEFINER=`admin`@`localhost` PROCEDURE `check_max` (IN `_guild_id` BIGINT(20) UNSIGNED)  READS SQL DATA
    SQL SECURITY INVOKER
    COMMENT 'Throws an error if the guild_id has more than 100 kv pairs'
BEGIN
DECLARE ERR_STATUS_NOT_VALID CONDITION FOR SQLSTATE '45000';
DECLARE matches INT(11) UNSIGNED;
SELECT COUNT(keyname) INTO matches FROM infobot_javascript_kv WHERE infobot_javascript_kv.guild_id = _guild_id;
IF matches > 1024 THEN
    SET @text = CONCAT('No more KV storage allowed for guild id ', new.guild_id);
    SIGNAL ERR_STATUS_NOT_VALID SET MESSAGE_TEXT = @text;
END IF;
END$$

DELIMITER ;

CREATE TABLE `infobot` (
  `value` longtext DEFAULT NULL,
  `word` enum('is','can','are','has','cant','r','will','was','can''t','had','aren''t','might','may','arent') NOT NULL DEFAULT 'is',
  `setby` varchar(512) NOT NULL,
  `whenset` bigint(20) UNSIGNED NOT NULL,
  `locked` tinyint(1) UNSIGNED NOT NULL DEFAULT 0,
  `key_word` varchar(768) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE `infobot_discord_counts` (
  `shard_id` bigint(20) UNSIGNED NOT NULL COMMENT 'Shard ID',
  `dev` tinyint(1) UNSIGNED NOT NULL DEFAULT 0 COMMENT 'true if development data',
  `user_count` bigint(20) NOT NULL,
  `server_count` bigint(20) NOT NULL,
  `shard_count` bigint(20) UNSIGNED NOT NULL DEFAULT 1 COMMENT 'number of shards',
  `channel_count` bigint(20) UNSIGNED NOT NULL,
  `sent_messages` bigint(20) UNSIGNED NOT NULL DEFAULT 0,
  `received_messages` bigint(20) UNSIGNED NOT NULL DEFAULT 0,
  `memory_usage` bigint(20) UNSIGNED NOT NULL DEFAULT 0,
  `last_updated` datetime NOT NULL DEFAULT current_timestamp() ON UPDATE current_timestamp()
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COMMENT='Counts of users/servers on a per-shard basis';

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

CREATE TABLE `infobot_discord_list_sites` (
  `id` bigint(20) UNSIGNED NOT NULL,
  `name` varchar(255) NOT NULL,
  `url` varchar(255) NOT NULL,
  `server_count_field` varchar(255) NOT NULL,
  `user_count_field` varchar(255) DEFAULT NULL,
  `shard_count_field` varchar(255) DEFAULT NULL,
  `sent_message_count_field` varchar(255) DEFAULT NULL,
  `received_message_count_field` varchar(255) DEFAULT NULL,
  `ram_used_field` varchar(255) DEFAULT NULL,
  `channels_field` varchar(255) DEFAULT NULL,
  `auth_field` varchar(100) DEFAULT NULL,
  `authorization` varchar(255) NOT NULL,
  `post_type` enum('json','post') NOT NULL DEFAULT 'json'
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `infobot_discord_settings` (
  `id` bigint(20) UNSIGNED NOT NULL COMMENT 'discord channel id',
  `parent_id` bigint(20) UNSIGNED DEFAULT NULL COMMENT 'Parent ID',
  `guild_id` bigint(20) UNSIGNED NOT NULL COMMENT 'Guild ID',
  `name` text CHARACTER SET utf8mb4 DEFAULT NULL COMMENT 'Channel name',
  `settings` longtext CHARACTER SET utf8mb4 NOT NULL,
  `tombstone` tinyint(1) UNSIGNED NOT NULL DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COMMENT='infobot settings for discord servers';

CREATE TABLE `infobot_discord_user_cache` (
  `id` bigint(20) UNSIGNED NOT NULL COMMENT 'Discord user ID',
  `username` varchar(750) NOT NULL COMMENT 'Username',
  `discriminator` char(4) CHARACTER SET latin1 COLLATE latin1_general_ci NOT NULL,
  `avatar` varchar(256) CHARACTER SET latin1 COLLATE latin1_general_ci DEFAULT NULL,
  `bot` tinyint(1) UNSIGNED NOT NULL DEFAULT 0 COMMENT 'User is a bot',
  `modified` timestamp NOT NULL DEFAULT current_timestamp() ON UPDATE current_timestamp() COMMENT 'Last modified timestamp'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='A database backed cache of discord users';

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

CREATE TABLE `infobot_shard_map` (
  `guild_id` bigint(20) UNSIGNED NOT NULL,
  `shard_id` int(10) UNSIGNED NOT NULL,
  `name` varchar(512) NOT NULL,
  `icon` varchar(200) NOT NULL,
  `unavailable` tinyint(1) UNSIGNED NOT NULL DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE `infobot_shard_status` (
  `id` int(10) UNSIGNED NOT NULL,
  `connected` tinyint(1) UNSIGNED NOT NULL,
  `online` tinyint(1) UNSIGNED NOT NULL,
  `uptime` bigint(20) UNSIGNED NOT NULL,
  `transfer` bigint(20) UNSIGNED NOT NULL,
  `transfer_compressed` bigint(20) UNSIGNED NOT NULL,
  `uptimerobot_heartbeat` varchar(300) DEFAULT NULL,
  `updated` timestamp NULL DEFAULT current_timestamp() ON UPDATE current_timestamp(),
  `uptimerobot_response` varchar(255) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Contains details of all active shards';

CREATE TABLE `infobot_votes` (
  `id` bigint(20) UNSIGNED NOT NULL,
  `snowflake_id` bigint(20) UNSIGNED NOT NULL,
  `vote_time` timestamp NULL DEFAULT current_timestamp(),
  `origin` varchar(50) NOT NULL,
  `rolegiven` tinyint(1) UNSIGNED NOT NULL DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE `infobot_web_requests` (
  `channel_id` bigint(20) UNSIGNED NOT NULL,
  `guild_id` bigint(20) UNSIGNED NOT NULL,
  `url` varchar(500) NOT NULL,
  `type` varchar(20) NOT NULL DEFAULT 'GET',
  `postdata` text DEFAULT NULL,
  `callback` varchar(500) NOT NULL,
  `returndata` longtext DEFAULT NULL,
  `statuscode` char(3) NOT NULL DEFAULT '000'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='List of queued web requests';


ALTER TABLE `infobot`
  ADD PRIMARY KEY (`key_word`),
  ADD KEY `word_idx` (`word`),
  ADD KEY `locked_idx` (`locked`),
  ADD KEY `setby_idx` (`setby`);

ALTER TABLE `infobot_discord_counts`
  ADD PRIMARY KEY (`shard_id`,`dev`),
  ADD KEY `last_updated` (`last_updated`);

ALTER TABLE `infobot_discord_javascript`
  ADD PRIMARY KEY (`id`),
  ADD KEY `dirty` (`dirty`),
  ADD KEY `created` (`created`),
  ADD KEY `updated` (`updated`);

ALTER TABLE `infobot_discord_list_sites`
  ADD PRIMARY KEY (`id`),
  ADD KEY `name` (`name`),
  ADD KEY `post_type` (`post_type`);

ALTER TABLE `infobot_discord_settings`
  ADD PRIMARY KEY (`id`),
  ADD KEY `guild_id` (`guild_id`),
  ADD KEY `parent_id` (`parent_id`);

ALTER TABLE `infobot_discord_user_cache`
  ADD PRIMARY KEY (`id`),
  ADD UNIQUE KEY `id` (`id`),
  ADD KEY `discriminator` (`discriminator`),
  ADD KEY `username` (`username`),
  ADD KEY `bot` (`bot`),
  ADD KEY `modified` (`modified`);

ALTER TABLE `infobot_javascript_kv`
  ADD PRIMARY KEY (`guild_id`,`keyname`);

ALTER TABLE `infobot_shard_map`
  ADD PRIMARY KEY (`guild_id`);

ALTER TABLE `infobot_shard_status`
  ADD PRIMARY KEY (`id`),
  ADD KEY `connected` (`connected`),
  ADD KEY `online` (`online`),
  ADD KEY `uptimerobot_heartbeat` (`uptimerobot_heartbeat`);

ALTER TABLE `infobot_votes`
  ADD PRIMARY KEY (`id`),
  ADD KEY `snowflake_id` (`snowflake_id`),
  ADD KEY `vote_time` (`vote_time`),
  ADD KEY `origin` (`origin`),
  ADD KEY `rolegiven` (`rolegiven`);


ALTER TABLE `infobot_discord_list_sites`
  MODIFY `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=16;
ALTER TABLE `infobot_votes`
  MODIFY `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=9;

