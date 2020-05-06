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

/* Infodef represents a definition from the database */
infodef::infodef() : key(""), value(""), word(""), setby(""), whenset(0), locked(false), found(false) {
}

infodef::~infodef() {
}

infodef get_def(const std::string &key);
uint64_t get_phrase_count();
void set_def(std::string key, const std::string &value, const std::string &word, const std::string &setby, time_t when, bool locked);
std::string expand(std::string str, const std::string &nick, time_t timeval, const std::string &mynick, const std::string &randuser);
void del_def(const std::string &key);
std::string getreply(std::string s, const std::string &delim = "|");
bool locked(const std::string &key);
std::string getreply(std::vector<std::string> v);

/* Reply templates */
std::map<std::string, std::vector<std::string>> replies = {

	/* Positive replies to question: Response found */
	{"replies",  {"I heard %k %w %v", "They say %k %w %v", "%k %w %v... I think", "someone said %k %w %v", "%k %w like, %v", "%k %w %v", "%k %w %v, maybe?", "%s once said %k %w %v"}},

	/* Negative replies to question: No response found */
	{"dontknow", {"Sorry %n I don't know what %k is.", "%k? no idea %n.", "I'm not a genius, %n...", "Its best to ask a real person about %k.", "Not a clue.", "Don't you know, %n?", "If i knew about %k i'd tell you.", "Never heard of %k", "%k isn't something im aware of", "%n, what are you jabbering about, fool?", "%n, i've not got any idea what %k is."}},

	/* Negative replies: Response found, but refusing to overwrite it */
	{"notnew",   {"but %k %w %v :(", "fool, %k %w %v :p", "%k already %w %v...", "Are you sure, %n? I am sure that %k %w %v!", "NO! %k %w %v!!!"}},

	/* Confirmation that the bot has learned a phrase */
	{"confirm",  {"Ok, %n", "Your wish is my command.", "Okay.", "Whatever...", "Gotcha.", "Ok.", "Right.", "If you say so.", "I understand", "Really? OK...", "Understood."}},

	/* Rejection of a new phrase due to the existing phrase being locked */
	{"locked",   {"You don't have the power, %n.", "No, I like that just the way it is.", "You can't edit that! The keyword '%k' has been locked against changes!"}},

	/* Plaintext version of the "who told you about" embed for talkative mode */
	{"heard", {"%s told me about %k on %d", "I learned that on %d, and i think it was %s that told me it.", "I think it was %s who said that, way back on %d...",  "%n: Back on %d, %s told me about %k"}},

	/* Confirmation that the bot has deleted a phrase */
	{"forgot",   {"I forgot %k", "%k is gone from my mind, %n", "As you wish.", "It's history.", "Done.", "%k is no more.", "Consider it gone.", "It's vanished." }}
};

/* Emoji to use in embeds for positive/negative feedback */
std::map<std::string, std::string> emoji = {
	{"replies", ""},
	{"dontknow", "<:wc_rs:667695516737470494>"},
	{"notnew", "<:wc_rs:667695516737470494>"},
	{"confirm", ":white_check_mark:"},
	{"locked", "<:wc_rs:667695516737470494>"},
	{"forgot", ":white_check_mark:"},
	{"heard", ":white_check_mark:"}
};

void copy_to_def(const infodef &source, infodef &dest)
{
	dest.found = source.found;
	dest.key = source.key;
	dest.value = source.value;
	dest.word = source.word;
	dest.setby = source.setby;
	dest.whenset = source.whenset;
	dest.locked = source.locked;
}

