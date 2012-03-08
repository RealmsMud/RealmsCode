/*
 * command5.cpp
 *	 Command handling/parsing routines.
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
#include "login.h"
#include "commands.h"
#include "guilds.h"
#include "web.h"
#include "calendar.h"


//*********************************************************************
//						who
//*********************************************************************
// This function outputs a list of all the players who are currently
// logged into the game.

int cmdWho(Player* player, cmd* cmnd) {
	int		cClass=0, chaos=0, clan=0, pledge=0;
	int		race=0, cls=0, law=0, guild=0;
	bool	found=false;

	player->clearFlag(P_AFK);

	if(player->isBraindead()) {
		player->print("You are brain-dead. You can't do that.\n");
		return(0);
	}

	if(player->isBlind()) {
		player->printColor("^CYou're blind!\n");
		return(0);
	}


	if(cmnd->num > 1) {

		lowercize(cmnd->str[1],0);

		switch (cmnd->str[1][0]) {
		case 'a':
			switch (cmnd->str[1][1]) {

			case 'd':
				cClass = -2;
				break;
			case 's':
				cClass = ASSASSIN;
				break;
			default:
				player->print("Parameter not unique.\n");
				return(0);
				break;
			}
			break;
			break;
		case 'b':
			switch (cmnd->str[1][3]) {
			case 'b':
				race = BARBARIAN;
				break;
			case 's':
				cClass = BERSERKER;
				break;
			case 'd':
				cClass = BARD;
				break;
			default:
				player->print("Parameter not unique.\n");
				return(0);
				break;
			}
			break;
		case 'c':
			switch (cmnd->str[1][1]) {
			case 'a':
				switch(cmnd->str[1][2]) {
				case 'm':
					race = CAMBION;
					break;
				case 'r':
					if(!player->isCt()) {
						player->print("You do not currently have that privilege.\n");
						return(0);
					} else
						cClass = CARETAKER;
					break;
				default:
					player->print("Parameter not unique.\n");
					return(0);
					break;
				}
				break;
			case 'h':
				chaos = 1;
				break;
			case 'l':
				switch (cmnd->str[1][2]) {
				case 'e':
					cClass = CLERIC;
					break;
				case 'a':
					switch(cmnd->str[1][3]) {
					case 's':
						cClass = player->getClass();
						break;
					case 'n':
						clan = 1;
						break;

					}
					break;
				default:
					player->print("Parameter not unique.\n");
					return(0);
					break;
				}
				break;
			default:
				player->print("Parameter not unique.\n");
				return(0);
				break;
			}
			break;

		case 'd':
			switch(cmnd->str[1][1]) {
			case 'a':
				race = DARKELF;
				break;
			case 'e':
				cClass = DEATHKNIGHT;
				break;
			case 'r':
				cClass = DRUID;
				break;
			case 'w':
				race = DWARF;
				break;
			case 'u':
				if(!player->isDm()) {
					player->print("You do not currently have that privilege.\n");
					return(0);
				} else
					cClass = DUNGEONMASTER;
				break;
			default:
				player->print("Parameter not unique.\n");
				return(0);
				break;
			}
			break;

		case 'e':
			race = ELF;
			break;
		case 'f':
			cClass = FIGHTER;
			break;
		case 'g':
			switch(cmnd->str[1][1]) {
			case 'n':
				race = GNOME;
				break;
			case 'o':
				race = GOBLIN;
				break;
			case 'u':
				guild = 1;
				break;
			default:
				player->print("Parameter not unique.\n");
				return(0);
				break;
			}
			break;

		case 'h':
			switch(cmnd->str[1][1]) {
			case 'u':
				race = HUMAN;
				break;
			case 'a':
				switch(cmnd->str[1][4]) {
				case 'e':
					race = HALFELF;
					break;
				case 'g':
					race = HALFGIANT;
					break;
				case 'l':
					race = HALFLING;
					break;
				case 'o':
					race = HALFORC;
					break;
				default:
					player->print("Parameter not unique.\n");
					return(0);
					break;
				}
				break;
			default:
				player->print("Parameter not unique.\n");
				return(0);
				break;
			}
			break;

		case 'k':
			switch(cmnd->str[1][1]) {
			case 'a':
				race = KATARAN;
				break;
			case 'o':
				race = KOBOLD;
				break;
			default:
				player->print("Parameter not unique.\n");
				return(0);
				break;
			}
			break;

		case 'l':
			switch(cmnd->str[1][1]) {
			case 'a':
				law = 1;
				break;
			case 'i':
				cClass = LICH;
				break;
			default:
				player->print("Parameter not unique.\n");
				return(0);
				break;
			}
			break;


		case 'm':
			switch(cmnd->str[1][1]) {
			case 'a':
				cClass = MAGE;
				break;
			case 'i':
				race = MINOTAUR;
				break;
			case 'o':
				cClass = MONK;
				break;
			default:
				player->print("Unknown parameter.\n");
				return(0);
				break;
			}
			break;

		case 'o':
			switch(cmnd->str[1][1]) {
			case 'g':
				race = OGRE;
				break;
			case 'r':
				race = ORC;
				break;
			default:
				player->print("Unknown parameter.\n");
				return(0);
				break;
			}
			break;

		case 'p':
			switch(cmnd->str[1][1]) {
			case 'a':
				cClass = PALADIN;
				break;
			case 'l':
				pledge = 1;
				break;
			default:
				player->print("Parameter not unique.\n");
				return(0);
				break;
			}
			break;

		case 'r':
			switch(cmnd->str[1][1]) {
			case 'a':
				cClass = RANGER;
				break;
			case 'o':
				cClass = ROGUE;
				break;
			default:
				player->print("Parameter not unique.\n");
				return(0);
				break;
			}
			break;

		case 's':
			race = SERAPH;
			break;
		case 't':
			switch(cmnd->str[1][1]) {
			case 'h':
				cClass = THIEF;
				break;
			case 'r':
				race = TROLL;
				break;
			case 'i':
				race = TIEFLING;
				break;
			default:
				player->print("Parameter not unique.\n");
				return(0);
				break;
			}
			break;

		case 'v':
			switch (cmnd->str[1][1]) {
			case 'a':
				cClass = VAMPIRE;
				break;
			case 'e':
				cClass = -3;
				break;
			default:
				player->print("Parameter not unique.\n");
				return(0);
				break;
			}
			break;
			break;
		case 'w':
			switch(cmnd->str[1][1]) {
			case 'a':
				cClass = -1;
				break;
			case 'e':
				cClass = WEREWOLF;
				break;
			default:
				player->print("Parameter not unique.\n");
				return(0);
				break;
			}
			break;
		case '.':
			cls = 1;
			break;

		default:
			player->print("Parameter not unique.\n");
			return(0);
			break;
		}// end main switch

	} // end if cmnd->num > 1

	std::ostringstream whoStr;
	bstring curStr;

	whoStr << "\n^BPlayers currently online:\n";
	whoStr << "-------------------------------------------------------------------------------^x\n";

	Player* target=0;
	for(std::pair<bstring, Player*> p : gServer->players) {
		target = p.second;

		if(!target->isConnected())
			continue;
		if(!player->canSee(target))
			continue;

		if(cls == 1)
			if(target->getClass() != player->getClass())
				continue;
		if(clan == 1)
			if(!target->getClan())
				continue;
		if(guild == 1)
			if(player->getGuild() && target->getGuild() != player->getGuild())
				continue;
		if(chaos == 1)
			if(!target->flagIsSet(P_CHAOTIC))
				continue;
		if(law == 1)
			if(target->flagIsSet(P_CHAOTIC))
				continue;
		if(pledge == 1)
			if((target->getClan() && target->getClan() == player->getClan()) || !target->getClan())
				continue;
		if(race > 0)
			if((player->willIgnoreIllusion() ? target->getRace() : target->getDisplayRace()) != race)
				continue;
		if(cClass == -1)
			if(!target->isPublicWatcher())
				continue;
		if(cClass == -2)
			if(!target->flagIsSet(P_ADULT))
				continue;
		if(cClass == -3)
			if(!target->flagIsSet(P_VETERAN))
				continue;
		if(cClass > 0)
			if(target->getClass() != cClass)
				continue;
		if(target->flagIsSet(P_MISTED) && !player->isEffected("true-sight") && !player->isCt() )
			continue;
		if(target->isInvisible() && !player->isEffected("detect-invisible") && !player->isCt() )
			continue;
		if(target->flagIsSet(P_GLOBAL_GAG) && player != target && !player->isCt())
			continue;

		found = true;

		curStr = target->getWhoString(false, true, player->willIgnoreIllusion());
		whoStr << curStr;
	}
	if(!found)
		player->print("Nobody found!\n");
	else {
		curStr = whoStr.str();
		player->printColor("%s\n", curStr.c_str());
	}

	return(0);
}

//*********************************************************************
//				classwho
//*********************************************************************
// This function outputs a list of all the players who are currently
// logged into the game which are the same class as the player doing
// the command.

int cmdClasswho(Player* player, cmd* cmnd) {
	cmnd->num = 2;
	strcpy(cmnd->str[0], "who");
	strcpy(cmnd->str[1], "class");
	return(cmdWho(player, cmnd));
}


//*********************************************************************
//						cmdWhois
//*********************************************************************
// The whois function displays a selected player's name, class, level
// title, age and gender

int cmdWhois(Player* player, cmd* cmnd) {
	Player	*target=0;

	player->clearFlag(P_AFK);

	if(player->isBraindead()) {
		player->print("You are brain-dead. You can't do that.\n");
		return(0);
	}

	if(cmnd->num < 2) {
		player->print("Whois who?\n");
		return(0);
	}

	lowercize(cmnd->str[1], 1);
	target = gServer->findPlayer(cmnd->str[1]);

	if(!target || !player->canSee(target)) {
		player->print("That player is not logged on.\n");
		return(0);
	}

	player->printColor("%s", target->getWhoString(true, true, player->willIgnoreIllusion()).c_str());
	return(0);
}

//*********************************************************************
//						cmdSuicide
//*********************************************************************
// This function is called whenever the player explicitly asks to
// commit suicide.

int cmdSuicide(Player* player, cmd* cmnd) {

	if(!player->ableToDoCommand())
		return(0);

	if(player->flagIsSet(P_NO_SUICIDE)) {
		player->print("You cannot suicide right now. You are in a 24 hour cooling-off period.\n");
		return(0);
	}
	// Prevents players from suiciding in jail
	if(player->inJail()) {
		player->print("You attempt to kill yourself and fail. It sure hurts though!\n");
		broadcast(player->getSock(), player->getParent(),"%M tries to kill %sself.", player, player->himHer());
		return(0);
	}

	if(cmnd->num < 2 || strcasecmp(cmnd->str[1], "yes")) {
		player->printColor("^rWARNING:^x This will completely erase your character!\n");
		player->print("To prevent accidental suicides you must confirm you want to suicide;\nto do so type 'suicide yes'\n");
		if(player->getGuild() && player->getGuildRank() == GUILD_MASTER)
			player->printColor("^yNote:^x if you want to control who takes over your guild after you suicide,\nbe sure to abdicate leadership first.\n");
		return(0);
	}

	//if(player->getLevel() > 2 && !player->isStaff())
		broadcast("### %s committed suicide! We'll miss %s dearly.", player->name, player->himHer());
	//else
	//	broadcast(isWatcher, "^C### %s committed suicide! We'll miss %s dearly.", player->name, player->himHer());

	logn("log.suicide", "%s(%s)-%d (%s)\n", player->name, player->getPassword().c_str(), player->getLevel(), player->getSock()->getHostname().c_str());

	deletePlayer(player);
	updateRecentActivity();
	return(0);
}


//*********************************************************************
//						deletePlayer
//*********************************************************************
// Does a true delete of a player and their files

void deletePlayer(Player* player) {
	char	file[80];
	bool hardcore = player->isHardcore();
	// cache the name because we will be deleting the player object
 	bstring name = player->name;

 	gConfig->deleteUniques(player);
 	gServer->clearAsEnemy(player);

	// remove minions before backup, since this affects other players as well
	player->clearMinions();

	// they are no longer the owner of any effects they have created
	gServer->removeEffectsOwner(player);

	// unassociate
	if(player->getForum() != "") {
		player->setForum("");
		webUnassociate(name);
	}

	// take the player out of any guilds
	updateGuild(player, GUILD_REMOVE);

	// save a backup - this will be the only copy of the player!
	if(player->getLevel() >= 7)
		player->save(false, LS_BACKUP);

	if(player->flagIsSet(P_CREATING_GUILD)) {
		// If they are the founder, this will return text, and we
		// need go no further. Otherwise, they are a supporter, and we
		// have to search for them.
		if(gConfig->removeGuildCreation(name) != "") {
			std::list<GuildCreation*>::iterator gcIt;
			for(gcIt = gConfig->guildCreations.begin(); gcIt != gConfig->guildCreations.end(); gcIt++) {
				// they were supporting this guild - stop
				if((*gcIt)->removeSupporter(name))
					break;
			}
			gConfig->saveGuilds();
		}
	}

	// this deletes the player object
	Socket* sock = player->getSock();
	player->uninit();
	free_crt(player,true);
	//gServer->clearPlayer(name);
	sock->setPlayer(NULL);

	// get rid of any files the player was using
	sprintf(file, "%s/%s.xml", Path::Player, name.c_str());
	unlink(file);

	sprintf(file, "%s/%s.txt", Path::Bank, name.c_str());
	unlink(file);
	sprintf(file, "%s/%s.txt", Path::Post, name.c_str());
	unlink(file);
	sprintf(file, "%s/%s.txt", Path::History, name.c_str());
	unlink(file);

	// handle removing and property this player owned
	gConfig->destroyProperties(name);

	if(hardcore)
		sock->reconnect(true);
	else
		sock->disconnect();
}


//*********************************************************************
//						cmdQuit
//*********************************************************************
// This function checks to see if a player is allowed to quit yet. It
// checks to make sure the player isn't in the middle of combat, and if
// so, the player is not allowed to quit (and 0 is returned).

int cmdQuit(Player* player, cmd* cmnd) {
	long	i=0, t=0;

	if(!player->ableToDoCommand())
		return(0);

	t = time(0);
	if(player->inCombat())
		i = player->getLTAttack() + 20;
	else
		i = player->getLTAttack() + 2;
	if(t < i) {
		player->pleaseWait(i-t);
		return(0);
	}
	if(player->inCombat())
		i = LT(player, LT_SPELL) + 20;
	else
		i = LT(player, LT_SPELL) + 2;

	if(t < i) {
		player->pleaseWait(i-t);
		return(0);
	}
	player->update();


	return(DISCONNECT);
}


//*********************************************************************
//						changeStats
//*********************************************************************

int cmdChangeStats(Player* player, cmd* cmnd) {
    player->print("Disabled\n");
    return(0);
	player->changeStats();
	return(0);
}

void Player::changeStats() {
	int a=0;

	if(!flagIsSet(P_CAN_CHANGE_STATS) && !isCt()) {
		print("You cannot change your stats at this time.\n");
		return;
	} else {

		for(a=0;a<5;a++) {
			tnum[a] = 0;
			tstat.num[a] = 0;
		}

		tstat.hp = 0;
		tstat.mp = 0;
		tstat.pp = 0;
		tstat.rp = 0;
		tstat.race = 0;
		tstat.cls = 0;
		tstat.cls2 = 0;
		tstat.level = 0;


		broadcast(::isCt, "^y### %s is choosing new stats.", name);
		printColor("^yPlease enter a new set of initial stats (56 points):\n");
		getSock()->setState(CON_CHANGING_STATS);
	}
}


//********************************************************************
//						changingStats
//********************************************************************
// This function allows a player to change their stats

void changingStats(Socket* sock, bstring str) {
	sock->getPlayer()->changingStats(str);
}
void Player::changingStats(bstring str) {
	int		a, n, i, k, l, sum=0;
	int		vnum[5];
	vstat	nstat, sendStat;

	switch(getSock()->getState()) {
	case CON_CHANGING_STATS:
		n = str.length();
		l = 0;
		k = 0;
		for(i=0; i<=n; i++) {
			if(str[i]==' ' || str[i]==0) {
				str[i] = 0;
				//bstring tmp = str.substr(l);
				vnum[k++] = atoi(&str[l]);
				l = i+1;
			}
			if(k>4)
				break;
		}
		if(k<5) {
			print("Please enter all 5 numbers.\n");
			print("Aborted.\n");
			getSock()->setState(CON_PLAYING);
			return;
		}
		sum = 0;
		for(i=0; i<5; i++) {
			if(vnum[i] < 3 || vnum[i] > 18) {
				print("No stats < 3 or > 18 please.\n");
				print("Aborted.\n");
				getSock()->setState(CON_PLAYING);
				return;
			}
			sum += vnum[i];
		}
		if(sum != 56) {
			print("Stat total must equal 56 points.\n");
			print("Aborted.\n");
			getSock()->setState(CON_PLAYING);
			return;
		}

		for(a=0;a<5;a++)
			tnum[a] = vnum[a];

		if(race == HUMAN) {
			print("Raise which stat?:\n[A] Strength, [B] Dexterity, [C] Constitution, [D] Intelligence, or [E] Piety.\n");
			getSock()->setState(CON_CHANGING_STATS_RAISE);
			return;
		}

		print("Your stats have been calculated.\n");
		print("Please press [ENTER].\n");
		getSock()->setState(CON_CHANGING_STATS_CALCULATE);
		return;
		break;
	case CON_CHANGING_STATS_CALCULATE:

		for(a=0;a<5;a++) {
			sendStat.num[a] = tnum[a];
		}

		calcStats(sendStat, &nstat);


		print("Your resulting stats will be as follows:\n");
		print("STR: %d   DEX: %d   CON: %d   INT: %d   PIE: %d\n", nstat.num[0], nstat.num[1], nstat.num[2], nstat.num[3], nstat.num[4]);
		print("Hit Points: %d\n", nstat.hp);
		print("Are these the stats you want? (Y/N)\n");


		for(a=0; a<5; a++)
			tstat.num[a] = nstat.num[a];
		tstat.hp = nstat.hp;

		getSock()->setState(CON_CHANGING_STATS_CONFIRM);

		// confirm y/n
		break;
	case CON_CHANGING_STATS_CONFIRM:
		if(low(str[0]) == 'y') {
			broadcast(::isCt, "^y### %s has chosen %s stats.", name, hisHer());

			print("New stats set.\n");

			strength.setCur((short)tstat.num[0]);
			dexterity.setCur((short)tstat.num[1]);
			constitution.setCur((short)tstat.num[2]);
			intelligence.setCur((short)tstat.num[3]);
			piety.setCur((short)tstat.num[4]);

			hp.setMax(tstat.hp);

			clearFlag(P_CAN_CHANGE_STATS);
			getSock()->setState(CON_PLAYING);
			return;

		} else {
			broadcast(::isCt,"^y### %s aborted choosing new stats.", name);
			print("Aborted.\n");
			getSock()->setState(CON_PLAYING);
			return;
		}
		break;
	case CON_CHANGING_STATS_RAISE:
		switch (low(str[0])) {
		case 'a':
			tnum[0]++;
			break;
		case 'b':
			tnum[1]++;
			break;
		case 'c':
			tnum[2]++;
			break;
		case 'd':
			tnum[3]++;
			break;
		case 'e':
			tnum[4]++;
			break;
		default:
			print("\nPlease choose one.\n");
			return;
			break;
		}

		print("Lower which stat?:\n[A] Strength, [B] Dexterity, [C] Constitution, [D] Intelligence, or [E] Piety.\n");
		getSock()->setState(CON_CHANGING_STATS_LOWER);
		return;
		break;
	case CON_CHANGING_STATS_LOWER:
		switch (low(str[0])) {
		case 'a':
			tnum[0]--;
			tnum[0] = MAX(1, tnum[0]);
			break;
		case 'b':
			tnum[1]--;
			tnum[1] = MAX(1, tnum[1]);
			break;
		case 'c':
			tnum[2]--;
			tnum[2] = MAX(1, tnum[2]);
			break;
		case 'd':
			tnum[3]--;
			tnum[3] = MAX(1, tnum[3]);
			break;
		case 'e':
			tnum[4]--;
			tnum[4] = MAX(1, tnum[4]);
			break;
		default:
			print("\nPlease choose one.");
			return;
			break;
		}

		print("Your stats have been calculated.\n");
		print("Please press [ENTER].\n");
		getSock()->setState(CON_CHANGING_STATS_CALCULATE);
		break;
	}
}



//*********************************************************************
//						timestr
//*********************************************************************

char *timestr(long t) {
	static char buf[BUFSIZ];
	if(t >= 60) {
		sprintf(buf, "%01ld:%02ld", t / 60, t % 60);
	} else {
		sprintf(buf, "%ld seconds", t);
	}
	return(buf);
}

#define TIMELEFT(name,ltflag) \
	tmp = MAX(0,(LT(player, (ltflag)) - t)); \
	player->print("Time left on current %s: %s.\n", name, timestr(tmp));
#define TIMEUNTIL(name,ltflag,time) \
	u = (time) - t + player->lasttime[(ltflag)].ltime;\
	if(u < 0) u = 0;\
	player->print("Time left before next %s: %s.\n", name, timestr(u));

//*********************************************************************
//						showAbility
//*********************************************************************

void showAbility(Player* player, const char *skill, const char *display, int lt, int tu, int flag = -1) {
	long	t = time(0), u=0, tmp=0;

	if(player->knowsSkill(skill)) {
		if(flag != -1 && player->flagIsSet(flag)) {
			TIMELEFT(display, lt);
		} else {
			TIMEUNTIL(display, lt, tu);
		}
	}
}


//*********************************************************************
//						cmdTime
//*********************************************************************
// This function outputs the time of day (realtime) to the player.

int cmdTime(Player* player, cmd* cmnd) {
	long	t = time(0), u=0, tmp=0, i=0;
	const CatRefInfo* cri = gConfig->getCatRefInfo(player->getRoomParent(), 1);
	bstring world = "";

	if(cri && cri->getWorldName() != "") {
		world += " of ";
		world += cri->getWorldName();
	}

	player->clearFlag(P_AFK);

	if(player->isBraindead()) {
		player->print("You are brain-dead. You can't do that.\n");
		return(0);
	}

	player->printColor("^cThe current time in the world%s:\n", world.c_str());
	gConfig->getCalendar()->printtime(player);

	if(gConfig->getCalendar()->isBirthday(player))
		player->printColor("^yToday is your birthday!\n");

	if(player->flagIsSet(P_PTESTER))
		player->print("Tick Interval: %d.\n", player->lasttime[LT_TICK].interval);

	if(cmnd->num < 2) {

		// for ceris/aramon when they resurrect dead people
		tmp = MAX(0,(LT(player, LT_NOMPTICK) - t));
		if(tmp > 0)
			player->print("Time until vitality is restored: %s.\n", timestr(tmp));


		// CT+ doesn't need to see all this info
		if(!player->isCt()) {
			if(player->getClass() == BARD && player->getLevel() >= 4) {
				TIMEUNTIL("bard song", LT_SING, player->lasttime[LT_SING].interval);
			}
			showAbility(player, "barkskin", "barkskin", LT_BARKSKIN, 600);
			showAbility(player, "berserk", "berserk", LT_BERSERK, 600);
			showAbility(player, "bite", "bite", LT_PLAYER_BITE, player->lasttime[LT_PLAYER_BITE].interval);
			showAbility(player, "bloodsac", "blood sacrifice", LT_BLOOD_SACRIFICE, 600);
			showAbility(player, "commune", "commune", LT_PRAY, 45);
			showAbility(player, "charm", "charm", LT_HYPNOTIZE, player->lasttime[LT_HYPNOTIZE].interval);
			showAbility(player, "creeping-doom", "creeping-doom", LT_SMOTHER, player->lasttime[LT_SMOTHER].interval);
			showAbility(player, "disarm", "disarm", LT_DISARM, player->lasttime[LT_DISARM].interval);
			showAbility(player, "drain", "drain life", LT_DRAIN_LIFE, player->lasttime[LT_DRAIN_LIFE].interval);
			showAbility(player, "smother", "earth-smother", LT_SMOTHER, player->lasttime[LT_SMOTHER].interval);
			showAbility(player, "enthrall", "enthrall", LT_HYPNOTIZE, player->lasttime[LT_HYPNOTIZE].interval);
			showAbility(player, "focus", "focus", LT_FOCUS, 600, P_FOCUSED);
			showAbility(player, "frenzy", "frenzy", LT_FRENZY, 600);
			showAbility(player, "harm", "harm touch", LT_LAY_HANDS, player->lasttime[LT_LAY_HANDS].interval);
			showAbility(player, "holyword", "holyword", LT_SMOTHER, player->lasttime[LT_SMOTHER].interval);
			showAbility(player, "howl", "howl", LT_HOWLS, 240);
			showAbility(player, "hypnotize", "hypnotize", LT_HYPNOTIZE, player->lasttime[LT_HYPNOTIZE].interval);
			if(	player->knowsSkill("identify") ||
				player->canBuildObjects()
			) {
				TIMEUNTIL("identify", LT_IDENTIFY, player->lasttime[LT_IDENTIFY].interval);
			}
			showAbility(player, "hands", "lay on hands", LT_LAY_HANDS, player->lasttime[LT_LAY_HANDS].interval);
			showAbility(player, "maul", "maul", LT_KICK, player->lasttime[LT_KICK].interval);
			showAbility(player, "meditate", "meditate", LT_MEDITATE, 90);
			showAbility(player, "mistbane", "mistbane", LT_FOCUS, 600, P_MISTBANE);
			showAbility(player, "poison", "poison", LT_DRAIN_LIFE, player->lasttime[LT_DRAIN_LIFE].interval);
			showAbility(player, "pray", "pray", LT_PRAY, 600);
			showAbility(player, "regenerate", "regenerate", LT_REGENERATE, player->lasttime[LT_REGENERATE].interval);
			showAbility(player, "renounce", "renounce", LT_RENOUNCE, player->lasttime[LT_RENOUNCE].interval);
			showAbility(player, "touch", "touch of death", LT_TOUCH_OF_DEATH, player->lasttime[LT_TOUCH_OF_DEATH].interval);
			showAbility(player, "turn", "turn undead", LT_TURN, player->lasttime[LT_TURN].interval);
			showAbility(player, "scout", "scout", LT_SCOUT, player->lasttime[LT_SCOUT].interval);
		}

		if(player->flagIsSet(P_OUTLAW)) {
			i = LT(player, LT_OUTLAW);
			player->print("Outlaw time remaining: ");
			if(i - t > 3600)
				player->print("%02d:%02d:%02d more hours.\n",(i - t) / 3600L, ((i - t) % 3600L) / 60L, (i - t) % 60L);
			else if((i - t > 60) && (i - t < 3600))
				player->print("%d:%02d more minutes.\n", (i - t) / 60L, (i - t) % 60L);
			else
				player->print("%d more seconds.\n", MAX((i - t),0));

		}

		if(player->flagIsSet(P_JAILED)) {
			if(	player->inUniqueRoom() &&
				player->getUniqueRoomParent()->flagIsSet(R_MOB_JAIL)
			)
				i = LT(player, LT_MOB_JAILED);
			else
				i = LT(player, LT_JAILED);

			player->print("NOTE: 0 Seconds means you're out any second.\n");
			player->print("Jailtime remaining(Approx): ");

			if(i - t > 3600)
				player->print("%02d:%02d:%02d more hours.\n",(i - t) / 3600L, ((i - t) % 3600L) / 60L, (i - t) % 60L);
			else if((i - t > 60) && (i - t < 3600))
				player->print("%d:%02d more minutes.\n", (i - t) / 60L, (i - t) % 60L);
			else
				player->print("%d more seconds.\n", MAX((i - t),0));
		}

		// All those confusing defines, i'll make up my own code
		i = 0;
		for(Monster* pet : player->pets) {
			if(pet->isMonster() && pet->isPet()) {
				i = 1;
				if(pet->isUndead())
					player->print("Time left before creature following you leaves/fades: %s\n",
						timestr(pet->lasttime[LT_ANIMATE].ltime+pet->lasttime[LT_ANIMATE].interval-t));
				else
					player->print("Time left before creature following you leaves/fades: %s\n",
						timestr(pet->lasttime[LT_INVOKE].ltime+pet->lasttime[LT_INVOKE].interval-t));

				//TIMEUNTIL("hire",LT_INVOKE,player->lasttime[LT_INVOKE].interval);
			}
		}

		// only show how long until next if they don't have a pet already
		if(!i) {
			if(	player->getClass() == DRUID ||
				(player->getClass() == CLERIC && player->getDeity() == GRADIUS))
			{
				TIMEUNTIL("conjure", LT_INVOKE, player->lasttime[LT_INVOKE].interval);
			}
			if(	player->getClass() == CLERIC &&
				player->getDeity() == ARAMON &&
				player->getAdjustedAlignment() <= REDDISH)
			{
				TIMEUNTIL("animate dead", LT_ANIMATE, player->lasttime[LT_ANIMATE].interval);
			}
		}
	}
	return(0);
}


//*********************************************************************
//						cmdSave
//*********************************************************************
// This is the function that is called when a players types save
// It calls save() (the real save function) and then tells the user
// the player was saved.  See save() for more details.

int cmdSave(Player* player, cmd* cmnd) {
	ASSERTLOG( player );

	player->clearFlag(P_AFK);

	if(player->isBraindead()) {
		player->print("You are brain-dead. You can't do that.\n");
		return(0);
	}

	player->save(true);
	// Save storage rooms
	if(player->inUniqueRoom() && player->getUniqueRoomParent()->info.isArea("stor"))
		gConfig->resaveRoom(player->getUniqueRoomParent()->info);

	player->print("Player saved.\n");
	return(0);
}
