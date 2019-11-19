# botnix-discord-cpp
Botnix Discord Connector, C++ verison using Aegis.cpp

![Discord](https://img.shields.io/discord/537746810471448576?label=discord) ![CircleCI](https://img.shields.io/circleci/build/github/braindigitalis/botnix-discord-cpp/master) ![Dashboard](https://img.shields.io/website?down_color=red&label=dashboard&url=https%3A%2F%2Fsporks.gg)
 
## Building

    mkdir build
    cmake ..
    make -j4

## Database

You should have a database configured with the mysql schemas from the [botnix repository](https://github.com/braindigitalis/botnix). This will include discord-specific tables.

## Configuration

Edit the config-example.json file and save it as config.json. The configuration variables in the file should be self explainatory.

## Running

    cd my-bot-dir
    ./run.sh

run.sh will restart the bot executable continually if it dies. Alternatively, if you want to shard the bot you should create your own set of run.sh files, one on each shard, which pass the correct command line parameters to the bot executable.

## Command line parameters

    ./bot [--dev]

| Argument        | Meaning                                                |
| --------------- |------------------------------------------------------- |
| --dev           | Run using the development token in the config file     |


## Reporting to bot list sites

To report your bot's server count to bot list sites (really, you shouldn't even do this, as you'd be running a fork of an existing bot!) you should add the simple script "update-bot-lists.php" to your crontab:

    user@server:~/cppbot$ crontab -l
    # m h  dom mon dow   command
    0,30 * * * *    cd /home/user/cppbot && /usr/bin/php update-bot-lists.php
    
and configure the infobot_discord_list_sites table with the sites your bot is registered upon:

    INSERT INTO `infobot_discord_list_sites` (`name`, `url`, `server_count_field`, `user_count_field`, `authorization`) VALUES
    ('Top.gg', 'https://top.gg/api/bots/your-bot-id/stats', 'server_count', NULL, 'your api key goes here'),
    ('DiscordBotList', 'https://discordbotlist.com/api/bots/your-bot-id/stats', 'guilds', 'users', 'another api key'),
    ('Bots On Discord', 'https://bots.ondiscord.xyz/bot-api/bots/your-bot-id/guilds', 'guildCount', NULL, 'api key for this other site'),
    ('Divine Discord Bots', 'https://divinediscordbots.com/bot/your-bot-id/stats', 'server_count', NULL, 'guess what goes here?');

## Requirements

* A discord API bot token
* A running instance of botnix with infobot.pm and telnet.pm loaded
* GCC 4 or newer
* cmake
