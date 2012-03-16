/*
 * web.cpp
 *	 web-based functions
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  Creative Commons - Attribution - Non Commercial - Share Alike 3.0 License
 *    http://creativecommons.org/licenses/by-nc-sa/3.0/
 *
 * 	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include "mud.h"
#include "web.h"
#include "clans.h"
#include "commands.h"
#include "guilds.h"

// C includes
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

int lastmod = 0;
struct stat lm_check;


// Static initialization
const char* WebInterface::fifoIn = "webInterface.in";
const char* WebInterface::fifoOut = "webInterface.out";
WebInterface* WebInterface::myInstance = NULL;

static char ETX = 3;
static char EOT = 4;
//static char ETB	= 23;

static char itemDelim = 6;
static char innerDelim = 7;
static char startSubDelim = 8;
static char endSubDelim = 9;
static char equipDelim = 10;



//*********************************************************************
//						updateRecentActivity
//*********************************************************************

void updateRecentActivity() {
	callWebserver((bstring)"mud.php?type=recent", true, true);
}

//*********************************************************************
//						latestPost
//*********************************************************************

void latestPost(const bstring& view, const bstring& subject, const bstring& username, const bstring& boardname, const bstring& post) {

	if(view == "" || boardname == "" || username == "" || post == "")
		return;

	for(Socket* sock : gServer->sockets) {
		const Player* player = sock->getPlayer();

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
			player->printColor("%s", post.c_str());
		else
			player->printColor("^C==>^x There is a new post titled ^W\"%s\"^x by ^W%s^x in the ^W%s^x board.\n", subject.c_str(), username.c_str(), boardname.c_str());
	}
}

//*********************************************************************
//						WebInterface
//*********************************************************************
// Verify the in/out fifos exist and open the in fifo in non-blocking mode

WebInterface::WebInterface() {
	openFifos();
}
WebInterface::~WebInterface() {
	closeFifos();
}

//*********************************************************************
//						open
//*********************************************************************

void WebInterface::openFifos() {
	checkFifo(fifoIn);
	checkFifo(fifoOut);

	char filename[80];
	snprintf(filename, 80, "%s/%s", Path::Game, fifoIn);
	inFd = open(filename, O_RDONLY|O_NONBLOCK);
	if(inFd == -1)
		throw new bstring("WebInterface: Unable to open " + bstring(filename) + ":" +strerror(errno));

	outFd = -1;

	std::cout << "WebInterface: monitoring " << fifoIn << std::endl;
}

//*********************************************************************
//						close
//*********************************************************************

void WebInterface::closeFifos() {
	//	unlink(gifo);
	close(outFd);
	close(inFd);
}

//*********************************************************************
//						initWebInterface
//*********************************************************************

bool Server::initWebInterface() {
	// Do a check for main port, hostname, etc here
	webInterface = WebInterface::getInstance();
	return(true);
}

//*********************************************************************
//						checkWebInterface
//*********************************************************************

bool Server::checkWebInterface() {
	if(!webInterface)
		return(false);
	return(webInterface->handleRequests());
}

//*********************************************************************
//						recreateFifos
//*********************************************************************

void Server::recreateFifos() {
	if(webInterface)
		webInterface->recreateFifos();
}

void WebInterface::recreateFifos() {
	char filename[80];
	closeFifos();

	snprintf(filename, 80, "%s/%s.xml", Path::Game, fifoIn);
	unlink(filename);

	snprintf(filename, 80, "%s/%s.xml", Path::Game, fifoOut);
	unlink(filename);

	openFifos();
}

//*********************************************************************
//						checkFifo
//*********************************************************************
// Check that the given fifo exists as a fifo. If it is a regular file, erase it.
// If it doesn't exist, create it.

bool WebInterface::checkFifo(const char* fifoFile) {
	struct stat statInfo;
	int retVal;
	bool needToCreate = false;

	char filename[80];
	snprintf(filename, 80, "%s/%s", Path::Game, fifoFile);
	retVal = stat(filename, &statInfo);
	if(retVal == 0) {
		if((statInfo.st_mode & S_IFMT) != S_IFIFO) {
			if(unlink(filename) != 0)
				throw bstring("WebInterface: Unable to unlink " + bstring(fifoFile) + ":" + strerror(errno));
			needToCreate = true;
		}
	} else {
		needToCreate = true;
	}

	if(needToCreate) {
		retVal = mkfifo(filename, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
		if(retVal != 0)
			throw bstring("WebInterface: Unable to mkfifo " + bstring(fifoFile) + ":" + strerror(errno));
	}
	return(true);
}

//*********************************************************************
//						getInstance
//*********************************************************************

WebInterface* WebInterface::getInstance() {
	if(myInstance == NULL)
		myInstance = new WebInterface;
	return(myInstance);
}

//*********************************************************************
//						destroyInstance
//*********************************************************************

void WebInterface::destroyInstance() {
	if(myInstance != NULL)
		delete myInstance;
	myInstance = NULL;
}

//*********************************************************************
//						handleRequests
//*********************************************************************

bool WebInterface::handleRequests() {
	checkInput();
	handleInput();

	sendOutput();
	return(true);
}

//*********************************************************************
//						checkInput
//*********************************************************************

bool WebInterface::checkInput() {
	if(inFd == -1)
		return(false);

	char tmpBuf[1024];
	int n = 0, total = 0;
	do {
		// Attempt to read from the socket
		n = read(inFd, tmpBuf, 1023);
		if(n <= 0) {
			//std::cout << "WebInterface: Unable to read - " << strerror(errno) << "\n";
			break;
		}
		tmpBuf[n] = '\0';
		total += n;
		inBuf += tmpBuf;

	} while(n > 0);

	inBuf.Replace("\r", "\n");
	if(total == 0)
		return(false);

	return(true);
}

//*********************************************************************
//						handleInput
//*********************************************************************

bool WebInterface::messagePlayer(bstring command, bstring tempBuf) {
	// MSG / SYSMSG = system sending
	// TXT = user sending
	bstring::size_type pos=0;

	// we don't know the command yet
	if(command == "") {
		pos = tempBuf.Find(' ');
		if(pos == bstring::npos)
			command = tempBuf;
		else
			command = tempBuf.left(pos);
	}

	pos = tempBuf.Find(' ');
	if(pos == bstring::npos) {
		std::cout << "WebInterface: Messaging failed; not enough data!" << std::endl;
		return("");
	}

	bstring user = tempBuf.left(pos);
	user = user.toLower();
	user.setAt(0, up(user.getAt(0)));

	tempBuf.erase(0, pos+1); // Clear out the user
	tempBuf = tempBuf.trim();

	std::cout << "WebInterface: Messaging user " << user << std::endl;
	const Player* player = gServer->findPlayer(user);
	if(!player) {
		outBuf += "That player is not logged on.";
	} else {
		bool txt = (command == "TXT");
		if(txt && (
			player->getClass() == BUILDER ||
			player->flagIsSet(P_IGNORE_ALL) ||
			player->flagIsSet(P_IGNORE_SMS)
		)) {
			std::cout << "WebInterface: Messaging failed; user does not want text messages." << std::endl;
			return(false);
		}
		if(command != "SYSMSG")
			player->printColor("^C==> ");
		if(!txt) {
			tempBuf.Replace("<cr>", "\n\t");
		}
		player->printColor("%s%s%s\n", txt ? "^WText Msg: \"" : "",
			tempBuf.c_str(),
			txt ? "\"" : "");
		if(!player->isDm()) {
			broadcast(watchingEaves, "^E--- %s to %s, \"%s\".",
				txt ? "Text message" : "System message", player->name, tempBuf.c_str());
		}
	}
	outBuf += EOT;
	return(true);
}

//*********************************************************************
//						getInventory
//*********************************************************************

bstring doGetInventory(const Player* player, otag* op);

bstring doGetInventory(const Player* player, Object* object, int loc=-1) {
	std::ostringstream oStr;
	oStr << itemDelim
		 << object->name
		 << innerDelim
		 << object->getUniqueId()
		 << innerDelim
		 << object->getWearflag()
		 << innerDelim
		 << object->getType();

	if(loc != -1)
		oStr << innerDelim << loc;

	if(object->first_obj) {
		oStr << startSubDelim
			 << doGetInventory(player, object->first_obj)
			 << endSubDelim;
	}
	return(oStr.str());
}

bstring doGetInventory(const Player* player, otag* op) {
	bstring inv = "";
	while(op) {
		if(player->canSee(op->obj))
			inv += doGetInventory(player, op->obj);
		op = op->next_tag;
	}
	return(inv);
}

bstring getInventory(const Player* player) {
	std::ostringstream oStr;

	oStr << player->getUniqueObjId()
		 << doGetInventory(player, player->first_obj)
		 << equipDelim;

	for(int i=0; i<MAXWEAR; i++) {
		if(player->ready[i])
			oStr << doGetInventory(player, player->ready[i], i);
	}

	return(oStr.str());
}

//*********************************************************************
//						handleInput
//*********************************************************************

bool webWield(Player* player, int id);
bool webWear(Player* player, int id);

bool webUse(Player* player, int id, int type) {
	switch(type) {
	case WEAPON:
		return(webWield(player, id));
	case ARMOR:
		return(webWear(player, id));
	case POTION:
		//return(cmdConsume(player, id));
	case SCROLL:
		//return(cmdReadScroll(player, id));
	case WAND:
		//return(cmdUseWand(player, id));
	case KEY:
		//return(cmdUnlock(player, id));
	case LIGHTSOURCE:
		//return(cmdHold(player, id));
	default:
		player->print("How does one use that?\n");
		return(false);
	}
}

//*********************************************************************
//						handleInput
//*********************************************************************
// Uses ETX (End of Text) as abort and EOT (End of Transmission Block) as EOF

bool webRemove(Player* player, int id);

bool WebInterface::handleInput() {
	if(inBuf.empty())
		return(false);

	bstring::size_type start=0, idx=0, pos=0;
	bool needData=false;

	// First, see if we have a clear command (ETX), if so erase up to there
	idx = inBuf.ReverseFind(ETX);
	if(idx != bstring::npos)
		inBuf.erase(0, idx);

	// Now lets ignore any unprintable characters that might have gotten tacked on to the start
	while(!isPrintable((unsigned char)inBuf[start]))
		start++;
	inBuf.erase(0, start);

	// Now lets look for a command
	idx = inBuf.find("\n", 0);
	if(idx == bstring::npos)
		return(false);

	// Copy the entire command to a temporary buffer and lets see if we can find
	// a valid command
	bstring tempBuf = inBuf.substr(0, idx); // Don't copy the \n
	bstring command; // the command being run
	bstring dataBuf; // the buffer for incoming data

	pos = tempBuf.Find(' ');
	if(pos == bstring::npos)
		command = tempBuf;
	else
		command = tempBuf.left(pos);

	idx += 1; // Consume the \n
	if(inBuf[idx] == '\n')
		idx += 1; // Consume the extra \n if applicable

	//const unsigned char* before = (unsigned char*)strdup(inBuf.c_str());

	// certain commands can only be called from the webserver
	if(gConfig->getWebserver() == "") {
		if(	command == "FORUM" ||
			command == "UNFORUM" ||
			command == "AUTOGUILD"
		)
			command = "";
	}

	// If we can jump right in (LOAD), do so.  If we need to wait until we've
	// read in the ETF from the builder, set needData to true
	if(command == "SAVE" || command == "LATESTPOST")
		needData = true;

	if(	command == "LOAD" ||
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
		(needData && inBuf.find(EOT, idx) != bstring::npos)
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

		if(command == "GETINVENTORY" || command == "EQUIP" || command == "UNEQUIP") {
			pos = tempBuf.Find(' ');
			int id=0, type=0;

			bstring user = "";
			if(pos != bstring::npos) {
				user = tempBuf.left(pos);
				tempBuf.erase(0, pos+1); // Clear out the user
				id = atoi(tempBuf.c_str());

				if(command == "EQUIP") {
					pos = tempBuf.Find(' ');
					tempBuf.erase(0, pos+1); // Clear out the user
					type = atoi(tempBuf.c_str());
				}
			} else {
				user = tempBuf;
				tempBuf = "";
			}

			Player* player = 0;
			if(user != "")
				player = gServer->findPlayer(user);
			if(!player) {
				outBuf += EOT;
				return(true);
			}

			if(command == "GETINVENTORY") {

				outBuf += getInventory(player);

			} else if(command == "EQUIP") {

				outBuf += webUse(player, id, type) ? "1" : "0";

			} else if(command == "UNEQUIP") {

				outBuf += webRemove(player, id) ? "1" : "0";

			}

			outBuf += EOT;
			return(true);
		} else if(command == "WHO") {
			std::cout << "WebInterface: Checking for users online" << std::endl;
			outBuf += webwho();
			outBuf += EOT;
			return(true);
		} else if(command == "WHOIS") {
			std::cout << "WebInterface: Whois for user " << tempBuf << std::endl;
			tempBuf = tempBuf.toLower();
			tempBuf.setAt(0, up(tempBuf.getAt(0)));
			const Player* player = gServer->findPlayer(tempBuf);
			if(	!player ||
				player->flagIsSet(P_DM_INVIS) ||
				player->flagIsSet(P_INCOGNITO) ||
				player->flagIsSet(P_MISTED) ||
				player->isInvisible()
			) {
				outBuf += "That player is not logged on.";
			} else {
				outBuf += player->getWhoString(false, false, false);
			}
			outBuf += EOT;
			return(true);
		} else if(command == "FINGER") {
			std::cout << "WebInterface: Fingering user " << tempBuf << std::endl;
			outBuf += doFinger(0, tempBuf, 0);
			outBuf += EOT;
			return(true);
		} else if(command == "WIKI") {
			return(wiki(command, tempBuf));
		} else if(command == "MSG" || command == "SYSMSG" || command == "TXT") {
			return(messagePlayer(command, tempBuf));
		} else if(command == "FORUM") {
			pos = tempBuf.Find(' ');
			if(pos == bstring::npos) {
				std::cout << "WebInterface: Forum association failed; not enough data!" << std::endl;
				return(false);
			}

			bstring user = tempBuf.left(pos);

			tempBuf.erase(0, pos+1); // Clear out the user
			std::cout << "WebInterface: Forum association for user " << user << std::endl;

			Player* player = gServer->findPlayer(user);
			if(player) {
				player->setForum(tempBuf);
				player->save(true);
				return(messagePlayer("MSG", user + " Your character has been associated with forum account " + tempBuf + "."));
			}

			// they logged off?
			if(!loadPlayer(user.c_str(), &player)) {
				// they were deleted? undo!
				webUnassociate(user);
				return(false);
			}

			player->setForum(tempBuf);
			player->save();
			free_crt(player);
			outBuf += EOT;
			return(true);
		} else if(command == "UNFORUM") {
			std::cout << "WebInterface: Forum unassociation for user " << tempBuf << std::endl;

			if(tempBuf == "") {
				std::cout << "WebInterface: Forum unassociation failed; not enough data!" << std::endl;
				return(false);
			}

			tempBuf = tempBuf.toLower();
			tempBuf.setAt(0, up(tempBuf.getAt(0)));

			Player* player = gServer->findPlayer(tempBuf);
			if(player) {
				if(player->getForum() != "") {
					messagePlayer("MSG", tempBuf + " Your character has been unassociated from forum account " + player->getForum() + ".");
					player->setForum("");
					player->save(true);
				}
			} else {

				// they logged off?
				if(!loadPlayer(tempBuf.c_str(), &player)) {
					// they were deleted? no problem!
				} else {
					player->setForum("");
					player->save();
					free_crt(player);
				}

			}
			outBuf += EOT;
			return(true);
		} else if(command == "AUTOGUILD") {
			if(tempBuf == "") {
				std::cout << "WebInterface: Autoguild failed; not enough data!" << std::endl;
				return(false);
			}

			const Guild* guild=0;
			if(tempBuf.getLength() <= 40)
				guild = gConfig->getGuild(tempBuf);
			if(!guild) {
				std::cout << "WebInterface: Autoguild failed; guild " << tempBuf << " not found!" << std::endl;
				return(false);
			}

			std::list<bstring>::const_iterator it;
			Player* player=0;
			bool online=true;
			for(it = guild->members.begin() ; it != guild->members.end() ; it++ ) {
				online = true;
				player = gServer->findPlayer(*it);

				if(!player) {
					if(!loadPlayer((*it).c_str(), &player))
						continue;

					online = false;
				}

				if(player->getForum() != "")
					callWebserver((bstring)"mud.php?type=autoguild&guild=" + guild->getName() + "&user=" + player->getForum() + "&char=" + player->name);

				if(!online)
					free_crt(player);
			}

			outBuf += EOT;
			return(true);
		}

		bstring type;
		CatRef cr;
		// We'll need these to load or save
		xmlNodePtr rootNode;
		xmlDocPtr xmlDoc;

		if(command == "LATESTPOST") {
			const unsigned char* latestBuffer = (unsigned char*)dataBuf.c_str();
			if((xmlDoc = xmlParseDoc(latestBuffer)) == NULL) {
				std::cout << "WebInterface: LatestPost - Error parsing xml\n";
				return(false);
			}
			rootNode = xmlDocGetRootElement(xmlDoc);

			xmlNodePtr curNode = rootNode->children;

			bstring view = "", subject = "", username = "", boardname = "", post = "";
			while(curNode) {
					 if(NODE_NAME(curNode, "View")) xml::copyToBString(view, curNode);
				else if(NODE_NAME(curNode, "Subject")) xml::copyToBString(subject, curNode);
				else if(NODE_NAME(curNode, "Username")) xml::copyToBString(username, curNode);
				else if(NODE_NAME(curNode, "Boardname")) xml::copyToBString(boardname, curNode);
				else if(NODE_NAME(curNode, "Post")) xml::copyToBString(post, curNode);

				curNode = curNode->next;
			}
			
			latestPost(view, subject, username, boardname, post);

			outBuf += EOT;
			return(true);
		}

		// The next 3 characters should be CRT, OBJ, or ROM and indicate what we're acting on
		type = tempBuf.left(3);
		tempBuf.erase(0, 4);

		// Now we need to find what area/index we're working on.  If no area is found we assume misc
		getCatRef(tempBuf, &cr, 0);

		std::cout << "WebInterface: Found command: " << command << " " << type << " " << cr.area << "." << cr.id << std::endl;


		if(command == "LOAD") {
			// Loading: We should grab the most current copy of what they want, and write it
			// to the webInterface.out file
			xmlDoc = xmlNewDoc(BAD_CAST "1.0");

			if(type == "CRT") {
				rootNode = xmlNewDocNode(xmlDoc, NULL, BAD_CAST "Creature", NULL);
				xmlDocSetRootElement(xmlDoc, rootNode);

				Monster *monster;
				if(loadMonster(cr, &monster)) {
					monster->saveToXml(rootNode, ALLITEMS, LS_FULL);
					std::cout << "Generated xml for " << monster->name << "\n";
					free_crt(monster);
				}
			}
			else if(type == "OBJ") {
				rootNode = xmlNewDocNode(xmlDoc, NULL, BAD_CAST "Object", NULL);
				xmlDocSetRootElement(xmlDoc, rootNode);

				Object* object;
				if(loadObject(cr, &object)) {
					object->saveToXml(rootNode, ALLITEMS, LS_FULL);
					std::cout << "Generated xml for " << object->name << "\n";
					delete object;
				}
			}
			else if(type == "ROM") {
				rootNode = xmlNewDocNode(xmlDoc, NULL, BAD_CAST "Room", NULL);
				xmlDocSetRootElement(xmlDoc, rootNode);

				UniqueRoom* room;
				if(loadRoom(cr, &room)) {
					room->saveToXml(rootNode, ALLITEMS);
					std::cout << "Generated xml for " << room->name << "\n";
				}
			}
			// Save the xml document to a character array
			int len=0;
			unsigned char* tmp = NULL;
			xmlDocDumpFormatMemory(xmlDoc, &tmp,&len,1);
			xmlFreeDoc(xmlDoc);
			// Send the xml document to the output buffer and append an EOT so they
			// know where to stop reading this construct in
			outBuf += tmp;
			outBuf += EOT;
			// Don't forget to free the char array we got from xmlDocDumpFormatMemory
			free(tmp);

		} else if(command == "SAVE") {
			// Save - They are sending us an object/room/monster to save to the database and
			// update the queue with

			const unsigned char* saveBuffer = (unsigned char*)dataBuf.c_str();
			if((xmlDoc = xmlParseDoc(saveBuffer)) == NULL) {
				std::cout << "WebInterface: Save - Error parsing xml\n";
				return(false);
			}
			rootNode = xmlDocGetRootElement(xmlDoc);
			int num = xml::getIntProp(rootNode, "Num");
			bstring newArea = xml::getProp(rootNode, "Area");
			// Make sure they're sending us the proper index!
			if(num != cr.id || newArea != cr.area) {
				std::cout << "WebInterface: MisMatched save - Got " << num << " - " << newArea << " Expected " << cr.str() << "\n";
				return(false);
			}
			if(type == "CRT") {
				Monster* monster = new Monster();
				monster->readFromXml(rootNode);
				monster->saveToFile();
				broadcast(isDm, "^y*** Monster %s - %s^y updated by %s.", monster->info.str().c_str(), monster->name, monster->last_mod);
				gConfig->replaceMonsterInQueue(monster->info, monster);
			}
			else if(type == "OBJ") {
				Object* object = new Object();
				object->readFromXml(rootNode);
				object->saveToFile();
				broadcast(isDm, "^y*** Object %s - %s^y updated by %s.", object->info.str().c_str(), object->name, object->lastMod.c_str());
				gConfig->replaceObjectInQueue(object->info, object);
			}
			else if(type == "ROM") {
				UniqueRoom* room = new UniqueRoom();
				room->readFromXml(rootNode);
				room->saveToFile(0);

				gConfig->reloadRoom(room);
				broadcast(isDm, "^y*** Room %s - %s^y updated by %s.", room->info.str().c_str(), room->name, room->last_mod);
			}
			std::cout << "WebInterface: Saved " << type << " " << cr.str() << "\n";
		}
	}

	return(true);
}

//*********************************************************************
//						sendOutput
//*********************************************************************

bool WebInterface::sendOutput() {
	if(outBuf.empty())
		return(false);

	int written=0;
	int n=0;

	char filename[80];
	snprintf(filename, 80, "%s/%s", Path::Game, fifoOut);

	int total = outBuf.length();
	if(outFd == -1)
		outFd = open(filename, O_WRONLY|O_NONBLOCK);

	if(outFd == -1) {
//		std::cout << "WebInterface: Unable to open " << fifoOut << ":" << strerror(errno);
		return(false);
	}

	// Write directly to the socket, otherwise compress it and send it
	do {
		n = write(outFd, outBuf.c_str(), outBuf.length());
		if(n < 0) {
			//std::cout << "WebInterface: sendOutput " << strerror(errno) << std::endl;
			return(false);
		}
		outBuf.erase(0, n);
		written += n;
	} while(written < total);

	return(true);
}


//*********************************************************************
//						webwho
//*********************************************************************

bstring webwho() {
	std::ostringstream oStr;
	const Player *player=0;

	for(std::pair<bstring, Player*> p : gServer->players) {
		player = p.second;

		if(!player->isConnected())
			continue;
		if(player->getClass() == BUILDER)
			continue;

		if(player->flagIsSet(P_DM_INVIS))
			continue;
		if(player->flagIsSet(P_INCOGNITO))
			continue;
		if(player->isInvisible())
			continue;
		if(player->flagIsSet(P_MISTED))
			continue;

		bstring cls = getShortClassName(player);
		oStr << player->getLevel() << "|" << cls.left(4) << "|";
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
	long t = time(0);
	oStr << ctime(&t);

	return(oStr.str());
}


//*********************************************************************
//						callWebserver
//*********************************************************************
// load the webserver with no return value
// wget must be installed for this call to work
// questionMark: is a question mark has already been added to the url (for GET parameters)

void callWebserver(bstring url, bool questionMark, bool silent) {
	if(gConfig->getWebserver() == "" || url == "" || url.getLength() > 450)
		return;

	if(!silent)
		broadcast(isDm, "^yWebserver called: ^x%s", url.c_str());

	// authorization query string
	if(gConfig->getQS() != "") {
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
	if(gConfig->getUserAgent() != "") {
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
//						dmFifo
//*********************************************************************
// delete and recreate the fifos

int dmFifo(Player* player, cmd* cmnd) {
	gServer->recreateFifos();
	player->print("Interface fifos have been recreated.\n");
	return(0);
}


//*********************************************************************
//						cmdForum
//*********************************************************************

int cmdForum(Player* player, cmd* cmnd) {

	if(player->getProxyName() != "") {
		*player << "You are unable to modify forum accounts of proxied characters.\n";
		return(0);
	}

	bstring::size_type pos=0;
	std::ostringstream url;
	url << "mud.php?type=forum&char=" << player->name;

	bstring user = cmnd->str[1];
	bstring pass = getFullstrText(cmnd->fullstr, 2, ' ');

	pos = user.Find('&');
	if(pos != bstring::npos)
		user = "";

	if(user == "remove" && player->getForum() != "") {

		player->printColor("Attempting to unassociate this character with forum account: ^C%s\n", player->getForum().c_str());
		player->setForum("");
		player->save(true);
		webUnassociate(player->name);
		return(0);

	} else if(user != "" && pass != "") {

		pass = md5(pass);
		player->printColor("Attempting to associate this character with forum account: ^C%s\n", user.c_str());
		url << "&user=" << user << "&pass=" << pass;
		callWebserver(url.str());

		// don't leave their password in last command
		player->setLastCommand(cmnd->str[0] + (bstring)" " + cmnd->str[1] + (bstring)" ********");
		return(0);

	} else if(player->getForum() == "") {

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
//						webUnassociate
//*********************************************************************

void webUnassociate(bstring user) {
	callWebserver("mud.php?type=forum&char=" + user + "&delete");
}

//*********************************************************************
//						webCrash
//*********************************************************************

void webCrash(bstring msg) {
	callWebserver("mud.php?type=crash&msg=" + msg);
}


//*********************************************************************
//						cmdWiki
//*********************************************************************
// This function allows a player to loop up a wiki entry

int cmdWiki(Player* player, cmd* cmnd) {
	struct stat f_stat;
	char	file[80];
	std::ostringstream url;
	bstring entry = getFullstrText(cmnd->fullstr, 1);

	player->clearFlag(P_AFK);

	if(player->isBraindead()) {
		player->print("You are brain-dead. You can't do that.\n");
		return(0);
	}

	if(entry == "") {
		entry = "Main_Page";
		player->printColor("Type ^ywiki [entry]^x to look up a specific entry.\n\n");
	}

	entry = entry.toLower();
	entry.Replace(":", "_colon_");
	if(!checkWinFilename(player->getSock(), entry.c_str()))
		return(0);
	sprintf(file, "%s/%s.txt", Path::Wiki, entry.c_str());

	// If the file exists and was modified within the last hour, use the local cache
	if(!stat(file, &f_stat) && (time(0) - f_stat.st_mtim.tv_sec) < 3600) {
		viewFile(player->getSock(), file);
		return(0);
	}

	player->print("Loading entry...\n");
	url << "mud.php?type=wiki&char=" << player->name << "&entry=" << entry;
	callWebserver(url.str(), true, true);
	return(0);
}


//*********************************************************************
//						wiki
//*********************************************************************
// This function allows a player to loop up a wiki entry

bool WebInterface::wiki(bstring command, bstring tempBuf) {
	bstring::size_type pos=0;

	// we don't know the command yet
	if(command == "") {
		pos = tempBuf.Find(' ');
		if(pos == bstring::npos)
			command = tempBuf;
		else
			command = tempBuf.left(pos);
	}

	pos = tempBuf.Find(' ');
	if(pos == bstring::npos) {
		std::cout << "WebInterface: Wiki help failed; not enough data!" << std::endl;
		return(false);
	}

	bstring user = tempBuf.left(pos);
	user = user.toLower();
	user.setAt(0, up(user.getAt(0)));

	tempBuf.erase(0, pos+1); // Clear out the user
	tempBuf = tempBuf.trim();


	if(	tempBuf == "" ||
		strchr(tempBuf.c_str(), '/') != NULL ||
		!checkWinFilename(0, tempBuf.c_str())
	) {
		std::cout << "WebInterface: Wiki help failed; invalid data" << std::endl;
		return(false);
	}

	std::cout << "WebInterface: Wiki help for user " << user << std::endl;
	const Player* player = gServer->findPlayer(user);
	if(!player) {
		outBuf += "That player is not logged on.";
	} else {
		char	file[80];

		sprintf(file, "%s/%s.txt", Path::Wiki, tempBuf.c_str());
		viewFile(player->getSock(), file);
	}
	outBuf += EOT;
	return(true);
}
