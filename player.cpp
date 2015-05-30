/*
 * player.cpp
 *	 Player routines.
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 * 	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

// Mud includes
#include <arpa/telnet.h>
#include "mud.h"
#include "guilds.h"
#include "login.h"
#include "commands.h"
#include "move.h"
#include "effects.h"
#include "clans.h"
#include "property.h"
#include "unique.h"

// C includes
#include <math.h>

#include <iomanip>
#include <locale>

//********************************************************************
//				fixLts
//********************************************************************

void Creature::fixLts() {
	long tdiff=0, t = time(0);
	int i=0;
	if(isPet())  {
		tdiff = t - getMaster()->lasttime[LT_AGE].ltime;
	}
	else
		tdiff = t - lasttime[LT_AGE].ltime;
	for(i=0; i<MAX_LT; i++) {
		if(i == LT_JAILED)
			continue;
		// Fix pet fade on login here
		if(lasttime[i].ltime == 0 && lasttime[i].interval == 0)
		    continue;
		lasttime[i].ltime += tdiff;
		lasttime[i].ltime = MIN(t, lasttime[i].ltime);
	}
}

//********************************************************************
//				init
//********************************************************************
// This function initializes a player's stats and loads up the room he
// should be in. This should only be called when the player first
// logs on.

void Player::init() {
	char	file[80], str[50], watchers[128];
	BaseRoom *newRoom=0;
	long	t = time(0);
	int		watch=0;

	statistics.setParent(this);

	// always make sure size matches up with race
	if(size == NO_SIZE) {
		size = gConfig->getRace(race)->getSize();

		EffectInfo *e=0;
		if(isEffected("enlarge"))
			e = getEffect("enlarge");
		else if(isEffected("reduce"))
			e = getEffect("reduce");

		if(e)
			changeSize(0, e->getStrength(), e->getName() == "enlarge");

		if(!size)
			size = SIZE_MEDIUM;
	}


	setFlag(P_KILL_AGGROS);

	clearFlag(P_LINKDEAD);
	clearFlag(P_SPYING);
	clearFlag(P_READING_FILE);
	clearFlag(P_AFK);
	clearFlag(P_DIED_IN_DUEL);
	clearFlag(P_VIEW_ZONES);

	if(cClass == CLERIC && level >= 7 && flagIsSet(P_CHAOTIC))
		setFlag(P_PLEDGED);

	if(level <= ALIGNMENT_LEVEL && !flagIsSet(P_CHOSEN_ALIGNMENT))
		clearFlag(P_CHAOTIC);

	if(level > ALIGNMENT_LEVEL)
		setFlag(P_CHOSEN_ALIGNMENT);

	if((cClass == CLERIC && level < 7) || (cClass == CLERIC && !flagIsSet(P_CHAOTIC)))
		clearFlag(P_PLEDGED);

	setFlag(P_SECURITY_CHECK_OK);

	if(isdm(getName()))
		cClass = DUNGEONMASTER;
	else if(isDm())
		cClass = CARETAKER;



	/* change this later
	   strength.getCur() = strength.max;
	dexterity.getCur() = dexterity.max;
	constitution.getCur() = constitution.max;
	intelligence.getCur() = intelligence.max;
	piety.getCur() = piety.max;
	*/
	if(!isStaff()) {
		daily[DL_ENCHA].max = 3;
		daily[DL_FHEAL].max = MAX(3, 3 + (level) / 3);
		daily[DL_TRACK].max = MAX(3, 3 + (level) / 3);
		daily[DL_DEFEC].max = 1;
		//	daily[DL_TELEP].max = MAX(3, MAX(2, level/4));
		daily[DL_TELEP].max = 3;
		if(level < 13)
			daily[DL_TELEP].max = 1;
		//	daily[DL_ETRVL].max = MAX(3, MAX(2, level/4));
		daily[DL_RCHRG].max = MAX(7, level / 2);
		daily[DL_HANDS].max = 3;

		daily[DL_RESURRECT].max = 1;
		daily[DL_SILENCE].max = 3;
		daily[DL_HARM].max = 2;
		daily[DL_SCARES].max = 3;
	} else {
		daily[DL_DEFEC].max = 100;
	}

	if(!gServer->isRebooting())
		initBuilder();


	if(isEffected("mist") && !canMistNow())
		unmist();

	if(!isStaff()) {
		clearFlag(P_DM_INVIS);
		removeEffect("incognito");
	} else {
		// staff logs on with dmInvis
		setFlag(P_DM_INVIS);
	}


	if(isDm())
		clearFlag(P_BUGGED);



	//if((race != KATARAN && race != TROLL))
	learnLanguage(LCOMMON);	// All races speak common but troll and kataran.


	if(!current_language) {
		initLanguages();
		current_language = LCOMMON;
		setFlag(P_LANGUAGE_COLORS);
	}



	//	Paladins get auto know-aur
	if(cClass==PALADIN)
		addPermEffect("know-aura");
	//	Mages get auto d-m
	if(	(	cClass==MAGE ||
			cClass2 == MAGE
		) &&
		!(	cClass == MAGE &&
			cClass2 == ASSASSIN
		))
	{
		addPermEffect("detect-magic");
	}

	if(cClass == DRUID && level >= 7)
		addPermEffect("pass-without-trace");

	if(cClass == THIEF && cClass2 == MAGE)
		addPermEffect("detect-magic");

	if((cClass == MAGE || cClass2 == MAGE) && level >= 7)
		learnSpell(S_ARMOR);

	if(cClass == CLERIC && deity == CERIS && level >= 13)
		learnSpell(S_REJUVENATE);


	//  Werewolves get auto Detect-Invisibility at level 7
	if(isEffected("lycanthropy") && level >= 7)
		addPermEffect("detect-invisible");

	//  Rangers level 13+ can detect misted vampires.
	if(cClass == RANGER && level >= 13)
		addPermEffect("true-sight");

	// clear ignore flags
	clearFlag(P_IGNORE_CLASS_SEND);
	clearFlag(P_IGNORE_RACE_SEND);
	clearFlag(P_NO_BROADCASTS);
	clearFlag(P_IGNORE_NEWBIE_SEND);
	clearFlag(P_IGNORE_GOSSIP);
	clearFlag(P_IGNORE_CLAN);
	clearFlag(P_NO_TELLS);
	clearFlag(P_PORTAL);


	if(!flagIsSet(P_OUTLAW))
		setFlag(P_NO_SUMMON);

	clearFlag(P_LAG_PROTECTION_OPERATING);
	clearFlag(P_ALIASING);
	if(!isStaff())
		setFlag(P_PROMPT);

	strcpy(str, "");

	lasttime[LT_PLAYER_SAVE].ltime = t;
	lasttime[LT_PLAYER_SAVE].interval = SAVEINTERVAL;

	// SEARCH FOR ME
	lasttime[LT_TICK].ltime = t+30;
	lasttime[LT_TICK_SECONDARY].ltime = t+30;
	lasttime[LT_TICK_HARMFUL].ltime = t+30;

	// spam check
	lasttime[LT_PLAYER_SEND].ltime = t;
	lasttime[LT_PLAYER_SEND].misc = 0;

	if(currentLocation.mapmarker.getArea() != 0) {
		Area *area = gConfig->getArea(currentLocation.mapmarker.getArea());
		if(area)
			newRoom = area->loadRoom(0, &currentLocation.mapmarker, false);

	}
	// load up parent_rom for the broadcast below, but don't add the
	// player to the room or the messages will be out of order
	if(!newRoom) {
		Property *p = gConfig->getProperty(currentLocation.room);
		if(	p &&
			p->getType() == PROP_STORAGE &&
			!p->isOwner(getName()) &&
			!p->isPartialOwner(getName()) )
		{
			// default to bound location
			Location l = bound;
			// but go to previous room, if possible
			if(previousRoom.room.id || previousRoom.mapmarker.getArea())
				l = previousRoom;

			if(l.room.id)
				currentLocation.room = l.room;
			else
				gConfig->areaInit(this, l.mapmarker);

			if(this->currentLocation.mapmarker.getArea() != 0) {
		        Area *area = gConfig->getArea(currentLocation.mapmarker.getArea());
		        if(area)
		            newRoom = area->loadRoom(0, &currentLocation.mapmarker, false);
			}
		}
	}


	// area_room might get set by areaInit, so check again
	if(!newRoom) {
		UniqueRoom	*uRoom=0;
		if(!loadRoom(currentLocation.room, &uRoom)) {
			loge("%s: %s (%s) Attempted logon to bad or missing room!\n", getCName(),
				getSock()->getHostname().c_str(), currentLocation.room.str().c_str());
			// NOTE: Using ::isCt to use the global function, not the local function
			broadcast(::isCt, "^y%s: %s (%s) Attempted logon to bad or missing room (normal)!", getCName(),
				getSock()->getHostname().c_str(), currentLocation.room.str().c_str());
			newRoom = abortFindRoom(this, "init_ply");
			uRoom = newRoom->getAsUniqueRoom();
		}


		if(uRoom && !isStaff() && !gServer->isRebooting()) {
			if(	(	uRoom->flagIsSet(R_LOG_INTO_TRAP_ROOM) ||
					uRoom->flagIsSet(R_SHOP_STORAGE) ||
					uRoom->whatTraining()
				) &&
				uRoom->getTrapExit().id &&
				!loadRoom(uRoom->getTrapExit(), &uRoom)
			) {
				broadcast(::isCt, "^y%s: %s (%s) Attempted logon to bad or missing room!", getCName(),
			    	getSock()->getHostname().c_str(), uRoom->getTrapExit().str().c_str());
				newRoom = abortFindRoom(this, "init_ply");
				uRoom = newRoom->getAsUniqueRoom();
			}

			if(	uRoom &&
				(	uRoom->isFull() ||
					uRoom->flagIsSet(R_NO_LOGIN) ||
					(!isStaff() && !flagIsSet(P_PTESTER) && uRoom->isConstruction()) ||
					(!isStaff() && uRoom->flagIsSet(R_SHOP_STORAGE))
				)
			) {
				newRoom = getRecallRoom().loadRoom(this);
				if(!newRoom) {
					broadcast(::isCt, "^y%s: %s (%s) Attempted logon to bad or missing room!", getCName(),
						getSock()->getHostname().c_str(), getRecallRoom().str().c_str());
					newRoom = abortFindRoom(this, "init_ply");
				}
				uRoom = newRoom->getAsUniqueRoom();
			}
		}

		if(uRoom) {
			uRoom->killMortalObjects();
            newRoom = uRoom;
		}
	}

	//	str[0] = 0;
	if(!isDm()) {
		loge("%s(L:%d) (%s) %s. Room - %s (Port-%d)\n", getCName(), level,
		     getSock()->getHostname().c_str(), gServer->isRebooting() ? "reloaded" : "logged on",
		    		 newRoom->fullName().c_str(), Port);
	}
	if(isStaff())
		logn("log.imm", "%s  (%s) %s.\n",
				getCName(), getSock()->getHostname().c_str(),
		     gServer->isRebooting() ? "reloaded" : "logged on");


	// broadcast
	if(!gServer->isRebooting()) {
		setSockColors();
		broadcast_login(this, newRoom, 1);
	}

	// don't do the actual adding until after broadcast
	addToRoom(newRoom);

	checkDarkness();



	for(Monster* pet : pets) {
		pet->setMaster(this);
		pet->fixLts();

		pet->updateAttackTimer();
		pet->lasttime[LT_SPELL].interval = 3;
		pet->lasttime[LT_SPELL].ltime =
		pet->lasttime[LT_MOB_THIEF].ltime = t;

		pet->addToRoom(getRoomParent());
		gServer->addActive(pet);
	}

	fixLts();

	if(cClass == FIGHTER && !cClass2 && flagIsSet(P_PTESTER)) {
	    mp.setInitial(0);
	    focus.setInitial(100);
	    focus.clearModifiers();
        focus.addModifier("UnFocused", -100, MOD_CUR);
	}

	if(!gServer->isRebooting())
		bug("%s logged into room %s.\n", getCName(), getRoomParent()->fullName().c_str());


	wearCursed();
	if(!flagIsSet(P_NO_AUTO_WEAR))
		wearAll(this, true);

	computeLuck();
	update();

	Socket* sock = getSock();

	if(!gServer->isRebooting()) {
		sprintf(file, "%s/news.txt",Path::Help);
		viewLoginFile(sock, file);

		sprintf(file, "%s/newbie_news.txt",Path::Help);
		viewLoginFile(sock, file);

		if(isCt()) {
			sprintf(file, "%s/news.txt", Path::DMHelp);
			viewLoginFile(sock, file);
		}
		if(isStaff() && getName() != "Bane") {
			sprintf(file, "%s/news.txt", Path::BuilderHelp);
			viewLoginFile(sock, file);
		}
		if(isCt() || flagIsSet(P_WATCHER)) {
			sprintf(file, "%s/watcher_news.txt", Path::DMHelp);
			viewLoginFile(sock, file);
		}

		sprintf(file, "%s/latest_post.txt", Path::Help);
		viewLoginFile(sock, file, false);

		hasNewMudmail();
	}

	if(!gServer->isRebooting()) {
		strcpy(watchers, "");
		printColor("^yWatchers currently online: ");
		Player* ply;
		for(std::pair<bstring, Player*> p : gServer->players) {
			ply = p.second;

			if(!ply->isConnected())
				continue;
			if(!ply->isPublicWatcher())
				continue;
			if(!canSee(ply))
				continue;

			strcat(watchers, ply->getCName());
			strcat(watchers, ", ");
			watch++;
		}

		if(watch) {
			watchers[strlen(watchers) - 2] = '.';
			watchers[strlen(watchers) - 1] = 0;
			printColor("%s\n", watchers);
		} else
			printColor("None.\n");
	}

	if(isCt())
		showGuildsNeedingApproval(this);

	if(hp.getCur() < 0)
		hp.setCur(1);


	// only players and builders are effected
	if(!isCt()) {
		// players can't set eavesdropper flag anymore
		clearFlag(P_EAVESDROPPER);
		clearFlag(P_SUPER_EAVESDROPPER);
		// DM/CT only flag
		clearFlag(P_NO_COMPUTE);

		setMonkDice();

		if(lasttime[LT_AGE].ltime > time(0) ) {
			lasttime[LT_AGE].ltime = time(0);
			lasttime[LT_AGE].interval = 0;
			broadcast(::isCt, "^yPlayer %s had negative age and is now validated.\n", getCName());
			logn("log.validate", "Player %s had negative age and is now validated.\n", getCName());
		}

	}

	// Make sure everything is in order with their guild.
	if(guild) {
		if(!gConfig->getGuild(guild)) {
			if(guildRank >= GUILD_PEON)
				print("You are now guildless because your guild has been disbanded.\n");
			guild = guildRank = 0;
		} else if(!gConfig->getGuild(guild)->isMember(getName())) {
			guild = guildRank = 0;
		}
	}


	if(actual_level < level)
		actual_level = MAX(level, actual_level);

	killDarkmetal();

	if(weaponTrains != 0)
		print("You have %d weapon train(s) available!\n", weaponTrains);


	computeInterest(t, true);
}

