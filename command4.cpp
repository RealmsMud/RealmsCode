/*
 * command4.cpp
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
#include "version.h"
#include "commands.h"
#include "clans.h"
#include "calendar.h"
#include "effects.h"
#include <sstream>
#include <iomanip>
#include <locale>


//*********************************************************************
//						cmdScore
//*********************************************************************
// This function shows a player their current hit points, magic points,
// experience, gold and level.

int cmdScore(Player* player, cmd* cmnd) {
	Player	*target = player;
	bool online=true;

	player->clearFlag(P_AFK);

	if(player->isBraindead()) {
		player->print("You are brain-dead. You can't do that.\n");
		return(0);
	}
	if(player->isBlind()) {
		player->printColor("^CYou're blind!\n");
		return(0);
	}
	player->clearFlag(P_AFK);


	if(player->isDm() && cmnd->num > 1) {
		cmnd->str[1][0] = up(cmnd->str[1][0]);
		target = gServer->findPlayer(cmnd->str[1]);
		if(!target) {
			loadPlayer(cmnd->str[1], &target);
			online = false;
		}
		if(!target) {
			player->print("That player does not exist.\n");
			return(0);
		}
	}
	
	target->score(player);

	if(!online)
		free_crt(target);
	return(0);
}

//*********************************************************************
//						score
//*********************************************************************
// This function shows a player their current hit points, magic points,
// experience, gold and level.

void Player::score(const Player* viewer) {
	const EffectInfo* eff=0;
	int		i=0;

	std::ostringstream oStr;


	viewer->print("%s the %s (level %d)", name, getTitle().c_str(), getLevel());

	oStr << gServer->delayedActionStrings(this);

	if(flagIsSet(P_HIDDEN))
		oStr << " ^c*Hidden*";
	if(isEffected("courage"))
		oStr << " ^r*Brave*";
	if(isEffected("fear"))
		oStr << " ^b*Fearful*";
	if(flagIsSet(P_SITTING))
		oStr << " ^x*Sitting*";
	if(flagIsSet(P_MISTED))
		oStr << " ^c*Misted*";
	if(isEffected("armor"))
		oStr << " ^y*Armor Spell*";
	if(isEffected("berserk"))
		oStr << " ^r*Berserked*";
	if(isPoisoned())
		oStr << " ^g^#*Poisoned*";


	if(isEffected("frenzy"))
	    oStr << " ^c*Frenzied*";
	if(isEffected("haste", true))
		oStr << " ^c*Hasted*";
	if(isEffected("slow", true))
		oStr << " ^g*Slowed*";
	if(isEffected("strength", true))
		oStr << " ^y*Strength*";
	if(isEffected("enfeeblement", true))
		oStr << " ^y*Enfeeblement*";
	if(isEffected("insight", true))
		oStr << " ^y*Insight*";
	if(isEffected("feeblemind", true))
		oStr << " ^y*Feeblemind*";
	if(isEffected("fortitude", true))
		oStr << " ^c*Fortitude*";
	if(isEffected("weakness", true))
		oStr << " ^g*Weakness*";
	if(isEffected("prayer", true))
		oStr << " ^y*Prayer*";
	if(isEffected("damnation", true))
		oStr << " ^y*Damnation*";


	eff = getEffect("drunkenness");
	if(eff)
		oStr << " ^o*" << eff->getDisplayName() << "^o*";

	if(isEffected("confusion"))
		oStr << " ^b^#*Confused*";
	if(flagIsSet(P_CHARMED))
		oStr << " ^C*Charmed*";
	if(flagIsSet(P_FOCUSED))
		oStr << " ^c*Focused*";
	if(isEffected("silence"))
		oStr << " ^m^#*Muted*";
	if(isDiseased())
		oStr << " ^r^#*Diseased*";

	if(isEffected("camouflage"))
		oStr << " ^g*Camouflaged*";
	if(isEffected("petrification"))
		oStr << " ^c*Petrified*";
	if(isEffected("hold-person"))
		oStr << " ^c*Magically Held*";
	if(isEffected("pray") || isEffected("dkpray"))
		oStr << " ^y*Prayed*";
	if(flagIsSet(P_UNCONSCIOUS)) {
		if(flagIsSet(P_SLEEPING))
			oStr << " ^c*Sleeping*";
		else
			oStr << " ^c*Unconscious*";
	}
	if(isBraindead())
		oStr << " ^y*Brain-dead*";
	if(isEffected("wounded"))
		oStr << " ^r*Wounded*";

	viewer->printColor(oStr.str().c_str());

	viewer->printColor("\n ^g%3d/%3d Hit Points", hp.getCur(), hp.getMax());
	if(getClass() == FIGHTER && flagIsSet(P_PTESTER))
		viewer->printColor("  ^g%3d/%3d Focus Points", focus.getCur(), focus.getMax());
	else if(hasMp())
		viewer->printColor("  ^g%3d/%3d Magic Points", mp.getCur(), mp.getMax());

	viewer->printColor("     ^rArmor: %d (%.0f%%)\n", getArmor(),
		(getDamageReduction(this)*100.0));
	viewer->printColor(" ^y%7ld Experience    %s    XP Needed: %7s\n",
		getExperience(), coins.str().c_str(),
		expToLevel(false).c_str());

	viewer->printColor("\n^MBank Balance: %s.\n",
		bank.str().c_str());
	viewer->printColor("Total Inventory Assets: %ld gold.\n",
		getInventoryValue());
	// if offline, player won't be in a room
	if(getConstRoomParent() && getConstRoomParent()->isWinter())
		viewer->printColor("Winter Protection: ^c%d%%\n", (int)(20 * winterProtection()) * 5);

	viewer->print("\n");
	viewer->printColor("Your size is: ^y%s^x.\n", getSizeName(getSize()).c_str());

	// show spells under also
	spellsUnder(viewer, this, false);

	if(getWarnings()) {
		viewer->printColor("\n^BWarnings: ");
		for(i=0; i<getWarnings(); i++)
			viewer->print("* ");
		viewer->print("\n");
	}

	// if they aren't logged on, they won't have a socket
	if(getSock()) {
		switch(getSock()->getMccp()) {
			case 1:
				viewer->printColor("^yMCCP V1 Enabled\n");
				break;
			case 2:
				viewer->printColor("^yMCCP V2 Enabled\n");
				break;
			default:
				viewer->printColor("^rMCCP Disabled\n");
				break;
		}
	}

	if(!statsAddUp())
		viewer->printColor("^RError:^x Your stats do not add up.\n");
}

//*********************************************************************
//						cmdDaily
//*********************************************************************

int cmdDaily(Player* player, cmd* cmnd) {
	Player* target = player;
	bool online = true;
	int		loop=0;

	player->clearFlag(P_AFK);

	if(player->isBraindead()) {
		player->print("You are brain-dead. You can't do that.\n");
		return(0);
	}

	if(player->isDm() && cmnd->num > 1) {
		cmnd->str[1][0] = up(cmnd->str[1][0]);
		target = gServer->findPlayer(cmnd->str[1]);
		if(!target) {
			loadPlayer(cmnd->str[1], &target);
			online = false;
		}
		if(!target) {
			player->print("That player does not exist.\n");
			return(0);
		}
	}

	while(loop++ <= DAILYLAST)
		update_daily(&target->daily[loop]);

	player->print("Daily actions remaining:\n\n");

	player->print("Broadcasts:     %d of %d remaining.\n", target->daily[DL_BROAD].cur, target->daily[DL_BROAD].max);
	if( target->isCt() || ((target->getClass() == CLERIC || target->getClass() == PALADIN) && target->spellIsKnown(S_HEAL)) ) {
		player->print("Heals:          %d of %d remaining.\n", target->daily[DL_FHEAL].cur, target->daily[DL_FHEAL].max);
	}
	if( target->isCt() || ((target->getClass() == RANGER || target->getClass() == DRUID) && target->spellIsKnown(S_TRACK))  ) {
		player->print("Tracks:         %d of %d remaining.\n", target->daily[DL_TRACK].cur,target->daily[DL_TRACK].max);
	}
	if( target->isCt() || ((target->getClass() == LICH || (target->getClass() == MAGE && !target->getSecondClass())) && target->getLevel() >= 10) ) {
		player->print("Enchants:       %d of %d remaining.\n", target->daily[DL_ENCHA].cur, target->daily[DL_ENCHA].max);
	}
	if( target->isCt() || (target->getClass() == MAGE && target->getLevel() >= 10)) {
		player->print("Transmutes:     %d of %d remaining.\n", target->daily[DL_RCHRG].cur, target->daily[DL_RCHRG].max);
	}
	if( target->isCt() || (target->getLevel() >= 9 && target->spellIsKnown(S_TELEPORT))) {
		player->print("Teleports:      %d of %d remaining.\n", target->daily[DL_TELEP].cur, target->daily[DL_TELEP].max);
	}
	if( target->isCt() || (target->getLevel() >= 16 && target->getClass() == PALADIN)) {
		player->print("Lay on Hands:   %d of %d remaining.\n", target->daily[DL_HANDS].cur, target->daily[DL_HANDS].max);
	}

	if( target->isCt() || (target->getLevel() >= 16 && target->getClass() == DEATHKNIGHT)) {
		player->print("Harm touches:   %d of %d remaining.\n", target->daily[DL_HANDS].cur, target->daily[DL_HANDS].max);
	}

	if( target->isDm() || (target->getLevel() >= 22 && target->getClass() == CLERIC && target->getDeity() == ARAMON && !target->getSecondClass())) {
		player->print("Bloodfusions:   %d of %d remaining.\n", target->daily[DL_RESURRECT].cur, target->daily[DL_RESURRECT].max);
	}
	if( target->isDm() || (target->getLevel() >= 19 && target->getClass() == CLERIC && target->getDeity() == CERIS)) {
		player->print("Resurrections:  %d of %d remaining.\n", target->daily[DL_RESURRECT].cur, target->daily[DL_RESURRECT].max);
	}

	/*	if( target->isCt() || (target->getLevel() >= 13 && target->spellIsKnown(S_ETHREAL_TRAVEL))) {
	player->print("E-Travels:      %d of %d remaining.\n", target->daily[DL_ETRVL].cur,target->daily[DL_ETRVL].max);
	} */
	player->print("\n");
	player->print("Can defecate?   %s\n", target->canDefecate() ? "Yes" : "No");
	player->print("Can be resurrected?   %s\n", (target->daily[DL_RESURRECT].cur > 0) ? "Yes" : "No");


	if(!online)
		free_crt(target);
	return(0);
}

