/************************************************************************************
 *
 * Sporks, the learning, scriptable Discord bot!
 *
 * Copyright 2019 Craig Edwards <support@sporks.gg>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ************************************************************************************/

#include <random>
#include <iterator>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <sporks/regex.h>
#include <sporks/database.h>
#include <sporks/stringops.h>
#include "backend.h"
#include "infobot.h"

infostats stats;

bool quiet = false;
bool found = false;

infodef get_def(const std::string &key);
uint64_t get_phrase_count();
void set_def(std::string key, const std::string &value, const std::string &word, const std::string &setby, time_t when, bool locked);
std::string expand(std::string str, const std::string &nick, time_t timeval, const std::string &mynick, const std::string &randuser);
void del_def(const std::string &key);
std::string getreply(std::string s, const std::string &delim = "|");
bool locked(const std::string &key);
std::string getreply(std::vector<std::string> v);

std::map<std::string, std::vector<std::string>> replies = {
	{"replies",  {"I heard %k %w %v", "They say %k %w %v", "%k %w %v... I think", "someone said %k %w %v", "%k %w like, %v", "%k %w %v", "%k %w %v, maybe?", "%s once said %k %w %v"}},
	{"dontknow", {"Sorry %n I don't know what %k is.", "%k? no idea %n.", "I'm not a genius, %n...", "Its best to ask a real person about %k.", "Not a clue.", "Don't you know, %n?"}},
	{"notnew",   {"but %k %w %v :(", "fool, %k %w %v :p", "%k already %w %v...", "Are you sure, %n? I am sure that %k %w %v!"}},
	{"heard",    {"%s told me about %k on %d", "I learned that on %d, and i think it was %s that told me it.", "I think it was %s who said that, way back on %d...", "%n: Back on %d, %s told me about %k"}},
	{"confirm",  {"Ok, %n", "Your wish is my command.", "Okay.", "Whatever...", "Gotcha.", "Ok.", "Right."}},
	{"loggedout",{"You're a mild idiot %n, log in and try again", "I can't do that %n because you dont have the power!", "I would do what you ask, but you are not logged in!", "Oh dear, someone locked that. log in and try again %n."}},
	{"locked",   {"You can't edit that! The keyword '%k' has been locked against changes!"}},
	{"forgot",   {"I forgot %k", "%k is gone from my mind, %n", "As you wish.", "It's history.", "Done." }}
};

void InfobotModule::infobot_init()
{
	stats.startup = time(NULL);
	stats.modcount = stats.qcount = 0;
}

std::string removepunct(std::string word)
{
	// $key =~ s/(\.|\,|\!|\?|\s+)$//g;
	while (word.length() && (word.back() == '?' || word.back() == '.' || word.back() == ',' || word.back() == '!' || word.back() == ' ')) {
		word.pop_back();
	}
	return word;
}