/* Create an embed from a JSON string and send it to a channel */
void InfobotModule::ProcessEmbed(const std::string &embed_json, int64_t channelID)
{
	json embed;
	std::string cleaned_json = embed_json;
	/* Put unicode zero-width spaces in @everyone and @here */
	cleaned_json = ReplaceString(cleaned_json, "@everyone", "@‎everyone");
	cleaned_json = ReplaceString(cleaned_json, "@here", "@‎here");
	aegis::channel* channel = bot->core.find_channel(channelID);
	try {
		/* Remove code markdown from the start and end of the code block if there is any */
		std::string s = ReplaceString(cleaned_json, "```js", "");
		s = ReplaceString(s, "```", "");
		/* Tabs to spaces */
		s = ReplaceString(s, "\t", " ");
		embed = json::parse(s);
	}
	catch (const std::exception &e) {
		if (channel) {
			if (!bot->IsTestMode() || from_string<uint64_t>(Bot::GetConfig("test_server"), std::dec) == channel->get_guild().get_id()) {
				channel->create_message("<:sporks_error:664735896251269130> I can't make an **embed** from this: ```js\n" + cleaned_json + "\n```**Error:** ``" + e.what() + "``");
				bot->sent_messages++;
			}
		}
	}
	if (channel) {
		if (!bot->IsTestMode() || from_string<uint64_t>(Bot::GetConfig("test_server"), std::dec) == channel->get_guild().get_id()) {
			channel->create_message_embed("", embed);
			bot->sent_messages++;
		}
	}
}

/* Make a string safe to send as a JSON literal */
std::string escape_json(const std::string &s) {
	std::ostringstream o;
	for (auto c = s.cbegin(); c != s.cend(); c++) {
		switch (*c) {
		case '"': o << "\\\""; break;
		case '\\': o << "\\\\"; break;
		case '\b': o << "\\b"; break;
		case '\f': o << "\\f"; break;
		case '\n': o << "\\n"; break;
		case '\r': o << "\\r"; break;
		case '\t': o << "\\t"; break;
		default:
			if ('\x00' <= *c && *c <= '\x1f') {
				o << "\\u"
				  << std::hex << std::setw(4) << std::setfill('0') << (int)*c;
			} else {
				o << *c;
			}
		}
	}
	return o.str();
}

/* Send an embed containing one or more fields */
void InfobotModule::EmbedWithFields(const std::string &title, std::map<std::string, std::string> fields, int64_t channelID)
{
	std::string json = "{\"title\":\"" + title + "\",\"color\":16767488,\"fields\":[";
	for (auto v = fields.begin(); v != fields.end(); ++v) {
		json += "{\"name\":\"" + v->first + "\",\"value\":\"" + v->second + "\",\"inline\":true}";
		auto n = v;
		if (++n != fields.end()) {
			json += ",";
		}
	}
	json += "],\"footer\":{\"link\":\"https://sporks.gg/\",\"text\":\"Powered by Sporks!\",\"icon_url\":\"https://www.sporks.gg/images/sporks_2020.png\"}}";
	ProcessEmbed(json, channelID);
}

/* Infobot initialisation */
void InfobotModule::infobot_init()
{
	stats.startup = time(NULL);
	stats.modcount = stats.qcount = 0;
}

/* Remove trailing punctuation from a string, e.g. ?, !, . etc */
std::string removepunct(std::string word)
{
	// $key =~ s/(\.|\,|\!|\?|\s+)$//g;
	while (word.length() && (word.back() == '?' || word.back() == '.' || word.back() == ',' || word.back() == '!' || word.back() == ' ')) {
		word.pop_back();
	}
	return word;
}