//*********************************************************************
//						cmdHelp
//*********************************************************************
// This function allows a player to get help in general, or help for a
// specific command. If help is typed by itself, a list of commands
// is produced. Otherwise, help is supplied for the command specified

int cmdHelp(Player* player, cmd* cmnd) {
	char	file[80];
	player->clearFlag(P_AFK);

	if(player->isBraindead()) {
		player->print("You are brain-dead. You can't do that.\n");
		return(0);
	}

	if(cmnd->num < 2) {
		sprintf(file, "%s/helpfile.txt", Path::Help);
		viewFile(player->getSock(), file);
		return(DOPROMPT);
	}
	if(!checkWinFilename(player->getSock(), cmnd->str[1]))
		return(0);
	if(strchr(cmnd->str[1], '/')!=NULL) {
		player->print("You may not use backslashes.\n");
		return(0);
	}
	sprintf(file, "%s/%s.txt", Path::Help, cmnd->str[1]);
	viewFile(player->getSock(), file);
	return(DOPROMPT);
}

//*********************************************************************
//						cmdWelcome
//*********************************************************************
// Outputs welcome file to user, giving them info on how to play
// the game

int cmdWelcome(Player* player, cmd* cmnd) {
	char	file[80];
	player->clearFlag(P_AFK);

	if(player->isBraindead()) {
		player->print("You are brain-dead. You can't do that.\n");
		return(0);
	}

	sprintf(file, "%s/welcomerealms.txt", Path::Help);

	viewFile(player->getSock(), file);
	return(0);
}

