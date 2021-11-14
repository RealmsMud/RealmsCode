/*
 * config-xml.cpp
 *   Handles config options for the entire mud - xml files
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

#include <fmt/format.h>                             // for format
#include <libxml/parser.h>                          // for xmlFreeDoc, xmlNe...
#include <sys/types.h>                              // for time_t
#include <boost/lexical_cast/bad_lexical_cast.hpp>  // for bad_lexical_cast
#include <libxml/xmlstring.h>                       // for BAD_CAST
#include <list>                                     // for list, operator==
#include <map>                                      // for operator==, _Rb_t...
#include <ostream>                                  // for basic_ostream::op...
#include <string>                                   // for allocator, basic_...

#include "config.hpp"                               // for Config, DiscordTo...
#include "paths.hpp"                                // for Config
#include "proto.hpp"                                // for file_exists
#include "xml.hpp"                                  // for NODE_NAME, copyTo...

bool Config::loadConfig(bool reload) {
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;
    xmlNodePtr  curNode;

    auto filename = fmt::format( "{}/config.xml", Path::Config);

    if(!file_exists(filename.c_str()))
        return(false);

    if((xmlDoc = xml::loadFile(filename.c_str(), "Config")) == nullptr)
        return(false);

    rootNode = xmlDocGetRootElement(xmlDoc);
    curNode = rootNode->children;

    // Reset current config
    reset(reload);
    while(curNode) {
        if(NODE_NAME(curNode, "General")) loadGeneral(curNode);
        else if(NODE_NAME(curNode, "Lottery")) loadLottery(curNode);

        curNode = curNode->next;
    }

    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();

    return(true);
}

bool Config::loadDiscordConfig() {
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;
    xmlNodePtr  curNode;

    auto filename = fmt::format("{}/discord.xml", Path::Config);

    if(!file_exists(filename.c_str()))
        return(false);

    if((xmlDoc = xml::loadFile(filename.c_str(), "Discord")) == nullptr)
        return(false);

    rootNode = xmlDocGetRootElement(xmlDoc);
    curNode = rootNode->children;

    while(curNode) {
        if(NODE_NAME(curNode, "BotToken")) {
            xml::copyToString(botToken, curNode);
            botEnabled = true;
        } else if(NODE_NAME(curNode, "WebhookTokens")) {
            loadWebhookTokens(curNode);
        }

        curNode = curNode->next;
    }

    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();

    return(true);
}

void Config::loadGeneral(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;

    while(curNode) {
        if(NODE_NAME(curNode, "AutoShutdown")) xml::copyToBool(autoShutdown, curNode);
        else if(NODE_NAME(curNode, "AprilFools")) xml::copyToBool(doAprilFools, curNode);
        else if(NODE_NAME(curNode, "FlashPolicyPort")) xml::copyToNum(flashPolicyPort, curNode);
        else if(NODE_NAME(curNode, "CharCreationDisabled")) xml::copyToBool(charCreationDisabled, curNode);
        else if(NODE_NAME(curNode, "CheckDouble")) xml::copyToBool(checkDouble, curNode);
        else if(NODE_NAME(curNode, "GetHostByName")) xml::copyToBool(getHostByName, curNode);
        else if(NODE_NAME(curNode, "LessExpLoss")) xml::copyToBool(lessExpLoss, curNode);
        else if(NODE_NAME(curNode, "LogDatabaseType")) xml::copyToString(logDbType, curNode);
        else if(NODE_NAME(curNode, "LogDatabaseUser")) xml::copyToString(logDbUser, curNode);
        else if(NODE_NAME(curNode, "LogDatabasePassword")) xml::copyToString(logDbPass, curNode);
        else if(NODE_NAME(curNode, "LogDatabaseDatabase")) xml::copyToString(logDbDatabase, curNode);
        else if(NODE_NAME(curNode, "LogDeath")) xml::copyToBool(logDeath, curNode);
        else if(NODE_NAME(curNode, "PkillInCombatDisabled")) xml::copyToBool(pkillInCombatDisabled, curNode);
        else if(NODE_NAME(curNode, "RecordAll")) xml::copyToBool(recordAll, curNode);
        else if(NODE_NAME(curNode, "LogSuicide")) xml::copyToBool(logSuicide, curNode);
        else if(NODE_NAME(curNode, "SaveOnDrop")) xml::copyToBool(saveOnDrop, curNode);
            //else if(NODE_NAME(curNode, "MaxGuild")) xml::copyToNum(maxGuilds, curNode);
        else if(NODE_NAME(curNode, "DmPass")) xml::copyToString(dmPass, curNode);
        else if(NODE_NAME(curNode, "MudName")) xml::copyToString(mudName, curNode);
        else if(NODE_NAME(curNode, "Webserver")) xml::copyToString(webserver, curNode);
        else if(NODE_NAME(curNode, "QS")) {
            xml::copyToString(qs, curNode);
            std::clog << "Loaded QS: " << qs << std::endl;
        }
        else if(NODE_NAME(curNode, "UserAgent")) xml::copyToString(userAgent, curNode);
        else if(NODE_NAME(curNode, "Reviewer")) xml::copyToString(reviewer, curNode);
        else if(NODE_NAME(curNode, "ShopNumObjects")) xml::copyToNum(shopNumObjects, curNode);
        else if(NODE_NAME(curNode, "ShopNumLines")) xml::copyToNum(shopNumLines, curNode);
        else if(NODE_NAME(curNode, "CustomColors")) xml::copyToCString(customColors, curNode);
        else if(!bHavePort && NODE_NAME(curNode, "Port")) xml::copyToNum(portNum, curNode);

        curNode = curNode->next;
    }
}


void Config::loadLottery(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;

    while(curNode) {
        if(NODE_NAME(curNode, "Enabled")) xml::copyToBool(lotteryEnabled, curNode);
        else if(NODE_NAME(curNode, "curNoderentCycle")) xml::copyToNum(lotteryCycle, curNode);
        else if(NODE_NAME(curNode, "curNoderentJackpot")) xml::copyToNum(lotteryJackpot, curNode);
        else if(NODE_NAME(curNode, "TicketPrice")) xml::copyToNum(lotteryTicketPrice, curNode);
        else if(NODE_NAME(curNode, "LotteryWon")) xml::copyToBool(lotteryWon, curNode);
        else if(NODE_NAME(curNode, "TicketsSold")) xml::copyToNum(lotteryTicketsSold, curNode);
        else if(NODE_NAME(curNode, "WinningsThisCycle")) xml::copyToNum(lotteryWinnings, curNode);
        else if(NODE_NAME(curNode, "LotteryRunTime")) xml::copyToNum(lotteryRunTime, curNode);

        else if(NODE_NAME(curNode, "WinningNumbers")) {
            xml::loadNumArray<short>(curNode, lotteryNumbers, "LotteryNum", 6);
        }
        else if(NODE_NAME(curNode, "Tickets")) {
            loadTickets(curNode);
        }

        curNode = curNode->next;
    }
}

void Config::loadTickets(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;
    LottoTicket* ticket=nullptr;

    while(curNode) {
        if(NODE_NAME(curNode, "Ticket")) {
            if((ticket = new LottoTicket(curNode)) != nullptr) {
                tickets.push_back(ticket);
            }
        }

        curNode = curNode->next;
    }
}

void Config::loadWebhookTokens(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;
    while(curNode) {
        if(NODE_NAME(curNode, "WebhookToken")) {
            xmlNodePtr tokenNode = curNode->children;
            long webhookID = -1;
            std::string token;
            while(tokenNode) {
                if(NODE_NAME(tokenNode, "ID")) xml::copyToNum(webhookID, tokenNode);
                else if(NODE_NAME(tokenNode, "Token")) xml::copyToString(token, tokenNode);

                tokenNode = tokenNode->next;
            }
            if (webhookID != -1 && !token.empty()) {
                webhookTokens.insert({webhookID, token});
            }
        }

        curNode = curNode->next;
    }
}

// Functions to get configured options
bool Config::saveConfig() const {
    xmlDocPtr   xmlDoc;
    xmlNodePtr      rootNode, curNode;

    xmlDoc = xmlNewDoc(BAD_CAST "1.0");
    rootNode = xmlNewDocNode(xmlDoc, nullptr, BAD_CAST "Config", nullptr);
    xmlDocSetRootElement(xmlDoc, rootNode);

    // Make general section
    curNode = xmlNewChild(rootNode, nullptr, BAD_CAST "General", nullptr);
    if(!bHavePort)
        xml::saveNonZeroNum(curNode, "Port", portNum);

    xml::saveNonNullString(curNode, "MudName", mudName);
    xml::saveNonNullString(curNode, "DmPass", dmPass);
    xml::saveNonNullString(curNode, "Webserver", webserver);
    xml::saveNonNullString(curNode, "QS", qs);
    xml::saveNonNullString(curNode, "UserAgent", userAgent);
    xml::saveNonNullString(curNode, "CustomColors", customColors);
    xml::saveNonNullString(curNode, "Reviewer", reviewer);

    xml::newStringChild(curNode, "LogDatabaseType", logDbType);
    xml::newStringChild(curNode, "LogDatabaseUser", logDbUser);
    xml::newStringChild(curNode, "LogDatabasePassword", logDbPass);
    xml::newStringChild(curNode, "LogDatabaseDatabase", logDbDatabase);

    xml::newBoolChild(curNode, "AprilFools", doAprilFools);
    xml::saveNonZeroNum(curNode, "FlashPolicyPort", flashPolicyPort);
    xml::newBoolChild(curNode, "AutoShutdown", autoShutdown);
    xml::newBoolChild(curNode, "CharCreationDisabled", charCreationDisabled);
    xml::newBoolChild(curNode, "CheckDouble", checkDouble);
    xml::newBoolChild(curNode, "GetHostByName", getHostByName);
    xml::newBoolChild(curNode, "LessExpLoss", lessExpLoss);
    xml::newBoolChild(curNode, "LogDeath", logDeath);
    xml::newBoolChild(curNode, "PkillInCombatDisabled", pkillInCombatDisabled);
    xml::newBoolChild(curNode, "RecordAll", recordAll);
    xml::newBoolChild(curNode, "LogSuicide", logSuicide);
    xml::newBoolChild(curNode, "SaveOnDrop", saveOnDrop);
    //xml::newBoolChild(curNode, "MaxGuild", maxGuilds);

    xml::saveNonZeroNum(curNode, "ShopNumObjects", shopNumObjects);
    xml::saveNonZeroNum(curNode, "ShopNumLines", shopNumLines);

    // Lottery Section
    curNode = xmlNewChild(rootNode, nullptr, BAD_CAST "Lottery", nullptr);
    xml::saveNonZeroNum(curNode, "CurrentCycle", lotteryCycle);
    xml::saveNonZeroNum(curNode, "CurrentJackpot", lotteryJackpot);
    xml::newBoolChild(curNode, "Enabled", BAD_CAST iToYesNo(lotteryEnabled));
    xml::saveNonZeroNum(curNode, "TicketPrice", lotteryTicketPrice);
    xml::saveNonZeroNum(curNode, "TicketsSold", lotteryTicketsSold);
    xml::saveNonZeroNum(curNode, "LotteryWon", lotteryWon);
    xml::saveNonZeroNum(curNode, "WinningsThisCycle", lotteryWinnings);
    xml::saveNonZeroNum(curNode, "LotteryRunTime", lotteryRunTime);
    saveShortIntArray(curNode, "WinningNumbers", "LotteryNum", lotteryNumbers, 6);
    xmlNodePtr ticketsNode = xml::newStringChild(curNode, "Tickets");
    for(LottoTicket* ticket : tickets) {
        ticket->saveToXml(ticketsNode);
    }

    // Discord Config
    curNode = xmlNewChild(rootNode, nullptr, BAD_CAST "Discord", nullptr);
    xml::saveNonNullString(curNode, "BotToken", botToken);
    xmlNodePtr webtokenNodes = xml::newStringChild(curNode, "WebhookTokens");
    for(const auto& [webhookID, token] : webhookTokens) {
        xmlNodePtr tokenNode = xml::newStringChild(webtokenNodes, "WebhookToken");
        xml::newNumChild<long>(tokenNode, "ID", webhookID);
        xml::newStringChild(tokenNode, "Token", token);
    }

    xml::saveFile(fmt::format("{}/config.xml", Path::Config).c_str(), xmlDoc);
    xmlFreeDoc(xmlDoc);

    return(true);
}