/* Process input from discord and produce a response, returning it as a string, or if talkative this function may directly generate an embed and send it */
std::string InfobotModule::infobot_response(std::string mynick, std::string otext, std::string usernick, std::string randuser, int64_t channelID, infodef &def, bool mentioned, bool talkative)
{
	/* Default reply level for the command is NOT_ADDRESSED which doesn't generate any feedback to the user */
	reply_level level = NOT_ADDRESSED;
	/* rpllist contains the name of the reply list to get the reply template from (see `replies` above) */
	std::string rpllist = "";
	infodef reply;
	/* Regex for identifying direct questions, e.g. ends in '?' */
	bool direct_question = (PCRE("[\\?!]$").Match(otext));
	std::vector<std::string> matches;

	otext = mynick + " " + otext;

	if (PCRE("^(no\\s*" + mynick + "[,: ]+|" + mynick + "[,: ]+|)(.*?)$", true).Match(otext, matches)) {
		std::string address = matches[1];
		std::string text = otext.substr(matches[1].length(), otext.length() - matches[1].length());
		
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
			if (reply.found) {
				/* Bot was mentioned and reply and key are short enough to fit in the embed fields */
				if ((mentioned || talkative) && reply.key.length() + reply.value.length() < 800) {
					char timestamp[255];
					reply.found = false;
					tm _tm;
					gmtime_r(&reply.whenset, &_tm);
					strftime(timestamp, sizeof(timestamp), "%H:%M:%S %d-%b-%Y", &_tm);
					EmbedWithFields("Fact Information", {{"Key", escape_json(reply.key)}, {"Set By", escape_json(reply.setby)},{"Set Date", escape_json(timestamp)}, {"Value", "```" + escape_json(reply.value) + "```"}}, channelID);
					return "";
				} else {
					/* Not mentioned or key+reply too long, return plaintext */
					rpllist = "heard";
				}
			} else {
				reply.key = key;
				rpllist = "dontknow";
			}
		}
		// Forget command
		else if (level == ADDRESSED_BY_NICKNAME && PCRE("^forget (.*?)$", true).Match(text, matches)) {
			std::string key = removepunct(matches[1]);
			reply = get_def(key);
			if (reply.found) {
				if (reply.locked) {
					/* Fact is locked, don't delete it */
					rpllist = "locked";
				} else {
					/* Fact is not locked, delete it and confirm */
					del_def(key);
					rpllist = "forgot";
				}
			} else {
				/* Fact didn't exist */
				reply.key = key;
				rpllist = "dontknow";
			}
		}
		// status command
		else if ((mentioned || talkative) && level >= ADDRESSED_BY_NICKNAME && PCRE("^status\\?*$", true).Match(text)) {
			
			time_t diff = bot->core.uptime() / 1000;
			int seconds = diff % 60;
			diff /= 60;
			int minutes = diff % 60;
			diff /= 60;
			int hours = diff % 24;
			diff /= 24;
			int days = diff;

			ShowStatus(days, hours, minutes, seconds, stats.modcount, stats.qcount, get_phrase_count(), stats.startup, channelID);
			def.found = false;
			return "";
		}
		// Literal command, print out key and value with no parsing
		else if (PCRE("^literal (.*)\\?*$", true).Match(text, matches)) {
			std::string key = removepunct(matches[1]);
			// This bit is a bit different, it bypasses a lot of the parsing for stuff like %n
			reply = get_def(key);
			def.found = true;
			if (reply.found) {
				std::string e = escape_json(reply.value);
				/* If bot is mentioned and key length and reply length short enough, send as a nice embed */
				if ((mentioned || talkative) && e.length() < 1020 && reply.key.length() < 254) {
					/* Send a fancy embed if its not excessively too long */
					EmbedWithFields("Literal Definition", {{"Key", escape_json(reply.key)}, {"Value", "```" + escape_json(reply.value) + "```"}}, channelID);
					return "";
				} else {
					/* Key or reply too long, or bot not mentioned, return plain text */
					return reply.key + " **is** " + reply.value;
				}
				return "";
			} else {
				/* Key not found */
				reply.key = key;
				rpllist = "dontknow";
			}
		}
		// Next option, someone is either adding a new phrase to the bot or editing an old one, a bit trickier...
		else if ((PCRE("^(.*?)\\s+=(is|are|was|arent|aren't|can|can't|cant|will|has|had|r|might|may)=\\s+", true).Match(text, matches) || PCRE("^(.*?)\\s+(is|are|was|arent|aren't|can|can't|cant|will|has|had|r|might|may)\\s+", true).Match(text, matches)) && (rpllist == "")) {
			std::string key = removepunct(matches[1]);
			std::string word = matches[2];
			std::string value = text.substr(matches[0].length(), text.length() - matches[0].length());
			// remove trailing tab/space only
			value.erase(value.find_last_not_of(" \t") + 1);

			if (key == "") {
				def.found = false;
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
				if (PCRE("^also\\s+(.*)$", true).Match(value, matches) || PCRE("^(.*)\\s(as well|too)$", true).Match(value, matches)) {
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
				reply.key = key;
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
			tm _tm;
			gmtime_r(&reply.whenset, &_tm);
			strftime(timestr, 255, "%c", &_tm);

			s_reply = ReplaceString(s_reply, "%k", reply.key);
			s_reply = ReplaceString(s_reply, "%w", reply.word);
			s_reply = ReplaceString(s_reply, "%n", usernick);
			s_reply = ReplaceString(s_reply, "%m", mynick);
			s_reply = ReplaceString(s_reply, "%d", timestr);
			s_reply = ReplaceString(s_reply, "%s", reply.setby);
			s_reply = ReplaceString(s_reply, "%l", reply.locked ? "locked" : "unlocked");

			// Gobble up empty reply
			if (lowercase(reply.value) == "<reply>" && rpllist == "replies") {
				def.found = false;
				return "";
			}

			if (rpllist == "replies" && PCRE("<alias>\\s*(.*)", true).Match(reply.value, matches)) {
				std::string oldkey = reply.key;
				reply.key = matches[1];
				infodef r = get_def(reply.key);
				if (!r.found) {
					/* Broken alias */
					def.found = false;
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

		if (rpllist == "replies" && PCRE("<(reply|action|embed)>\\s*", true).Match(reply.value, matches)) {
			std::string ml_reply = reply.value.substr(matches[0].length(), reply.value.length() - matches[0].length());
			/* Just a <reply>? bog off... */
			if (trim(ml_reply) == "") {
				def.found = false;
				return "";
			}

			if (matches[1] == "embed" && (mentioned || talkative)) {
				ml_reply = expand(ml_reply, usernick, reply.whenset, mynick, randuser);
				ProcessEmbed(ReplaceString(ml_reply, "<embed>", ""), channelID);
				def.found = false;
				return "";
			}

			reply.value = (lowercase(matches[1]) == "action") ? "*" + ml_reply + "*" : ml_reply;

			std::string x = expand(ml_reply, usernick, reply.whenset, mynick, randuser);
			if (x == "%v") {
				def.found = false;
				return "";
			}

			copy_to_def(reply, def);
			return x;
		}

		s_reply = ReplaceString(s_reply, "%v", reply.value);

		if (s_reply == "%v" || s_reply == "") {
			def.found = false;
			return "";
		}

		// If the bot is directly mentioned, we can answer with an embed.
		// Otherwise it's plaintext all the way and it can be discarded if the channel
		// isnt a talkative channel.
		if (mentioned && rpllist != "replies") {
			std::string json = "{\"color\":16767488,\"description\":\"" + emoji[rpllist] + " " + escape_json(s_reply) + "\"}";
			ProcessEmbed(json, channelID);
			def.found = false;
			return "";
		}

		copy_to_def(reply, def);
		return s_reply;
	}
	def.found = false;
	return "";
}

infodef get_def(const std::string &key)
{
	infodef d;
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
	/* Don't use `SELECT COUNT(*)` here. It will take seconds to complete, as opposed to `SHOW TABLE STATUS` which returns near-instantly. */
	db::resultset r = db::query("show table status like '?'", {std::string("infobot")});
	return r.size() > 0 ? from_string<uint64_t>(r[0]["Rows"], std::dec) : 0;
}

void set_def(std::string key, const std::string &value, const std::string &word, const std::string &setby, time_t when, bool locked)
{
	key = lowercase(key);
	db::query("INSERT INTO infobot (key_word,value,word,setby,whenset,locked) VALUES ('?','?','?','?','?','?') ON DUPLICATE KEY UPDATE value = '?', word = '?', setby = '?', whenset = '?', locked = '?'",
	{
		key, value, word, setby, when, locked,
		value, word, setby, when, locked
	});

}

void del_def(const std::string &key)
{
	db::query("DELETE FROM infobot WHERE key_word = '?'", {key});
}

std::string expand(std::string str, const std::string &nick, time_t timeval, const std::string &mynick, const std::string &randuser)
{
	char timestr[256];
	char currentstr[256];
	tm _tm;
	time_t now = time(NULL);
	gmtime_r(&timeval, &_tm);
	strftime(timestr, 255, "%c", &_tm);
	strftime(currentstr, 255, "%c", localtime(&now));


	str = ReplaceString(str, "<me>", mynick);
	str = ReplaceString(str, "<who>", nick);
	str = ReplaceString(str, "<random>", randuser);
	str = ReplaceString(str, "<date>", std::string(timestr));
	str = ReplaceString(str, "<now>", std::string(currentstr));

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