//*********************************************************************
//						getTimePlayed
//*********************************************************************

bstring Player::getTimePlayed() const {
	std::ostringstream oStr;
	long	played = lasttime[LT_AGE].interval;

	if(played > 86400L)
		oStr << (played / 86400) << " Day" << ((played / 86400 == 1) ? ", " : "s, ");
	if(played > 3600L)
		oStr << (played % 86400L) / 3600L << " Hour" << ((played % 86400L) / 3600L == 1 ? ", " : "s, ");
	oStr << (played % 3600L) / 60L << " Minute" << ((played % 3600L) / 60L == 1 ? "" : "s");

	return(oStr.str());
}

//*********************************************************************
//						showAge
//*********************************************************************

void Player::showAge(const Player* viewer) const {
	if(birthday) {
		viewer->printColor("^gAge:^x  %d\n^gBorn:^x the %s of %s, the %s month of the year,\n      ",
			getAge(), getOrdinal(birthday->getDay()).c_str(),
			gConfig->getCalendar()->getMonth(birthday->getMonth())->getName().c_str(),
			getOrdinal(birthday->getMonth()).c_str());
		if(birthday->getYear()==0)
			viewer->print("the year the Kingdom of Bordia fell.\n");
		else
			viewer->print("%d year%s %s the fall of the Kingdom of Bordia.\n",
				abs(birthday->getYear()), abs(birthday->getYear())==1 ? "" : "s",
				birthday->getYear() > 0 ? "after" : "before");


		if(gConfig->getCalendar()->isBirthday(this)) {
			if(viewer == this)
				viewer->printColor("^yToday is your birthday!\n");
			else
				viewer->printColor("^yToday is %s's birthday!\n", name);
		}
	} else
		viewer->printColor("^gAge:^x  unknown\n");

	viewer->print("\n");

	bstring str = getCreatedStr();
	if(str != "")
		viewer->printColor("^gCharacter Created:^x %s\n", str.c_str());

	viewer->printColor("^gTime Played:^x %s\n\n", getTimePlayed().c_str());
}