std::string InfobotModule::infobot_response(std::string mynick, std::string otext, std::string usernick, std::string randuser, int64_t channelID)
{
	reply_level level = NOT_ADDRESSED;
	std::string rpllist = "";
	infodef reply;
	bool direct_question = (PCRE("[\\?!]$").Match(otext));
	std::vector<std::string> matches;

	otext = mynick + " " + otext;

	if (PCRE("^(no\\s*" + mynick + "[,: ]+|" + mynick + "[,: ]+|)(.*?)$", true).Match(otext, matches)) {
		std::string address = matches[1];
		std::string text = matches[2];
		
		// If it was addressing us, remove the part with our nick in it, and any punctuation after it...
		if (PCRE("^no\\s*" + mynick + "[,: ]+$", true).Match(address)) {
			level = ADDRESSED_BY_NICKNAME_CORRECTION;
		}
		if (PCRE("^" + mynick + "[,: ]+$", true).Match(address)) {
			level = ADDRESSED_BY_NICKNAME;
		}
		
		if (PCRE("^(who|what|where)\\s+(is|was|are)\\s+(.+?)[\?!\\.]*$", true).Match(text, matches)) {
			text = matches[3] + "?";
			direct_question = true;
		}
		
		// First option, someone is asking who told the bot something, simple enough...
		if (PCRE("^who told you about (.*?)\\?*$", true).Match(text, matches)) {
			std::string key = removepunct(matches[1]);
			reply = get_def(key);
			rpllist = reply.found ? "heard" : "dontknow";
		}
		// Forget command
		else if (level == ADDRESSED_BY_NICKNAME && PCRE("^forget (.*?)$", true).Match(text, matches)) {
			std::string key = removepunct(matches[1]);
			reply = get_def(key);
			if (reply.found) {
				if (reply.locked) {
					rpllist = "locked";
				} else {
					del_def(key);
					rpllist = "forgot";
				}
			} else {
				rpllist = "dontknow";
			}
		}
		// status command
		else if (level >= ADDRESSED_BY_NICKNAME && PCRE("^status\\?*$", true).Match(text)) {
			
			time_t diff = time(NULL) - stats.startup;
			int seconds = diff % 60;
			diff /= 60;
			int minutes = diff % 60;
			diff /= 60;
			int hours = diff % 24;
			diff /= 24;
			int days = diff;

			ShowStatus(days, hours, minutes, seconds, stats.modcount, stats.qcount, get_phrase_count(), stats.startup, channelID);
			return "";
		}
		// Literal command, print out key and value with no parsing
		else if (PCRE("^literal (.*)\\?*$", true).Match(text, matches)) {
			std::string key = removepunct(matches[1]);
			// This bit is a bit different, it bypasses a lot of the parsing for stuff like %n
			reply = get_def(key);
			if (reply.found) {
				return key + " is " + reply.value;
			} else {
				rpllist = "dontknow";
			}
		}
		// Next option, someone is either adding a new phrase to the bot or editing an old one, a bit trickier...
		else if ((PCRE("^(.*?)\\s+=(is|are|was|arent|aren't|can|can't|cant|will|has|had|r|might|may)=\\s+(.*)\\s*$", true).Match(text, matches) || PCRE("^(.*?)\\s+(is|are|was|arent|aren't|can|can't|cant|will|has|had|r|might|may)\\s+(.*)\\s*$", true).Match(text, matches)) && (rpllist == "")) {
			std::string key = removepunct(matches[1]);
			std::string word = matches[2];
			std::string value = matches[3];
			
			if (key == "") {
				return "";
			}
			
			reply = get_def(key);
			
			if (reply.locked) {
				rpllist = "locked";
			} else if (level == ADDRESSED_BY_NICKNAME_CORRECTION || reply.found == false) {
				set_def(key, value, word, usernick, time(NULL), false);
				stats.modcount++;
				if (level >= ADDRESSED_BY_NICKNAME) {
					rpllist = "confirm";
				}
			} else {
				if (PCRE("^also\\s+(.*)$", true).Match(value, matches)) {
					std::string newvalue = matches[1];
					if (PCRE("^\\|").Match(newvalue)) {
						reply.value = reply.value + " " + newvalue;
					} else {
						reply.value = reply.value + " or " + newvalue;
					}
					set_def(key, reply.value, reply.word, usernick, time(NULL), false);
					if (level >= ADDRESSED_BY_NICKNAME) {
						rpllist = "confirm";
					}
				} else if (lowercase(reply.value) != lowercase(value)) {
					if (level >= ADDRESSED_BY_NICKNAME) {
						rpllist = "notnew";
					}
				}
			}
		}
		
		if (PCRE("(.*?)\\?*\\s*$", true).Match(text, matches) && rpllist == "") {
			std::string key = removepunct(matches[1]);
			stats.qcount++;
			reply = get_def(key);

			if (reply.found) {
				if (direct_question || level >= ADDRESSED_BY_NICKNAME /* did contain: || rand(15) > 13 */) {
					rpllist = "replies";
				}
			} else if (level >= ADDRESSED_BY_NICKNAME) {
				rpllist = "dontknow";
			}
		}
	}
	
	/* Parse reply message from templates in replies map */
	
	if (rpllist != "") {
		bool repeat = false;
		std::string s_reply = "";
		
		do {
			repeat = false;
			s_reply = expand(getreply(replies[rpllist]), usernick, reply.whenset, mynick, randuser);

			char timestr[256];
			tm* _tm = gmtime(&reply.whenset);
			strftime(timestr, 255, "%c", _tm);

			s_reply = ReplaceString(s_reply, "%k", reply.key);
			s_reply = ReplaceString(s_reply, "%w", reply.word);
			s_reply = ReplaceString(s_reply, "%n", usernick);
			s_reply = ReplaceString(s_reply, "%m", mynick);
			s_reply = ReplaceString(s_reply, "%d", timestr);
			s_reply = ReplaceString(s_reply, "%s", reply.setby);
			s_reply = ReplaceString(s_reply, "%l", reply.locked ? "locked" : "unlocked");

			// Gobble up empty reply
			if (lowercase(reply.value) == "<reply>" && rpllist == "replies") {
				return "";
			}

			if (rpllist == "replies" && PCRE("<alias>\\s*(.*)", true).Match(reply.value, matches)) {
				std::string oldkey = reply.key;
				reply.key = matches[1];
				infodef r = get_def(reply.key);
				if (!r.found) {
					/* Broken alias */
					return "";
				}
				reply = r;
				/* Prevent alias loops */
				if (!PCRE("<alias>\\s*(.*)", true).Match(reply.value)) {
					repeat = true;
				}
			}
		} while (repeat);

		reply.value = getreply(reply.value);

		if (rpllist == "replies" && PCRE("<(reply|action)>\\s*(.*)", true).Match(reply.value, matches)) {
			/* Just a <reply>? bog off... */
			if (PCRE("^\\s*<reply>\\s*$", true).Match(reply.value)) {
				return "";
			}

			matches[2] = ReplaceString(matches[2], "<action>", "");
			matches[2] = ReplaceString(matches[2], "<reply>", "");

			reply.value = (lowercase(matches[1]) == "action") ? "*" + trim(matches[2]) + "*" : matches[2];

			std::string x = expand(reply.value, usernick, reply.whenset, mynick, randuser);
			if (x == "%v") {
				return "";
			}

			return x;
		}

		s_reply = ReplaceString(s_reply, "%v", reply.value);

		if (s_reply == "%v" || s_reply == "") {
			return "";
		}
		return s_reply;
	}
	return "";
}