//*********************************************************************
//						uninit
//*********************************************************************
// This function de-initializes a player who has left the game. This
// is called whenever a player quits or disconnects, right before save
// is called.

void Player::uninit() {
	int		i=0;
	long	t=0;
	char	str[50];

	// Save storage rooms
	if(inUniqueRoom() && getUniqueRoomParent()->info.isArea("stor"))
		gConfig->resaveRoom(getUniqueRoomParent()->info);

	courageous();
	clearMaybeDueling();
    removeFromGroup(!gServer->isRebooting());

	for(Monster* pet : pets) {
		if(pet->isPet()) {
			gServer->delActive(pet);
			pet->deleteFromRoom();
			free_crt(pet);
		} else {
			pet->setMaster(NULL);
		}
	}
	pets.clear();

	for(i=0; i<MAXWEAR; i++) {
		if(ready[i]) {
			// i is wearloc-1, so add 1
			unequip(i+1);
		}
	}

	if(!gServer->isRebooting())
		broadcast_login(this, this->getRoomParent(), 0);

	if(this->inRoom())
		deleteFromRoom();

	// TODO: Handle deleting from non rooms

	t = time(0);
	strcpy(str, (char *)ctime(&t));
	str[strlen(str)-1] = 0;
	if(!isDm() && !gServer->isRebooting())
		loge("%s logged off.\n", getCName());

	// Clean up the old "Extra" struct
	etag	*crm, *ctemp;

	crm = first_charm;
	while(crm) {
		ctemp = crm;
		crm = crm->next_tag;
		delete ctemp;
	}
	first_charm = 0;

	if(alias_crt) {
		alias_crt->clearFlag(M_DM_FOLLOW);
		alias_crt = 0;
	}
}

//*********************************************************************
//						courageous
//*********************************************************************

void Player::courageous() {
	int	**scared;

	scared = &(scared_of);
	if(*scared) {
		free(*scared);
		*scared = NULL;
	}
}

//*********************************************************************
//						checkTempEnchant
//*********************************************************************

void Player::checkTempEnchant( Object* object) {
	long i=0, t=0;
	if(object) {
		if( object->flagIsSet(O_TEMP_ENCHANT)) {
			t = time(0);
			i = LT(object, LT_ENCHA);
			if(i < t) {
				object->setArmor(MAX(0, object->getArmor() - object->getAdjustment()));
				object->setAdjustment(0);
				object->clearFlag(O_TEMP_ENCHANT);
				object->clearFlag(O_RANDOM_ENCHANT);
				if(isEffected("detect-magic"))
					printColor("The enchantment on your %s fades.\n", object->getCName());
			}
		}
		if(object->getType() == CONTAINER) {
			for(Object* subObj : object->objects) {
				checkTempEnchant(subObj);
			}
		}
	}
}

//*********************************************************************
//						checkEnvenom
//*********************************************************************

void Player::checkEnvenom( Object* object) {
	long i=0, t=0;
	if(object && object->flagIsSet(O_ENVENOMED)) {
		t = time(0);
		i = LT(object, LT_ENVEN);
		if(i < t) {
			object->clearFlag(O_ENVENOMED);
			object->clearEffect();
			printColor("The poison on your %s deludes.\n", object->getCName());
		}
	}
}

//*********************************************************************
//						checkInventory
//*********************************************************************
// Check inventory for temp enchant or envenom

void Player::checkInventory( ) {
	int i=0;

	// Check for temp enchant items carried/inventory/in containers
	for(Object* obj : objects) {
		checkTempEnchant(obj);
	}
	for(i=0; i<MAXWEAR; i++) {
		checkTempEnchant(ready[i]);
	}
}

//*********************************************************************
//						checkEffectsWearingOff
//*********************************************************************

