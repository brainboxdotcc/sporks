# Sporks, the backchatting, learning, scriptable discord bot
This github project contains the source code for the Discord enhancements for Sporks, the infobot/botnix bot, written in C++ using the aegis.cpp library.
Remember you can still find my original botnix version of Sporks on IRC at irc.chatspike.net!

## Project and System status

![Discord](https://img.shields.io/discord/537746810471448576?label=discord) ![CircleCI](https://img.shields.io/circleci/build/github/braindigitalis/botnix-discord-cpp/master) ![Dashboard](https://img.shields.io/website?down_color=red&label=dashboard&url=https%3A%2F%2Fsporks.gg)

[Service Status](https://status.sporks.gg)

## Listing Badges

![Discord Boats](https://discord.boats/api/widget/630730262765895680) 
![DiscordBotList](https://discordbotlist.com/bots/630730262765895680/widget)

## Supported Platforms

Currently only Linux is supported, but other UNIX-style platforms should build and run the bot fine. I build the bot under Debian Linux.

## Dependencies

* [botnix](https://github.com/braindigitalis/botnix) (version 2.0 running infobot.pm and telnet.pm)
* [cmake](https://cmake.org/) (version 3.13+)
* [g++](https://gcc.gnu.org) (version 8+)
* [aegis.cpp](https://github.com/zeroxs/aegis.cpp) (development branch)
* [asio](https://think-async.com/Asio/) (included with aegis.cpp)
* [websocketpp](https://github.com/zaphoyd/websocketpp) (included with aegis.cpp)
* [nlohmann::json](https://github.com/nlohmann/json) (included with aegis.cpp)
* [duktape](https://github.com/svaarala/duktape) (master branch)
* [PCRE](https://www.pcre.org/) (whichever -dev package comes with your OS)
* [MySQL Client Libraries](https://dev.mysql.com/downloads/c-api/) (whichever -dev package comes with your OS)
* [ZLib](https://www.zlib.net/) (whichever -dev package comes with your OS)
 
## Building

    mkdir build
    cmake ..
    make -j4
    
Replace the number after -j with a number suitable for your setup, usually the same as the number of cores on your machine.

## Database

You should have a database configured with the mysql schemas from the [botnix repository](https://github.com/braindigitalis/botnix). This will include discord-specific tables.

## Configuration

Edit the config-example.json file and save it as config.json. The configuration variables in the file should be self explainatory.

## Running

    cd my-bot-dir
    ./run.sh

run.sh will restart the bot executable continually if it dies. 

## Command line parameters

    ./bot [--dev]

| Argument        | Meaning                                                |
| --------------- |------------------------------------------------------- |
| --dev           | Run using the development token in the config file     |
| --debug         | Run using the live token but only respond on the development server  |


## Reporting to bot list sites

To report your bot's server count to bot list sites (really, you shouldn't even do this, as you'd be running a fork of an existing bot!) you should add the simple script "update-bot-lists.php" to your crontab:

    user@server:~/bot$ crontab -l
    # m h  dom mon dow   command
    0,30 * * * *    cd /home/user/bot && /usr/bin/php update-bot-lists.php
    
and configure the infobot_discord_list_sites table with the sites your bot is registered upon:

    INSERT INTO `infobot_discord_list_sites` (`name`, `url`, `server_count_field`, `user_count_field`, `authorization`) VALUES
    ('Top.gg', 'https://top.gg/api/bots/your-bot-id/stats', 'server_count', NULL, 'your api key goes here'),
    ('DiscordBotList', 'https://discordbotlist.com/api/bots/your-bot-id/stats', 'guilds', 'users', 'another api key'),
    ('Bots On Discord', 'https://bots.ondiscord.xyz/bot-api/bots/your-bot-id/guilds', 'guildCount', NULL, 'api key for this other site'),
    ('Divine Discord Bots', 'https://divinediscordbots.com/bot/your-bot-id/stats', 'server_count', NULL, 'guess what goes here?');

