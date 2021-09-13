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
#include <dpp/dpp.h>
#include "fmt/core.h"
#include "random.hpp"           // for Random
#include "config.hpp"           // for Config
#include "communication.hpp"    // for COM_EMOTE
#include "server.hpp"           // for Server, MonsterList
#include <boost/algorithm/string/replace.hpp>

void Config::clearWebhookTokens() {
    webhookTokens.clear();
}

std::string getUsername(dpp::guild *guild, dpp::user *author) {
    const auto member = guild->members.find(author->id);
    return ((member != guild->members.end() && !member->second.nickname.empty()) ? member->second.nickname : author->username);
}

bool Server::initDiscordBot() {
    if (!gConfig->isBotEnabled()) {
        std::cout << "Discord bot NOT enabled" << std::endl;
        return (false);
    }

    std::cout << "Initializing Discord bot" << std::endl;
    discordBot = new dpp::cluster(gConfig->getBotToken());

    discordBot->on_ready([bot=this->discordBot](const dpp::ready_t & event) {
        std::cout << "Logged in as " << bot->me.username << "!\n";
    });
    discordBot->on_message_create([bot=this->discordBot](const dpp::message_create_t & event) {
        if (event.msg->author->is_bot()) {
            // Don't respond to bot messages
            return;
        }
        // TODO: First handle slash commands

        channelPtr chan = getChannelByDiscordChannel(event.msg->channel_id);
        auto guild = dpp::find_guild(event.msg->guild_id);
        if (chan) {
            // Second: See if we have a channel
            const std::string username = getUsername(guild, event.msg->author) + " [Discord]";
            std::ostringstream contentStr;

            if (!event.msg->content.empty()) {
                if (!event.msg->mentions.empty()) {
                    // Replace all mentions with the actual users
                    std::string content = event.msg->content;
                    for (auto& mention : event.msg->mentions) {
                        const auto& mentionedUser = dpp::find_user(mention);
                        boost::replace_all(content, fmt::format("<@!{}>", mention), fmt::format("@{}", getUsername(guild, mentionedUser)));
                    }
                    contentStr << content;
                } else {
                    contentStr << event.msg->content;
                }
            }

            if (!event.msg->attachments.empty())
                for (auto& attachment : event.msg->attachments)
                    contentStr << " " << attachment.url;

            if (!event.msg->embeds.empty())
                for (auto& embed: event.msg->embeds)
                    contentStr << " " << embed.url;

            sendGlobalComm(nullptr, contentStr.str(), "", 0, chan, "", username, username);
        } else {
            // Third? Generic fallback... or do nothing
            std::cout << "Got msg " << event.msg->content << " in channel " << event.msg->channel_id << " from "
                      << event.msg->author->username << " isBot: " << event.msg->author->is_bot() << std::endl;
            if (event.msg->content == "!ping") {
                bot->message_create(dpp::message(event.msg->channel_id, "Pong!"));
            }
        }
    });
    discordBot->start(true);

    return (true);
}

bool Server::sendDiscordWebhook(long webhookID, int type, const bstring &author, const bstring &msg) {
    // Send a message from the mud, to a discord channel
    dpp::message myMessage;
    if (type == COM_EMOTE) {
        myMessage = dpp::message("*** " + author + " " + msg);
    } else {
        /*
         * TODO: We allow mentioning users, however for this to actually work we'll need to parse out any @<name>'s, look up the user id, and replace it with @<user_id>
         */
        myMessage = dpp::message(msg).set_allowed_mentions(true, false, false, false, {}, {});

        auto myAuthor = new dpp::user();
        myAuthor->username = author;
        myMessage.author = myAuthor;
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