void Player::checkEffectsWearingOff() {
	long t = time(0);
	int staff = isStaff();

	// Added P_STUNNED and LT_PLAYER_STUNNED stun for dodge code.
	if(flagIsSet(P_STUNNED)) {
		if(t > LT(this, LT_PLAYER_STUNNED)) {
			clearFlag(P_STUNNED);
		}
	}

	if(flagIsSet(P_FOCUSED) && (cClass == MONK || staff)) {
		if(t > LT(this, LT_FOCUS)) {
			printColor("^cYou lose your concentration.\n");
			clearFlag(P_FOCUSED);
			//computeAC();
			computeAttackPower();
		}
	}
	if(flagIsSet(P_MISTBANE)) {
		if(t > LT(this, LT_FOCUS)) {
			printColor("^bYour mistbane is ended.\n");
			clearFlag(P_MISTBANE);
		}
	}

	if(flagIsSet(P_OUTLAW)) {
		if(t > LT(this, LT_OUTLAW)) {
			printColor("^yYou are no longer an outlaw.\n");
			clearFlag(P_OUTLAW);
			clearFlag(P_OUTLAW_WILL_BE_ATTACKED);
			setFlag(P_NO_SUMMON);
			clearFlag(P_OUTLAW_WILL_LOSE_XP);
			clearFlag(P_NO_GET_ALL);
		}
	}

	// only force them to wake from sleeping unconsciousness when they're
	// completely healed.
	if(flagIsSet(P_UNCONSCIOUS)) {
		if(	(	!flagIsSet(P_SLEEPING) &&
				t > LT(this, LT_UNCONSCIOUS)
			) ||
			(	flagIsSet(P_SLEEPING) && (
				(hp.getCur() >= hp.getMax() && mp.getCur() >= mp.getMax()) ||
				(cClass == PUREBLOOD && !getRoomParent()->vampCanSleep(getSock()))
			) )
		) {
			printColor("^cYou wake up.\n");
			clearFlag(P_UNCONSCIOUS);
			wake();

			clearFlag(P_DIED_IN_DUEL);
			broadcast(getSock(), getRoomParent(), "%M wakes up.", this);
		}
	}

	if(flagIsSet(P_NO_SUICIDE)) {
		if(t > LT(this, LT_MOBDEATH)) {
			printColor("^yYour cooling-off period has ended.\n");
			clearFlag(P_NO_SUICIDE);
		}
	}
	if(flagIsSet(P_HIDDEN) && !staff) {
		if(t - lasttime[LT_HIDE].ltime > 300L) {
			printColor("^cShifting shadows expose you.\n");
			unhide(false);
		}
	}
	if(flagIsSet(P_FREE_ACTION)) {
		if(t > LT(this, LT_FREE_ACTION)) {
			printColor("^c^#You no longer magically move freely.\n");
			clearFlag(P_FREE_ACTION);
			computeAC();
			computeAttackPower();
		}
	}

	if(flagIsSet(P_NO_PKILL)) {
		if(t > LT(this, LT_NO_PKILL)) {
			printColor("^c^#You can now be pkilled again.\n");
			clearFlag(P_NO_PKILL);
		}
	}



	if(flagIsSet(P_DOCTOR_KILLER)) {
		if(t > LT(this, LT_KILL_DOCTOR) || staff) {
			printColor("^y^#The doctors have forgiven you.\n");
			clearFlag(P_DOCTOR_KILLER);
		}
	}


	if(flagIsSet(P_NO_TICK_MP)) {
		if(t > LT(this, LT_NOMPTICK) || staff) {
			printColor("^cYour magical vitality has returned.\n");
			clearFlag(P_NO_TICK_MP);
		}

	}

	if(flagIsSet(P_NO_TICK_HP)) {
		if(t > LT(this, LT_NOHPTICK) || staff) {
			printColor("^gYou now heal normally again.\n");
			clearFlag(P_NO_TICK_HP);
		}
	}

	if(flagIsSet(P_CANT_BROADCAST)) {
		if(t > LT(this, LT_NO_BROADCAST)) {
			printColor("^rYou can broadcast again, now don't abuse it this time.\n");
			clearFlag(P_CANT_BROADCAST);
		}
	}

	if(isEffected("mist")) {
		if(isDay() && !staff)
			unmist();
	}
	if(flagIsSet(P_CHARMED)) {
		if(t > LT(this, LT_CHARMED) || staff) {
			printColor("^yYou are again in control of your actions.\n");
			clearFlag(P_CHARMED);
		}
	}

	if(negativeLevels) {
		if(t > LT(this, LT_LEVEL_DRAIN) || staff) {
			long expTemp=0;
			negativeLevels--;
			if(negativeLevels) {
				printColor("^WYou have regained a lost level.\n");
				expTemp = experience;
				upLevel();
				experience = expTemp;

				lasttime[LT_LEVEL_DRAIN].ltime = t;
				lasttime[LT_LEVEL_DRAIN].interval = 60L + 5*bonus((int)constitution.getCur());
			} else {
				printColor("^WYou have recovered all your lost levels.\n");
				expTemp = experience;
				upLevel();
				experience = expTemp;
			}
		}
	}

	if(t > LT(this, LT_JAILED) && flagIsSet(P_JAILED)) {
		printColor("^rA demonic jailer just arrived.\n");
		printColor("The demonic jailer says, \"You have been released from your torment.\"\n");
		printColor("The demonic jailer casts word of recall on you.\n");

		broadcast(getSock(), getRoomParent(), "A demonic jailer just arrived.\nThe demonic jailer casts word of recall on %s.", getCName());
		broadcast(getSock(), getRoomParent(), "The demonic jailer sneers evilly and spits on you.\nThe demonic jailer vanishes.");
		broadcast("^R### Cackling demons shove %s from the Dungeon of Despair.", getCName());
		doRecall();

		clearFlag(P_JAILED);
	}


	if(	t > LT(this, LT_JAILED) &&
		inUniqueRoom() && getUniqueRoomParent()->flagIsSet(R_MOB_JAIL) &&
		!staff
	) {
		printColor("A jailer just arrived.\n");
		printColor("The jailer says, \"You're free to go...get out!\"\n");
		printColor("The jailer opens the cell door and shoves you out.\n");
		printColor("The jailer goes back to napping.\n");

		doRecall();
	}
}

//*********************************************************************
//						doPetrificationDmg
//*********************************************************************

bool Creature::doPetrificationDmg() {
	if(!isEffected("petrification"))
		return(false);

	wake("Terrible nightmares disturb your sleep!");
	printColor("^c^#Petrification spreads toward your heart.\n");
	hp.decrease(MAX(1,(hp.getMax()/15 - bonus((int)constitution.getCur()))));

	if(hp.getCur() < 1) {
		Player* pThis = getAsPlayer();
		if(pThis)
			pThis->die(PETRIFIED);
		else
			die(this);
		return(true);
	}
	return(false);
}



//*********************************************************************
//						update
//*********************************************************************
// This function checks up on all a player's time-expiring flags to see
// if some of them have expired.  If so, flags are set accordingly.

void Player::update() {
	BaseRoom* room=0;
	long 	t = time(0);
	int		item=0;
	bool	fighting = inCombat();

	lasttime[LT_AGE].interval += (t - lasttime[LT_AGE].ltime);
	lasttime[LT_AGE].ltime = t;

	checkInventory();

	if(flagIsSet(P_LAG_PROTECTION_SET) && flagIsSet(P_LAG_PROTECTION_ACTIVE) && level > 1) {
		// Suspends lag protect if this not in battle.
		if(!fighting)
			clearFlag(P_LAG_PROTECTION_ACTIVE);
	}

	// All mobs will not attack a petrified opponent.
	if(isEffected("petrification"))
		clearAsEnemy();

	if(flagIsSet(P_UNIQUE_TO_DECAY) && !fighting) {
		gConfig->uniqueDecay(this);
		clearFlag(P_UNIQUE_TO_DECAY);
	}


	checkEffectsWearingOff();

	if(isDiseased() && immuneToDisease())
		cureDisease();
	if(isPoisoned() && immuneToPoison())
		curePoison();

	//pulseEffects(t);

	if(mp.getCur() < 0)
		mp.setCur(0);

	room = getRoomParent();
	if(room && !flagIsSet(P_LINKDEAD))
		doRoomHarms(room, this);

	if(t > LT(this, LT_PLAYER_SAVE)) {
		lasttime[LT_PLAYER_SAVE].ltime = t;
		cmdSave(this, 0);
	}

	item = getLight();
	if(item && item != MAXWEAR+1) {
		if(ready[item-1]->getType() == LIGHTSOURCE) {
			ready[item-1]->decShotsCur();
			if(ready[item-1]->getShotsCur() < 1) {
				print("Your %s died out.\n", ready[item-1]->getCName());
				broadcast(getSock(), room, "%M's %s died out.", this, ready[item-1]->getCName());
			}
		}
	}

	if(isStaff() && flagIsSet(P_AUTO_INVIS) && !flagIsSet(P_DM_INVIS) && getSock() && (t - getSock()->ltime) > 6000) {
		printColor("^g*** Automatically enabling DM invisibility ***\n");
		setFlag(P_DM_INVIS);
	}

	// Fix funky stuff
	setMonkDice();
	computeAC();
	computeAttackPower();
}

//*********************************************************************
//						addObj
//*********************************************************************
// This function adds the object pointer to by the first parameter to
// the inventory of the player pointed to by the second parameter.

void Creature::addObj(Object* object) {
	Player* pPlayer = getAsPlayer();

	object->validateId();

	Hooks::run(this, "beforeAddObject", object, "beforeAddToCreature");
	object->clearFlag(O_JUST_LOADED);

	// players have big inventories; to keep the mud from searching them when it
	// doesnt need to, record a flag on the player
	if(pPlayer && object->flagIsSet(O_DARKMETAL))
		setFlag(P_DARKMETAL);
	if(object->flagIsSet(O_DARKNESS))
		setFlag(pPlayer ? P_DARKNESS : M_DARKNESS);

	object->addTo(this);
	//add(object);

	if(pPlayer)
		pPlayer->updateItems(object);

	Hooks::run(this, "afterAddObject", object, "afterAddToCreature");

	killDarkmetal();
}

//*********************************************************************
//						finishDelObj
//*********************************************************************
// This function removes the object pointer to by the first parameter
// from the player pointed to by the second. This does NOT DELETE THE
// OBJECT. You will have to do that yourself, if desired.

// we put in a choice to do darkmetal or not so we won't have
// recursion problems
// if you pass false to darkness, you MUST run checkDarkness() unless
// you are certain the item you are deleted isn't flagged O_DARKNESS

