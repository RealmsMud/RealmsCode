/*
 * clans.cpp
 *   xml parsing for clans
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include "mud.h"
#include "commands.h"
#include "clans.h"
#include "dm.h"

#include <sstream>
#include <iomanip>
#include <locale>

//*********************************************************************
//                      Clan
//*********************************************************************

Clan::Clan() {
    id = join = rescind = deity = 0;
    name = "";
}

unsigned int Clan::getId() const { return(id); }
unsigned int Clan::getJoin() const { return(join); }
unsigned int Clan::getRescind() const { return(rescind); }
unsigned int Clan::getDeity() const { return(deity); }
bstring Clan::getName() const { return(name); }

short Clan::getSkillBonus(bstring skill) const {
    std::map<bstring, short>::const_iterator it = skillBonus.find(skill);

    if(it != skillBonus.end())
        return((*it).second);
    return(0.0);
}

//*********************************************************************
//                      load
//*********************************************************************

void Clan::load(xmlNodePtr rootNode) {
    xmlNodePtr childNode, curNode = rootNode->children;
    bstring temp;

    id = xml::getIntProp(rootNode, "Id");
    while(curNode) {
             if(NODE_NAME(curNode, "Name")) xml::copyToBString(name, curNode);
        else if(NODE_NAME(curNode, "Join")) join = xml::toNum<int>(curNode);
        else if(NODE_NAME(curNode, "Rescind")) rescind = xml::toNum<int>(curNode);
        else if(NODE_NAME(curNode, "Deity")) {
            xml::copyToBString(temp, curNode);
            deity = gConfig->deitytoNum(temp);
        }
        else if(NODE_NAME(curNode, "SkillBonus")) {
            childNode = curNode->children;
            while(childNode) {
                if(NODE_NAME(childNode, "Skill")) {
                    xml::copyPropToBString(temp, childNode, "Name");
                    skillBonus[temp] = xml::toNum<short>(childNode);
                }
                childNode = childNode->next;
            }
        }
        curNode = curNode->next;
    }
}


//*********************************************************************
//                      getClan
//*********************************************************************

const Clan* Config::getClan(unsigned int id) const {
    std::map<int, Clan*>::const_iterator it = clans.find(id);

    if(it == clans.end())
        it = clans.begin();

    return((*it).second);
}

//*********************************************************************
//                      getClanByDeity
//*********************************************************************

const Clan* Config::getClanByDeity(unsigned int deity) const {
    std::map<int, Clan*>::const_iterator it;
    Clan* clan=0;

    for(it = clans.begin() ; it != clans.end() ; it++) {
        clan = (*it).second;
        if(clan->getDeity() == deity)
            return(clan);
    }
    // clan 0 is unknown
    it = clans.begin();
    return((*it).second);
}

//*********************************************************************
//                      clearClans
//*********************************************************************

void Config::clearClans() {
    std::map<int, Clan*>::iterator it;
    Clan* clan=0;

    for(it = clans.begin() ; it != clans.end() ; it++) {
        clan = (*it).second;
        delete clan;
    }
    clans.clear();
}

//*********************************************************************
//                      loadClans
//*********************************************************************

bool Config::loadClans() {
    xmlDocPtr xmlDoc;
    xmlNodePtr curNode;
    int     i=0;

    char filename[80];
    snprintf(filename, 80, "%s/clans.xml", Path::Game);
    xmlDoc = xml::loadFile(filename, "Clans");

    if(xmlDoc == NULL)
        return(false);

    curNode = xmlDocGetRootElement(xmlDoc);

    curNode = curNode->children;
    while(curNode && xmlIsBlankNode(curNode))
        curNode = curNode->next;

    if(curNode == 0) {
        xmlFreeDoc(xmlDoc);
        return(false);
    }

    clearClans();
    while(curNode != NULL) {
        if(NODE_NAME(curNode, "Clan")) {
            i = xml::getIntProp(curNode, "Id");

            if(clans.find(i) == clans.end()) {
                clans[i] = new Clan;
                clans[i]->load(curNode);
            }
        }
        curNode = curNode->next;
    }
    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
    return(true);
}

//*********************************************************************
//                      getDeityClan
//*********************************************************************

int Player::getDeityClan() const {
    return(gConfig->getClanByDeity(deity)->getId());
}

//*********************************************************************
//                      cmdPledge
//*********************************************************************
// The pledge command allows a player to pledge allegiance to a given
// monster. A pledge players allows the player to use selected items,
// and exits, as well a being able to kill players of the opposing
// kingdoms for experience (regardless if one player is lawful.) In
// order for a player to pledge, the player needs to be in the correct
// room with a correct monster to pledge to.

int cmdPledge(Player* player, cmd* cmnd) {
    Monster* creature=0;
    const Clan* clan=0;

    player->clearFlag(P_AFK);
    if(!player->ableToDoCommand())
        return(0);

    if(player->getLevel() < 5) {
        player->print("You must be at least level 5 to pledge to a clan.\n");
        return(0);
    }

    if(player->getDeity()) {
        player->print("You are only allowed to be in the service of %s.\n", gConfig->getDeity(player->getDeity())->getName().c_str());
        return(0);
    }


    if(cmnd->num < 2) {
        player->print("Join whom's organization?\n");
        return(0);
    }
    if(player->flagIsSet(P_PLEDGED)) {
        player->print("You have already joined.\n");
        return(0);
    }
    if(!player->getRoomParent()->flagIsSet(R_PLEDGE)) {
        player->print("You cannot join here.\n");
        return(0);
    }

    if(player->getLevel() < 5) {
        player->print("You are not experienced enough to join yet.\n");
        return(0);
    }

    creature = player->getParent()->findMonster(player, cmnd);

    if(!creature) {
        player->print("You don't see that here.\n");
        return(0);
    }


    if(!creature->flagIsSet(M_CAN_PLEDGE_TO)) {
        player->print("You cannot join %N's organization.\n", creature);
        return(0);
    }

    if(creature->getClan())
        clan = gConfig->getClan(creature->getClan());
    if(!clan || creature->getClan() != clan->getId()) {
        player->print("%M is not a member of a clan.\n", creature);
        return(0);
    }
    if(!creature->flagIsSet(M_CAN_PLEDGE_TO)) {
        player->print("%M cannot induct you into a clan.\n", creature);
        return(0);
    }
    if(clan->getName() == "Clan of Gradius" && player->getRace() != DWARF) {
        player->print("You must be a Dwarf to join the Clan of Gradius.\n");
        return(0);
    }

    broadcast(player->getSock(), player->getParent(), "%M pledges %s allegiance to %N.",
        player, creature->hisHer(), creature);
    player->print("You swear your allegiance to %N as you join %s clan.\n", creature, creature->hisHer());
    player->print("You are now a member of the %s.\n", clan->getName().c_str());
    player->print("You %s %d experience and %d gold!\n", gConfig->isAprilFools() ? "lose" : "gain", clan->getJoin(), clan->getJoin() * 5);

    broadcast("### %s has just pledged %s allegiance to the %s!", player->getCName(),
        player->hisHer(), clan->getName().c_str());

    logn("log.pledge", "%s(L%d) pledges allegiance to %d-%s.\n",
         player->getCName(), player->getLevel(), clan->getId(), clan->getName().c_str());

    if(!player->halftolevel())
        player->addExperience(clan->getJoin());

    player->coins.add(clan->getJoin() * 5, GOLD);
    gServer->logGold(GOLD_IN, player, Money(clan->getJoin() * 5, GOLD), NULL, "ClanPledge");
    player->hp.restore();
    player->mp.restore();

    player->unhide();
    player->setFlag(P_PLEDGED);
    player->setClan(clan->getId());

    return(0);
}

//*********************************************************************
//                      cmdRescind
//*********************************************************************
// The rescind command allows a player to rescind allegiance to a given
// monster. Once a player has rescinded their allegiance, they will
// lose all the privileges of rescinded kingdom as well as a
// specified amount of experience and gold.

int cmdRescind(Player* player, cmd* cmnd) {
    Monster* creature=0;
    unsigned int amte=0;
    const Clan* clan=0;

    player->clearFlag(P_AFK);
    if(!player->ableToDoCommand())
        return(0);

    if(cmnd->num < 2) {
        player->print("Rescind to whom?\n");
        return(0);
    }

    if(!player->flagIsSet(P_PLEDGED)) {
        player->print("You have not joined a clan.\n");
        return(0);
    }

    if(!player->getRoomParent()->flagIsSet(R_RESCIND)) {
        player->print("You cannot rescind from your clan here.\n");
        return(0);
    }

    creature = player->getParent()->findMonster(player, cmnd);

    if(!creature) {
        player->print("You don't see that here.\n");
        return(0);
    }

    if(creature->getClan())
        clan = gConfig->getClan(creature->getClan());
    if(!clan || creature->getClan() != clan->getId()) {
        player->print("%M is not a member of a clan.\n", creature);
        return(0);
    }
    if(creature->getClan() != player->getClan()) {
        player->print("%M is not a member of your clan! You can't rescind here!\n", creature);
        return(0);
    }

    if(player->coins[GOLD] < clan->getRescind() * 2) {
        player->print("You do not have sufficient gold to rescind.\n");
        return(0);
    }

    if(!creature->flagIsSet(M_CAN_RESCIND_TO)) {
        player->print("%M cannot remove you from your clan.\n", creature);
        return(0);
    }

    if(amte >= player->getExperience()) {
        player->print("You cannot rescind until a later level.\n");
        return(0);
    }

    broadcast(player->getSock(), player->getParent(), "%M rescinds %s allegiance to %N.",
        player, creature->hisHer(), creature);
    player->print("%M scourns you as %s strips you of all your rights and privileges!\n",
        creature, creature->heShe());
    /*player->print("\nThe room fills with boos and hisses as you are ostracized from %N's organization.\n",
        creature);*/
    player->print("You are no longer a member of the %s.\n", clan->getName().c_str());

    broadcast("### %s has just rescinded %s allegiance to the %s!", player->getCName(),
        player->hisHer(), clan->getName().c_str());

    logn("log.pledge", "%s(L%d) rescinds allegiance to %d-%s.\n",
         player->getCName(), player->getLevel(), clan->getId(), clan->getName().c_str());


    amte = MIN(clan->getRescind(), player->getExperience());
    player->print("You lose %d experience and %d gold!\n", amte, clan->getRescind() * 2);
    player->subExperience(amte);
    player->coins.sub(clan->getRescind(), GOLD);
    gServer->logGold(GOLD_OUT, player, Money(clan->getRescind(), GOLD), NULL, "Rescind");
    player->hp.setCur(player->hp.getMax() / 3);
    player->mp.setCur(0);

    player->unhide();
    player->clearFlag(P_PLEDGED);
    player->setClan(0);

    return(0);
}


//*********************************************************************
//                      dmClanList
//*********************************************************************

int dmClanList(Player* player, cmd* cmnd) {
    std::map<int, Clan*>::iterator it;
    Clan *clan=0;
    bool    all = player->isCt() && cmnd->num > 1 && !strcmp(cmnd->str[1], "all");
    std::ostringstream oStr;
    oStr.imbue(std::locale(""));

    oStr << std::setfill(' ') << "Displaying Clans:";
    if(player->isCt() && !all)
        oStr << "  Type ^y*clanlist all^x to view all information.";
    oStr << "\n";

    for(it = gConfig->clans.begin() ; it != gConfig->clans.end() ; it++) {
        clan = (*it).second;

        oStr.setf(std::ios::right, std::ios::adjustfield);
        oStr << "Id: ^c" << std::setw(2) << clan->getId() << "^x   ^c";
        oStr.setf(std::ios::left, std::ios::adjustfield);

        oStr << std::setw(20) << clan->getName() << "^x  Join: ^c"
             << std::setw(7) << clan->getJoin() << "^x  Rescind: ^c"
             << std::setw(7) << clan->getRescind() << "^x\n";
    }
    player->printColor("%s\n", oStr.str().c_str());
    return(0);
}