//*********************************************************************
//						cmdAge
//*********************************************************************

int cmdAge(Player* player, cmd* cmnd) {
	Player	*target = player;

	player->clearFlag(P_AFK);

	if(player->isBraindead()) {
		player->print("You are brain-dead. You can't do that.\n");
		return(0);
	}

	if(player->isCt()) {
		if(cmnd->num > 1) {
			cmnd->str[1][0] = up(cmnd->str[1][0]);
			target = gServer->findPlayer(cmnd->str[1]);
			if(!target || (target->isDm() && !player->isDm())) {
				player->print("That player is not logged on.\n");
				return(0);
			}
		}
	}

	player->print("%s the %s (level %d)\n", target->name, target->getTitle().c_str(), target->getLevel());
	player->print("\n");

	target->showAge(player);
	return(0);
}

//*********************************************************************
//						cmdVersion
//*********************************************************************
// Shows the players the version of the mud and last compile time

int cmdVersion(Player* player, cmd* cmnd) {
	player->print("Mud Version: " VERSION "\nLast compiled " __TIME__ " on " __DATE__ ".\n");
	return(0);
}

//*********************************************************************
//						cmdInfo
//*********************************************************************

int cmdInfo(Player* player, cmd* cmnd) {
	Player	*target = player;

	player->clearFlag(P_AFK);

	if(player->isBraindead()) {
		player->print("You are brain-dead. You can't do that.\n");
		return(0);
	}

	if(player->isCt()) {
		if(cmnd->num > 1) {
			cmnd->str[1][0] = up(cmnd->str[1][0]);
			target = gServer->findPlayer(cmnd->str[1]);
			if(!target || (target->isDm() && !player->isDm())) {
				player->print("That player is not logged on.\n");
				return(0);
			}
			target->information(player);
			return(0);
		}
	}

	target->information();
	return(0);
}

//*********************************************************************
//						showSavingThrow
//*********************************************************************
// formatted saving throws

bstring showSavingThrow(const Player* viewer, const Player* player, bstring text, int st) {
	char str[100];

	if(	(viewer && viewer->isCt()) ||
		player->isCt()
	) {
		sprintf(str, "(%3d%%)", player->saves[st].chance);
		text += str;
	}
	if(	st==POI &&
		player->immuneToPoison() &&
		player->immuneToDisease()
	) {
		text += "^WN/A^x";
	} else {
		int save = MIN((1+player->saves[st].chance)/10, MAX_SAVE_COLOR-1);
		sprintf(str, "^%c%s^x", get_save_color(save), get_save_string(save));
		text += str;
	}
	return(text);
}

//*********************************************************************
//						information
//*********************************************************************
// This function displays a player's entire list of information, including
// player stats, proficiencies, level and class.