void Creature::finishDelObj(Object* object, bool breakUnique, bool removeUnique, bool darkmetal, bool darkness, bool keep) {
	if(darkmetal)
		killDarkmetal();
	if(breakUnique || removeUnique) {
		Player* player = getPlayerMaster();
		if(player) {
			if(breakUnique)
				Limited::remove(player, object);
			else if(removeUnique)
				Limited::deleteOwner(player, object);
		}
	}
	if(darkness)
		checkDarkness();
	if(!keep)
		object->clearFlag(O_KEEP);

	Hooks::run(this, "afterRemoveObject", object, "afterRemoveFromCreature");
}

//*********************************************************************
//						delObj
//*********************************************************************

void Creature::delObj(Object* object, bool breakUnique, bool removeUnique, bool darkmetal, bool darkness, bool keep) {
	Hooks::run(this, "beforeRemoveObject", object, "beforeRemoveFromCreature");

	// don't run checkDarkness if this isnt a dark item
	if(!object->flagIsSet(O_DARKNESS))
		darkness = false;
	object->clearFlag(O_BEING_PREPARED);
	object->clearFlag(O_HIDDEN);
	object->clearFlag(O_JUST_LOADED);

	// if it doesnt have a parent_crt, it's either being worn or is in a bag
	if(!object->inCreature()) {
		// the object is being worn
		if(object->getWearflag() && ready[object->getWearflag()-1] == object) {
			unequip(object->getWearflag(), UNEQUIP_NOTHING, false);
			finishDelObj(object, breakUnique, removeUnique, darkmetal, darkness, keep);
		} else {
			// the object is in a bag somewhere
			// problem is, we don't know which bag
			for(Object* obj : objects) {
				if(obj->getType() == CONTAINER) {
					for(Object* subObj : obj->objects ) {
						if(subObj == object) {
							obj->delObj(object);
							finishDelObj(object, breakUnique, removeUnique, darkmetal, darkness, keep);
							return;
						}
					}
				}
			}

			// not in their inventory? they must be wearing a bag
			for(int i=0; i < MAXWEAR; i++) {
				if(!ready[i])
					continue;
				if(ready[i]->getType() == CONTAINER) {
					for(Object* obj : ready[i]->objects) {
						if(obj == object) {
							ready[i]->delObj(object);
							finishDelObj(object, breakUnique, removeUnique, darkmetal, darkness, keep);
							return;
						}
					}
				}
			}
		}
		return;
	}
	object->removeFrom();
	finishDelObj(object, breakUnique, removeUnique, darkmetal, darkness, keep);
}

//*********************************************************************
//						computeAC
//*********************************************************************
// This function computes a player's armor class by
// examining their stats and the items they are holding.

void Player::computeAC() {
	int		ac=0, i;

	// Monks are a little more impervious to damage than other classes due to
	// their practice in meditation
	if(cClass == MONK)
		ac += level * 10;

	// Wolves have a tough skin that grows tougher as they level up
	if(isEffected("lycanthropy"))
		ac += level * 20;

	// Vamps have a higher armor at night
	if(isEffected("vampirism") && !isDay())
		ac += level * 5 / 2;


	for(i=0; i<MAXWEAR; i++) {
		if(ready[i] && ready[i]->getType() == ARMOR) {
			ac += ready[i]->getArmor();
			// penalty for wearing armor of the wrong size
			if(size && ready[i]->getSize() && size != ready[i]->getSize())
				ac -= ready[i]->getArmor()/2;
			if(ready[i]->getWearflag() < FINGER || ready[i]->getWearflag() > HELD)
				ac += (int)(ready[i]->getAdjustment()*4.4);
		}
	}

	if((cClass == DRUID || isCt()) && isEffected("barkskin")) {
		EffectInfo* barkskin = getEffect("barkskin");
		ac += (int)(((level+bonus(constitution.getCur())) * barkskin->getStrength())*4.4);
	}

	if(isEffected("armor"))
		ac += 200 + 200 * level /30;

// TODO: Possibly add in +armor for higher constitution?

	if(isEffected("berserk"))
		ac -= 100;
	if(isEffected("fortitude"))
		ac += 100;
	if(isEffected("weakness"))
		ac -= 100;

	armor = MAX(0, ac);
}


//*********************************************************************
//						getArmorWeight
//*********************************************************************

int Player::getArmorWeight() const {
	int weight=0;

	for(int i=0; i<MAXWEAR; i++) {
		if(	ready[i] && ready[i]->getType() == ARMOR &&
			(	(ready[i]->getWearflag() < FINGER) ||
				(ready[i]->getWearflag() > HELD)
			)
		)
			weight += ready[i]->getActualWeight();
	}

	return(weight);
}


//*********************************************************************
//						mprofic
//*********************************************************************
// This function returns the magical realm proficiency as a percentage

int mprofic(const Creature* player, int index) {
	const Player *pPlayer = player->getAsConstPlayer();
	long	prof_array[12];
	int	i=0, n=0, prof=0;

	switch(player->getClass()) {
	case MAGE:
		if(pPlayer && (pPlayer->getSecondClass() == ASSASSIN || pPlayer->getSecondClass() == THIEF)) {
			prof_array[0] = 0L;
			prof_array[1] = 1024L;
			prof_array[2] = 4092L;
			prof_array[3] = 8192L;
			prof_array[4] = 16384L;
			prof_array[5] = 32768L;
			prof_array[6] = 70536L;
			prof_array[7] = 119000L;
			prof_array[8] = 226410L;
			prof_array[9] = 709410L;
			prof_array[10] = 2973307L;
			prof_array[11] = 500000000L;
		} else {
			prof_array[0] = 0L;
			prof_array[1] = 1024L;
			prof_array[2] = 2048L;
			prof_array[3] = 4096L;
			prof_array[4] = 8192L;
			prof_array[5] = 16384L;
			prof_array[6] = 35768L;
			prof_array[7] = 85536L;
			prof_array[8] = 140000L;
			prof_array[9] = 459410L;
			prof_array[10] = 2073306L;
			prof_array[11] = 500000000L;

		}
		break;
	case LICH:
		prof_array[0] = 0L;
		prof_array[1] = 1024L;
		prof_array[2] = 2048L;
		prof_array[3] = 4096L;
		prof_array[4] = 8192L;
		prof_array[5] = 16384L;
		prof_array[6] = 35768L;
		prof_array[7] = 85536L;
		prof_array[8] = 140000L;
		prof_array[9] = 459410L;
		prof_array[10] = 2073306L;
		prof_array[11] = 500000000L;
		break;
	case CLERIC:
		if(pPlayer && (pPlayer->getSecondClass() == ASSASSIN || pPlayer->getSecondClass() == FIGHTER)) {
			prof_array[0] = 0L;
			prof_array[1] = 1024L;
			prof_array[2] = 8192L;
			prof_array[3] = 16384L;
			prof_array[4] = 32768L;
			prof_array[5] = 65536L;
			prof_array[6] = 105000L;
			prof_array[7] = 165410L;
			prof_array[8] = 287306L;
			prof_array[9] = 809410L;
			prof_array[10] = 3538232L;
			prof_array[11] = 500000000L;
		} else {
			prof_array[0] = 0L;
			prof_array[1] = 1024L;
			prof_array[2] = 4092L;
			prof_array[3] = 8192L;
			prof_array[4] = 16384L;
			prof_array[5] = 32768L;
			prof_array[6] = 70536L;
			prof_array[7] = 119000L;
			prof_array[8] = 226410L;
			prof_array[9] = 709410L;
			prof_array[10] = 2973307L;
			prof_array[11] = 500000000L;
		}
		break;
	case THIEF:
		if(pPlayer && pPlayer->getSecondClass() == MAGE) {
			prof_array[0] = 0L;
			prof_array[1] = 1024L;
			prof_array[2] = 8192L;
			prof_array[3] = 16384L;
			prof_array[4] = 32768L;
			prof_array[5] = 65536L;
			prof_array[6] = 105000L;
			prof_array[7] = 165410L;
			prof_array[8] = 287306L;
			prof_array[9] = 809410L;
			prof_array[10] = 3538232L;
			prof_array[11] = 500000000L;
		} else {
			prof_array[0] = 0L;
			prof_array[1] = 1024L;
			prof_array[2] = 40000L;
			prof_array[3] = 80000L;
			prof_array[4] = 120000L;
			prof_array[5] = 160000L;
			prof_array[6] = 205000L;
			prof_array[7] = 222000L;
			prof_array[8] = 380000L;
			prof_array[9] = 965410L;
			prof_array[10] = 5495000;
			prof_array[11] = 500000000L;
		}
		break;

	case FIGHTER:
		if(pPlayer && pPlayer->getSecondClass() == MAGE) {
			prof_array[0] = 0L;
			prof_array[1] = 1024L;
			prof_array[2] = 8192L;
			prof_array[3] = 16384L;
			prof_array[4] = 32768L;
			prof_array[5] = 65536L;
			prof_array[6] = 105000L;
			prof_array[7] = 165410L;
			prof_array[8] = 287306L;
			prof_array[9] = 809410L;
			prof_array[10] = 3538232L;
			prof_array[11] = 500000000L;
		} else {
			prof_array[0] = 0L;
			prof_array[1] = 1024L;
			prof_array[2] = 40000L;
			prof_array[3] = 80000L;
			prof_array[4] = 120000L;
			prof_array[5] = 160000L;
			prof_array[6] = 205000L;
			prof_array[7] = 222000L;
			prof_array[8] = 380000L;
			prof_array[9] = 965410L;
			prof_array[10] = 5495000;
			prof_array[11] = 500000000L;
		}
		break;
	case PALADIN:
	case BARD:
	case PUREBLOOD:
	case DRUID:
		prof_array[0] = 0L;
		prof_array[1] = 1024L;
		prof_array[2] = 4092L;
		prof_array[3] = 8192L;
		prof_array[4] = 16384L;
		prof_array[5] = 32768L;
		prof_array[6] = 70536L;
		prof_array[7] = 119000L;
		prof_array[8] = 226410L;
		prof_array[9] = 709410L;
		prof_array[10] = 2973307L;
		prof_array[11] = 500000000L;
		break;
	case DEATHKNIGHT:
	case MONK:
	case RANGER:
		prof_array[0] = 0L;
		prof_array[1] = 1024L;
		prof_array[2] = 8192L;
		prof_array[3] = 16384L;
		prof_array[4] = 32768L;
		prof_array[5] = 65536L;
		prof_array[6] = 105000L;
		prof_array[7] = 165410L;
		prof_array[8] = 287306L;
		prof_array[9] = 809410L;
		prof_array[10] = 3538232L;
		prof_array[11] = 500000000L;
		break;
	default:
		prof_array[0] = 0L;
		prof_array[1] = 1024L;
		prof_array[2] = 40000L;
		prof_array[3] = 80000L;
		prof_array[4] = 120000L;
		prof_array[5] = 160000L;
		prof_array[6] = 205000L;
		prof_array[7] = 222000L;
		prof_array[8] = 380000L;
		prof_array[9] = 965410L;
		prof_array[10] = 5495000;
		prof_array[11] = 500000000L;
		break;
	}

	n = player->getRealm((Realm)index);
	for(i=0; i<11; i++)
		if(n < prof_array[i+1]) {
			prof = 10*i;
			break;
		}

	prof += ((n - prof_array[i])*10) / (prof_array[i+1] - prof_array[i]);

	return(prof);
}

