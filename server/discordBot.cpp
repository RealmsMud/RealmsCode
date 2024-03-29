/*
 * DiscordBot.cpp
 *   DiscordBot Integration
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include <dpp/dpp.h>                           // for dpp, cluster
#include <dpp/commandhandler.h>                // for commandHandler

#include <fmt/format.h>                        // for format
#include <regex>
#include <iostream>                            // for operator<<, basic_ostream
#include <map>                                 // for operator==, _Rb_tree_i...
#include <stdexcept>                           // for out_of_range
#include <string>                              // for string, allocator, cha...
#include <unordered_map>                       // for _Node_iterator, operat...

#include "color.hpp"                           // for escapeColor
#include "communication.hpp"                   // for getChannelByDiscordCha...
#include "config.hpp"                          // for Config, gConfig, Disco...
#include "flags.hpp"                           // for P_DM_INVIS
#include "mudObjects/players.hpp"              // for Player
#include "server.hpp"                          // for Server, PlayerMap, gSe...
#include "socket.hpp"                          // for Socket
#include "toNum.hpp"

void Config::clearWebhookTokens() {
    webhookTokens.clear();
}

std::string getUsername(dpp::guild *guild, const dpp::user& author) {
    const auto member = guild->members.find(author.id);
    return ((member != guild->members.end() && !member->second.nickname.empty()) ? member->second.nickname : author.username);
}


std::string getWho() {
    bool found = false;
    std::ostringstream whoStr;

    whoStr << "```\n";
    auto cmp = [](const std::shared_ptr<Player> a, const std::shared_ptr<Player> b) { return a->getSock()->getHostname() < b->getSock()->getHostname(); };
    std::multiset<std::shared_ptr<Player>, decltype(cmp)> sortedPlayers;
    for(const auto& [pId, ply] : gServer->players) sortedPlayers.insert(ply);

    for(const auto& player : sortedPlayers) {
        if(!player->isConnected()) continue;
        if(player->flagIsSet(P_DM_INVIS)) continue;
        if(player->isEffected("incognito")) continue;

        found = true;

        whoStr << player->getWhoString(false, false, false);
    }
    if(!found)
        return std::string("Nobody found!\n");
    else {
        whoStr << "```\n";
        return whoStr.str();
    }

}

bool Server::initDiscordBot() {
    if (!gConfig->isBotEnabled()) {
        std::cout << "Discord bot NOT enabled" << std::endl;
        return (false);
    }

    std::cout << "Initializing Discord bot" << std::endl;
    discordBot = new dpp::cluster(gConfig->getBotToken(), dpp::i_message_content | dpp::i_default_intents | dpp::i_guild_members | dpp::i_default_intents);

    /* Create command handler, and specify prefixes */
    commandHandler = new dpp::commandhandler (discordBot);

    /* Specifying a prefix of "/" tells the command handler it should also expect slash commands */
    commandHandler->add_prefix(".").add_prefix("/");

    discordBot->on_ready([bot=this->discordBot, command_handler=this->commandHandler](const dpp::ready_t & event) {
        std::cout << "Logged in as " << bot->me.username << "!\n";
        command_handler->add_command(
                /* Command name */
                "who",
                /* Parameters */
                {},
                /* Command handler */
                [command_handler](const std::string& command, const dpp::parameter_list_t& parameters, dpp::command_source src) {
                    command_handler->reply(dpp::message().add_embed((dpp::embed().set_title("Players currently online").set_description(getWho()))), src);
                },
                /* Command description */
                "Get a list of who is currently logged into the mud."
        );
    });

    discordBot->on_message_create([bot=this->discordBot](const dpp::message_create_t & event) {
        if (event.msg.author.is_bot()) {
            // Don't respond to bot messages
            return;
        }

        channelPtr chan = getChannelByDiscordChannel(event.msg.channel_id);
        auto guild = dpp::find_guild(event.msg.guild_id);
        if (chan) {
            // Second: See if we have a channel
            const std::string username = getUsername(guild, event.msg.author) + " [Discord]";
            std::ostringstream contentStr;

            if (!event.msg.content.empty()) {
                const auto &content = event.msg.content;
                std::unordered_map<dpp::snowflake, std::string> userMentions;
                for (auto &[user, guildMember]: event.msg.mentions) {
                    const auto &mentionName = !guildMember.nickname.empty() ? guildMember.nickname : user.username;
                    userMentions.emplace(guildMember.user_id, mentionName);
                }

                std::regex re(R"(\<([^\>]*)\>)");
                std::string::const_iterator it = content.cbegin(), end = content.cend();
                for (std::smatch match; std::regex_search(it, end, match, re); it = match[0].second) {
                    contentStr << match.prefix();
                    if (match.size() == 2) {
                        const auto matchedId = match[1].str();
                        std::string::const_iterator idIt = matchedId.cbegin(), idEnd = matchedId.cend();
                        for (; idIt != idEnd; idIt++) {
                            if (std::isdigit(*idIt)) {
                                break;
                            }
                        }
                        std::string_view idType(matchedId.cbegin(), idIt), idVal(idIt, idEnd);
                        auto mentionId = toNumSV<uint64_t>(idVal);
                        if (idType == "@" || idType == "@!") {
                            // Finding a user
                            const auto &userMention = userMentions.find(mentionId);
                            contentStr << "@" << ((userMention != userMentions.end()) ? userMention->second : "@<Unknown User>");

                        } else if (idType == "@&") {
                            // Finding a role
                            auto role = dpp::find_role(mentionId);
                            contentStr << "@" << ((role != nullptr) ? role->name : "@<Unknown Role>");

                        } else if (idType == "#") {
                            // Finding a Channel
                            auto channel = dpp::find_channel(mentionId);
                            contentStr << "#" << ((channel != nullptr) ? channel->name : "#<Unknown Channel>");
                        }

                    } else {
                        contentStr << match.str();
                    }
                }
                contentStr << std::string_view(it, end);
            }

            if (!event.msg.attachments.empty())
                for (auto& attachment : event.msg.attachments)
                    contentStr << "<" << attachment.url << ">";

            if (!event.msg.embeds.empty())
                for (auto& embed: event.msg.embeds)
                    contentStr << "<" << embed.url << ">";

            sendGlobalComm(nullptr, escapeColor(contentStr.str()), "", 0, chan, "", username, username);
        } else {
            // Third? Generic fallback... or do nothing
            std::cout << "Got msg " << event.msg.content << " in channel " << event.msg.channel_id << " from "
                      << event.msg.author.username << " isBot: " << event.msg.author.is_bot() << std::endl;
            if (event.msg.content == "!ping") {
                bot->message_create(dpp::message(event.msg.channel_id, "Pong!"));
            }
        }
    });
    discordBot->start(true);

    return (true);
}


void Server::cleanupDiscordBot() {
    delete commandHandler;
    delete discordBot;
}


bool Server::sendDiscordWebhook(long webhookID, int type, const std::string &author, const std::string &msg) {
    if(!discordBot)
        return(false);

    // Send a message from the mud, to a discord channel
    dpp::message myMessage;
    if (type == COM_EMOTE) {
        myMessage = dpp::message("*** " + author + " " + msg);
    } else {
        /*
         * TODO: We allow mentioning users, however for this to actually work we'll need to parse out any @<name>'s, look up the user id, and replace it with @<user_id>
         */
        myMessage = dpp::message(msg).set_allowed_mentions(true, false, false, false, {}, {});

        if(!author.empty()) {
            myMessage.author.username = author;
        }
    }

    auto wh = dpp::webhook();
    try {
        wh.token = gConfig->getWebhookToken(webhookID);
    } catch (std::out_of_range &e) {
        std::cout << "Unable to find token for webhook id " << webhookID << std::endl;
        return false;
    }
    wh.id = webhookID;


    discordBot->execute_webhook(wh, myMessage);
    return true;
}
