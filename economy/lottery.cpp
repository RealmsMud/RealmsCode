/*
 * lottery.cpp
 *   Code for the Highport Lottery
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

#include <bits/types/struct_tm.h>                   // for tm
#include <boost/lexical_cast/bad_lexical_cast.hpp>  // for bad_lexical_cast
#include <cstdio>                                   // for sprintf
#include <cstring>                                  // for memset, strcmp
#include <ctime>                                    // for ctime, localtime
#include <list>                                     // for list, operator==
#include <ostream>                                  // for basic_ostream::op...
#include <string>                                   // for allocator, string

#include "cmd.hpp"                                  // for cmd
#include "config.hpp"                               // for Config, gConfig
#include "factions.hpp"                             // for Faction
#include "flags.hpp"                                // for O_NO_DROP, R_LOTT...
#include "money.hpp"                                // for GOLD, Money
#include "mud.hpp"                                  // for TICKET_OBJ
#include "mudObjects/objects.hpp"                   // for Object, ObjectType
#include "mudObjects/players.hpp"                   // for Player
#include "mudObjects/rooms.hpp"                     // for BaseRoom
#include "mudObjects/uniqueRooms.hpp"               // for UniqueRoom
#include "proto.hpp"                                // for broadcast, logn
#include "random.hpp"                               // for Random
#include "server.hpp"                               // for GOLD_IN, Server
#include "xml.hpp"                                  // for newStringChild


// Max lottery #
#define MAXBONE 25

void bubblesort(int numbers[], int array_size) {
    int i, j, temp;

    for(i = (array_size - 1); i >= 0; i--) {
        for(j = 1; j <= i; j++) {
            if(numbers[j-1] > numbers[j]) {
                temp = numbers[j-1];
                numbers[j-1] = numbers[j];
                numbers[j] = temp;
            }
        }
    }
}
LottoTicket::LottoTicket(const char* name, const int pNumbers[], int pCycle) {
    for(int i = 0; i<6; i++) {
        numbers[i] = pNumbers[i];
    }
    owner = name;
    lottoCycle = pCycle;
}

void LottoTicket::saveToXml(xmlNodePtr rootNode) {
    xmlNodePtr curNode = xml::newStringChild(rootNode, "Ticket");
    
    xml::newStringChild(curNode, "Owner", owner);
    xml::newNumChild(curNode, "LottoCycle", lottoCycle);
    saveShortIntArray(curNode, "Numbers", "LotteryNum", numbers, 6);
}
LottoTicket::LottoTicket(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;
    
    while(curNode) {
            if(NODE_NAME(curNode, "Owner")) xml::copyToString(owner, curNode);
        else if(NODE_NAME(curNode, "LottoCycle")) xml::copyToNum(lottoCycle, curNode);
        else if(NODE_NAME(curNode, "Numbers")) xml::loadNumArray<short>(curNode, numbers, "LotteryNum", 6);
        
        curNode = curNode->next;
    }
}
void Config::setLotteryRunTime() {
    struct tm *timeToRun=nullptr;
    struct tm *curTime=nullptr;
    long i=0;
    // Sets the run time to the sunday of the next week

    timeToRun = new tm;
    memset(timeToRun, 0, sizeof(*timeToRun));
    i = time(nullptr);
    curTime = localtime(&i);
    timeToRun->tm_hour = 20;
    timeToRun->tm_min = 0;
    timeToRun->tm_sec = 0;
    timeToRun->tm_mon = curTime->tm_mon;
    timeToRun->tm_mday = curTime->tm_mday + (7 - curTime->tm_wday);
    timeToRun->tm_year = curTime->tm_year;

    i = mktime(timeToRun);

    delete timeToRun;

    lotteryRunTime = i;
}

void Config::runLottery() {
    int numbers[6];
    int go=1, x=0, j=0, reCalc=0;

    // Lottery hasn't been rigged
    if(true) {
        while(go) {
            numbers[x] = Random::get(1,MAXBONE);
            for(j = 0; j < x; j++) {
                if(numbers[x] == numbers[j]) {
                    reCalc = 1;
                    break;
                }
            }
            if(reCalc) {
                reCalc = 0;
                continue;
            }
            x++;
            if(x>5)
                go = 0;
        }
        bubblesort(numbers, 5);
        numbers[5] = Random::get(1,MAXBONE);

        for(x=0; x < 6; x++)
            lotteryNumbers[x] = numbers[x];
    } else {
        // It has been rigged!
    }
    lotteryCycle++;
    lotteryTicketsSold=0; // Reset tickets sold
    for( LottoTicket* ticket : tickets) {
        delete ticket;
    }
    tickets.clear();
    
    lotteryWinnings = 0; // Reset amount won
    if(lotteryWon == 1) // Reset the jackpot
        lotteryJackpot = 500000;
    else
        lotteryJackpot = (long)(lotteryJackpot * 1.15);
    lotteryJackpot = std::min<long>(lotteryJackpot, 3000000);
    lotteryWon = false;
    broadcast(
            "### The Highport Powerbone numbers have been drawn.\n### The jackpot is $%ld!",
            lotteryJackpot);
    broadcast(
            "### The winning numbers are %02d %02d %02d %02d %02d with a power bone of %02d!",
            lotteryNumbers[0], lotteryNumbers[1], lotteryNumbers[2],
            lotteryNumbers[3], lotteryNumbers[4], lotteryNumbers[5]);
    save();
    setLotteryRunTime();
}

int dmLottery(const std::shared_ptr<Player>& player, cmd* cmnd) {
    if(cmnd->num < 2 || strcmp(cmnd->str[1], "run") != 0) {
        player->print("Type \"*lottery run\" to run the lottery.\nTo view lottery info, type \"lottery\".\n");
        return(0);
    }
    gConfig->runLottery();
    return(0);
}

int createLotteryTicket(std::shared_ptr<Object>& object, const char *name) {
    int numbers[6];
    int go=1, x=0, j=0, reCalc=0;
    char desc[80];

    strcpy(desc, "");

    if(!loadObject(TICKET_OBJ, object))
        return(-1);

    while(go) {
        numbers[x] = Random::get(1,MAXBONE);
        for(j = 0; j < x; j++) {
            if(numbers[x] == numbers[j]) {
                reCalc = 1;
                break;
            }
        }
        if(reCalc) {
            reCalc = 0;
            continue;
        }
        x++;
        if(x>5)
            go = 0;
    }
    bubblesort(numbers, 5);
    numbers[5] = Random::get(1,MAXBONE);

    sprintf(
            desc,
            "A lottery ticket with the numbers: %02d %02d %02d %02d %02d  (%02d)",
            numbers[0], numbers[1], numbers[2], numbers[3], numbers[4], numbers[5]);
    object->description = desc;

    object->setType(ObjectType::LOTTERYTICKET);
    // Not valid untill next week
    object->setLotteryCycle(gConfig->getCurrentLotteryCycle() + 1);
    gConfig->increaseJackpot(gConfig->getLotteryTicketPrice()/2);
    object->setFlag(O_NO_DROP);
    for(x = 0; x < 6; x++)
        object->setLotteryNumbers(x, numbers[x]);

    auto* ticket = new LottoTicket(name, numbers, gConfig->getCurrentLotteryCycle()+1);
    gConfig->addTicket(ticket);
    return(0);
}
void Config::increaseJackpot(int amnt) {
    if(amnt < 0)
        return;
    lotteryJackpot += amnt;
    lotteryJackpot = std::min<long>(lotteryJackpot, 10000000);
}
void Config::addTicket(LottoTicket* ticket) {
    tickets.push_back(ticket);
    gConfig->lotteryTicketsSold++;
    gConfig->save();
}
int Config::getCurrentLotteryCycle() {
    return(lotteryCycle);
}
int Config::getLotteryEnabled() {
    return(lotteryEnabled);
}
long Config::getLotteryJackpot() {
    return(lotteryJackpot);
}
int Config::getLotteryTicketPrice() {
    return(lotteryTicketPrice);
}
void Config::getNumbers(short numberArray[]) {
    for(int i = 0; i < 6; i++)
        numberArray[i] = lotteryNumbers[i];
}
long Config::getLotteryWinnings() {
    return(lotteryWinnings);
}
void Config::winLottery() {
    lotteryWon = false;
    lotteryJackpot = 500000;
}
int Config::getLotteryTicketsSold() {
    return(lotteryTicketsSold);
}
void Config::addLotteryWinnings(long prize) {
    lotteryWinnings += prize;
}
std::string Config::getLotteryRunTimeStr() {
    return(ctime(&gConfig->lotteryRunTime));
}
time_t Config::getLotteryRunTime() {
    return(lotteryRunTime);
}
int cmdClaim(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<BaseRoom> inRoom = player->getRoomParent();
    std::shared_ptr<Object> ticket=nullptr;
    long    prize=0;

    if(!player->ableToDoCommand())
        return(0);

    if(!inRoom->flagIsSet(R_LOTTERY_OFFICE)) {
        player->print("You can only claim a prize in the lottery office.\n");
        return(0);
    }

    if(cmnd->num < 2) {
        player->print("Claim your prize from what?\n");
        return(0);
    }

    player->unhide();

    ticket = player->findObject(player, cmnd, 1);

    if(!ticket) {
        player->print("You don't have that.\n");
        return(0);
    }
    if(ticket->getType() != ObjectType::LOTTERYTICKET) {
        player->print("That isn't a lottery ticket!\n");
        return(0);
    }
    if(ticket->getLotteryNumbers(0) == 0) {
        player->print("That isn't a lottery ticket!\n");
        return(0);
    }


    if(player->inUniqueRoom() && !Faction::willDoBusinessWith(player, player->getUniqueRoomParent()->getFaction())) {
        player->print("The shopkeeper refuses to do business with you.\n");
        return(0);
    }


    if(ticket->getLotteryCycle() > gConfig->getCurrentLotteryCycle()) {
        player->print("Sorry that ticket can't be claimed untill after the next drawing!\n");
        return(0);
    }
    if(ticket->getLotteryCycle() < gConfig->getCurrentLotteryCycle()) {
        player->print("Sorry, that ticket's too old!\n");
        return(0);
    }
    prize = checkPrize(ticket);

    player->delObj(ticket, true);
    ticket.reset();

    if(prize == 0) {
        player->print("Sorry, this ticket isn't worth anything.\n");
        return(0);
    }
    logn("log.prizes", "%s just won %ld.\n", player->getCName(), prize);
    gConfig->addLotteryWinnings(prize);

    broadcast(player->getSock(), player->getParent(), "%s claims a Powerbone ticket.", player->getCName());

    if(prize != gConfig->getLotteryJackpot()) { // Didn't win the big pot
        player->print("Sorry you didn't win the jackpot this time, but you did win $%ld today!\n", prize);
        player->print("The lottery official hands you %ld gold coin%s.\n", prize, prize != 1 ? "s" : "");
        player->coins.add(prize, GOLD);
        Server::logGold(GOLD_IN, player, Money(prize, GOLD), nullptr, "Lottery");
        return(0);
    } else { // Big winner
        broadcast("### %s just won the Highport Powerbones' Jackpot of %ld gold coin%s!", player->getCName(), prize, prize != 1 ? "s" : "");
        // Reset the pot!
        player->print("The lottery official hands you %ld gold coin%s.\n", prize, prize != 1 ? "s" : "");
        player->coins.add(prize, GOLD);
        Server::logGold(GOLD_IN, player, Money(prize, GOLD), nullptr, "LotteryJackpot");
        gConfig->winLottery();
        return(0);
    }
}

int lotteryPrizes[] = { 0, // 0
        500, // 1
        1000, // 2
        3000, // 3
        7000, // 4
        7000, // 5
        10000, // 6
        10000, // 7
        50000, // 8
        250000, // 9
        500000 // 10
        };

int checkPrize(std::shared_ptr<Object>ticket) {
    short numbers[6];
    int matches=0, pb=0, i=0, j=0;

    if( ticket->getType() != ObjectType::LOTTERYTICKET ||
        ticket->getLotteryCycle() != gConfig->getCurrentLotteryCycle()
    )
        return(0);

    gConfig->getNumbers(numbers);

    if(ticket->getLotteryNumbers(5) == numbers[5])
        pb = 1;
    for(i=0; i<5; i++) {
        for(j = 0; j < 5; j++) {
            if(ticket->getLotteryNumbers(i) == numbers[j]) {
                matches++;
            }
        }
    }
    if(matches == 0 && pb == 1)
        return(lotteryPrizes[1]);
    else if(matches == 2 && pb == 0)
        return(lotteryPrizes[2]);
    else if(matches == 1 && pb == 1)
        return(lotteryPrizes[3]);
    else if(matches == 2 && pb == 1)
        return(lotteryPrizes[4]);
    else if(matches == 3 && pb == 0)
        return(lotteryPrizes[5]);
    else if(matches == 3 && pb == 1)
        return(lotteryPrizes[6]);
    else if(matches == 4 && pb == 0)
        return(lotteryPrizes[7]);
    else if(matches == 4 && pb == 1)
        return(lotteryPrizes[8]);
    else if(matches == 5 && pb == 0)
        return(lotteryPrizes[9]);
    else if(matches == 5 && pb == 1)
        return(gConfig->getLotteryJackpot());

    return(0);
}

int cmdLottery(const std::shared_ptr<Player>& player, cmd* cmnd) {
    short numbers[6];

    if(!player->ableToDoCommand())
        return(0);

    gConfig->getNumbers(numbers);
    player->print("Current Powerbone cycle: %-3d Current Jackpot: $%ld\nCurrent winning numbers: %02d %02d %02d %02d %02d  Powerbone: %02d\n",
        gConfig->getCurrentLotteryCycle(), gConfig->getLotteryJackpot(),
        numbers[0], numbers[1], numbers[2], numbers[3], numbers[4], numbers[5]);
    player->print("Time of the next drawing: %s", gConfig->getLotteryRunTimeStr().c_str());
    if(player->isDm()) {
        player->print("Winnings so far this cycle: %ld\nTickets sold: %d\n",
            gConfig->getLotteryWinnings(), gConfig->getLotteryTicketsSold());
    }
    return(0);
}