//*********************************************************************
//						getFallBonus
//*********************************************************************
// This function computes a player's bonus (or susceptibility) to falling
// while climbing.

int Player::getFallBonus()  {
	int	fall = bonus((int)dexterity.getCur())*5;

	for(int j=0; j<MAXWEAR; j++)
		if(ready[j])
			if(ready[j]->flagIsSet(O_CLIMBING_GEAR))
				fall += ready[j]->damage.getPlus()*3;
	return(fall);
}



//*********************************************************************
//						lowest_piety
//*********************************************************************
// This function finds the player with the lowest piety in a given room.
// The pointer to that player is returned. In the case of a tie, one of
// them is randomly chosen.

Player* lowest_piety(BaseRoom* room, bool invis) {
	Creature* player=0;
	int		totalpiety=0, pick=0;

	if(room->players.empty())
		return(0);

	for(Player* ply : room->players) {
		if(	ply->flagIsSet(P_HIDDEN) ||
			(	ply->isInvisible() &&
				!invis
			) ||
			ply->flagIsSet(P_DM_INVIS) )
		{
			continue;
		}
		totalpiety += MAX(1, (25 - ply->piety.getCur()));
	}

	if(!totalpiety)
		return(0);
	pick = mrand(1, totalpiety);

	totalpiety = 0;

    for(Player* ply : room->players) {
		if(	ply->flagIsSet(P_HIDDEN) ||
			(	ply->isInvisible() &&
				!invis
			) ||
			ply->flagIsSet(P_DM_INVIS) )
		{
			continue;
		}
		totalpiety += MAX(1, (25 - ply->piety.getCur()));
		if(totalpiety >= pick) {
			player = ply;
			break;
		}
	}

	return(player->getAsPlayer());
}

//*********************************************************************
//						getLight
//*********************************************************************
// This function returns true if the player in the first parameter is
// holding or wearing anything that generates light.

int Player::getLight() const {
	int i=0, light=0;

	for(i = 0; i < MAXWEAR; i++) {
		if (!ready[i])
			continue;
		if (ready[i]->flagIsSet(O_LIGHT_SOURCE)) {
			if ((ready[i]->getType() == LIGHTSOURCE &&
				ready[i]->getShotsCur() > 0) ||
				ready[i]->getType() != LIGHTSOURCE) {
				light = 1;
				break;
			}
		}
	}

	if(light)
		return (i + 1);
	return(0);
}

//***********************************************************************
//						sendPrompt
//***********************************************************************
// This function returns the prompt that the player should be seeing

void Player::sendPrompt() {
	bstring toPrint;

	if(fd < 0)
		return;

	// no prompt in this situation
	if(flagIsSet(P_SPYING) || flagIsSet(P_READING_FILE))
		return;

	if(flagIsSet(P_PROMPT) || flagIsSet(P_ALIASING)) {
		std::ostringstream promptStr;

		if(!flagIsSet(P_NO_EXTRA_COLOR))
			promptStr << alignColor();

		if(flagIsSet(P_ALIASING) && alias_crt) {
			promptStr << "(" << alias_crt->hp.getCur() << " H " << alias_crt->mp.getCur() << " M): ";
		} else {
			promptStr << "(" << hp.getCur() << " H";
			if(cClass != LICH && cClass != BERSERKER
					&& (cClass != FIGHTER || !flagIsSet(P_PTESTER)))
				promptStr << " " << mp.getCur() << " M";
			else if(cClass == FIGHTER && flagIsSet(P_PTESTER))
				promptStr << " " << focus.getCur() << " F";

			if(flagIsSet(P_SHOW_XP_IN_PROMPT))
				promptStr << " " << expToLevel(true);
			promptStr << "):^x ";

		}
		toPrint = promptStr.str();
	} else
		toPrint = ": ";

	if(flagIsSet(P_AFK))
		toPrint += "^r[AFK]^x ";
	if(flagIsSet(P_NEWLINE_AFTER_PROMPT))
		toPrint += "\n";

	// Send EOR if they want it, otherwise send GA
	if(getSock()->getEor() == 1) {
		unsigned char eor_str[] = {IAC, EOR, '\0' };
		toPrint += eor_str;
	} else if(!getSock()->isDumbClient()){
		unsigned char ga_str[] = {IAC, GA, '\0' };
		toPrint += ga_str;
	}

	mySock->write(toPrint);
}

//*********************************************************************
//						computeLuck
//*********************************************************************
// This sets the luck value for a given player

int Player::computeLuck() {
	int		num=0, alg=0, con=0, smrt=0;

	alg = abs(alignment);
	alg = alg + 1;

	alg /= 10;


	// alignment only matters for these classes
	if(cClass != PALADIN && cClass != CLERIC && cClass != DEATHKNIGHT && cClass != LICH)
		alg = 0;


	if(	!alg ||
		(cClass == PALADIN && deity != GRADIUS && getAdjustedAlignment() > NEUTRAL) ||
		(cClass == DEATHKNIGHT && getAdjustedAlignment() < NEUTRAL) ||
		(cClass == LICH && alignment <= -500) ||
		(cClass == CLERIC && (deity == ENOCH || deity == LINOTHAN || deity == KAMIRA) && getAdjustedAlignment() >= LIGHTBLUE) ||
		(cClass == CLERIC && (deity == ARAMON || deity == ARACHNUS) && getAdjustedAlignment() <= PINKISH)
	)
		alg = 1;

	if(cClass != LICH)  // Balances mages with liches for luck.
		con = constitution.getCur()/10;
	else
		con = piety.getCur()/10;

	smrt = intelligence.getCur()/10;

	num = 100*(smrt+con);
	num /= alg;

	if(ready[HELD-1] && ready[HELD-1]->flagIsSet(O_LUCKY))
		num += ready[HELD-1]->damage.getPlus();

	// Carrying around alot of gold isn't very lucky!
	if(!isStaff())
		num -= (coins[GOLD] / 20000);

	num = MAX(1, MIN(99, num));

	luck = num;
	return(num);
}

//*********************************************************************
//						getStatusStr
//*********************************************************************
// returns a status string that describes the hp condition of the creature

const char* Creature::getStatusStr(int dmg) {
	int health = hp.getCur() - dmg;

	if(health < 1)
		return "'s dead!";

	switch(MIN(health * 10 / (hp.getMax() ? hp.getMax() : 1), 10)) {
	case 10:
		return("'s unharmed.");
	case 9:
		return("'s relatively unscathed.");
	case 8:
		return("'s a little battered.");
	case 7:
		return("'s getting bruised.");
	case 6:
		return("'s noticeably bleeding.");
	case 5:
		return("'s having some trouble.");
	case 4:
		return(" doesn't look too good.");
	case 3:
		return("'s beginning to stagger.");
	case 2:
		return(" has some nasty wounds.");
	case 1:
		return(" isn't going to last much longer.");
	case 0:
		return(" is about to die!");
	}
	return("");
}


//*********************************************************************
//						checkForSpam
//*********************************************************************

bool Player::checkForSpam() {
	int		t = time(0);

	if(!isCt()) {
		if(lasttime[LT_PLAYER_SEND].ltime == t) {
			// 4 in one second is spam
			if(++lasttime[LT_PLAYER_SEND].misc > 3) {
				silenceSpammer();
				return(true);
			}
		} else if(lasttime[LT_PLAYER_SEND].ltime + 1 == t) {
			// 6 in 2 seconds is spam
			if(++lasttime[LT_PLAYER_SEND].misc > 5) {
				silenceSpammer();
				return(true);
			}
		} else if(lasttime[LT_PLAYER_SEND].ltime + 2 == t) {
			// 7 in 3 seconds is spam
			if(++lasttime[LT_PLAYER_SEND].misc > 6 ) {
				silenceSpammer();
				return(true);
			}
		} else {
			// reset spam counter
			lasttime[LT_PLAYER_SEND].ltime = t;
			lasttime[LT_PLAYER_SEND].misc = 1;
		}
	}


	return(false);
}


//*********************************************************************
//						silenceSpammer
//*********************************************************************

void Player::silenceSpammer() {
	if(isEffected("silence")) {
		// already spamming
		EffectInfo* silenceEffect = getEffect("silence");
		silenceEffect->setDuration(silenceEffect->getDuration() + 300);
		printColor("^rYou have been given an additional 5 minutes of silence for spamming again!\n");
	} else {
		// first spam
		addEffect("silence", 120, 1);

		printColor("^rYou have been silenced for 2 minutes for spamming!\n");
		broadcast(getSock(), getRoomParent(), "%s has been silenced for spamming!\n",getCName());
	}
}

//*********************************************************************
//						setMonkDice
//*********************************************************************

