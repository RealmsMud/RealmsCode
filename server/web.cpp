/*
 * web.cpp
 *   web-based functions
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

#include <fcntl.h>                               // for open, O_NONBLOCK
#include <fmt/format.h>                          // for format
#include <libxml/parser.h>                       // for xmlDocSetRootElement
#include <libxml/xmlstring.h>                    // for BAD_CAST
#include <sys/stat.h>                            // for stat, mkfifo, S_IFIFO
#include <unistd.h>                              // for unlink, close, fork
#include <algorithm>                             // for replace
#include <boost/algorithm/string/case_conv.hpp>  // for to_lower, to_lower_copy
#include <boost/algorithm/string/replace.hpp>    // for replace_all
#include <boost/algorithm/string/trim.hpp>       // for trim
#include <boost/iterator/iterator_facade.hpp>    // for operator!=
#include <boost/iterator/iterator_traits.hpp>    // for iterator_value<>::type
#include <cerrno>                                // for errno
#include <cstdio>                                // for snprintf, sprintf
#include <cstdlib>                               // for exit, free, system
#include <cstring>                               // for strcat, strerror
#include <ctime>                                 // for time, ctime, timespec
#include <deque>                                 // for _Deque_iterator
#include <list>                                  // for operator==, list
#include <map>                                   // for operator==, _Rb_tree...
#include <ostream>                               // for operator<<, basic_os...
#include <set>                                   // for set
#include <stdexcept>                             // for runtime_error
#include <string>                                // for string, allocator
#include <string_view>                           // for operator==, string_view
#include <utility>                               // for pair

#include "catRef.hpp"                            // for CatRef
#include "clans.hpp"                             // for Clan
#include "cmd.hpp"                               // for cmd
#include "commands.hpp"                          // for getFullstrText, doFi...
#include "config.hpp"                            // for Config, gConfig, Eff...
#include "creatureStreams.hpp"                   // for Streamable
#include "deityData.hpp"                         // for DeityData
#include "effects.hpp"                           // for Effect
#include "enums/loadType.hpp"                    // for LoadType, LoadType::...
#include "flags.hpp"                             // for P_CHAOTIC, P_DM_INVIS
#include "global.hpp"                            // for ALLITEMS, CreatureClass
#include "guilds.hpp"                            // for Guild
#include "mud.hpp"                               // for GUILD_PEON
#include "mudObjects/container.hpp"              // for ObjectSet
#include "mudObjects/monsters.hpp"               // for Monster
#include "mudObjects/objects.hpp"                // for Object
#include "mudObjects/players.hpp"                // for Player
#include "mudObjects/rooms.hpp"                  // for BaseRoom
#include "mudObjects/uniqueRooms.hpp"            // for UniqueRoom
#include "paths.hpp"                             // for Game, Wiki
#include "proto.hpp"                             // for broadcast, up, isDm
#include "raceData.hpp"                          // for RaceData
#include "server.hpp"                            // for Server, gServer, Mon...
#include "socket.hpp"                            // for Socket, xmlNode, xml...
#include "web.hpp"                               // for WebInterface, callWe...
#include "xml.hpp"                               // for copyToString, loadPl...


int lastmod = 0;
struct stat lm_check;


// Static initialization
const char* WebInterface::fifoIn = "webInterface.in";
const char* WebInterface::fifoOut = "webInterface.out";
WebInterface* WebInterface::myInstance = nullptr;

static char ETX = 3;
static char EOT = 4;
//static char ETB   = 23;

static char itemDelim = 6;
static char innerDelim = 7;
static char startSubDelim = 8;
static char endSubDelim = 9;
static char equipDelim = 10;



//*********************************************************************
//                      updateRecentActivity
//*********************************************************************

void updateRecentActivity() {
    callWebserver((std::string)"mud.php?type=recent", true, true);
}

//*********************************************************************
//                      latestPost
//*********************************************************************

void latestPost(std::string_view view, std::string_view subject, std::string_view username, std::string_view boardname, std::string_view post) {

    if(view.empty() || boardname.empty() || username.empty() || post.empty())
        return;

    for(Socket &sock : gServer->sockets) {
        const std::shared_ptr<Player> player = sock.getPlayer();

        if(!player || player->fd < 0)
            continue;
        if(player->flagIsSet(P_HIDE_FORUM_POSTS))
            continue;

        if(view == "dungeonmaster" && !player->isDm())
            continue;
        if(view == "caretaker" && !player->isCt())
            continue;
        if(view == "watcher" && !player->isWatcher())
            continue;
        if(view == "builder" && !player->isStaff())
            continue;

        bool fullMode = player->flagIsSet(P_FULL_FORUM_POSTS);
        if(player->inCombat())
            fullMode = false;

        if(fullMode)
            player->bPrint(post);
        else
            player->bPrint(fmt::format("^C==>^x There is a new post titled ^W\"{}\"^x by ^W{}^x in the ^W{}^x board.\n", subject, username, boardname));
    }
}

//*********************************************************************
//                      WebInterface
//*********************************************************************
// Verify the in/out fifos exist and open the in fifo in non-blocking mode

WebInterface::WebInterface() {
    openFifos();
}
WebInterface::~WebInterface() {
    closeFifos();
}

//*********************************************************************
//                      open
//*********************************************************************

void WebInterface::openFifos() {
    checkFifo(fifoIn);
    checkFifo(fifoOut);

    char filename[80];
    snprintf(filename, 80, "%s/%s", Path::Game.c_str(), fifoIn);
    inFd = open(filename, O_RDONLY|O_NONBLOCK);
    if(inFd == -1)
        throw(std::runtime_error("WebInterface: Unable to open " + std::string(filename) + ":" +strerror(errno)));

    outFd = -1;

    std::clog << "WebInterface: monitoring " << fifoIn << std::endl;
}

//*********************************************************************
//                      close
//*********************************************************************

void WebInterface::closeFifos() {
    //  unlink(gifo);
    close(outFd);
    close(inFd);
}

//*********************************************************************
//                      initWebInterface
//*********************************************************************

bool Server::initWebInterface() {
    // Do a check for main port, hostname, etc here
    webInterface = WebInterface::getInstance();
    return(true);
}

//*********************************************************************
//                      checkWebInterface
//*********************************************************************

bool Server::checkWebInterface() {
    if(!webInterface)
        return(false);
    return(webInterface->handleRequests());
}

//*********************************************************************
//                      recreateFifos
//*********************************************************************

void Server::recreateFifos() {
    if(webInterface)
        webInterface->recreateFifos();
}

void WebInterface::recreateFifos() {
    char filename[80];
    closeFifos();

    snprintf(filename, 80, "%s/%s.xml", Path::Game.c_str(), fifoIn);
    unlink(filename);

    snprintf(filename, 80, "%s/%s.xml", Path::Game.c_str(), fifoOut);
    unlink(filename);

    openFifos();
}

//*********************************************************************
//                      checkFifo
//*********************************************************************
// Check that the given fifo exists as a fifo. If it is a regular file, erase it.
// If it doesn't exist, create it.

bool WebInterface::checkFifo(const char* fifoFile) {
    struct stat statInfo{};
    int retVal;
    bool needToCreate = false;

    char filename[80];
    snprintf(filename, 80, "%s/%s", Path::Game.c_str(), fifoFile);
    retVal = stat(filename, &statInfo);
    if(retVal == 0) {
        if((statInfo.st_mode & S_IFMT) != S_IFIFO) {
            if(unlink(filename) != 0)
                throw std::runtime_error("WebInterface: Unable to unlink " + std::string(fifoFile) + ":" + strerror(errno));
            needToCreate = true;
        }
    } else {
        needToCreate = true;
    }

    if(needToCreate) {
        retVal = mkfifo(filename, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
        if(retVal != 0)
            throw std::runtime_error("WebInterface: Unable to mkfifo " + std::string(fifoFile) + ":" + strerror(errno));
    }
    return(true);
}

//*********************************************************************
//                      getInstance
//*********************************************************************

WebInterface* WebInterface::getInstance() {
    if(myInstance == nullptr)
        myInstance = new WebInterface;
    return(myInstance);
}

//*********************************************************************
//                      destroyInstance
//*********************************************************************

void WebInterface::destroyInstance() {
    delete myInstance;
    myInstance = nullptr;
}

//*********************************************************************
//                      handleRequests
//*********************************************************************

bool WebInterface::handleRequests() {
    checkInput();
    handleInput();

    sendOutput();
    return(true);
}

//*********************************************************************
//                      checkInput
//*********************************************************************

bool WebInterface::checkInput() {
    if(inFd == -1)
        return(false);

    char tmpBuf[1024];
    int n, total = 0;
    do {
        // Attempt to read from the socket
        n = read(inFd, tmpBuf, 1023);
        if(n <= 0) {
            //std::clog << "WebInterface: Unable to read - " << strerror(errno) << "\n";
            break;
        }
        tmpBuf[n] = '\0';
        total += n;
        inBuf += tmpBuf;

    } while(n > 0);

    std::replace(inBuf.begin(), inBuf.end(), '\r', '\n');
    if(total == 0)
        return(false);

    return(true);
}

//*********************************************************************
//                      handleInput
//*********************************************************************

bool WebInterface::messagePlayer(std::string command, std::string tempBuf) {
    // MSG / SYSMSG = system sending
    // TXT = user sending
    std::string::size_type pos;

    // we don't know the command yet
    if(command.empty()) {
        pos = tempBuf.find(' ');
        if(pos == std::string::npos)
            command = tempBuf;
        else
            command = tempBuf.substr(0, pos);
    }

    pos = tempBuf.find(' ');
    if(pos == std::string::npos) {
        std::clog << "WebInterface: Messaging failed; not enough data!" << std::endl;
        return(false);
    }

    std::string user = tempBuf.substr(0, pos);
    boost::to_lower(user);
    user.at(0) = up(user.at(0));

    tempBuf.erase(0, pos+1); // Clear out the user
    boost::trim(tempBuf);

    std::clog << "WebInterface: Messaging user " << user << std::endl;
    const std::shared_ptr<Player> player = gServer->findPlayer(user);
    if(!player) {
        outBuf += "That player is not logged on.";
    } else {
        bool txt = (command == "TXT");
        if(txt && (
            player->getClass() == CreatureClass::BUILDER ||
            player->flagIsSet(P_IGNORE_ALL) ||
            player->flagIsSet(P_IGNORE_SMS)
        )) {
            std::clog << "WebInterface: Messaging failed; user does not want text messages." << std::endl;
            return(false);
        }
        if(command != "SYSMSG")
            player->printColor("^C==> ");
        if(!txt) {
            boost::replace_all(tempBuf, "<cr>", "\n\t");
        }
        player->printColor("%s%s%s\n", txt ? "^WText Msg: \"" : "",
            tempBuf.c_str(),
            txt ? "\"" : "");
        if(!player->isDm()) {
            broadcast(watchingEaves, "^E--- %s to %s, \"%s\".",
                txt ? "Text message" : "System message", player->getCName(), tempBuf.c_str());
        }
    }
    outBuf += EOT;
    return(true);
}

//*********************************************************************
//                      getInventory
//*********************************************************************

std::string doGetInventory(const std::shared_ptr<const Player>& player, const ObjectSet &set);

std::string doGetInventory(const std::shared_ptr<const Player>& player, const std::shared_ptr<Object>&  object, int loc=-1) {
    std::ostringstream oStr;
    oStr << itemDelim
         << object->getName()
         << innerDelim
         << object->getId()
         << innerDelim
         << object->getWearflag()
         << innerDelim
         << static_cast<int>(object->getType());

    if(loc != -1)
        oStr << innerDelim << loc;

    if(!object->objects.empty()) {
        oStr << startSubDelim
             << doGetInventory(player, object->objects)
             << endSubDelim;
    }
    return(oStr.str());
}

std::string doGetInventory(const std::shared_ptr<const Player>& player, const ObjectSet &set) {
    std::string inv;
    for(const auto& obj : set ) {
        if(player->canSee(obj))
            inv += doGetInventory(player, obj);
    }
    return(inv);
}

std::string getInventory(const std::shared_ptr<const Player>& player) {
    std::ostringstream oStr;

    oStr << player->getUniqueObjId()
         << doGetInventory(player, player->objects)
         << equipDelim;

    for(int i=0; i<MAXWEAR; i++) {
        if(player->ready[i])
            oStr << doGetInventory(player, player->ready[i], i);
    }

    return(oStr.str());
}

//*********************************************************************
//                      handleInput
//*********************************************************************

bool webWield(std::shared_ptr<Player> player);
bool webWear(std::shared_ptr<Player> player);

bool webUse(const std::shared_ptr<Player>& player, int id, int type) {

    // Disabled
    return(false);
}

//*********************************************************************
//                      handleInput
//*********************************************************************
// Uses ETX (End of Text) as abort and EOT (End of Transmission Block) as EOF

bool WebInterface::handleInput() {
    if(inBuf.empty())
        return(false);

    std::string::size_type start=0, idx, pos;
    bool needData=false;

    // First, see if we have a clear command (ETX), if so erase up to there
    idx = inBuf.find_last_of(ETX);
    if(idx != std::string::npos)
        inBuf.erase(0, idx);

    // Now lets ignore any unprintable characters that might have gotten tacked on to the start
    while(!isPrintable((unsigned char)inBuf[start]))
        start++;
    inBuf.erase(0, start);

    // Now lets look for a command
    idx = inBuf.find('\n', 0);
    if(idx == std::string::npos)
        return(false);

    // Copy the entire command to a temporary buffer and lets see if we can find
    // a valid command
    std::string tempBuf = inBuf.substr(0, idx); // Don't copy the \n
    std::string command; // the command being run
    std::string dataBuf; // the buffer for incoming data

    pos = tempBuf.find(' ');
    if(pos == std::string::npos)
        command = tempBuf;
    else
        command = tempBuf.substr(0, pos);

    idx += 1; // Consume the \n
    if(inBuf[idx] == '\n')
        idx += 1; // Consume the extra \n if applicable

    //const unsigned char* before = (unsigned char*)strdup(inBuf.c_str());

    // certain commands can only be called from the webserver
    if(gConfig->getWebserver().empty()) {
        if( command == "FORUM" ||
            command == "UNFORUM" ||
            command == "AUTOGUILD"
        )
            command = "";
    }

    // If we can jump right in (LOAD), do so.  If we need to wait until we've
    // read in the ETF from the builder, set needData to true
    if(command == "SAVE" || command == "LATESTPOST")
        needData = true;

    if( command == "LOAD" ||
        command == "WHO" ||
        command == "MSG" || command == "SYSMSG" ||
        command == "TXT" ||
        command == "WIKI" ||
        command == "FINGER" || // data requirement is minimal for the following
        command == "WHOIS" ||
        command == "FORUM" ||
        command == "UNFORUM" ||
        command == "AUTOGUILD" ||
        command == "GETINVENTORY" ||
        command == "EQUIP" ||
        command == "UNEQUIP" ||
        command == "EFFECTLIST" ||
        (needData && inBuf.find(EOT, idx) != std::string::npos)
    ) {
        // We've got a valid command so we can erase it out of the input buffer
        inBuf.erase(0, idx);
        //const unsigned char* leftOver = (unsigned char*)strdup(inBuf.c_str());

        // If we need more data, we have the command terminated by a '\n'
        // and then the rest should be the data we need (xml we're supposed to save), terminated
        // by EOT, so copy that into buffer
        if(needData) {
            idx = inBuf.find(EOT, 0);
            // If we've gotten this far, we know the EOT exists, so no need to check the find result
            dataBuf = inBuf.substr(0, idx); // Don't copy the EOT
            inBuf.erase(0, idx+1); // but do erase the EOT
        }

        tempBuf.erase(0, pos+1); // Clear out the command

//      if(command == "GETINVENTORY" || command == "EQUIP" || command == "UNEQUIP") {
//          pos = tempBuf.Find(' ');
//          int id=0, type=0;
//
//          std::string user = "";
//          if(pos != std::string::npos) {
//              user = tempBuf.left(pos);
//              tempBuf.erase(0, pos+1); // Clear out the user
//              id = atoi(tempBuf.c_str());
//
//              if(command == "EQUIP") {
//                  pos = tempBuf.Find(' ');
//                  tempBuf.erase(0, pos+1); // Clear out the user
//                  type = atoi(tempBuf.c_str());
//              }
//          } else {
//              user = tempBuf;
//              tempBuf = "";
//          }
//
//          std::shared_ptr<Player> player = 0;
//          if(user != "")
//              player = gServer->findPlayer(user);
//          if(!player) {
//              outBuf += EOT;
//              return(true);
//          }
//
//          if(command == "GETINVENTORY") {
//
//              outBuf += getInventory(player);
//
//          } else if(command == "EQUIP") {
//
//              outBuf += webUse(player, id, type) ? "1" : "0";
//
//          } else if(command == "UNEQUIP") {
//
//              outBuf += webRemove(player, id) ? "1" : "0";
//
//          }
//
//          outBuf += EOT;
//          return(true);
//      } else
        if(command == "WHO") {
            std::clog << "WebInterface: Checking for users online" << std::endl;
            outBuf += webwho();
            outBuf += EOT;
            return(true);
        }
        else if(command == "EFFECTLIST") {
            const Effect* effect;
            std::ostringstream oStr;
            for(const auto& sp : gConfig->effects) {
                effect = &(sp.second);
                oStr << "Name,Display,OppositeEffect,Type,Pulsed,UsesStrength";
                oStr << effect->getName().c_str() << "," << effect->getDisplay().c_str() << "," <<
                        effect->getType().c_str() << "," << (effect->isPulsed() ? "yes" : "no") << "," <<
                        (effect->usesStrength() ? "Y" : "N") ;
            }
            outBuf += oStr.str();
            outBuf += EOT;
        }
        else if(command == "WHOIS") {
            std::clog << "WebInterface: Whois for user " << tempBuf << std::endl;
            boost::to_lower(tempBuf);
            tempBuf.at(0) = up(tempBuf.at(0));
            const std::shared_ptr<Player> player = gServer->findPlayer(tempBuf);
            if( !player ||
                player->flagIsSet(P_DM_INVIS) ||
                player->isEffected("incognito") ||
                player->isEffected("mist") ||
                player->isInvisible()
            ) {
                outBuf += "That player is not logged on.";
            } else {
                outBuf += player->getWhoString(false, false, false);
            }
            outBuf += EOT;
            return(true);
        } else if(command == "FINGER") {
            std::clog << "WebInterface: Fingering user " << tempBuf << std::endl;
            outBuf += doFinger(nullptr, tempBuf, CreatureClass::NONE);
            outBuf += EOT;
            return(true);
        } else if(command == "WIKI") {
            return(wiki(command, tempBuf));
        } else if(command == "MSG" || command == "SYSMSG" || command == "TXT") {
            return(messagePlayer(command, tempBuf));
        } else if(command == "FORUM") {
            pos = tempBuf.find(' ');
            if(pos == std::string::npos) {
                std::clog << "WebInterface: Forum association failed; not enough data!" << std::endl;
                return(false);
            }

            std::string user = tempBuf.substr(0, pos);

            tempBuf.erase(0, pos+1); // Clear out the user
            std::clog << "WebInterface: Forum association for user " << user << std::endl;

            std::shared_ptr<Player> player = gServer->findPlayer(user);
            if(player) {
                player->setForum(tempBuf);
                player->save(true);
                return(messagePlayer("MSG", user + " Your character has been associated with forum account " + tempBuf + "."));
            }

            // they logged off?
            if(!loadPlayer(user.c_str(), player)) {
                // they were deleted? undo!
                webUnassociate(user);
                return(false);
            }

            player->setForum(tempBuf);
            player->save();
            outBuf += EOT;
            return(true);
        } else if(command == "UNFORUM") {
            std::clog << "WebInterface: Forum unassociation for user " << tempBuf << std::endl;

            if(tempBuf.empty()) {
                std::clog << "WebInterface: Forum unassociation failed; not enough data!" << std::endl;
                return(false);
            }

            boost::to_lower(tempBuf);
            tempBuf.at(0) = up(tempBuf.at(0));

            std::shared_ptr<Player> player = gServer->findPlayer(tempBuf);
            if(player) {
                if(!player->getForum().empty()) {
                    messagePlayer("MSG", tempBuf + " Your character has been unassociated from forum account " + player->getForum() + ".");
                    player->setForum("");
                    player->save(true);
                }
            } else {

                // they logged off?
                if(!loadPlayer(tempBuf.c_str(), player)) {
                    // they were deleted? no problem!
                } else {
                    player->setForum("");
                    player->save();
                }

            }
            outBuf += EOT;
            return(true);
        } else if(command == "AUTOGUILD") {
            if(tempBuf.empty()) {
                std::clog << "WebInterface: Autoguild failed; not enough data!" << std::endl;
                return(false);
            }

            const Guild* guild=nullptr;
            if(tempBuf.length() <= 40)
                guild = gConfig->getGuild(tempBuf);
            if(!guild) {
                std::clog << "WebInterface: Autoguild failed; guild " << tempBuf << " not found!" << std::endl;
                return(false);
            }

            std::list<std::string>::const_iterator it;
            std::shared_ptr<Player> player=nullptr;
            for(it = guild->members.begin() ; it != guild->members.end() ; it++ ) {
                player = gServer->findPlayer(*it);

                if(!player) {
                    if(!loadPlayer((*it).c_str(), player))
                        continue;

                }

                if(!player->getForum().empty())
                    callWebserver((std::string)"mud.php?type=autoguild&guild=" + guild->getName() + "&user=" + player->getForum() + "&char=" + player->getName());

            }

            outBuf += EOT;
            return(true);
        }

        std::string type;
        CatRef cr;
        // We'll need these to load or save
        xmlNodePtr rootNode;
        xmlDocPtr xmlDoc;

        if(command == "LATESTPOST") {
            const unsigned char* latestBuffer = (unsigned char*)dataBuf.c_str();
            if((xmlDoc = xmlParseDoc(latestBuffer)) == nullptr) {
                std::clog << "WebInterface: LatestPost - Error parsing xml\n";
                return(false);
            }
            rootNode = xmlDocGetRootElement(xmlDoc);

            xmlNodePtr curNode = rootNode->children;

            std::string view = "", subject = "", username = "", boardname = "", post = "";
            while(curNode) {
                     if(NODE_NAME(curNode, "View")) xml::copyToString(view, curNode);
                else if(NODE_NAME(curNode, "Subject")) xml::copyToString(subject, curNode);
                else if(NODE_NAME(curNode, "Username")) xml::copyToString(username, curNode);
                else if(NODE_NAME(curNode, "Boardname")) xml::copyToString(boardname, curNode);
                else if(NODE_NAME(curNode, "Post")) xml::copyToString(post, curNode);

                curNode = curNode->next;
            }
            
            latestPost(view, subject, username, boardname, post);

            outBuf += EOT;
            return(true);
        }

        // The next 3 characters should be CRT, OBJ, or ROM and indicate what we're acting on
        type = tempBuf.substr(0, 3);
        tempBuf.erase(0, 4);

        // Now we need to find what area/index we're working on.  If no area is found we assume misc
        getCatRef(tempBuf, &cr, nullptr);

        std::clog << "WebInterface: Found command: " << command << " " << type << " " << cr.area << "." << cr.id << std::endl;


        if(command == "LOAD") {
            // Loading: We should grab the most current copy of what they want, and write it
            // to the webInterface.out file
            xmlDoc = xmlNewDoc(BAD_CAST "1.0");

            if(type == "CRT") {
                rootNode = xmlNewDocNode(xmlDoc, nullptr, BAD_CAST "Creature", nullptr);
                xmlDocSetRootElement(xmlDoc, rootNode);

                std::shared_ptr<Monster> monster;
                if(loadMonster(cr, monster)) {
                    monster->saveToXml(rootNode, ALLITEMS, LoadType::LS_FULL);
                    std::clog << "Generated xml for " << monster->getName() << "\n";
                }
            }
            else if(type == "OBJ") {
                rootNode = xmlNewDocNode(xmlDoc, nullptr, BAD_CAST "Object", nullptr);
                xmlDocSetRootElement(xmlDoc, rootNode);

                std::shared_ptr<Object>  object;
                if(loadObject(cr, object)) {
                    object->saveToXml(rootNode, ALLITEMS, LoadType::LS_FULL);
                    std::clog << "Generated xml for " << object->getName() << "\n";
                }
            }
            else if(type == "ROM") {
                rootNode = xmlNewDocNode(xmlDoc, nullptr, BAD_CAST "Room", nullptr);
                xmlDocSetRootElement(xmlDoc, rootNode);

                std::shared_ptr<UniqueRoom> room;
                if(loadRoom(cr, room)) {
                    room->saveToXml(rootNode, ALLITEMS);
                    std::clog << "Generated xml for " << room->getName() << "\n";
                }
            }
            // Save the xml document to a character array
            int len=0;
            unsigned char* tmp = nullptr;
            xmlDocDumpFormatMemory(xmlDoc, &tmp,&len,1);
            xmlFreeDoc(xmlDoc);
            // Send the xml document to the output buffer and append an EOT so they
            // know where to stop reading this construct in
            outBuf += std::string((char*)tmp);
            outBuf += EOT;
            // Don't forget to free the char array we got from xmlDocDumpFormatMemory
            free(tmp);

        } else if(command == "SAVE") {
            // Save - They are sending us an object/room/monster to save to the database and
            // update the queue with

            const unsigned char* saveBuffer = (unsigned char*)dataBuf.c_str();
            if((xmlDoc = xmlParseDoc(saveBuffer)) == nullptr) {
                std::clog << "WebInterface: Save - Error parsing xml\n";
                return(false);
            }
            rootNode = xmlDocGetRootElement(xmlDoc);
            int num = xml::getIntProp(rootNode, "Num");
            std::string newArea = xml::getProp(rootNode, "Area");
            // Make sure they're sending us the proper index!
            if(num != cr.id || newArea != cr.area) {
                std::clog << "WebInterface: MisMatched save - Got " << num << " - " << newArea << " Expected " << cr.displayStr() << "\n";
                return(false);
            }
            if(type == "CRT") {
                auto monster = std::make_shared<Monster>();
                monster->readFromXml(rootNode);
                monster->saveToFile();
                broadcast(isDm, "^y*** Monster %s - %s^y updated by %s.", monster->info.displayStr().c_str(), monster->getCName(), monster->last_mod);
                gServer->monsterCache.insert(monster->info, *monster);
            }
            else if(type == "OBJ") {
                auto object = std::make_shared<Object>();
                object->readFromXml(rootNode);
                object->saveToFile();
                broadcast(isDm, "^y*** Object %s - %s^y updated by %s.", object->info.displayStr().c_str(), object->getCName(), object->lastMod.c_str());
                gServer->objectCache.insert(object->info, *object);
            }
            else if(type == "ROM") {
                auto room = std::make_shared<UniqueRoom>();
                room->readFromXml(rootNode);
                room->saveToFile(0);

                gServer->reloadRoom(room);
                broadcast(isDm, "^y*** Room %s - %s^y updated by %s.", room->info.displayStr().c_str(), room->getCName(), room->last_mod);
            }
            std::clog << "WebInterface: Saved " << type << " " << cr.displayStr() << "\n";
        }
    }

    return(true);
}

//*********************************************************************
//                      sendOutput
//*********************************************************************

bool WebInterface::sendOutput() {
    if(outBuf.empty())
        return(false);

    int written=0;
    int n;

    char filename[80];
    snprintf(filename, 80, "%s/%s", Path::Game.c_str(), fifoOut);

    int total = outBuf.length();
    if(outFd == -1)
        outFd = open(filename, O_WRONLY|O_NONBLOCK);

    if(outFd == -1) {
//      std::clog << "WebInterface: Unable to open " << fifoOut << ":" << strerror(errno);
        return(false);
    }

    // Write directly to the socket, otherwise compress it and send it
    do {
        n = write(outFd, outBuf.c_str(), outBuf.length());
        if(n < 0) {
            //std::clog << "WebInterface: sendOutput " << strerror(errno) << std::endl;
            return(false);
        }
        outBuf.erase(0, n);
        written += n;
    } while(written < total);

    return(true);
}


//*********************************************************************
//                      webwho
//*********************************************************************

std::string webwho() {
    std::ostringstream oStr;

    for(const auto& [pId, player] : gServer->players) {
        if(!player->isConnected())
            continue;
        if(player->getClass() == CreatureClass::BUILDER)
            continue;

        if(player->flagIsSet(P_DM_INVIS))
            continue;
        if(player->isEffected("incognito"))
            continue;
        if(player->isInvisible())
            continue;
        if(player->isEffected("mist"))
            continue;

        std::string cls = getShortClassName(player);
        oStr << player->getLevel() << "|" << cls.substr(0, 4) << "|";
        if(player->flagIsSet(P_OUTLAW))
            oStr << "O";
        else if((player->flagIsSet(P_NO_PKILL) || player->flagIsSet(P_DIED_IN_DUEL) ||
                (player->getConstRoomParent()->isPkSafe())) && (player->flagIsSet(P_CHAOTIC) || player->getClan()) )
            oStr << "N";
        else if(player->flagIsSet(P_CHAOTIC)) // Chaotic
            oStr << "C";
        else
            oStr << "L";
        oStr << "|";

        if(player->isPublicWatcher())
            oStr << "(w)";

        oStr << player->fullName() << "|"
             << gConfig->getRace(player->getDisplayRace())->getAdjective() << "|"
             << player->getTitle() << "|";

        if(player->getClan())
            oStr << "(" << gConfig->getClan(player->getClan())->getName() << ")";
        else if(player->getDeity())
            oStr << "(" << gConfig->getDeity(player->getDeity())->getName() << ")";
        oStr << "|";
        if(player->getGuild()  && player->getGuildRank() >= GUILD_PEON)
            oStr << "[" << getGuildName(player->getGuild()) << "]";
        oStr << "\n";
    }
    long t = time(nullptr);
    oStr << ctime(&t);

    return(oStr.str());
}


//*********************************************************************
//                      callWebserver
//*********************************************************************
// load the webserver with no return value
// wget must be installed for this call to work
// questionMark: is a question mark has already been added to the url (for GET parameters)

void callWebserver(std::string url, bool questionMark, bool silent) {
    if(gConfig->getWebserver().empty() || url.empty() || url.length() > 450)
        return;

    if(!silent)
        broadcast(isDm, "^yWebserver called: ^x%s", url.c_str());

    // authorization query string
    if(!gConfig->getQS().empty()) {
        if(!questionMark)
            url += "?";
        else
            url += "&";
        url += gConfig->getQS();
    }

    // build command here incase we ever want to audit
    char command[512];
    sprintf(command, "wget \"%s%s\" -q -O /dev/null", gConfig->getWebserver().c_str(), url.c_str());

    // set the user agent, if applicable
    if(!gConfig->getUserAgent().empty()) {
        strcat(command, " -U \"");
        strcat(command, gConfig->getUserAgent().c_str());
        strcat(command, "\"");
    }

    if(!fork()) {
        system(command);
        exit(0);
    }
}

//*********************************************************************
//                      dmFifo
//*********************************************************************
// delete and recreate the fifos

int dmFifo(const std::shared_ptr<Player>& player, cmd* cmnd) {
    gServer->recreateFifos();
    player->print("Interface fifos have been recreated.\n");
    return(0);
}


//*********************************************************************
//                      cmdForum
//*********************************************************************

int cmdForum(const std::shared_ptr<Player>& player, cmd* cmnd) {

    if(!player->getProxyName().empty()) {
        *player << "You are unable to modify forum accounts of proxied characters.\n";
        return(0);
    }

    std::string::size_type pos;
    std::ostringstream url;
    url << "mud.php?type=forum&char=" << player->getName();

    std::string user = cmnd->str[1];
    std::string pass = getFullstrText(cmnd->fullstr, 2, ' ');

    pos = user.find('&');
    if(pos != std::string::npos)
        user = "";

    if(user == "remove" && !player->getForum().empty()) {

        player->printColor("Attempting to unassociate this character with forum account: ^C%s\n", player->getForum().c_str());
        player->setForum("");
        player->save(true);
        webUnassociate(player->getName());
        return(0);

    } else if(!user.empty() && !pass.empty()) {

        pass = md5(pass);
        player->printColor("Attempting to associate this character with forum account: ^C%s\n", user.c_str());
        url << "&user=" << user << "&pass=" << pass;
        callWebserver(url.str());

        // don't leave their password in last command
        player->setLastCommand(cmnd->str[0] + (std::string)" " + cmnd->str[1] + (std::string)" ********");
        return(0);

    } else if(player->getForum().empty()) {

        player->print("There is currently no forum account associated with this character.\n");

    } else {

        player->printColor("Forum account associated with this character: ^C%s\n", player->getForum().c_str());
        player->print("To unassociate this character from this forum account type:\n");
        player->printColor("   forum ^Cremove\n");

    }

    player->print("To associate this character with a forum account type:\n");
    player->printColor("   forum ^C<forum account> <forum password>\n\n");
    player->printColor("Type ^WHELP LATEST_POST^x to see the latest forum post.\n");
    player->printColor("Type ^WHELP LATEST_POSTS^x to see the 6 most recent forum posts.\n");
    return(0);
}

//*********************************************************************
//                      webUnassociate
//*********************************************************************

void webUnassociate(const std::string &user) {
    callWebserver(fmt::format("mud.php?type=forum&char={}&delete", user));
}

//*********************************************************************
//                      webCrash
//*********************************************************************

void webCrash(const std::string &msg) {
    callWebserver(fmt::format("mud.php?type=crash&msg={}", msg));
}


//*********************************************************************
//                      cmdWiki
//*********************************************************************
// This function allows a player to loop up a wiki entry

int cmdWiki(const std::shared_ptr<Player>& player, cmd* cmnd) {
    struct stat f_stat{};
    std::ostringstream url;
    std::string entry = getFullstrText(cmnd->fullstr, 1);

    player->clearFlag(P_AFK);

    if(player->isBraindead()) {
        player->print("You are brain-dead. You can't do that.\n");
        return(0);
    }

    if(entry.empty()) {
        entry = "Main_Page";
        player->printColor("Type ^ywiki [entry]^x to look up a specific entry.\n\n");
    }

    boost::to_lower(entry);
    boost::replace_all(entry, ":", "_colon_");
    auto file = (Path::Wiki / entry).replace_extension("txt");

    // If the file exists and was modified within the last hour, use the local cache
    if(!stat(file.c_str(), &f_stat) && (time(nullptr) - f_stat.st_mtim.tv_sec) < 3600) {
        player->getSock()->viewFile(file, true);
        return(0);
    }

    player->print("Loading entry...\n");
    url << "mud.php?type=wiki&char=" << player->getName() << "&entry=" << entry;
    callWebserver(url.str(), true, true);
    return(0);
}


//*********************************************************************
//                      wiki
//*********************************************************************
// This function allows a player to loop up a wiki entry

bool WebInterface::wiki(std::string command, std::string tempBuf) {
    std::string::size_type pos;

    // we don't know the command yet
    if(command.empty()) {
        pos = tempBuf.find(' ');
        if(pos == std::string::npos)
            command = tempBuf;
        else
            command = tempBuf.substr(0, pos);
    }

    pos = tempBuf.find(' ');
    if(pos == std::string::npos) {
        std::clog << "WebInterface: Wiki help failed; not enough data!" << std::endl;
        return(false);
    }

    std::string user = boost::to_lower_copy(tempBuf.substr(0, pos));
    user.at(0) = up(user.at(0));

    tempBuf.erase(0, pos+1); // Clear out the user
    boost::trim(tempBuf);


    if( tempBuf.empty() ||
        strchr(tempBuf.c_str(), '/') != nullptr) {
        std::clog << "WebInterface: Wiki help failed; invalid data" << std::endl;
        return(false);
    }

    std::clog << "WebInterface: Wiki help for user " << user << std::endl;
    const std::shared_ptr<Player> player = gServer->findPlayer(user);
    if(!player) {
        outBuf += "That player is not logged on.";
    } else {
        player->getSock()->viewFile((Path::Wiki / tempBuf).replace_extension("txt"), true);
    }
    outBuf += EOT;
    return(true);
}
