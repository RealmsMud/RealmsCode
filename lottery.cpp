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
 *  Creative Commons - Attribution - Non Commercial - Share Alike 3.0 License
 *    http://creativecommons.org/licenses/by-nc-sa/3.0/
 *  
 * 	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include "mud.h"
#include "factions.h"
#include <fcntl.h>

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
LottoTicket::LottoTicket(const char* name, int pNumbers[], int pCycle) {
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
			if(NODE_NAME(curNode, "Owner")) xml::copyToBString(owner, curNode);
		else if(NODE_NAME(curNode, "LottoCycle")) xml::copyToNum(lottoCycle, curNode);
		else if(NODE_NAME(curNode, "Numbers")) xml::loadNumArray<short>(curNode, numbers, "LotteryNum", 6);
		
		curNode = curNode->next;
	}
}
void Config::setLotteryRunTime() {
	struct tm *timeToRun=0;
	struct tm *curTime=0;
	long i=0;
	// Sets the run time to the sunday of the next week

	timeToRun = new tm;
	memset(timeToRun, 0, sizeof(*timeToRun));
	i = time(0);
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
	if(1) {
		while(go) {
			numbers[x] = mrand(1,MAXBONE);
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
		numbers[5] = mrand(1,MAXBONE);

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
	lotteryJackpot = MIN(lotteryJackpot, 3000000);
	lotteryWon = 0;
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

int dmLottery(Player* player, cmd* cmnd) {
	if(cmnd->num < 2 || strcmp(cmnd->str[1], "run")) {
		player->print("Type \"*lottery run\" to run the lottery.\nTo view lottery info, type \"lottery\".\n");
		return(0);
	}
	gConfig->runLottery();
	return(0);
}

int createLotteryTicket(Object **object, char *name) {
	int numbers[6];
	int go=1, x=0, j=0, reCalc=0;
	char desc[80];

	strcpy(desc, "");

	if(!loadObject(TICKET_OBJ, object))
		return(-1);

	while(go) {
		numbers[x] = mrand(1,MAXBONE);
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
	numbers[5] = mrand(1,MAXBONE);

	sprintf(
			desc,
			"A lottery ticket with the numbers: %02d %02d %02d %02d %02d  (%02d)",
			numbers[0], numbers[1], numbers[2], numbers[3], numbers[4],
			numbers[5]);
	(*object)->description = desc;

	(*object)->setType(LOTTERYTICKET);
	// Not valid untill next week
	(*object)->setLotteryCycle(gConfig->getCurrentLotteryCycle() + 1);
	gConfig->increaseJackpot(gConfig->getLotteryTicketPrice()/2);
	(*object)->setFlag(O_NO_DROP);
	for(x = 0; x < 6; x++)
		(*object)->setLotteryNumbers(x, numbers[x]);

	LottoTicket* ticket = new LottoTicket(name, numbers, gConfig->getCurrentLotteryCycle()+1);
	gConfig->addTicket(ticket);
	return(0);
}
void Config::increaseJackpot(int amnt) {
	if(amnt < 0)
		return;
	lotteryJackpot += amnt;
	lotteryJackpot = MIN(lotteryJackpot, 10000000);
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
	lotteryWon = 0;
	lotteryJackpot = 500000;
}
int Config::getLotteryTicketsSold() {
	return(lotteryTicketsSold);
}
void Config::addLotteryWinnings(long prize) {
	lotteryWinnings += prize;
}
bstring Config::getLotteryRunTimeStr() {
	return(ctime(&gConfig->lotteryRunTime));
}
time_t Config::getLotteryRunTime() {
	return(lotteryRunTime);
}
int cmdClaim(Player* player, cmd* cmnd) {
	BaseRoom *inRoom = player->getRoom();
	Object	*ticket=0;
	long	prize=0;

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

	ticket = findObject(player, player->first_obj, cmnd);

	if(!ticket) {
		player->print("You don't have that.\n");
		return(0);
	}
	if(ticket->getType() != LOTTERYTICKET) {
		player->print("That isn't a lottery ticket!\n");
		return(0);
	}
	if(ticket->getLotteryNumbers(0) == 0) {
		player->print("That isn't a lottery ticket!\n");
		return(0);
	}


	if(player->parent_rom && !Faction::willDoBusinessWith(player, player->parent_rom->getFaction())) {
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
	delete ticket;

	if(prize == 0) {
		player->print("Sorry, this ticket isn't worth anything.\n");
		return(0);
	}
	logn("log.prizes", "%s just won %ld.\n", player->name, prize);
	gConfig->addLotteryWinnings(prize);

	broadcast(player->getSock(), player->getParent(), "%s claims a Powerbone ticket.", player->name);

	if(prize != gConfig->getLotteryJackpot()) { // Didn't win the big pot
		player->print("Sorry you didn't win the jackpot this time, but you did win $%ld today!\n", prize);
		player->print("The lottery official hands you %ld gold coin%s.\n", prize, prize != 1 ? "s" : "");
		player->coins.add(prize, GOLD);
		gServer->logGold(GOLD_IN, player, Money(prize, GOLD), NULL, "Lottery");
		return(0);
	} else { // Big winner
		broadcast("### %s just won the Highport Powerbones' Jackpot of %ld gold coin%s!", player->name, prize, prize != 1 ? "s" : "");
		// Reset the pot!
		player->print("The lottery official hands you %ld gold coin%s.\n", prize, prize != 1 ? "s" : "");
		player->coins.add(prize, GOLD);
		gServer->logGold(GOLD_IN, player, Money(prize, GOLD), NULL, "LotteryJackpot");
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

int checkPrize(Object *ticket) {
	short numbers[6];
	int matches=0, pb=0, i=0, j=0;

	if(	ticket->getType() != LOTTERYTICKET ||
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

int cmdLottery(Player* player, cmd* cmnd) {
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