void Player::setMonkDice() {
	int	nLevel = MAX(0, MIN(level, MAXALVL));

	// reset monk dice?
	if(cClass == MONK) {
		damage = monk_dice[nLevel];
	} else if(cClass == WEREWOLF) {
		damage = wolf_dice[nLevel];
	}
}

//*********************************************************************
//						getMultiClassID
//*********************************************************************

int	getMultiClassID(char cls, char cls2) {
	int		id=0;

	if(!cls2)
		return(0);

	switch(cls) {
	case FIGHTER:
		switch(cls2) {
		case MAGE:
			id = 1;
			break;
		case THIEF:
			id = 2;
			break;
		}
		break;

	case CLERIC:
		switch(cls2) {
		case ASSASSIN:
			id = 3;
			break;
		case FIGHTER:
			id = 6;
			break;
		}
		break;

	case MAGE:
		switch(cls2) {
		case ASSASSIN:
			id = 7;
			break;
		case THIEF:
			id = 4;
			break;
		}
		break;

	case THIEF:
		id = 5;  // thief/mage is only thief-first combo
		break;
	}

	return(id);
}

//*********************************************************************
//						isOutdoors
//*********************************************************************

bool isOutdoors(Socket* sock) {
	if(sock && sock->getPlayer())
		return(sock->getPlayer()->getRoomParent()->isOutdoors());
	return(false);
}

//*********************************************************************
//						initLanguages
//*********************************************************************

void Player::initLanguages() {

	switch(race) {
	case DWARF:
		learnLanguage(LDWARVEN);
		learnLanguage(LGNOMISH);
		learnLanguage(LKOBOLD);
		learnLanguage(LGOBLINOID);
		learnLanguage(LORCISH);
		break;
	case ELF:
		learnLanguage(LELVEN);
		learnLanguage(LGNOMISH);
		learnLanguage(LORCISH);
		break;
	case HALFELF:
		learnLanguage(LELVEN);
		learnLanguage(LHALFLING);
		break;
	case HALFLING:
		learnLanguage(LHALFLING);
		learnLanguage(LGNOMISH);
		break;
	case ORC:
		learnLanguage(LORCISH);
		learnLanguage(LGIANTKIN);
		break;
	case HALFGIANT:
		learnLanguage(LGIANTKIN);
		learnLanguage(LOGRISH);
		learnLanguage(LTROLL);
		break;
	case GNOME:
		learnLanguage(LGNOMISH);
		learnLanguage(LDWARVEN);
		learnLanguage(LGOBLINOID);
		learnLanguage(LKOBOLD);
		break;
	case TROLL:
		learnLanguage(LTROLL);
		break;
	case HALFORC:
		learnLanguage(LORCISH);
		break;
	case OGRE:
		learnLanguage(LOGRISH);
		learnLanguage(LGIANTKIN);
		break;
	case DARKELF:
		learnLanguage(LDARKELVEN);
		learnLanguage(LORCISH);
		learnLanguage(LELVEN);
		learnLanguage(LDWARVEN);
		learnLanguage(LGNOMISH);
		learnLanguage(LKOBOLD);
		learnLanguage(LGOBLINOID);
		break;
	case GOBLIN:
		learnLanguage(LGOBLINOID);
		learnLanguage(LORCISH);
		break;
	case MINOTAUR:
		learnLanguage(LMINOTAUR);
		break;
	case SERAPH:
		learnLanguage(LELVEN);
		learnLanguage(LCELESTIAL);
		learnLanguage(LINFERNAL);
		learnLanguage(LGNOMISH);
		learnLanguage(LHALFLING);
		learnLanguage(LABYSSAL);
		break;
	case KOBOLD:
		learnLanguage(LKOBOLD);
		learnLanguage(LGNOMISH);
		learnLanguage(LDARKELVEN);
		learnLanguage(LOGRISH);
		learnLanguage(LGIANTKIN);
		break;
	case CAMBION:
		learnLanguage(LINFERNAL);
		learnLanguage(LDARKELVEN);
		learnLanguage(LELVEN);
		learnLanguage(LCELESTIAL);
		learnLanguage(LORCISH);
		learnLanguage(LGIANTKIN);
		learnLanguage(LGOBLINOID);
		break;
	case BARBARIAN:
		learnLanguage(LBARBARIAN);
		break;
	case KATARAN:
		learnLanguage(LKATARAN);
		break;
	case TIEFLING:
		learnLanguage(LHALFLING);
		learnLanguage(LINFERNAL);
		learnLanguage(LABYSSAL);
		learnLanguage(LORCISH);
		learnLanguage(LGOBLINOID);
		learnLanguage(LTIEFLING);
		break;
	} // End switch.

	switch(cClass) {
	case THIEF:
	case ASSASSIN:
	case BARD:
	case ROGUE:
		learnLanguage(LTHIEFCANT);
		break;
	case DRUID:
	case RANGER:
		learnLanguage(LDRUIDIC);
		break;
	case WEREWOLF:
		learnLanguage(LWOLFEN);
		break;
	case MAGE:
	case LICH:
		learnLanguage(LARCANIC);
		break;
	case CLERIC:
		switch(deity) {
		case ARAMON:
			learnLanguage(LINFERNAL);
			break;
		case ENOCH:
			learnLanguage(LCELESTIAL);
			break;
		case ARES:
			learnLanguage(LBARBARIAN);
			break;
		case KAMIRA:
			learnLanguage(LTHIEFCANT);
			break;
		}
		break;
	}
	return;
}

//*********************************************************************
//						doRecall
//*********************************************************************

void Player::doRecall(int roomNum) {
	UniqueRoom	*new_rom=0;
	BaseRoom *newRoom=0;

	if(roomNum == -1) {
		newRoom = recallWhere();
	} else {
		if(!loadRoom(roomNum, &new_rom))
			newRoom = abortFindRoom(this, "doRecall");
		else
			newRoom = new_rom;
	}

	courageous();
	deleteFromRoom();
	addToRoom(newRoom);
	doPetFollow();
}


//*********************************************************************
//						recallWhere
//*********************************************************************
// Because of ethereal plane, we don't always know where we're going to
// recall to. We need a function to figure out where we are going.

BaseRoom* Creature::recallWhere() {
	// A builder should never get this far, but let's not chance it.
	// Only continue if they can't load the perm_low_room.
	if(cClass == BUILDER) {
		UniqueRoom* uRoom=0;
		CatRef cr;
		cr.setArea("test");
		cr.id = 1;
		if(loadRoom(cr, &uRoom))
			return(uRoom);
	}

	if(	getRoomParent()->flagIsSet(R_ETHEREAL_PLANE) &&
		(mrand(1,100) <= 50)
	) {
		return(teleportWhere());
	}

	BaseRoom* room = getRecallRoom().loadRoom(getAsPlayer());
	// uh oh!
	if(!room)
		return(abortFindRoom(this, "recallWhere"));
	return(room);

}

//*********************************************************************
//						teleportWhere
//*********************************************************************
// Loops through rooms and finds us a place we can teleport to.
// This function will always return a room or it will crash trying to.

BaseRoom* Creature::teleportWhere() {
	BaseRoom *newRoom=0;
	const CatRefInfo* cri = gConfig->getCatRefInfo(getRoomParent());
	int		i=0, zone = cri ? cri->getTeleportZone() : 0;
	Area	*area=0;
	Location l;
	bool	found = false;



	// A builder should never get this far, but let's not chance it.
	// Only continue if they can't load the perm_low_room.
	if(cClass == BUILDER) {
		CatRef cr;
		cr.setArea("test");
		cr.id = 1;
		UniqueRoom* uRoom =0;
		if(loadRoom(cr, &uRoom))
			return(uRoom);
	}

	do {
		if(i>250)
			return(abortFindRoom(this, "teleportWhere"));
		cri = gConfig->getRandomCatRefInfo(zone);

		// if this fails, we have nowhere to teleport to
		if(!cri)
			return(getRoomParent());

		// special area used to signify overland map
		if(cri->getArea() == "area") {
			area = gConfig->getArea(cri->getId());
			l.mapmarker.set(area->id, mrand(0, area->width), mrand(0, area->height), mrand(0, area->depth));
			if(area->canPass(0, &l.mapmarker, true)) {
				//area->adjustCoords(&mapmarker.x, &mapmarker.y, &mapmarker.z);

				// don't bother sending a creature because we've already done
				// canPass check here
				//aRoom = area->loadRoom(0, &mapmarker, false);
				if(Move::getRoom(this, 0, &newRoom, false, &l.mapmarker)) {
					if(newRoom->isUniqueRoom()) {
						// recheck, just to be safe
						found = newRoom->getAsUniqueRoom()->canPortHere(this);
						if(!found)
							newRoom = 0;
					} else {
						found = true;
					}
				}
			}
		} else {
			l.room.setArea(cri->getArea());
			// if misc, first 1000 rooms are off-limits
			l.room.id = mrand(l.room.isArea("misc") ? 1000 : 1, cri->getTeleportWeight());
			UniqueRoom* uRoom = 0;

			if(loadRoom(l.room, &uRoom))
				found = uRoom->canPortHere(this);
			if(found)
				newRoom = uRoom;
		}

		i++;
	} while(!found);

	if(!newRoom)
		return(abortFindRoom(this, "teleportWhere"));
	return(newRoom);
}


//*********************************************************************
//						canPortHere
//*********************************************************************

