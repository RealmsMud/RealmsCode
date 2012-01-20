/*
 * die.cpp
 *	 New death code
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
// Mud includes
#include "mud.h"
#include "die.h"
#include "factions.h"
#include "effects.h"
#include "commands.h"
#include "guilds.h"
#include "bank.h"
#include "unique.h"
#include "web.h"
#include <math.h>

//********************************************************************
//						isHardcore
//********************************************************************
// Return: true if player is flagged as Hardcore (death = permanent)

bool Player::isHardcore() const {
	return(flagIsSet(P_HARDCORE) && !isStaff());
}

//********************************************************************
//						hardcoreDeath
//********************************************************************
bool canDrop(const Player* player, const Object* object, const Property* p);
bool delete_drop_obj(const BaseRoom* room, const Object* object, bool factionCanRecycle);

void hardcoreDeath(Player* player) {
	if(!player->isHardcore())
		return;
	bool factionCanRecycle = !player->parent_rom || Faction::willDoBusinessWith(player, player->parent_rom->getFaction());
	player->hooks.execute("preHardcoreDeath");

	for(int i=0; i<MAXWEAR; i++) {
		if(player->ready[i] && (!(player->ready[i]->flagIsSet(O_CURSED) && player->ready[i]->getShotscur() > 0))) {
			player->ready[i]->clearFlag(O_WORN);
			player->doRemove(i);
		}
	}
	player->computeAC();
	player->computeAttackPower();


	BaseRoom* room = player->getRoom();
	Object* object=0;
	otag *op = player->first_obj,*oprev=0;

	while(op) {
		oprev = op;
		op = op->next_tag;
		object = oprev->obj;

		if(delete_drop_obj(room, object, factionCanRecycle) || !canDrop(player, object, 0) || object->flagIsSet(O_STARTING))
			continue;

		player->delObj(object, false, true, true, false);
		object->addToRoom(room);
	}
	player->checkDarkness();

	if(player->coins[GOLD]) {
		object=0;
		loadObject(MONEY_OBJ, &object);
		object->nameCoin("gold", player->coins[GOLD]);
		object->setDroppedBy(player, "HardcoreDeath");
		object->value.set(player->coins[GOLD], GOLD);
		player->coins.sub(player->coins[GOLD], GOLD);
		gServer->logGold(GOLD_OUT, player, object->value, NULL, "HardcoreDeath");

		object->addToRoom(room);
	}

	player->printColor("\n\n                 ^YYou have died.\n\n");
	player->printColor("^RAs a hardcore character, this death is permanent.\n");
	player->print("\n\n");
	player->statistics.display(player, true);
	player->print("\n");
	broadcast("^#^R### %s's soul is lost forever.", player->name);
	player->hooks.execute("postHardcoreDeath");
	deletePlayer(player);
}

//********************************************************************
//						isHoliday
//********************************************************************
// if a string is returned, player will get bonus exp

bstring isHoliday() {
	bstring str = gConfig->getMonthDay();

	// see if today is a holiday
	if(str == "Oct 31")
		return("Happy Halloween!");

	if(str == "Dec 24" || str == "Dec 25")
		return("Merry Christmas!");

	if(str == "Dec 31" || str == "Jan  1")
		return("Happy New Year!");

	return("");
}


// TODO: Add in perm killing code

//********************************************************************
//						dropCorpse
//********************************************************************
// Parameters:	<killer> The creature attacking
// Handles the dropping of items from creatures

void Monster::dropCorpse(Creature *killer) {
	BaseRoom* room = getRoom();
	bstring str = "", carry = "";
	otag		*op=0;
	Object		*object=0;
	Player*		player=0;
	Player*		pMaster = isPet() ? following->getPlayer() : 0;
	bool		destroy = room->isDropDestroy();

	if(killer)
		player = killer->getMaster();

	killDarkmetal();
	killUniques();

	// Drop weapons also
	if(ready[WIELD-1]) {
		Lore::remove(pMaster, ready[WIELD-1], true);
		if(destroy) {
			unequip(WIELD, UNEQUIP_DELETE);
		} else {
			if(player)
				player->printColor("%M dropped their weapon: %1P.\n", this, ready[WIELD - 1]);
			Object* drop = unequip(WIELD, UNEQUIP_NOTHING, false);
			if(drop) {
				if(drop->flagIsSet(O_JUST_LOADED))
					drop->setDroppedBy(this, "MobDeath");
				drop->addToRoom(room);
			}
		}
	}


	// list all on death
	// check for player: for mob killing pet, player is null, and then we don't
	// care about listing items dropped
	if(!destroy && player)
		str = listObjects(player, first_obj, true);

	op = first_obj;
	while(op) {
		object = op->obj;
		op = op->next_tag;
		if(object->flagIsSet(O_JUST_LOADED))
			object->setDroppedBy(this, "MobDeath");

		Lore::remove(pMaster, object, true);
		delObj(object, false, false, true, false);

		if(destroy)
			delete object;
		else if(!flagIsSet(M_TRADES) || !object->info.id) {
			object->clearFlag(O_BODYPART);
			object->addToRoom(room);
		}
	}
	checkDarkness();

	if(!destroy) {
		if(!coins.isZero()) {
			loadObject(MONEY_OBJ, &object);
			object->value.set(coins);

			object->setDroppedBy(this, "MobDeath");

			object->nameCoin("gold", object->value[GOLD]);
			object->addToRoom(room);

			if(player) {
				if(str != "")
					str += ", ";
				str += object->name;
			}
		}

		if(player && str != "") {
			carry = crt_str(this, 0, CAP | INV);
			carry += " was carrying: ";
			carry += str;
			carry += ".\n";

			player->printColor("%s", carry.c_str());
			if(!player->flagIsSet(P_DM_INVIS))
				broadcastGroup(true, killer, "%s", carry.c_str());
		}
	}
}

//********************************************************************
//						die
//********************************************************************
// wrapper for die

void Creature::die(Creature *killer) {
	bool freeTarget=true;
	die(killer, freeTarget);
}

//********************************************************************
//						die
//********************************************************************
// Parameters:	<killer> The creature attacking
// Handles the death of people

void Creature::die(Creature *killer, bool &freeTarget) {
	Player*  pVictim = getPlayer();
	Monster* mVictim = getMonster();

	Player*  pKiller = killer->getPlayer();
	Monster* mKiller = killer->getMonster();
	bool duel = induel(pVictim, pKiller);

	if(pKiller) {
		pKiller->statistics.kill();
		if(mVictim)
			pKiller->statistics.monster(mVictim);
		if(pKiller->hasCharm(name))
			pKiller->delCharm(this);
	}

	if(pVictim)
		freeTarget = false;

	// Any time a level 7+ player dies, they are auto backed up.
	if(pVictim && pVictim->getLevel() >= 7 && !duel)
		pVictim->save(true, LS_BACKUP);

	Hooks::run(killer, "preKill", this, "preDeath", duel);

	if(mKiller && mKiller->isPet() && pVictim) {
		pVictim->dieToPet(mKiller);

	} else if(mKiller && mKiller->isPet() && mVictim) {
		mVictim->dieToPet(mKiller, freeTarget);

	} else if(mKiller && mVictim) {
		mVictim->dieToMonster(mKiller, freeTarget);

	} else if(mKiller && pVictim) {
		pVictim->dieToMonster(mKiller);

	} else if(pKiller && mVictim) {
		mVictim->dieToPlayer(pKiller, freeTarget);

	} else if(pKiller && pVictim) {
		pVictim->dieToPlayer(pKiller);

	}
	if(pVictim) {
	    pVictim->clearFocus();
	}

	// 10% chance to get porphyria/lycanthropy when killed by a vampire/werewolf
	if(pVictim && !duel) {
		pVictim->addPorphyria(killer, 10);
		pVictim->addLycanthropy(killer, 10);
	}
}

//********************************************************************
//						dieToMonster
//********************************************************************
// Parameters:	<killer> The creature attacking
// Handles monsters killing players

void Player::dieToMonster(Monster *killer) {
	// Some sanity checks here -- Should never fail these
	ASSERTLOG( !killer->isPet() );

	// no real penalties for staff dying
	if(isStaff()) {
		printColor("^r*** You just died ***\n");
		broadcast(::isStaff, "^G### Sadly, %s just died.", name);

		clearAsEnemy();

		hp.restore();
		mp.restore();

		return;
	}

	print("%M killed you!\n", killer);

	if(flagIsSet(P_OUTLAW) && killer->getLevel() > 10)
		clearFlag(P_OUTLAW);

	setFlag(P_KILLED_BY_MOB);

	// Stop level 13+ players from suiciding right after dieing
	if(level >= 13) {
		lasttime[LT_MOBDEATH].ltime = time(0);
		lasttime[LT_MOBDEATH].interval = 60*60*24L;
		setFlag(P_NO_SUICIDE);
	}

	unsigned long oldxp = experience;
	unsigned short oldlvl = level;

	broadcast("### Sadly, %s was killed by %1N.", name, killer);
	killer->clearEnemy(this);
	clearAsEnemy();
	gServer->clearAsEnemy(this);
	loseExperience(killer);
	dropEquipment(false);
	logDeath(killer);

	// more info for staff
	broadcast(::isCt, "^rOld Exp: %u Lost Exp: %d, Old Lvl: %u, Lvl: %u, Room: %s",
		oldxp, (int)(oldxp - experience), oldlvl, level, getRoom()->fullName().c_str());

	resetPlayer(killer);
}

//********************************************************************
//						dieToPet
//********************************************************************
// Parameters:	<killer> The creature attacking
// Handles pets killing players

void Player::dieToPet(Monster *killer) {
	Player	*master=0;

	if(killer->following)
		master = killer->following->getPlayer();
	else {
		broadcast(::isCt, "^y*** Pet %s has no master and is trying to kill a player. Room %s",
			killer->name, killer->room.str().c_str());
		return;
	}

	bool dueling = induel(this, master);

	if(dueling)
		broadcast("^R### Sadly, %s was killed by %s's %s in a duel.", name, master->name, killer->name);
	else {
		if(level > 2)
			broadcast("### Sadly, %s was killed by %s's %s.", name, master->name, killer->name);
		else {
			broadcast(::isCt, "^y### Sadly, %s was killed by %s's %s.", name, master->name, killer->name);
			print("### Sadly, %s was killed by %s's %s.", name, master->name, killer->name);
		}
	}

	killer->clearEnemy(this);
	getPkilled(master, dueling);
}

//********************************************************************
//						dieToPet
//********************************************************************
// Parameters:	<killer> The creature attacking
// Handles pets killing monsters

void Monster::dieToPet(Monster *killer, bool &freeTarget) {
	Creature* petKiller=0;
	Player*	pKiller=0;
	logDeath(killer);

	if(killer->following) {
		petKiller = killer;
		pKiller = killer->following->getPlayer();
	} else {
		broadcast(::isCt, "^y*** Pet %s has no master and is trying to kill a mob. Room %s",
			killer->name, killer->room.str().c_str());
		return;
	}


	broadcast(NULL, pKiller->getRoom(), "%M's %s killed %N.", pKiller, petKiller->name, this);

	mobDeath(pKiller, freeTarget);
}


//********************************************************************
//						dieToMonster
//********************************************************************
// Parameters:	<this> The creature being attacked
//				<killer> The creature attacking

// Handles monsters killing monsters
void Monster::dieToMonster(Monster *killer, bool &freeTarget) {
	if(this != killer)
		broadcast(NULL, killer->getRoom(), "%M killed %N.", killer, this);
	mobDeath(killer, freeTarget);
}



//********************************************************************
//						dieToPlayer
//********************************************************************
// Parameters:	<killer> The creature attacking
// Handles players killing monsters

void Monster::dieToPlayer(Player *killer, bool &freeTarget) {
	logDeath(killer);
	if(following != killer) {
		killer->print("You killed %N.\n", this);
		broadcast(killer->getSock(), killer->getRoom(), "%M killed %N.", killer, this);
	}
	mobDeath(killer, freeTarget);
}

//********************************************************************
//						mobDeath
//********************************************************************
// Common routine for all mob's dying.

void Monster::mobDeath(Creature *killer) {
	bool freeTarget=true;
	mobDeath(killer, freeTarget);
}
void Monster::mobDeath(Creature *killer, bool &freeTarget) {
	if(killer)
		killer->checkDoctorKill(this);

	distributeExperience(killer);
	dropCorpse(killer);
	cleanFollow(killer);
	clearAsEnemy();

	// does the calling function want us to free the target?
	if(freeTarget)
		finishMobDeath(killer);

	// freeTarget now means: do I (calling function) need to free the target?
	freeTarget = !freeTarget;
}

//********************************************************************
//						finishMobDeath
//********************************************************************

void Monster::finishMobDeath(Creature *killer) {
	//if(killer)
	// a null killer is valid, so the called function should handle that properly
	Hooks::run(killer, "postKill", this, "postDeath", "0");

	deleteFromRoom();
	gServer->delActive(this);
	free_crt(this);
}


//********************************************************************
//						dieToPlay
//********************************************************************
// Parameters:	<killer> The creature attacking
// Handles players killing players

void Player::dieToPlayer(Player *killer) {
	char	deathstring[80];

	bool dueling = induel(this, killer);

	killer->print("You killed %N.\n", this);
	print("%M killed you.\n", killer);
	broadcast(killer->getSock(), getSock(), killer->getRoom(), "%M killed %N.", killer, this);

	if(killer->isEffected("lycanthropy") && killer->getLevel() >= 13)
		strcpy(deathstring, "eaten");
	else if(killer->getClass() == ASSASSIN && killer->getLevel() >= 13)
		strcpy(deathstring, "assassinated");
	else
		strcpy(deathstring, "killed");

	if(dueling)
		broadcast("^R### Sadly, %s was %s by %s in a duel.", name, deathstring, killer->name);
	else// if(level > 2)
		broadcast("### Sadly, %s was %s by %1N.", name, deathstring, killer);
	//else {
	//	broadcast(::isWatcher, "^C### Sadly, %s was %s by %1N.", name, deathstring, killer);
	//	print("### Sadly, %s was %s by %1N.\n", name, deathstring, killer);
	//}

	getPkilled(killer, dueling);
}

//********************************************************************
//						getPkilled
//********************************************************************

void Player::getPkilled(Player *killer, bool dueling, bool reset) {
	unsigned long oldxp = experience;
	unsigned short oldlvl = level;

	if(isGuildKill(killer))
		guildKill(killer);

	if(isGodKill(killer))
		godKill(killer);
	else if(isClanKill(killer))
		clanKill(killer);

	// make the pets stop attacking the players
	clearAsPetEnemy();
	killer->clearAsPetEnemy();

	if(!dueling) {
		updatePkill(killer);

		dropBodyPart(killer);

		if(!killer->isStaff())
			dropEquipment(true, killer->getSock());

		logDeath(killer);

		// more info for staff
		broadcast(::isCt, "^rOld Exp: %u Lost Exp: %d, Old Lvl: %u, Lvl: %u, Room: %s",
			oldxp, (int)(oldxp - experience), oldlvl, level, getRoom()->fullName().c_str());

	}

	if(reset)
		resetPlayer(killer);
}

//********************************************************************
//						isGodKill
//********************************************************************

bool Player::isGodKill(const Player *killer) const {
	if(induel(killer, this))
		return(false);

	if(killer->getLevel() < 7 || level < 7)
		return(false);

	if(!killer->getDeity() || !deity)
		return(false);

	if(killer->isStaff())
		return(false);

	return(true);
}

//********************************************************************
//						isClanKill
//********************************************************************

bool Player::isClanKill(const Player *killer) const {

	if(flagIsSet(P_OUTLAW_WILL_LOSE_XP))
		return(true);

	if(induel(killer, this))
		return(false);

	if(killer->getLevel() < 7 || level < 7)
		return(false);

	if(clan && killer->getClan())
		return(true);


	// Dk vs Paly, Paly vs Dk, or Dk vs Dk
	if(	(killer->getClass() == DEATHKNIGHT && cClass == PALADIN) ||
		(killer->getClass() == DEATHKNIGHT && (cClass == PALADIN || cClass == DEATHKNIGHT))
	)
		return(true);

	if( ((killer->getClass() == CLERIC && killer->flagIsSet(P_CHAOTIC) && killer->getLevel() >= 7) && flagIsSet(P_PLEDGED)) ||
		((cClass == CLERIC && flagIsSet(P_CHAOTIC) && level >= 7) && killer->flagIsSet(P_PLEDGED))
	)
		return(true);

	return(false);
}

//********************************************************************
//						guildKill
//********************************************************************

int Player::guildKill(Player *killer) {
	int		bns=0, penalty=0, levelDiff=0;
	Guild *killerGuild, *thisGuild;
	int		same=0, base=0;
	long	total=0;

	killerGuild = gConfig->getGuild(killer->getGuild());
	thisGuild = gConfig->getGuild(guild);

	base = level * 50;

	levelDiff = level - killer->getLevel();

	if(levelDiff < 0)
		bns += -5 * level * levelDiff;
	else
		bns += levelDiff * 50;

	if(guildRank == GUILD_MASTER)
		bns *= 3;
	else if(guildRank == GUILD_BANKER)
		bns *= 2;
	else if(guildRank == GUILD_OFFICER)
		bns = (bns*3)/2;

	total = MAX(1, mrand((base + bns)/2, base + bns));

	if(killer->halftolevel())
		total = 0;

	if(killer->getSecondClass())
		total = total * 3 / 4;

	penalty = MIN(mrand(1000,1500), (bns*3)/2);

	if(killer->getLevel() > level + 6)
		penalty = 100;

	if(killerGuild == thisGuild) {
		total = -1000;
		penalty = 1000;
		same = 1;
	} else {
		killerGuild->incPkillsIn();
		killerGuild->incPkillsWon();
		thisGuild->incPkillsIn();

		killerGuild->bank.add(coins);
		Bank::guildLog(killer->getGuild(), "Guild PKILL: %s by %s [Balance: %s]\n",
			coins.str().c_str(), killer->name, killerGuild->bank.str().c_str());
		coins.zero();
		
		gConfig->saveGuilds();
	}


	if(!same) {
		killer->printColor("^gYou have defeated a member of a rival guild!\n");
		killer->print("Your guild honors you with %d experience.\n", abs(total));
		if(coins[GOLD])
			killer->printColor("You grab %N's coins and put them in your guild bank.^x\n", this);

		printColor("^gYou have been defeated by a member of a rival guild!\n");
		print("Your guild shames you by taking %d experience.\n", abs(penalty));
		if(coins[GOLD])
			printColor("%M grabs all of your coins!\n", killer);
	} else {
		killer->printColor("^gYou have shamelessly defeated a member of your own guild!\n");
		killer->printColor("Your guild penalizes you for %d experience.\n", abs(total));

		printColor("^gYou have been defeated during senseless guild infighting!\n");
		printColor("Your guild penalizes you for %d experience.\n", abs(penalty));
	}

	killer->addExperience(total);
	subExperience(penalty);

	return(1);
}

//********************************************************************
//						godKill
//********************************************************************

int Player::godKill(Player *killer) {
	int		bns=0, penalty=0, levelDiff=0;
	int		same=0, base=0;
	Guild *killerGuild, *thisGuild;
	long	total=0;

	killerGuild = gConfig->getGuild(killer->getGuild());
	thisGuild = gConfig->getGuild(guild);

	base = level * 50;
	levelDiff = level - killer->getLevel();

	if(levelDiff < 0)
		bns += 5 * level * levelDiff;
	else
		bns += levelDiff * 50;


	total = MAX(1, mrand((base + bns)/2, base + bns));

	if(killer->halftolevel())
		total = 0;

	if(killer->getSecondClass())
		total = total * 3 / 4;

	penalty = MIN(mrand(1000,1500), (bns*3)/2);

	if(killer->getLevel() > level + 6)
		penalty = 100;

	if(killer->getDeity() == deity) {
		total = -50;
		penalty = 50;
		same = 1;
	}
	if(killerGuild != 0 && thisGuild != 0) {
		if(killerGuild != thisGuild) {
			killerGuild->incPkillsIn();
			killerGuild->incPkillsWon();
			thisGuild->incPkillsIn();
		}
	}


	if(!same) {
		killer->printColor("^cYou have defeated a member of an enemy religion!\n");
		killer->printColor("Your order honors you with %d experience.\n", abs(total));

		printColor("^cYou have been defeated by a member of a rival religion!\n");
		printColor("Your order shames you by taking %d experience.\n", abs(penalty));
	} else {
		killer->printColor("^cYou have shamelessly defeated a member of your own religion!\n");
		killer->printColor("Your order penalizes you for %d experience.\n", abs(total));

		printColor("^cYou have been defeated during senseless religious infighting!\n");
		printColor("Your order penalizes you for %d experience.\n", abs(penalty));
	}

	killer->addExperience(total);
	subExperience(penalty);

	return(1);
}

//********************************************************************
//						clanKill
//********************************************************************

int Player::clanKill(Player *killer) {
	bool penalty = false; // Do we give a penalty for killing this person?
	int expGain = 0, expLoss = 0;

	// Paly's shouldn't be killing each other
	if(cClass == PALADIN && killer->getClass() == PALADIN && deity == killer->getDeity())
		penalty = true;
	// Clan members should not be killing members of their own clan
	if(clan == killer->getClan())
		penalty = true;
	// Never a penalty for killing an outlaw!
	if(flagIsSet(P_OUTLAW_WILL_LOSE_XP))
		penalty = false;

	// No penalty
	if(!penalty) {
		expGain = (level * killer->getLevel()) * 10;
		expLoss = expGain * 2;

		if(killer->halftolevel())
			expGain = 0;
		if(killer->getSecondClass())
			expGain = expGain * 3 / 4;

		print("You have been bested!\nYou lose %d experience.\n", expLoss);
		subExperience(expLoss);
		
		if(!killer->flagIsSet(P_OUTLAW)) {
			killer->print("You have vanquished %s.\n", name);
			killer->print("You %s %d experience for your heroic deed.\n", gConfig->isAprilFools() ? "lose" : "gain", expGain);
			killer->addExperience(expGain);
		}
	}
	return(1);
}

//********************************************************************
//						checkDoctorKill
//********************************************************************
// Parameters:	<player>
//				<killer> The creature attacking
// Checks for doctor killers

void Creature::checkDoctorKill(Creature *victim) {
	if(!strcmp(victim->name, "doctor")) {
		if(	(isPlayer() && !isStaff()) ||
			(isMonster() && isPet() && !following->isStaff())
		) {
			Creature* target = isPlayer() ? this : following;

			target->setFlag(P_DOCTOR_KILLER);
			if(!target->flagIsSet(P_DOCTOR_KILLER))
				broadcast("### %s is a doctor this!", target->name);
			target->lasttime[LT_KILL_DOCTOR].ltime = time(0);
			target->lasttime[LT_KILL_DOCTOR].interval = 72000L;
		}
	}
}


//********************************************************************
//						checkLevel
//********************************************************************
// Parameters:	<player>
// Checks if a player has deleveled or releveled
//void doTrain(Player* player) {
//
//
//}

int Player::checkLevel() {
	// Staff doesn't delevel or relevel
	if(isCt())
		return(0);

	int n = exp_to_lev(experience);
	// De-Level!
	if(level > n) {
		print("You have deleveled to level %s!\n", int_to_text(n));
		while(level > n)
			downLevel();

		negativeLevels = 0;

		return(-1); // Delevel
	}

	// Re-Level!
	if(level < n && level < actual_level && n <= actual_level && !negativeLevels) {
		if(parent_rom && parent_rom->getHighLevel() && level == parent_rom->getHighLevel()) {
			print("You have enough experience to relevel, but cannot do so in this room.\n");
			return(0);
		}
		print("You have releveled to level %s!\n", int_to_text(n));
		logn("log.relevel", "%s just releveled to level %d from level %d in room %s.\n",
			name, n, level, getRoom()->fullName().c_str());
		if(!isStaff())
			broadcast("### %s just releveled to %s!", name, int_to_text(n));
		while(level < n)
			upLevel();
		return(1); // Relevel
	}

	// OCEANCREST: Dom: HC
	//if(isHardcore() && level < n && !negativeLevels) {
	//	doTrain(this);
	//	return(2); // Level
	//}

	return(0); // No change
}

//********************************************************************
//						updatePkill
//********************************************************************

void Player::updatePkill(Player *killer) {
	// No pkill changes if either party is staff
	if(isStaff() || killer->isStaff())
		return;

	statistics.losePk();
	killer->statistics.winPk();
}

//********************************************************************
//						dropEquipment
//********************************************************************
// Parameters:	<killer> The creature attacking
// Function to make players drop the equipment they are currently wearing
// via a pkill

void Player::dropEquipment(bool dropAll, Socket* killerSock) {
	bstring dropString = "";
	int		i=0;

	// dropping weapons is handled separately from equipment, but we still need their
	// names for dropping later
	Object* main = ready[WIELD-1];
	Object* held = ready[HELD-1];

	dropWeapons();

	// be nice to lowbies
	if(dropAll && level > 3) {


		// don't make them lose all their inventory in a drop-destroy room
		if(!getRoom()->isDropDestroy()) {

			// we reassign these for the purposes of having them printed in the list;
			// they will be reset afterwards
			Object* rMain = ready[WIELD-1];
			Object* rHeld = ready[HELD-1];
			ready[WIELD-1] = main;
			ready[HELD-1] = held;

			for(i=0; i<MAXWEAR; i++) {
				if(	!ready[i] ||
					ready[i]->flagIsSet(O_CURSED) ||
					ready[i]->flagIsSet(O_NO_DROP) ||
					ready[i]->flagIsSet(O_STARTING)
				)
					continue;
				// 20% chance each item will be dropped (ignore this chance for wielded items
				// as we want them added to the list of stuff dropped)
				if((i != WIELD-1 && i != HELD-1) && mrand(1,5) > 1)
					continue;

				if(dropString != "")
					dropString += ", ";
				dropString += ready[i]->name;

				if(i != WIELD-1 && i != HELD-1) {
					// I is wearloc-1, so add one to it
					Object* temp = unequip(i+1, UNEQUIP_NOTHING, false);
					if(temp)
						finishDropObject(temp, getRoom(), this);
				}
			}

			// reset these wear locations
			ready[WIELD-1] = rMain;
			ready[HELD-1] = rHeld;

			if(dropString != "") {
				if(killerSock)
					killerSock->printColor("%s dropped: %s.\n", name, dropString.c_str());

				print("You dropped your inventory where you died!\n");
			}
		}
	}

	checkDarkness();
	computeAC();
	computeAttackPower();
}


//********************************************************************
//						dropBodyPart
//********************************************************************
// Parameters:	<killer> The creature attacking
// Makes a player drop a body part

void Player::dropBodyPart(Player *killer) {
	ASSERTLOG( killer->isPlayer());

	if(getRoom()->isDropDestroy())
		return;

	bool nopart = false;
	int dueling = 0, num = 0;
	Object *body_part;
	char part[12], partName[25];

	dueling = induel(this,killer);

	if(mrand (1,100) > 5 && !killer->isDm() && !flagIsSet(P_OUTLAW))
		nopart = true;

	if(level <= 4 && !killer->isDm() && !flagIsSet(P_OUTLAW))
		nopart = true;

	if(	isStaff() || (
			!killer->isPet() &&
			!killer->isDm() &&
			(killer->getLevel() > level + 5) &&
			!flagIsSet(P_OUTLAW)
		) || (
			killer->isPet() &&
			!killer->following->isDm() &&
			(killer->following->getLevel() > level + 5) &&
			!flagIsSet(P_OUTLAW)
		)
	) {
		nopart = true;
	}

	if(!nopart && !dueling) {

		if(loadObject(BODYPART_OBJ, &body_part)) {
			num = mrand(1,14);

			switch(num) {
			case 1:
				strcpy(part, "skull");
				break;
			case 2:
				strcpy(part, "head");
				break;
			case 3:
				strcpy(part, "ear");
				break;
			case 4:
				strcpy(part, "scalp");
				break;
			case 5:
				strcpy(part, "hand");
				break;
			case 6:
				strcpy(part, "spleen");
				break;
			case 7:
				strcpy(part, "nose");
				break;
			case 8:
				strcpy(part, "arm");
				break;
			case 9:
				strcpy(part, "foot");
				break;
			case 10:
				strcpy(part, "spine");
				break;
			case 11:
				strcpy(part, "heart");
				break;
			case 12:
				strcpy(part, "brain");
				break;
			case 13:
				strcpy(part, "eyeball");
				break;
			case 14:
				if(isEffected("vampirism"))
					strcpy(part, "fangs");
				else
					strcpy(part, "teeth");
				break;
			default:
				strcpy(part, "skull");
				break;
			}

			strcpy(partName, name);
			sprintf(body_part->name, "%s's %s", partName, part);

			lowercize(partName, 0);
			strncpy(body_part->key[0], partName, 20);
			strncpy(body_part->key[1], part, 20);
			strncpy(body_part->key[2], part, 20);

			body_part->setAdjustment(mrand(1,2));
			if(mrand(1,100) == 1)
				body_part->setAdjustment(3);

			body_part->addToRoom(getRoom());
		}
	}

}

//********************************************************************
//						logDeath
//********************************************************************

void Player::logDeath(Creature *killer) {
	char	file[16], killerName[80];
	Player*	pKiller = killer->getPlayer();

	if(!killer->isStaff())
		statistics.die();
	strcpy(killerName, "");

	if(killer->isPet())
		sprintf(killerName, "%s's %s.", killer->following->name, killer->name);
	else
		strcpy(killerName, killer->name);

	if(pKiller)
		strcpy(file, "log.pkill");
	else
		strcpy(file, "log.death");

	if(!(!pKiller && killer->flagIsSet(M_NO_EXP_LOSS))) {
		logn(file, "%s(%d) was killed by %s(%d) in room %s.\n", name, level,
		     killer->name, killer->getLevel(), killer->getRoom()->fullName().c_str());

	}

	if(pKiller && pKiller->isStaff()) {
		log_immort(false, pKiller, "%s(%d) was killed by %s(%d) in room %s.\n", name, level,
			pKiller->name, pKiller->getLevel(), pKiller->getRoom()->fullName().c_str());
	}
	
	updateRecentActivity();
}

//********************************************************************
//						resetPlayer
//********************************************************************
// Parameters:	<killer> The creature attacking
// Reset a player after death -- Move them to limbo, broadcast
// they were killed, clear deterimental effects, etc

void Player::resetPlayer(Creature *killer) {
	int		duel=0;
	bool	same=false;
	Player* pKiller = killer->getPlayer();
	BaseRoom *newRoom = getLimboRoom().loadRoom(this);

	duel = induel(this, pKiller);

	if(	inJail() ||
		duel ||
		!newRoom
	)
		same = true;

	if(cClass == BUILDER) {
		same = true;
		print("*** You died ***\n");
		broadcast(::isStaff, "^G### Sadly, %s was killed by %s.", name, killer->name);
	}

	// a hardcore player about to die doesnt need to worry about room movement
	if(isHardcore() && !killer->isStaff())
		same = true;

	if(!same) {
		Hooks::run(killer, "postKillPreLimbo", this, "postDeathPreLimbo");

		deleteFromRoom();
		addToRoom(newRoom);
		doPetFollow();
	}
	//doClearPetEnemy(this, killer->name);

	curePoison();
	cureDisease();
	tickDmg = 0;
	removeCurse();
	removeEffect("hold-person");
	removeEffect("petrification");
	removeEffect("confusion");
	removeEffect("drunkenness");
	unhide();

	if(killer->isPlayer() || duel) {
		hp.setCur( MAX(1, MAX(hp.getMax()/2, hp.getCur())));
		mp.setCur(MAX(mp.getCur(),(mp.getMax())/10));
	} else {
		hp.restore();
		mp.restore();
	}

	if(duel) {
		setFlag(P_DIED_IN_DUEL);
		knockUnconscious(10);
		broadcast(getSock(), getRoom(), "%s is knocked unconscious!", name);

		updateAttackTimer(true, 300);
		pKiller->delDueling(name);
		delDueling(killer->name);
	} else if(isHardcore() && !killer->isStaff()) {
		hardcoreDeath(this);
		// the player is invalid after this
		return;
	} else {
		courageous();
	}

	killer->hooks.execute("postKill", this, duel);
	hooks.execute("postDeath", killer, duel, same);
}

//void clearMobEnemies(Creature *monster) {
//	etag		*ep=0;
//
//	if(monster->isPlayer())
//		return;
//
//	ep = monster->first_enm;
//	while(ep) {
//		del_enm_crt(ep->enemy, monster);
//		ep = ep->next_tag;
//	}
//}

//********************************************************************
//						hearMobDeath
//********************************************************************

bool hearMobDeath(Socket* sock) {
	if(!sock->getPlayer() || !isCt(sock))
		return(false);
	return(!sock->getPlayer()->flagIsSet(P_NO_DEATH_MSG));
}

//********************************************************************
//						logDeath
//********************************************************************

void Monster::logDeath(Creature *killer) {
	int			solo=0, logType=0;
	ctag		*fp=0;
	Creature *leader=0, *pet=0;
	BaseRoom* room = killer->getRoom();
	char		file[80], killerString[1024];
	char		logStr[2096];


	strcpy(file, "");
	strcpy(killerString, "");

	if(flagIsSet(M_WILL_BE_LOGGED)) {
		strcpy(file, "log.mdeath");
		logType = 1;
	} else if(flagIsSet(M_PERMENANT_MONSTER) && (killer->isPlayer() || killer->isPet())) {
		strcpy(file, "log.perm");
		logType = 2;
	} else if(killer->pFlagIsSet(P_BUGGED) ) {
		sprintf(file, "%s/%s", Path::BugLog, killer->name);
		logType = 3;
	} else if(killer->pFlagIsSet(P_KILLS_LOGGED) ) {
		sprintf(file, "%s/%s.kills", Path::BugLog, killer->name);
		logType = 4;
	} else
		return; //Mob's death not logged

	// There are 4 cases for perm broadcast, otherwise we do single broadcast
	// (which includes the pet message).

	// group broadcast (aka solo = 1)
	//		killer is pet
	//			pet's leader has a player follower
	//			pet's leader is following
	//		killer is a player
	//			killer has a player follower
	//			killer is following

	if(killer->isPlayer() || killer->isPet()) {
		solo = 1;

		// we have the leader of the pet
		if(killer->isPet())
			leader = killer->following;
		else
			leader = killer;

		// see if they have a leader or followers
		if(leader->following)
			solo = 0;

		// we always need to check for a pet
		fp = leader->first_fol;
		while(fp) {

			if(fp->crt != leader) {
				if(fp->crt->isPet() && fp->crt->following == leader)
					pet = fp->crt;
				if(!(fp->crt->isPet() && fp->crt->following == leader))
					solo = 0;
			}
			fp = fp->next_tag;
		}
	}

	if(killer->isPet() || pet) {
		sprintf(killerString, "%s and %s %s",
			leader->name,
			leader->hisHer(),
			pet->name);
	} else {
		sprintf(killerString, "%s", killer->name);
	}


	switch(logType) {
	case 1:	// unsaved mob
		sprintf(logStr, "%s(L%d) was killed by %s(L%d) in room %s for %lu experience.",
		     name, level, killerString, killer->getLevel(), room->fullName().c_str(), experience);
		broadcast(hearMobDeath, "^g*** %s(L%d) was killed by %s(L%d) in room %s for %lu experience.",
			name, level, killerString, killer->getLevel(), room->fullName().c_str(), experience);
		break;
	case 2: // Mob is a perm

		// if the killer was a pet, leader is pointing to the pet's leader
		// make it point to the leader of the pet's leader
		if(leader->following)
			leader = leader->following;

		if(!solo)
			sprintf(logStr, "%s(L%d) was killed by %s(L%d) in %s(L%d)'s group in room %s.",
			     name, level, killerString, killer->getLevel(),
			     leader->name, leader->getLevel(), room->fullName().c_str());
		else
			sprintf(logStr, "%s(L%d) was killed by %s(L%d) in room %s [SOLO].",
			     name, level, killerString, killer->getLevel(), room->fullName().c_str());


		if(!killer->isStaff() && flagIsSet(M_NO_PREFIX)) {
			if(!solo)
				broadcast(wantsPermDeaths, "^m### Sadly, %s was killed by %s and %s followers.", name, leader->name, leader->hisHer());
			else
				broadcast(wantsPermDeaths, "^m### Sadly, %s was killed by %s.", name, killerString);
		}

		break;
	case 3: // bugged player
	case 4: // all player's kills logged
		sprintf(logStr, "%s(L%d) was killed by %s(L%d) in room %s for %lu experience%s.",
		     name, level, killerString, killer->getLevel(),
		     room->fullName().c_str(), experience, solo == 1 ? "[SOLO]" : "");
		break;
	}

	logn(file, "%s\n", logStr);
	// I don't want to see bugged kill logs...too much spam
	if((killer->isPlayer() || killer->isPet()) && !killer->isCt() && logType != 4)
		broadcast(hearMobDeath, "^y*** %s", logStr);
}

//********************************************************************
//						loseExperience
//********************************************************************
// Parameters:	<killer> The creature attacking
// Handles experience/realms loss & death effects

void Player::loseExperience(Monster *killer) {
	float	xploss=0.0;
	long	n=0;
	int		count=0;


	// No exp loss from possesed monsters
	if(killer->flagIsSet(M_DM_FOLLOW))
		return;

	if(killer->flagIsSet(M_NO_EXP_LOSS) || (killer->isPet() && killer->following->isStaff()))
		return;

	if(level >= 7)
		addEffect("death-sickness");

	if(level < 10) {
		// Under level 10, 10% exp loss
		xploss = ((float)experience / 10.0);
		lostExperience += (long)xploss;
		experience -= (long)xploss;

	} else {
		// Level 10 and over, 2% exp loss with a minimum of 10k
		xploss = MAX((long)( (float)experience * 0.02), 10000);
		print("You have lost %ld experience.\n", (long)xploss);
		lostExperience += (long)xploss;
		experience -= (long)xploss;
	}

	n = level - exp_to_lev(experience);

	if(n > 1) {
		if(level < (MAXALVL+2))
			experience = needed_exp[level-3];
		else
			experience = (long)((needed_exp[MAXALVL-1]*(level-2)));
	}

	checkLevel();
//	n = exp_to_lev(experience);
//	while(level > n)
//		down_level(this);

	for(count=0; count < 6; count++) {		// Random loss to saving throws due to death.
		if(mrand(1,100) <= 25) {
			saves[count].chance -= 1;
			saves[count].gained -= 1;
			saves[count].chance = MAX(1, saves[count].chance);
			saves[count].gained = MAX(1, saves[count].gained);
		}
	}
}

//********************************************************************
//						giveExperience
//********************************************************************
// Parameters:	<killer> The creature attacking
// Handles experience gain on monster death

void Monster::distributeExperience(Creature *killer) {
	long	expGain = 0;

	Monster* pet=0;
	Player*	player=0, *follower=0;
	Player* leader=0;

	if(isPet())
		return;

	if(flagIsSet(M_PERMENANT_MONSTER))
		diePermCrt();

	if(killer) {
		if(killer->isPet())
			player = killer->following->getPlayer();
		else
			player = killer->getPlayer();

		if(player) {
			if(player->isGroupLeader(this) && player->flagIsSet(P_XP_DIVIDE))
				leader = player;
			else if(player->following && player->following->flagIsSet(P_XP_DIVIDE) && player->inSameRoom(player->following))
				leader = player->following->getPlayer();
		}

		std::map<Player*, int> expList;
		// If we have a leader, means we've got group exp set, so lets distribute it
		if(leader) {
			// Split exp evenly amounsgt the group
			int numGroupMembers=0, totalGroupLevel=0, totalGroupDamage=0;
			long n = 0;
			Creature* crt=0;
			ctag* cp = leader->first_fol;

			// Calculate how many people are in the group, see how much damage they have done
			// and remove them from the enemy list
			if(	isEnemy(leader) &&
				inSameRoom(leader) &&
				leader->getsGroupExperience(this))
			{
				numGroupMembers++;
				totalGroupLevel += leader->getLevel();
				n = clearEnemy(leader);
				totalGroupDamage += n;
				expList[leader] = n;
			}
			while(cp) {
				crt = cp->crt;
				cp = cp->next_tag;
				if(isEnemy(crt) && inSameRoom(crt)) {
					if(crt->getsGroupExperience(this)) {
						// Group member
						follower = crt->getPlayer();
						numGroupMembers++;
						totalGroupLevel += follower->getLevel();
						n = clearEnemy(follower);
						totalGroupDamage += n;
						expList[follower] = n;
						pet = follower->getPet();
						if(pet) {
							n = clearEnemy(pet);
							totalGroupDamage += n;
							expList[pet->following->getPlayer()] += n;
						}
					} else if(crt->isPet()) {
						// Leader's pet
						n = clearEnemy(crt);
						totalGroupDamage += n;
						expList[crt->following->getPlayer()] += n;
					}
				}
			}
			float xpPercent = (float)MIN(totalGroupDamage, hp.getMax())/(float)hp.getMax();
			long adjustedExp = (long)((xpPercent*experience) * (1.0 + (.25*numGroupMembers)));

			// Exp is split amoungst the group based on their level,
			// since we split it evenly, in this case pets do NOT give their master
			// any extra experience.

			int averageEffort = totalGroupDamage / expList.size();
			std::cout << "GROUP EXP: TGD:" << totalGroupDamage << " Num:" << expList.size() << " AVG EFF:" << averageEffort << std::endl;
			for(std::pair<Player*, int> p : expList) {
				Player* ply = p.first;
				int effort = p.second;

				if(ply) {
					expGain = (long)(((float)ply->getLevel()/(float)totalGroupLevel) * adjustedExp);

//					ply->printColor("You contributed ^m%d^x effort.  Average effort was ^m%d^x.\n", effort, averageEffort);
					// Adjust based on reduced effort
					if(effort < (averageEffort/2)) {
						ply->printColor("You receive reduced experience because you contributed less than half of the average effort.\n");
						expGain *= (((float)effort)/totalGroupDamage);
					}
					expGain = MAX(1, expGain);
					ply->gainExperience(this, killer, expGain, true);
				}
			}
		}
	}

	// Now handle everyone else on the list
	std::map<Player*, int> expList;

	ThreatSet::iterator tIt = threatTable->threatSet.begin();
	ThreatEntry* threat = 0;
	Creature* crt = 0;
	while(tIt != threatTable->threatSet.end()) {
	// Iterate it because we will be invaliding this iterator
		threat = (*tIt++);
	    crt = gServer->lookupCrtId(threat->getUid());
	    if(!crt) continue;

		if(crt->isPet())
		    expList[crt->following->getPlayer()] += clearEnemy(crt);
		else if(crt->isPlayer())
		    expList[crt->getPlayer()] += clearEnemy(crt);
	}

	for(std::pair<Player*, int> p : expList) {
        Player* ply = p.first;
        int effort = p.second;

        expGain = (experience * effort) / MAX(hp.getMax(), 1);
        expGain = MIN(MAX(0,expGain), experience);

        ply->gainExperience(this, killer, expGain);


        // TODO: Why is this here?
        // Adjust monk/wolf ac and thac0 for extreme aligment
        // **************************************************
        ply->alignAdjustAcThaco();
        // **************************************************
	}
}

//********************************************************************
//						adjustExperience
//********************************************************************

void Creature::adjustExperience(Monster* victim, int& expAmount, int& holidayExp) {
	Player* player;
	if(isPet())
		player = following->getPlayer();
	else
		player = getPlayer();

	if(player->halftolevel()) {
		expAmount = 0;
		holidayExp = 0;
		return;
	}

	if(player->getRace() == HUMAN && expAmount)
		expAmount += MAX(mrand(4,6),expAmount/3/10);

		if(player->getSecondClass()) {
			// Penalty is 12.5% at level 30 and above
			if(player->level >= 30)
				expAmount = (expAmount*7)/8;
			else // and 25% below 30
				expAmount = (expAmount*3)/4;
		}
//	// All experience is multiplied by 3/4 for a multi-classed player
//	if(player->getSecondClass())
//		expAmount = (expAmount*3)/4;

	int levelDiff = abs((int)player->getLevel() - (int)victim->getLevel());
	float multiplier=1.0;

	if(levelDiff <= 5)
		multiplier = 1.0;
	// 6 - 8 levels = 90%
	else if(levelDiff >= 6 && levelDiff <= 8) {
		multiplier = 0.9;
	} // 8 - 10 = 70%
	else if(levelDiff >= 8 && levelDiff <= 10 ) {
		multiplier = 0.70;
	} // 10 - 15 = 50%
	else if(levelDiff >= 10 && levelDiff <= 15) {
		multiplier = 0.50;
	} // 15 - 25 = 25%
	else if(levelDiff >= 15 && levelDiff <= 25) {
		multiplier = 0.25;
	} // 26+ = 10%
	else
		multiplier = 0.10;
	if(multiplier < 1.0) {
//		player->printColor("^YExp Adjustment: %d%% (%d level difference) %d -> %d\n", (int)(multiplier*100), levelDiff, expAmount, (int)(expAmount*multiplier));
		expAmount = (int)(expAmount * multiplier);
		expAmount = MAX(1, expAmount);
	}

	// Add in holiday experience
	bstring holidayStr = isHoliday();

	if(holidayStr != "")
		holidayExp = MAX(1, (int)(expAmount * 0.1));

	if(victim->flagIsSet(M_PERMENANT_MONSTER))
		holidayExp = 0;
}

//********************************************************************
//						gainExperience
//********************************************************************

void Player::gainExperience(Monster* victim, Creature* killer, int expAmount, bool groupExp) {
	int holidayExp = 0;
	adjustExperience(victim, expAmount, holidayExp);
	bool notlocal = false, af = gConfig->isAprilFools();

	if(!inSameRoom(victim))
		notlocal = true;

	bstring holidayStr = isHoliday();
	if(	groupExp ||
		this == killer ||
		!killer || !(notlocal &&
					!(killer->isPlayer() && killer->flagIsSet(P_DM_INVIS)) &&
					!(killer->isPet() && killer->following->flagIsSet(P_DM_INVIS)))
	) {
		printColor("You %s ^y%d^x %sexperience for the death of %N.\n", af ? "lose" : "gain", expAmount, groupExp ? "group " : "", victim);
	} else {
		if(killer->isPet())
			print("%M's %s %s you %d experience for the death of %N.\n", killer->following, killer->name, af ? "cost" : "gained", expAmount, victim);
		else
			print("%M %s you %d experience for the death of %N.\n", killer, af ? "cost" : "gained", expAmount, victim);
	}
	if(holidayExp > 0)
		printColor("%s You %s an extra ^y%d^x experience!\n", holidayStr.c_str(), af ? "lost" : "gained", holidayExp);

	adjustFactionStanding(victim->factions);
	adjustAlignment(victim);
	updateMobKills(victim);

	addExperience(expAmount + holidayExp);

	if(victim->flagIsSet(M_WILL_BE_LOGGED))
		logn("log.mdeath", "%s was killed by %s, for %d experience.\n", victim->name, name, expAmount);

	ctag* fp = first_fol;
	while(fp) {
		if(fp->crt->isPet()) {
			fp->crt->getMonster()->clearEnemy(victim);
			break;
		}
		fp = fp->next_tag;
	}
}

//********************************************************************
//						gainExperience
//********************************************************************

void Monster::gainExperience(Monster* victim, Creature* killer, int expAmount, bool groupExp) {
	Creature* master = following;
	if(!master || !isPet())
		return;
	bool af = gConfig->isAprilFools();

	int holidayExp = 0;
	adjustExperience(victim, expAmount, holidayExp);

	master->printColor("Your %s %s you ^y%d^x experience for the death of %N.\n", this->name, af ? "cost" : "earned", expAmount, victim);

	bstring holidayStr = isHoliday();
	if(holidayExp > 0)
		master->printColor("%s You %s an extra ^y%d^x experience!\n", holidayStr.c_str(), af ? "lost" : "gained", holidayExp);

	master->addExperience(expAmount + holidayExp);
	clearEnemy(victim);
}


//********************************************************************
//						checkDie
//********************************************************************
// wrapper for checkDie
// Return: true if they died, false if they didn't

bool Creature::checkDie(Creature *killer) {
	bool freeTarget=true;
	return(checkDie(killer, freeTarget));
}

//********************************************************************
//						checkDie
//********************************************************************
// Parameters:	<killer> The creature attacking
// If the victim has 0 hp or less, kill them off
// Return: true if they died, false if they didn't

bool Creature::checkDie(Creature *killer, bool &freeTarget) {
	if(hp.getCur() < 1) {
		die(killer, freeTarget);
		return(true);
	}
	return(false);
}

//********************************************************************
//						checkDieRobJail
//********************************************************************
// wrapper for checkDieRobJail

int Creature::checkDieRobJail(Monster *killer) {
	bool freeTarget=true;
	return(checkDieRobJail(killer, freeTarget));
}

//********************************************************************
//						checkDieRobJail
//********************************************************************
// Parameters:	<killer> The creature attacking
// Similar to checkDie, but this function will see if the attacker
// is a mugger, or a jailer and handle appropriately.
// Return Value:  1 <Death>
//				  2 <Unconscious/Jail/Mug>
//				  0 <Nothing>

int Creature::checkDieRobJail(Monster *killer, bool &freeTarget) {
	BaseRoom* room = getRoom();
	Player	*pVictim = getPlayer();
	ASSERTLOG(killer->isMonster());

	// Less than 1 hp & not police/greedy monster or it's monster, then die
	if(hp.getCur() < 1
		&& ((!killer->flagIsSet(M_POLICE) && ! killer->flagIsSet(M_GREEDY)) || isMonster() )
	) {
		die(killer, freeTarget);
		return(1);
	}
	// Police/Greedy & less than 1/10th hp & pVictim
	else if((killer->flagIsSet(M_GREEDY) || killer->flagIsSet(M_POLICE)) &&
		pVictim && pVictim->hp.getCur() < pVictim->hp.getMax()/10)
	{
		freeTarget = false;
		ctag *rcp;
		pVictim->printColor("^r%M knocks you unconscious.\n",killer);
		pVictim->knockUnconscious(39);
		broadcast(pVictim->getSock(), room, "%M knocked %N unconscious.", killer, pVictim);
		pVictim->hp.setCur( pVictim->hp.getMax()/20);
		rcp = room->first_mon;
		while(rcp) {
			rcp->crt->getMonster()->clearEnemy(pVictim);
			rcp = rcp->next_tag;
		}
		killer->clearEnemy(pVictim);
		if(killer->flagIsSet(M_POLICE)) {
			broadcast(pVictim->getSock(), room, "%M picks %N up and hauls %s off.",killer, pVictim, pVictim->himHer());
			if(!killer->jail.id && !pVictim->isStaff())
				broadcast("### Unfortunately, %N was hauled off to jail by %N.",pVictim, killer);
			killer->toJail(pVictim);
			return(2);
		}
		if(killer->flagIsSet(M_GREEDY)) {
			broadcast(pVictim->getSock(), room, "%M rummages through %N's inventory.", killer, pVictim);
			broadcast("### Unfortunately, %N was mugged by %N.", pVictim, killer);
			killer->grabCoins(pVictim);
			return(2);
		}
		return(2);
	}

	freeTarget = false;
	return(0);
}


//********************************************************************
//						die
//********************************************************************
// Parameters:	<killer> The creature attacking
// This function is called when a player dies to something other then a
// player or a monster. -- TC

void Player::die(DeathType dt) {
	bstring death = "";
	Player* killer=0;
	Object* statue=0;
	char	deathStr[2048];
	unsigned long oldxp=0;
	int		n=0;
	unsigned short oldlvl=0;
	bool killedByPlayer = dt == POISON_PLAYER || dt == CREEPING_DOOM;

	deathtype = DT_NONE;
	statistics.die();

	if(isStaff()) {
		printColor("^r*** You just died ***\n");
		broadcast(::isStaff, "^G### Sadly, %s just died.", name);

		clearAsEnemy();

		hp.restore();
		mp.restore();
		return;
	}

	// Any time a level 7+ this dies, they are backed up.
	if(level >= 7)
		save(true, LS_BACKUP);
	if(!killedByPlayer) {
		setFlag(P_KILLED_BY_MOB);
		if(level >= 13) {
			lasttime[LT_MOBDEATH].ltime = time(0);
			lasttime[LT_MOBDEATH].interval = 60*60*24L;
			setFlag(P_NO_SUICIDE);
		}
	}

	switch(dt) {
	case POISON_PLAYER:
	case CREEPING_DOOM:
		// these are player-caused death types
		switch(dt) {
		case POISON_PLAYER:
			sprintf(deathStr, "### Sadly, %s was poisoned to death.", name);
			logn("log.death", "%s was poisoned to death by %s.\n", name, poisonedBy.c_str());
			break;
		case CREEPING_DOOM:
			removeEffect("creeping-doom", false);
			sprintf(deathStr, "### Sadly, %s was killed by cursed spiders.", name);
			logn("log.death", "%s was poisoned to death by %s.\n", name, poisonedBy.c_str());
			break;
		default:
			break;
		}

		if(poisonedBy != "") {
			switch(dt) {
			case POISON_PLAYER:
				sprintf(deathStr, "### Sadly, %s was poisoned to death by %s.", name, poisonedBy.c_str());
				break;
			case CREEPING_DOOM:
				sprintf(deathStr, "### Sadly, %s was killed by %s's cursed spiders.", name, poisonedBy.c_str());
				break;
			default:
				break;
			}

			killer = gServer->findPlayer(poisonedBy.c_str());
			if(killer)
				getPkilled(killer, false, false);
		}

		break;
	case POISON_MONSTER:
		sprintf(deathStr, "### Sadly, %s was poisoned to death by %s.", name, poisonedBy.c_str());
		logn("log.death", "%s was poisoned to death by %s.\n", name, poisonedBy.c_str());
		death = "poison";
		break;
	case POISON_GENERAL:
		sprintf(deathStr, "### Sadly, %s was poisoned to death.", name);
		logn("log.death", "%s was poisoned to death.\n", name);
		death = "poison";
		break;
	case FALL:
		sprintf(deathStr, "### Sadly, %s fell to %s death.", name, hisHer());
		logn("log.death", "%s was killed by a fall.\n", name);
		death = "a fall";
		break;
	case PETRIFIED:
		sprintf(deathStr, "### Sadly, %s was turned to stone.\n### %s statue crumbles and breaks.",
			name, upHisHer());

		// all inventory, equipment, and gold are destroyed
		lose_all(this, true, "petrification");
		coins.set(0, GOLD);
		printColor("^rAll your possessions were lost!\n");

		if(level >= 10) {
			if(loadObject(STATUE_OBJ, &statue)) {
				sprintf(statue->name, "broken statue of %s", name);
				statue->description = name;
				statue->description += " is forever frozen in stone.";
				strncpy(statue->key[0], "broken", 20);
				strncpy(statue->key[1], "statue", 20);
				strncpy(statue->key[2], name, 20);

				statue->setWeight(100 + getWeight());
				statue->setBulk(50);
				statue->setFlag(O_NO_BREAK);

				statue->addToRoom(getRoom());
			}
		}

		logn("log.death", "%s died from petrification.\n", name);
		death = "petrification";

		removeEffect("petrification", false);

		break;
	case DISEASE:
		sprintf(deathStr, "### Sadly, %s died from disease.", name);
		logn("log.death", "%s was killed by disease.\n", name);
		death = "disease";
		break;
	case WOUNDED:
		logn("log.death", "%s was killed by festering wounds.\n", name);
		sprintf(deathStr, "### Sadly, %s has bled to death.", name);
		death = "festering wounds";
		break;
	case ELVEN_ARCHERS:
		logn("log.death", "%s was shot to death by elven archers.\n", name);
		sprintf(deathStr, "### Sadly, %s was shot to death by elven archers.", name);
		death = "elven archers";
		break;
	case DEADLY_MOSS:
		logn("log.death", "%s was choked to death by deadly underdark moss.\n", name);
		sprintf(deathStr, "### Sadly, %s was choked to death by deadly underdark moss.", name);
		death = "deadly underdark moss";
	case PIERCER:
		sprintf(deathStr, "### Sadly, %s was impaled to death by a piercer.", name);
		logn("log.death", "%s was killed by a piercer.\n", name);
		death = "a piercer";
		break;
	case SMOTHER:
		sprintf(deathStr, "### Sadly, %s was engulfed by the earth.", name);
		logn("log.death", "%s was killed by an earth damage room.\n", name);
		death = "engulfing earth";
		break;
	case FROZE:
		sprintf(deathStr, "### Sadly, %s froze to death.", name);
		logn("log.death", "%s was killed by an air damage room.\n", name);
		death = "hypothermia";
		break;
	case LIGHTNING:
		sprintf(deathStr, "### Sadly, %s was blasted to bits by electricity.", name);
		logn("log.death", "%s was killed by an electricity damage room.\n", name);
		death = "electricity";
		break;
	case WINDBATTERED:
		sprintf(deathStr, "### Sadly, %s was ripped apart by the wind.", name);
		logn("log.death", "%s was killed by an air damage room.\n", name);
		death = "battering winds";
		break;
	case BURNED:
		sprintf(deathStr, "### Sadly, %s was burned alive.", name);
		logn("log.death", "%s was killed by a fire damage room or effect.\n", name);
		death = "a raging fire";
		break;
	case THORNS:
		sprintf(deathStr, "### Sadly, %s was killed by a wall of thorns.", name);
		logn("log.death", "%s was killed by a wall of thorns.\n", name);
		death = "a wall of thorns";
		break;
	case DROWNED:
		sprintf(deathStr, "### Sadly, %s drowned.", name);
		logn("log.death", "%s was killed by drowning.\n", name);
		death = "drowning";
		break;
	case DRAINED:
		sprintf(deathStr, "### Sadly, %s's life force was completely drained.", name);
		logn("log.death", "%s was killed by a pharm room.\n", name);
		death = "life-drain";
		break;
	case ZAPPED:
		sprintf(deathStr, "### Sadly, %s was zapped to death.", name);
		logn("log.death", "%s was killed by a combo lock.\n", name);
		death = "a fatal shock";
		break;
	case SHOCKED:
		sprintf(deathStr, "### Sadly, %s was shocked to death.", name);
		logn("log.death", "%s was killed by shocking.\n", name);
		death = "a fatal shock";
		break;
	case PIT:
		sprintf(deathStr, "### Sadly, %s fell into a pit and died.", name);
		logn("log.death", "%s was killed by a pit trap.\n", name);
		death = "a fall";
		break;
	case BLOCK:
		sprintf(deathStr, "### Sadly, %s was crushed to death by a giant stone block.", name);
		logn("log.death", "%s was killed by a stone block trap.\n", name);
		death = "a giant stone block";
		break;
	case DART:
		sprintf(deathStr, "### Sadly, %s was killed by a poisoned dart.", name);
		logn("log.death", "%s was killed by a poison dart trap.\n", name);
		death = "a poisoned dart";
		break;
	case ARROW:
		sprintf(deathStr, "### Sadly, %s was killed by a flight of arrows.", name);
		logn("log.death", "%s was killed by an arrow trap.\n", name);
		death = "a flight of arrows";
		break;
	case SPIKED_PIT:
		sprintf(deathStr, "### Sadly, %s fell into a pit and was impaled by spikes.", name);
		logn("log.death", "%s was killed by a spiked pit trap.\n", name);
		death = "a fall";
		break;
	case FIRE_TRAP:
		sprintf(deathStr, "### Sadly, %s was engulfed by flames and died.", name);
		logn("log.death", "%s was killed by a fire trap.\n", name);
		death = "a raging fire";
		break;
	case FROST:
		sprintf(deathStr, "### Sadly, %s was frozen alive, and died.", name);
		logn("log.death", "%s was killed by a frost trap.\n", name);
		death = "hypothermia";
		break;
	case ELECTRICITY:
		sprintf(deathStr, "### Sadly, %s was killed by electrocution.", name);
		logn("log.death", "%s was killed by an electricity trap.\n", name);
		death = "electricity";
		break;
	case ACID:
		sprintf(deathStr, "### Sadly, %s was dissolved by acid.", name);
		logn("log.death", "%s was killed by an acid trap.\n", name);
		death = "acid";
		break;
	case ROCKS:
		sprintf(deathStr, "### Sadly, %s was crushed to death in a rockslide.", name);
		logn("log.death", "%s was killed by a rockslide trap.\n", name);
		death = "a rockslide";
		break;
	case ICICLE_TRAP:
		sprintf(deathStr, "### Sadly, %s was impaled by a giant icicle.", name);
		logn("log.death", "%s was killed by a falling icicle trap.\n", name);
		death = "a giant icicle";
		break;
	case SPEAR:
		sprintf(deathStr, "### Sadly, %s was killed by a giant spear.", name);
		logn("log.death", "%s was killed by a spear trap.\n", name);
		death = "a spear";
		break;
	case CROSSBOW_TRAP:
		sprintf(deathStr, "### Sadly, %s was killed by a crossbow trap.", name);
		logn("log.death", "%s was killed by a crossbow trap.\n", name);
		death = "a crossbow bolt";
		break;
	case VINES:
		sprintf(deathStr, "### Sadly, %s was ripped apart by crawling vines.", name);
		logn("log.death", "%s was killed by deadly vines.\n", name);
		death = "deadly vines";
		break;
	case COLDWATER:
		sprintf(deathStr, "### Sadly, %s died from hypothermia.", name);
		logn("log.death", "%s was killed from hypothermia.\n", name);
		death = "hypothermia";
		break;
	case EXPLODED:
		sprintf(deathStr, "### Sadly, %s exploded.", name);
		logn("log.death", "%s was killed by exploding.\n", name);
		death = "self-combustion";
		break;
	case SPLAT:
		sprintf(deathStr, "### Sadly, %s tumbled to %s death. (SPLAT!)", name, hisHer());
		logn("log.death", "%s tumbled to %s death.\n", name, hisHer());
		death = "a fall";
		break;
	case BOLTS:
		sprintf(deathStr, "### Sadly, %s was blasted to death by energy bolts.", name);
		logn("log.death", "%s was killed by energy bolts.\n", name);
		death = "energy bolts";
		break;
	case BONES:
		sprintf(deathStr, "### Sadly, %s was crushed to death under an avalanche of bones.", name);
		logn("log.death", "%s was killed by a bone avalanche.\n", name);
		death = "an avalanche of bones";
		break;
	case EXPLOSION:
		sprintf(deathStr, "### Sadly, %s was vaporized in a magical explosion.", name);
		logn("log.death", "%s was killed by a magical explosion.\n", name);
		death = "a magical explosion";
		break;
	case SUNLIGHT:
		sprintf(deathStr, "### Sadly, %s was disintegrated by sunlight.", name);
		logn("log.death", "%s was disintegrated by sunlight.\n", name);
		death = "sunlight";
		break;
	default:
		sprintf(deathStr, "### Sadly, %s died.", name);
		logn("log.death", "%s was killed by %s.\n", name, name);
		death = "misfortune";
		break;
	}
	//if(level > 2)
	//	broadcast(deathStr);
	//else {
	//	broadcast(::isWatcher, "^C%s", deathStr);
	//	print(deathStr);
	//}
	broadcast(deathStr);


	oldxp = experience;
	oldlvl = level;

	if(!killedByPlayer) {
		cureDisease();
		removeCurse();
	}
	if(!killedByPlayer || dt == POISON_PLAYER)
		curePoison();

	// only drop all if killed by player
	dropEquipment(killedByPlayer && (!killer || !killer->isStaff()), killer ? killer->getSock() : 0);

	/*
	if(ready[WIELD - 1] && !ready[WIELD - 1]->flagIsSet(O_CURSED))
		unequip(WIELD);

	if(	ready[HELD - 1] &&
		ready[HELD - 1]->getWearflag() == WIELD &&
		!ready[HELD - 1]->flagIsSet(O_CURSED)
	) {
		Object* held = unequip(HELD, UNEQUIP_NOTHING);
		if(held) {
			held->addToRoom(getRoom());
			held->tempPerm();
		}
	}

	Object* temp=0;
	for(i=0; i<MAXWEAR; i++) {
		if(ready[i] && !ready[i]->flagIsSet(O_CURSED)) {
			if(!killedByPlayer) {
				// i is wearloc-1 so add 1
				unequip(i+1);
			}
			else {
				temp = unequip(i+1, UNEQUIP_NOTHING);
				if(temp)
					temp->addToRoom(getRoom());
			}
		}
	}
	*/
	checkDarkness();
	computeAC();
	computeAttackPower();
	courageous();


	int xploss = 0;
	if(!killedByPlayer) {
		if(level >= 5)
			addEffect("death-sickness");

		if(level < 10) {
			// Under level 10, 10% exp loss
			xploss = (int)((float)experience / 10.0);
			lostExperience += (long)xploss;
		} else {
			// Level 10 and over, 2% exp loss with a minimum of 10k
			xploss = MAX((long)( (float)experience * 0.02), 10000);
			print("You have lost %ld experience.\n", (long)xploss);
			lostExperience += (long)xploss;
		}
		subExperience((long)xploss);

	}


	n = level - exp_to_lev(experience);

	if(n > 1) {
		if(level < (MAXALVL+2))
			experience = needed_exp[level-3];
		else
			experience = (long)((needed_exp[MAXALVL-1]*(level-2)));
	}


	if(level >= 7)
		logn("log.death", "L:%dXP:%dE:%luA:%luF:%luW:%luE:%luC:%lu\n",
		     oldlvl, oldxp,
		     getRealm(EARTH), getRealm(WIND), getRealm(FIRE), getRealm(WATER),
		     getRealm(ELEC), getRealm(COLD));



	if(!killedByPlayer) {
		n = exp_to_lev(experience);
		while(level > n)
			downLevel();

		negativeLevels = 0;
	}

	hp.restore();
	mp.restore();

	unhide();

	// more info for staff
	broadcast(::isCt, "^rOld Exp: %u Lost Exp: %d, Old Lvl: %u, Lvl: %u, Room: %s",
		oldxp, xploss, oldlvl, level, getRoom()->fullName().c_str());

	if(isHardcore()) {
		hardcoreDeath(this);
	// if you die in jail, you stay in jail
	} else if(!inJail()) {
		BaseRoom *newRoom = getLimboRoom().loadRoom(this);
		if(newRoom) {
			deleteFromRoom();
			addToRoom(newRoom);
			doPetFollow();
		}
	}
}