infodef get_def(const std::string &key)
{
	infodef d;
	d.key = d.value = d.word = d.setby = "";
	d.whenset = 0;
	d.locked = d.found = false;
	db::resultset r = db::query("SELECT key_word, value, word, setby, whenset, locked FROM infobot WHERE key_word = '?'", {key});
	if (r.size()) {
		d.key = r[0]["key_word"];
		d.value = r[0]["value"];
		d.word = r[0]["word"];
		d.setby = r[0]["setby"];
		d.whenset = from_string<time_t>(r[0]["whenset"], std::dec);
		d.locked = (r[0]["locked"] == "1");
		d.found = true;
	}
	return d;
}

uint64_t get_phrase_count()
{
	db::resultset r = db::query("SELECT COUNT(key_word) AS total FROM infobot", {});
	return r.size() > 0 ? from_string<uint64_t>(r[0]["total"], std::dec) : 0;
}

void set_def(std::string key, const std::string &value, const std::string &word, const std::string &setby, time_t when, bool locked)
{
	key = lowercase(key);
	db::query("INSERT INTO infobot (key_word,value,word,setby,whenset,locked) VALUES ('?','?','?','?','?','?') ON DUPLICATE KEY UPDATE value = '?', word = '?', setby = '?', whenset = '?', locked = '?'",
	{
		key, value, word, setby, std::to_string(when), locked ? "1" : "0",
		value, word, setby, std::to_string(when), locked ? "1" : "0"
	});

}

void del_def(const std::string &key)
{
	db::query("DELETE FROM infobot WHERE key_word = '?'", {key});
}

std::string expand(std::string str, const std::string &nick, time_t timeval, const std::string &mynick, const std::string &randuser)
{
	char timestr[256];
	tm* _tm;
	_tm = gmtime(&timeval);
	strftime(timestr, 255, "%c", _tm);

	str = ReplaceString(str, "<me>", mynick);
	str = ReplaceString(str, "<who>", nick);
	str = ReplaceString(str, "<random>", randuser);
	str = ReplaceString(str, "<date>", std::string(timestr));

	std::vector<std::string> m;
	while (PCRE("<list:(.+?)>", true).Match(str, m)) {
		std::string list = m[1];
		std::string choice = getreply(m[1], ",");
		str = ReplaceString(str, m[0], choice);
	}

	if (str == "%v") {
		return "";
	}
	return str;
}

std::string getreply(std::vector<std::string> v)
{
	auto randIt = v.begin();
	if (v.begin() != v.end()) {
		std::advance(randIt, std::rand() % v.size());
		return *randIt;
	} else {
		return "";
	}
}

std::string getreply(std::string s, const std::string &delim)
{
	size_t pos = 0;
	std::vector<std::string> v;
	std::string token;
	while ((pos = s.find(delim)) != std::string::npos) {
		token = s.substr(0, pos);
		v.push_back(token);
		s.erase(0, pos + delim.length());
	}
	auto randIt = v.begin();
	if (v.begin() != v.end()) {
		std::advance(randIt, std::rand() % v.size());
		return *randIt;
	} else {
		return s;
	}
}

bool locked(const std::string &key)
{
	infodef d = get_def(key);
	return d.locked;
}