bool UniqueRoom::canPortHere(const Creature* creature) const {
	// check creature-specific settings
	if(creature) {
		if(size && creature->getSize() && creature->getSize() > size)
			return(false);
		if(deityRestrict(creature))
			return(false);
		if(getLowLevel() > creature->getLevel())
			return(false);
		if(getHighLevel() && creature->getLevel() > getHighLevel())
			return(false);
	}

	// check room-specific settings
	if(	flagIsSet(R_NO_TELEPORT) ||
		flagIsSet(R_LIMBO) ||
		flagIsSet(R_VAMPIRE_COVEN) ||
		flagIsSet(R_SHOP_STORAGE) ||
		flagIsSet(R_JAIL) ||
		flagIsSet(R_IS_STORAGE_ROOM) ||
		flagIsSet(R_ETHEREAL_PLANE) ||
		isConstruction() ||
		whatTraining()
	)
		return(false);
	if(isFull())
		return(false);
	// artificial limits for the misc area
	if(info.isArea("misc") && info.id <= 1000)
		return(false);
	if(exits.empty())
		return(false);

	return(true);
}

//*********************************************************************
//						checkSkillsGain
//*********************************************************************
// setToLevel: Set skill level to player level - 1, otherwise set to whatever the skill gain tells us to

void Creature::checkSkillsGain(std::list<SkillGain*>::const_iterator begin, std::list<SkillGain*>::const_iterator end, bool setToLevel) {
	SkillGain *sGain=0;
	std::list<SkillGain*>::const_iterator sgIt;
	for(sgIt = begin ; sgIt != end ; sgIt++) {
		sGain = (*sgIt);
		if(sGain->getName() == "")
			continue;
		if(!sGain->hasDeities() || sGain->deityIsAllowed(deity)) {
			if(!knowsSkill(sGain->getName())) {
				if(setToLevel)
					addSkill(sGain->getName(), (level-1)*10);
				else
					addSkill(sGain->getName(), sGain->getGained());
				print("You have learned the fine art of '%s'.\n", gConfig->getSkillDisplayName(sGain->getName()).c_str());
			}
		}
	}
}


//*********************************************************************
//						loseRage
//*********************************************************************

void Player::loseRage() {
	if(!flagIsSet(P_BERSERKED_OLD))
		return;
	printColor("^rYour rage diminishes.^x\n");
	clearFlag(P_BERSERKED_OLD);

	if(cClass == CLERIC && deity == ARES)
		strength.upgradeSetCur(strength.getCur(false) - 30);
	else
		strength.upgradeSetCur(strength.getCur(false) - 50);
	computeAC();
	computeAttackPower();
}

//*********************************************************************
//						losePray
//*********************************************************************

void Player::losePray() {
	if(!flagIsSet(P_PRAYED_OLD))
		return;
	if(cClass != DEATHKNIGHT) {
		printColor("^yYou feel less pious.\n");
		piety.upgradeSetCur(piety.getCur(false) - 50);
	} else {
		printColor("^rYour demonic strength leaves you.\n");
		strength.upgradeSetCur(strength.getCur(false) - 30);
		computeAC();
		computeAttackPower();
	}
	clearFlag(P_PRAYED_OLD);
}

//*********************************************************************
//						loseFrenzy
//*********************************************************************

void Player::loseFrenzy() {
	if(!flagIsSet(P_FRENZY_OLD))
		return;
	printColor("^gYou feel slower.\n");
	clearFlag(P_FRENZY_OLD);
	dexterity.upgradeSetCur(dexterity.getCur(false) - 50);
}

//*********************************************************************
//						halftolevel
//*********************************************************************
// The following function will return true if a player is more then halfway in experience
// to their next level, causing them to not gain exp until they go level. If they are
// not yet half way, it will return false. Staff and players under level 5 are exempt.

bool Player::halftolevel() const {
	if(isStaff())
		return(false);

	// can save up xp until level 5.
	if(experience <= gConfig->expNeeded(5))
		return(false);

	if(experience >= gConfig->expNeeded(level) + (((unsigned)(gConfig->expNeeded(level+1) - gConfig->expNeeded(level)))/2)) {
		print("You have enough experience to train.\nYou are half way to the following level.\n");
		print("You must go train in order to gain further experience.\n");
		return(true);
	}

	return(false);
}

//*********************************************************************
//						getVision
//*********************************************************************

int Player::getVision() const {

	if(isStaff())
		return(MAX_VISION);

	int vision = 8;
	if(race == ELF)
		vision++;

	if(race == DARKELF) {
		if(isDay())
			vision--;
		else
			vision++;
	}

	if(isEffected("farsight"))
		vision *= 2;

	return(MIN(MAX_VISION, vision));
}


//*********************************************************************
//						getSneakChance
//*********************************************************************
// determines out percentage chance of being able to sneak

int Player::getSneakChance()  {
	int sLvl = (int)getSkillLevel("sneak");

	if(isStaff())
		return(101);

	// this is the base chance for most classes
	int chance = MIN(70, 5 + 2 * sLvl + 3 * bonus((int) dexterity.getCur()));

	switch(cClass) {
	case THIEF:
		if(cClass2 == MAGE)
			chance = tMIN(90, 5 + 8 * MAX(1,sLvl-2) + 3 * bonus((int) dexterity.getCur()));
		else
			chance = tMIN(90, 5 + 8 * sLvl + 3 * bonus((int) dexterity.getCur()));

		break;
	case ASSASSIN:
		chance = MIN(90, 5 + 8 * sLvl + 3 * bonus((int) dexterity.getCur()));
		break;
	case CLERIC:
		if(cClass2 == ASSASSIN)
			chance = tMIN(90, 5 + 8 * MAX(1,sLvl-2) + 3 * bonus((int) dexterity.getCur()));
		else if(deity == KAMIRA || deity == ARACHNUS)
			chance = tMIN(90, 5 + 8 * MAX(1,sLvl-2) + 3 * bonus((int) piety.getCur()));

		break;
	case FIGHTER:
		if(cClass2 == THIEF)
			chance = tMIN(90, 5 + 8 * MAX(1,sLvl-2) + 3 * bonus((int) dexterity.getCur()));

		break;
	case MAGE:
		if(cClass2 == THIEF || cClass2 == ASSASSIN)
			chance = tMIN(90, 5 + 8 * MAX(1,sLvl-3) + 3 * bonus((int) dexterity.getCur()));

		break;
	case DRUID:
		if(getConstRoomParent()->isForest())
			chance = tMIN(95 , 5 + 10 * sLvl + 3 * bonus((int) dexterity.getCur()));

		break;
	case RANGER:
		if(getConstRoomParent()->isForest())
			chance = tMIN(95 , 5 + 10 * sLvl + 3 * bonus((int) dexterity.getCur()));
		else
			chance = tMIN(83, 5 + 8 * sLvl + 3 * bonus((int) dexterity.getCur()));
		break;
	case ROGUE:
		chance = tMIN(85, 5 + 7 * sLvl + 3 * bonus((int) dexterity.getCur()));
		break;
	default:
		break;
	}

	if(isBlind())
		chance = tMIN(20, chance);

	if(isEffected("camouflage")) {
		if(getConstRoomParent()->isOutdoors())
			chance += 15;
		if(cClass == DRUID && getConstRoomParent()->isForest())
			chance += 5;
	}

	return(MIN(99, chance));
}



//*********************************************************************
//						breakObject
//*********************************************************************
// breaks a worn object and readds it to the player's inventory

bool Player::breakObject(Object* object, int loc) {
	bool darkness = false;

	if(!object)
		return(false);

	if(object->getShotsCur() < 1) {
		printColor("Your %s is broken.\n", object->getCName());
		broadcast(getSock(), getRoomParent(), "%M broke %s %s.", this, hisHer(), object->getCName());

		if(object->compass) {
			delete object->compass;
			object->compass = 0;
		}

		object->clearFlag(O_WORN);
		Limited::remove(this, object);

		if(object->flagIsSet(O_DARKNESS)) {
			darkness = true;
			object->clearFlag(O_DARKNESS);
		}

		if(object->flagIsSet(O_EQUIPPING_BESTOWS_EFFECT)) {
			object->clearFlag(O_EQUIPPING_BESTOWS_EFFECT);
			object->setEffect("");
			object->setEffectDuration(0);
			object->setEffectStrength(0);
		}

		if(object->getAdjustment())
			object->setAdjustment(0);
		if(object->flagIsSet(O_TEMP_ENCHANT)) {
			object->damage.setPlus(0);
			object->clearFlag(O_TEMP_ENCHANT);
			object->clearFlag(O_RANDOM_ENCHANT);
			if(isEffected("detect-magic"))
				print("The enchantment on your %s fades.\n", object->getCName());
		}
		if(object->flagIsSet(O_ENVENOMED)) {
			object->clearFlag(O_ENVENOMED);
			object->clearEffect();
			print("The poison on your %s becomes useless.\n", object->getCName());
		}

		if(loc != -1) {
			unequip(loc);
		} else {
			broadcast(::isDm, "^g>>> BreakObject: BadLoc (Loc:%d) %'s %s", loc, getCName(), object->getCName());
			if(ready[WIELD-1] == object) {
				unequip(WIELD);
			} else if(ready[HELD-1] == object) {
				unequip(HELD);
			}
		}
		if(darkness)
			checkDarkness();
		return(true);
	}
	return(false);
}


//********************************************************************
//				getWhoString
//********************************************************************