//********************************************************************
//						clearAsPetEnemy
//********************************************************************

void Creature::clearAsPetEnemy() {
	ctag *cp = getRoom()->first_mon;
	while(cp) {
		if(cp->crt->isPet())
			cp->crt->getMonster()->clearEnemy(this);
		cp = cp->next_tag;
	}
}

//********************************************************************
//						cleanFollow
//********************************************************************

void Monster::cleanFollow(Creature *killer) {
	if(isMonster() && following) {
		Player* player = following->getPlayer();

		// This should fix the bug with having a dm's pet killed while possessing a mob
		if(flagIsSet(M_DM_FOLLOW)) {
			player->setAlias(0);
			player->clearFlag(P_ALIASING);
		}
		if(following != killer) {
			player->printColor("^r%M's body has been destroyed.\n", this);
			if(killer)
				broadcast(killer->getSock(), getRoom(), "%M was killed by %N.", this, killer);
		}
		doStopFollowing(this, FALSE);
	}
}

//*********************************************************************
//						dropWeapons
//*********************************************************************
// handle dropping of weapons when you flee or die
// Return: true if any weapons were dropped, false if none were dropped

bool Player::dropWeapons() {
	BaseRoom* room = getRoom();
	Object* weapon;
	bool	dropMain=false, dropSec=false, fumbleMain=false, fumbleSec=false;
	bool	dropDestroy = room->isDropDestroy();
	bool	jump=false;

	if(isStaff())
		return(false);

	// check main weapons
	weapon = ready[WIELD-1];
	if(weapon && !weapon->flagIsSet(O_CURSED)) {
		if(weapon->flagIsSet(O_NO_DROP) || weapon->flagIsSet(O_STARTING) || dropDestroy) {
			// Add to inventory
			unequip(WIELD);
			fumbleMain = true;
		} else {
			dropMain = true;
			// Unequip, and add to the room
			weapon = unequip(WIELD, UNEQUIP_NOTHING, false);
			finishDropObject(weapon, room, this);
		}
	}

	// check secondary weapons
	weapon = ready[HELD-1];
	if(weapon && weapon->getWearflag() == WIELD) {
		if(weapon->flagIsSet(O_CURSED)) {
			if(!ready[WIELD-1]) {
				ready[WIELD-1] = ready[HELD-1];
				ready[HELD-1] = 0;
				jump = true;
			}
		} else {
			if(weapon->flagIsSet(O_NO_DROP) || weapon->flagIsSet(O_STARTING) || dropDestroy) {
				fumbleSec = true;
				// Add to inventory
				unequip(HELD);
			} else {
				dropSec = true;
				// Unequip and add to the room
				weapon = unequip(HELD, UNEQUIP_NOTHING, false);
				finishDropObject(weapon, room, this);
			}
		}
	}

	if(dropMain || dropSec || fumbleMain || fumbleSec) {
		// only fumble
		if(!dropMain && !dropSec)
			if(fumbleMain && fumbleSec)
				print("You fumble your weapons.\n");
			else
				print("You fumble your %sweapon.\n", fumbleSec ? "secondary " : "");

		// only drop
		else if(!fumbleMain && !fumbleSec)
			if(dropMain && dropSec)
				print("You drop your weapons.\n");
			else
				print("You drop your %sweapon.\n", dropSec ? "secondary " : "");

		// one of each
		else
			print("You drop your %s weapon and fumble your %s weapon.\n",
				dropMain ? "main" : "secondary", fumbleMain ? "main" : "secondary");

		if(jump)
			print("%s%s jumped to your primary hand! It's cursed!\n",
				!ready[WIELD-1]->flagIsSet(O_NO_PREFIX) ? "The " : "", ready[WIELD-1]->name);

		// checkDarkness(), computeAttackPower(), computeAC() should be handled outside this function
		return(true);
	}

	return(false);
}

//********************************************************************