void Player::information(const Player* viewer, bool online) {
	int		numItems=0;
	long	i=0;
	bool	auth = viewer && viewer->isCt();
	std::ostringstream oStr;
	bstring txt = "";

	if(!auth)
		update();

	oStr.setf(std::ios::left, std::ios::adjustfield);
	// no commas when displaying staff screens
	oStr.imbue(std::locale(isStaff() ? "C" : ""));
	oStr << std::setfill(' ')
		 << "+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+\n";
	if(lastPassword != "" && viewer && viewer->isDm())
		oStr << "| Last Password: " << std::setw(14) << lastPassword << "                               ^W|\\^x            |\n";
	else
		oStr << "|                                                             ^W|\\^x            |\n";
	if(clan && auth)
		oStr << "+ Clan: " << std::setw(21) << gConfig->getClan(clan)->getName() << "                                 ^W<<\\         _^x +\n";
	else
		oStr << "+                                                             ^W<<\\         _^x +\n";

	txt = name;
	if(viewer && viewer->isCt()) {
		txt += "(" + getId() + ")";
	}
	txt += " the ";
	txt += getTitle();

	// use i to center the title for us
	i = 47 - txt.length();

	oStr << "|                                                              ^W/ \\       //^x |\n"
		<< "+ ^W.------------------------------------------------------------{o}______/|^x  +\n"
		<< "|^W<^x " << std::setw((int)i/2) << " " << txt << std::setw(i-(int)i/2) << " " << " ^W=====:::{*}///////////]^x  |\n"
		<< "+ ^W`------------------------------------------------------------{o}~~~~~~\\|^x  +\n";

	if(auth) {
		oStr << "| Password: " << std::setw(14) << password << " Pkills (in/won): ";
		oStr.setf(std::ios::right, std::ios::adjustfield);
		oStr << std::setw(5) << statistics.getPkwon();
		oStr.setf(std::ios::left, std::ios::adjustfield);
		oStr << "/" << std::setw(5) << statistics.getPkin() << " (";
		oStr.setf(std::ios::right, std::ios::adjustfield);
		oStr << std::setw(3) << statistics.pkRank();
		oStr << "%) ^W\\ /       \\\\^x |\n";
		oStr.setf(std::ios::left, std::ios::adjustfield);
	} else
		oStr << "|                                                              ^W\\ /       \\\\^x |\n";
	oStr << "+ ^W__________________________________                          <</         ~^x +\n"
		<< "|^W/\\                                 \\                         |/^x            |\n"
		<< "+^W\\_|^x Level: " << std::setw(2) << level << "    Race: " << std::setw(13)
			<< gConfig->getRace(race)->getName(12) << "^W|  _________________________________^x   +\n";

	txt = !cClass2 ? get_class_string(cClass) : getClassName(this);
	if(deity) {
		txt += ", ";
		txt += gConfig->getDeity(deity)->getName();
	}

	oStr << "|  ^W|^x Class: " << std::setw(23) << txt << "  ^W| /\\                                \\^x  |\n"
		<< "+  ^W|^x Alignment: ";

	oStr.setf(std::ios::right, std::ios::adjustfield);
	oStr << std::setw(7) << (flagIsSet(P_CHAOTIC) ? "Chaotic" : "Lawful");
	oStr.setf(std::ios::left, std::ios::adjustfield);

	txt = "Neutral";
	if(getAdjustedAlignment() < NEUTRAL)
		txt = "Evil";
	else if(getAdjustedAlignment() > NEUTRAL)
		txt = "Good";

	for(i=0; i<MAXWEAR; i++)
		if(ready[i])
			numItems++;
	numItems += count_bag_inv(this);


	oStr << " " << std::setw(13) << txt << "^W| \\_|^x Total Experience : " << std::setw(11) << experience << "^W|^x  +\n"
		<< "|  ^W|^x Time Played:                    ^W|   |^x Experience Needed: "
		<< std::setw(11) << expToLevel(false) << "^W|^x  |\n"
		<< "+  ^W|^x ";
	oStr.setf(std::ios::right, std::ios::adjustfield);
	oStr << std::setw(31) << getTimePlayed();
	oStr.setf(std::ios::left, std::ios::adjustfield);
	oStr << " ^W|   |^x Gold             : " << std::setw(11) << coins[GOLD] << "^W|^x  +\n"
		<< "|  ^W|   ______________________________|_  |^x Inventory Weight : " << std::setw(4) << getWeight() << "       ^W|^x  |\n"
		<< "+   ^W\\_/_______________________________/  |^x Inventory Bulk   : "
			<< std::setw(3) << getTotalBulk() << "(" << std::setw(3) << getMaxBulk() << ")   ^W|^x  +\n"
		<< "|                                        ^W|^x Total Items      : " << std::setw(4) << numItems << "       ^W|^x  |\n"
		<< "+       Hit Points   : ";

	oStr.setf(std::ios::right, std::ios::adjustfield);
	// no more commas
	oStr.imbue(std::locale("C"));
	oStr << std::setw(5) << hp.getCur() << "/" << std::setw(5) << hp.getMax()
		<< "       ^W|   ____________________________|_^x +\n"
		<< "|       ";

	if(cClass == FIGHTER && !cClass2 && flagIsSet(P_PTESTER))
		oStr << "Focus Points : " << std::setw(5) << focus.getCur() << "/" << std::setw(5) << focus.getMax();
	else if(hasMp())
		oStr << "Magic Points : " << std::setw(5) << mp.getCur() << "/" << std::setw(5) << mp.getMax();
	else {
		oStr.setf(std::ios::left, std::ios::adjustfield);
		oStr << "Armor        :   " << std::setw(9) << armor;
	}

	oStr.setf(std::ios::left, std::ios::adjustfield);
	oStr << "        ^W\\_/______________________________/^x|\n";

	if(!hasMp())
		oStr << "+                                                                           +\n";
	else
		oStr << "+       Armor        :   " << std::setw(9) << armor << "                                          +\n";

	oStr << "|                                                                           |\n"
		<< "+^W     /\\       ^x             _         ^WSaving Throws^x                         +\n"
		<< "|^W____/_ \\____  ^x Str: " << (strength.getCur() < 100 ? " " : "") << strength.getCur() << "^W |\\_\\\\-\\^x       " << std::setw(41) << showSavingThrow(viewer, this, "Poison/Disease:    ", POI) << "|\n"
		<< "+^W\\  ___\\ \\  /^x   Dex: " << (dexterity.getCur() < 100 ? " " : "") << dexterity.getCur() << "^W |   \\\\-|^x      " << std::setw(41) << showSavingThrow(viewer, this, "Death/Traps:       ", DEA) << "+\n"
		<< "|^W \\/ /  \\/ /  ^x  Con: " << (constitution.getCur() < 100 ? " " : "") << constitution.getCur() << "^W  \\ /~\\\\^x       " << std::setw(41) << showSavingThrow(viewer, this, "Breath/Explosions: ", BRE) << "|\n"
		<< "+^W / /\\__/_/\\  ^x  Int: " << (intelligence.getCur() < 100 ? " " : "") << intelligence.getCur() << "^W   `   \\\\^x      " << std::setw(41) << showSavingThrow(viewer, this, "Mental Attacks:    ", MEN) << "+\n"
		<< "|^W/__\\ \\_____\\^x   Pie: " << (piety.getCur() < 100 ? " " : "") << piety.getCur() << "^W        \\\\^x     " << std::setw(41) << showSavingThrow(viewer, this, "Spells:            ", SPL) << "|\n"
		<< "+^W    \\  /                        \\^x                                          +\n"
		<< "|^W     \\/                          `^x                                         |\n"
		<< "+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+\n";

	if(!auth && !isStaff())
		oStr << "^WMore Information:^x age, daily, effects, factions, forum, languages, property,\n"
			 << "                  quests, recipes, score/health, skills, spells, statistics\n";

	if(viewer)
		viewer->printColor("%s\n", oStr.str().c_str());
	else {
		printColor("%s", oStr.str().c_str());
		hasNewMudmail();
		print("\n");
	}
	if(!statsAddUp()) {
		if(viewer)
			viewer->printColor("^RError:^x %s's stats do not add up.\n", name);
		else
			printColor("^RError:^x Your stats do not add up.\n");
	}

	if(auth) {
		showAge(viewer);
		viewer->print("Bank: %-10lu  \n", bank[GOLD]);
		if(getRoomParent())
			viewer->print("Room: %s  \n", getRoomParent()->fullName().c_str());
		else
			viewer->print("Room: %s  \n", currentLocation.room.str().c_str());
		if(!online)
			viewer->print("Last login:  \n%s", ctime(&lastLogin));
		else
			viewer->print("Currently logged on.\n");
	}

}