bstring Player::getWhoString(bool whois, bool color, bool ignoreIllusion) const {
	std::ostringstream whoStr;

	if(whois) {
		whoStr << "^bWhois for [" << getName() << "]\n";
		whoStr << "----------------------------------------------------------------------------------\n";
	}

	whoStr << (color ? "^x[^c" : "[") << std::setw(2) << level
		   << ":" << std::setw(4) << bstring(getShortClassName(this)).left(4)
		   << (color ? "^x] " : "] ");

	if(isHardcore())
		whoStr << (color ? "^y" : "") << "H ";
	else if(flagIsSet(P_OUTLAW))
		whoStr << (color ? "^r" : "") << "O ";
	else if( (flagIsSet(P_NO_PKILL) || flagIsSet(P_DIED_IN_DUEL) ||
			getConstRoomParent()->isPkSafe()) &&
			(flagIsSet(P_CHAOTIC) || clan || cClass == CLERIC) )
		whoStr << (color ? "^y" : "") << "N ";
	else if(flagIsSet(P_CHAOTIC)) // Chaotic
		whoStr << (color ? "^y" : "") << "C ";
	else // Lawful
		whoStr << (color ? "^y" : "") << "L ";

	if(color)
		whoStr << (isPublicWatcher() ? "^w" : "^g");

	whoStr << fullName() << " the ";
	if(whois)
		whoStr << getSexName(getSex()) << " ";

	whoStr << gConfig->getRace(getDisplayRace())->getAdjective();
	// will they see through the illusion?
	if(ignoreIllusion && getDisplayRace() != getRace())
		whoStr << " (" << gConfig->getRace(getRace())->getAdjective() << ")";
	whoStr << " " << getTitle();
	if(whois)
		whoStr << " (Age:" << getAge() << ")";


	if(	flagIsSet(P_DM_INVIS) ||
		isEffected("incognito") ||
		isInvisible() ||
		isEffected("mist") ||
		(flagIsSet(P_LINKDEAD) && !isPublicWatcher())
	) {
		if(color)
			whoStr << " ^w";
		if(flagIsSet(P_DM_INVIS))
			whoStr << "[+]";
		if(isEffected("incognito") )
			whoStr << "[g]";
		if(isInvisible() )
			whoStr << "[*]";
		if(isEffected("mist") )
			whoStr << "[m]";
		if(flagIsSet(P_LINKDEAD) && !isPublicWatcher() )
			whoStr << "[l]";
	}


	if(flagIsSet(P_AFK))
		whoStr << (color ? "^R" : "") << " [AFK]";

	if(deity) {
		whoStr << (color ? "^r" : "") << " (" << gConfig->getDeity(deity)->getName() << ")";
	} else if(clan) {
		whoStr << (color ? "^r" : "") << " (" << gConfig->getClan(clan)->getName() << ")";
	}

	if(guild && guildRank >= GUILD_PEON) {
		whoStr << (color ? "^y" : "") << " [" << getGuildName(guild) << "]";
	}

	whoStr << (color ? "^x" : "") << "\n";
	return(whoStr.str());
}


//*********************************************************************
//						getBound
//*********************************************************************

Location Player::getBound() {
	if(bound != getLimboRoom() && bound != getRecallRoom())
		return(bound);
	else {
		// we hardcode highport in case of failure
		const StartLoc* location = gConfig->getStartLoc("highport");
		bind(location);
		if(location)
			return(location->getBind());
	}
	// too many errors to handle!
	Location cr;
	cr.room.id = ROOM_BOUND_FAILURE;
	return(cr);
}


//*********************************************************************
//						expToLevel
//*********************************************************************

unsigned long Player::expToLevel() const {
	return(experience > gConfig->expNeeded(level) ? 0 : gConfig->expNeeded(level) - experience);
}

bstring Player::expToLevel(bool addX) const {
	if(level < MAXALVL) {
		std::ostringstream oStr;
		oStr.imbue(std::locale(isStaff() ? "C" : ""));
		oStr << expToLevel();
		if(addX)
			oStr << "X";
		return(oStr.str());
	}
	return("infinity");
}

bstring Player::expInLevel() const {
	int displayLevel = level;
	if(level > MAXALVL) {
		displayLevel = MAXALVL;
	}

	std::ostringstream oStr;
	oStr << tMIN<long>((experience - gConfig->expNeeded(displayLevel-1)),
			(gConfig->expNeeded(displayLevel) - gConfig->expNeeded(displayLevel-1)));
	return(oStr.str());
}

// TNL Max
bstring Player::expForLevel() const {
	int displayLevel = level;
	if(level > MAXALVL) {
		displayLevel = MAXALVL;
	}

	std::ostringstream oStr;
	oStr << (gConfig->expNeeded(displayLevel) - gConfig->expNeeded(displayLevel-1));
	return(oStr.str());
}

bstring Player::expNeededDisplay() const {
	if(level < MAXALVL) {
		std::ostringstream oStr;
		oStr.imbue(std::locale(isStaff() ? "C" : ""));
		oStr << gConfig->expNeeded(level);
		return(oStr.str());
	}
	return("infinity");
}
//*********************************************************************
//						exists
//*********************************************************************

bool Player::exists(bstring name) {
	char	file[80];
	sprintf(file, "%s/%s.xml", Path::Player, name.c_str());
	return(file_exists(file));
}

//*********************************************************************
//						inList functions
//*********************************************************************

bool Player::inList(const std::list<bstring>* list, bstring name) const {
	std::list<bstring>::const_iterator it;

	for(it = list->begin(); it != list->end() ; it++) {
		if((*it) == name)
			return(true);
	}
	return(false);
}


bool Player::isIgnoring(bstring name) const {
	return(inList(&ignoring, name));
}
bool Player::isGagging(bstring name) const {
	return(inList(&gagging, name));
}
bool Player::isRefusing(bstring name) const {
	return(inList(&refusing, name));
}
bool Player::isDueling(bstring name) const {
	return(inList(&dueling, name));
}
bool Player::isWatching(bstring name) const {
	return(inList(&watching, name));
}

//*********************************************************************
//						showList
//*********************************************************************

bstring Player::showList(const std::list<bstring>* list) const {
	std::ostringstream oStr;
	std::list<bstring>::const_iterator it;
	bool initial=false;

	for(it = list->begin(); it != list->end() ; it++) {
		if(initial)
			oStr << ", ";
		initial = true;

		oStr << (*it);
	}

	if(!initial)
		oStr << "No one";
	oStr << ".";

	return(oStr.str());
}


bstring Player::showIgnoring() const {
	return(showList(&ignoring));
}
bstring Player::showGagging() const {
	return(showList(&gagging));
}
bstring Player::showRefusing() const {
	return(showList(&refusing));
}
bstring Player::showDueling() const {
	return(showList(&dueling));
}
bstring Player::showWatching() const {
	return(showList(&watching));
}

//*********************************************************************
//						addList functions
//*********************************************************************

void Player::addList(std::list<bstring>* list, bstring name) {
	list->push_back(name);
}


void Player::addIgnoring(bstring name) {
	addList(&ignoring, name);
}
void Player::addGagging(bstring name) {
	addList(&gagging, name);
}
void Player::addRefusing(bstring name) {
	addList(&refusing, name);
}
void Player::addDueling(bstring name) {
	delList(&maybeDueling, name);

	// if they aren't dueling us, add us to their maybe dueling list
	Player* player = gServer->findPlayer(name);
	if(player && !player->isDueling(name))
		player->addMaybeDueling(getName());

	addList(&dueling, name);
}
void Player::addMaybeDueling(bstring name) {
	addList(&maybeDueling, name);
}
void Player::addWatching(bstring name) {
	addList(&watching, name);
}

//*********************************************************************
//						delList functions
//*********************************************************************

void Player::delList(std::list<bstring>* list, bstring name) {
	std::list<bstring>::iterator it;

	for(it = list->begin(); it != list->end() ; it++) {
		if((*it) == name) {
			list->erase(it);
			return;
		}
	}
}


void Player::delIgnoring(bstring name) {
	delList(&ignoring, name);
}
void Player::delGagging(bstring name) {
	delList(&gagging, name);
}
void Player::delRefusing(bstring name) {
	delList(&refusing, name);
}
void Player::delDueling(bstring name) {
	delList(&dueling, name);
}
void Player::delWatching(bstring name) {
	delList(&watching, name);
}

//*********************************************************************
//						clear list functions
//*********************************************************************

void Player::clearIgnoring() {
	ignoring.clear();
}
void Player::clearGagging() {
	gagging.clear();
}
void Player::clearRefusing() {
	refusing.clear();
}
void Player::clearDueling() {
	dueling.clear();
}
void Player::clearMaybeDueling() {
	std::list<bstring>::iterator it;

	Player* player=0;
	for(it = maybeDueling.begin(); it != maybeDueling.end() ; it++) {
		player = gServer->findPlayer(*it);
		if(!player)
			continue;
		player->delDueling(getName());
	}

	maybeDueling.clear();
}
void Player::clearWatching() {
	watching.clear();
}

//*********************************************************************
//						renamePlayerFiles
//*********************************************************************

void renamePlayerFiles(const char *old_name, const char *new_name) {
	char	file[80], file2[80];

	sprintf(file, "%s/%s.xml", Path::Player, old_name);
	unlink(file);

	sprintf(file, "%s/%s.txt", Path::Post, old_name);
	sprintf(file2, "%s/%s.txt", Path::Post, new_name);
	rename(file, file2);

	sprintf(file, "%s/%s.txt", Path::History, old_name);
	sprintf(file2, "%s/%s.txt", Path::History, new_name);
	rename(file, file2);

	sprintf(file, "%s/%s.txt", Path::Bank, old_name);
	sprintf(file2, "%s/%s.txt", Path::Bank, new_name);
	rename(file, file2);
}

//*********************************************************************
//						checkHeavyRestrict
//*********************************************************************

bool Player::checkHeavyRestrict(const bstring& skill) const {
	// If we aren't one of the classes that can use heavy armor, but with restrictions
	// immediately return false
	if(! ((getClass() == FIGHTER && getSecondClass() == THIEF) ||
	      (getClass() == RANGER)) )
	      return(false);

	// Allows us to do a blank check and see if the player's class is one of the heavy armor
	// restricted classes.
	if(skill == "")
		return(true);


	bool mediumOK = (getClass() == RANGER);

	for(int i = 0 ; i < MAXWEAR ; i++) {
		if(	ready[i] &&
			(	ready[i]->isHeavyArmor() ||
				(	!mediumOK &&
					ready[i]->isMediumArmor()
				)
			)
		) {
			printColor("You can't ^W%s^x while wearing heavy armor!\n", skill.c_str());
			printColor("^W%O^x would hinder your movement too much!\n", ready[i]);
			return(true);
		}
	}
	return(false);
}


void Player::validateId() {
    if(id.empty() || id.equals("-1")) {
        setId(gServer->getNextPlayerId());
    }
}
