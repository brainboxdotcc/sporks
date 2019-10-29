# botnix-discord-cpp
Botnix Discord Connector, C++ verison

## Building

    mkdir build
    cmake ..
    make -j4

## Database

You should have a database configured with the mysql schemas from the [https://github.com/braindigitalis/botnix](botnix repository). This will include discord-specific tables.

## Configuration

Edit the config-example.json file and save it as config.json. The configuration variables in the file should be self explainatory.

## Running

    cd my-bot-dir
    ./run.sh

run.sh will restart the bot executable continually if it dies. Alternatively, if you want to shard the bot you should create your own set of run.sh files, one on each shard, which pass the correct command line parameters to the bot executable.

## Command line parameters

    ./bot [--dev] [--shardid=<n>] [--numshards=<m>]

+-----------------+--------------------------------------------------------+
| Argument        | Meaning                                                |
+-----------------+--------------------------------------------------------+
| --dev           | Run using the development token in the config file     |
| --shardid=<n>   | Run as shard id <n> in a sharded environment           |
| --numshards=<n> | The number of shards in total in a sharded environment | 
+-----------------+--------------------------------------------------------+

## Reporting to bot list sites

To report your bot's server count to bot list sites (really, you shouldn't even do this, as you'd be running a fork of an existing bot!) you should add the simple script "update-bot-lists.php" to your crontab:

    brain@neuron:~/cppbot$ crontab -l
    # Edit this file to introduce tasks to be run by cron.
    # 
    # Each task to run has to be defined through a single line
    # indicating with different fields when the task will be run
    # and what command to run for the task
    # 
    # To define the time you can provide concrete values for
    # minute (m), hour (h), day of month (dom), month (mon),
    # and day of week (dow) or use '*' in these fields (for 'any').# 
    # Notice that tasks will be started based on the cron's system
    # daemon's notion of time and timezones.
    # 
    # Output of the crontab jobs (including errors) is sent through
    # email to the user the crontab file belongs to (unless redirected).
    # 
    # For example, you can run a backup of all your user accounts
    # at 5 a.m every week with:
    # 0 5 * * 1 tar -zcf /var/backups/home.tgz /home/
    # 
    # For more information see the manual pages of crontab(5) and cron(8)
    # 
    # m h  dom mon dow   command
    0,30 * * * *    cd /home/brain/cppbot && /usr/bin/php update-bot-lists.php
    
and configure the infobot_discord_list_sites table with the sites your bot is registered upon:

    CREATE TABLE IF NOT EXISTS `infobot_discord_list_sites` (
    `id` bigint(20) unsigned NOT NULL,
      `name` varchar(255) NOT NULL,
      `url` varchar(255) NOT NULL,
      `server_count_field` varchar(255) NOT NULL,
      `user_count_field` varchar(255) DEFAULT NULL,
      `authorization` varchar(255) NOT NULL
    ) ENGINE=InnoDB AUTO_INCREMENT=5 DEFAULT CHARSET=latin1;
    
    INSERT INTO `infobot_discord_list_sites` (`name`, `url`, `server_count_field`, `user_count_field`, `authorization`) VALUES
    ('Top.gg', 'https://top.gg/api/bots/630730262765895680/stats', 'server_count', NULL, 'your api key goes here'),
    ('DiscordBotList', 'https://discordbotlist.com/api/bots/630730262765895680/stats', 'guilds', 'users', 'another api key'),
    ('Bots On Discord', 'https://bots.ondiscord.xyz/bot-api/bots/630730262765895680/guilds', 'guildCount', NULL, 'api key for this other site'),
    ('Divine Discord Bots', 'https://divinediscordbots.com/bot/630730262765895680/stats', 'server_count', NULL, 'guess what goes here?');

## Requirements

* A discord API bot token
* A running instance of botnix with infobot.pm and telnet.pm loaded
* GCC 4 or newer
* cmake
