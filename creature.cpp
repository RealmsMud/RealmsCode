/*
 * creature.cpp
 *   Routines that act on creatures
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
#include "commands.h"
#include "effects.h"
#include "calendar.h"

//#include <time.h>
#include <fcntl.h>
#include "move.h"

const short Creature::OFFGUARD_REMOVE=0;
const short Creature::OFFGUARD_NOREMOVE=1;
const short Creature::OFFGUARD_NOPRINT=2;

//*********************************************************************
//						sameRoom
//*********************************************************************

bool Creature::inSameRoom(const Creature *b) const {
	return(getRoom() == b->getRoom());
}

//*********************************************************************
//						find_exact_crt
//*********************************************************************
// This function will attempt to locate a given creature within a given
// list.  The first parameter contains a pointer to the creature doing
// the search, the second contains a tag pointer to the first tag in
// the list.  The third contains the match string, and the fourth
// contains the number of times to match the string.

Creature *find_exact_crt(const Creature* player, ctag *first_ct, char *str, int val) {
	Creature* target=0;
	ctag	*cp=0;
	int		match=0;

	ASSERTLOG( player );
	ASSERTLOG( str );

	if(!player || !str || !first_ct)
		return(0);

	cp = first_ct;
	while(cp) {
		target = cp->crt;
		cp = cp->next_tag;

		if(!target)
			continue;
		if(!player->canSee(target))
			continue;

		if(!strcmp(target->name, str)) {
			match++;
			if(match == val)
				return(target);
		}
	}
	return(0);
}

//*********************************************************************
//						getFirstAggro
//*********************************************************************

Creature *getFirstAggro(Creature* creature, Creature* player) {
	ctag		*cp=0;
	Creature *foundCrt=0;

	if(creature->isPlayer())
		return(creature);

	cp = creature->getRoom()->first_mon;
	while(cp) {
		if(strcmp(cp->crt->name, creature->name)) {
			cp = cp->next_tag;
			continue;
		}
		if(cp->crt->getMonster()->isEnemy(player)) {
			foundCrt = cp->crt;
			break;
		}

		cp = cp->next_tag;
	}

	if(foundCrt)
		return(foundCrt);
	else
		return(creature);
}

//**********************************************************************
//* AddEnemy
//**********************************************************************

// Add target to the threat list of this monster

bool Monster::addEnemy(Creature* target, bool print) {
    if(isEnemy(target))
        return false;

    if(print) {
      if(aggroString[0])
          broadcast(NULL, getRoom(), "%M says, \"%s.\"", this, aggroString);

      target->printColor("^r%M attacks you.\n", this);
      broadcast(target->getSock(), getRoom(), "%M attacks %N.", this, target);
    }

    if(target->isPlayer()) {
      // take pity on these people
      if(target->isEffected("petrification") || target->isUnconscious())
          return(0);
      // pets should not attack master
      if(isPet() && getMaster() == target)
          return(false);
    }
    if(target->isPet()) {
        addEnemy(target->getMaster());
    }

    threatTable->adjustThreat(target, 0);
    return(true);
}
bool Monster::isEnemy(const Creature* target) const {
    return(threatTable->isEnemy(target));
}

bool Monster::hasEnemy() const {
    return(threatTable->hasEnemy());
}

long Monster::adjustContribution(Creature* target, long modAmt) {
    return(threatTable->adjustThreat(target, modAmt, 0));
}
long Monster::adjustThreat(Creature* target, long modAmt, double threatFactor) {
    return(threatTable->adjustThreat(target, modAmt, threatFactor));
}
long Monster::clearEnemy(Creature* target) {
    return(threatTable->removeThreat(target));
}
//**********************************************************************
//* getTarget
//**********************************************************************
// Returns the first valid creature with the highest threat
// (in the same room if sameRoom == true)
Creature* Monster::getTarget(bool sameRoom) {
    return(threatTable->getTarget(sameRoom));
}


//*********************************************************************
//						nearEnemy
//*********************************************************************

bool Monster::nearEnemy() const {
	ctag	*cp=0;

	if(nearEnmPly())
		return(true);

	cp = getRoom()->first_mon;
	while(cp) {
		if(isEnemy(cp->crt) && !cp->crt->flagIsSet(M_FAST_WANDER))
			return(true);
		cp = cp->next_tag;
	}

	return(false);
}

//*********************************************************************
//						nearEnmPly
//*********************************************************************

bool Monster::nearEnmPly() const {
	ctag	*cp = getRoom()->first_ply;

	while(cp) {
		if(isEnemy(cp->crt) && !cp->crt->flagIsSet(P_DM_INVIS))
			return(true);
		cp = cp->next_tag;
	}

	return(false);
}

//*********************************************************************
//						nearEnemy
//*********************************************************************
// Searches for a near enemy, excluding the current player being attacked.

bool Monster::nearEnemy(const Creature* target) const {
	ctag	*cp=0;
	BaseRoom* room = getRoom();

	cp = room->first_ply;
	while(cp) {
		if(	isEnemy(cp->crt) &&
			!cp->crt->flagIsSet(P_DM_INVIS) &&
			strcmp(target->name, cp->crt->name)
		)
			return(true);
		cp = cp->next_tag;
	}

	cp = room->first_mon;
	while(cp) {
		if(isEnemy(cp->crt) && !cp->crt->flagIsSet(M_FAST_WANDER))
			return(true);
		cp = cp->next_tag;
	}

	return(false);
}


//*********************************************************************
//						tempPerm
//*********************************************************************

void Object::tempPerm() {
	otag	*op=0;

	setFlag(O_PERM_INV_ITEM);
	setFlag(O_TEMP_PERM);
	op = first_obj;
	while(op) {
		op->obj->tempPerm();
		op = op->next_tag;
	}
}

//*********************************************************************
//						diePermCrt
//*********************************************************************
// This function is called whenever a permanent monster is killed. The
// room the monster was in has its permanent monster list checked to
// if the monster was loaded from that room. If it was, then the last-
// time field for that permanent monster is updated.

void Monster::diePermCrt() {
	std::map<int, crlasttime>::iterator it;
	crlasttime* crtm=0;
	Monster *temp_mob=0;
	UniqueRoom	*room=0;
	char	perm[80];
	long	t = time(0);
	int		i=0;

	strcpy(perm,name);

	if(!parent_rom)
		return;
	room = parent_rom;

	for(it = room->permMonsters.begin(); it != room->permMonsters.end() ; it++) {
		crtm = &(*it).second;
		if(!crtm->cr.id)
			continue;
		if(crtm->ltime + crtm->interval > t)
			continue;
		if(!loadMonster(crtm->cr, &temp_mob))
			continue;
		if(!strcmp(temp_mob->name, name)) {
			crtm->ltime = t;
			free_crt(temp_mob);
			break;
		}
		free_crt(temp_mob);
	}

	if(flagIsSet(M_DEATH_SCENE) && !flagIsSet(M_FOLLOW_ATTACKER)) {
		int     fd,n;
		char    tmp[2048], file[80],pName[80];

		strcpy(pName, name);
		for(i=0; pName[i]; i++)
			if(pName[i] == ' ')
				pName[i] = '_';

		sprintf(file,"%s/%s_%d", Path::Desc, pName, level);
		fd = open(file,O_RDONLY,0);
		if(fd) {
			n = read(fd,tmp,2048);
			tmp[n] = 0;
			broadcast(NULL, getRoom(), "\n%s", tmp);
		}
		close(fd);
	}
}

//*********************************************************************
//						consider
//*********************************************************************
// This function allows the player pointed to by the first argument to
// consider how difficult it would be to kill the creature in the
// second parameter.

bstring Player::consider(Creature* creature) const {
	int		diff=0;
	bstring str = "";

	if(creature->mFlagIsSet(M_UNKILLABLE))
		return("");

	// staff always needle
	if(creature->isStaff() && !isStaff())
		diff = -4;
	else if(isStaff() && !creature->isStaff())
		diff = 4;
	else {
		diff = level - creature->getLevel();
		diff = MAX(-4, MIN(4, diff));
	}

	// upHeShe is used most often
	str = creature->upHeShe();

	switch(diff) {
	case 0:
		str += " is a perfect match for you!\n";
		break;
	case 1:
		str += " is not quite as good as you.\n";
		break;
	case -1:
		str += " is a little better than you.\n";
		break;
	case 2:
		str += " shouldn't be too tough to kill.\n";
		break;
	case -2:
		str += " might be tough to kill.\n";
		break;
	case 3:
		str += " should be easy to kill.\n";
		break;
	case -3:
		str += " should be really hard to kill.\n";
		break;
	case 4:
		str = "You could kill ";
		str += creature->himHer();
		str += " with a needle.\n";
		break;
	case -4:
		str += " could kill you with a needle.\n";
		break;
	}
	return(str);
}

//********************************************************************
//						hasCharm
//********************************************************************
// This function returns true if the name passed in the first parameter
// is in the charm list of the creature

bool Creature::hasCharm(bstring charmed) {
	etag    *cp=0;

	if(!this || charmed == "")
		return(false);

	Player* player = getPlayer();

	if(!player)
		return(false);

	cp = player->first_charm;

	while(cp) {
		if(!strcmp(cp->enemy, charmed.c_str()))
			return(true);
		cp = cp->next_tag;
	}

	return(false);
}

//********************************************************************
//						addCharm
//********************************************************************
// This function adds the creature pointed to by the first
// parameter to the charm list of the creature

int Player::addCharm(Creature* creature) {
	etag    *cp=0;

	ASSERTLOG( creature );

	if(hasCharm(creature->name))
		return(0);

	if(creature && !creature->isDm()) {
		cp = new etag;
		if(!cp)
			merror("add_charm_crt", FATAL);

		strcpy(cp->enemy, creature->name);
		cp->next_tag = first_charm;
		first_charm = cp;
	}
	return(0);
}

//********************************************************************
//						delCharm
//********************************************************************
// This function deletes the creature pointed to by the first
// parameter from the charm list of the creature

int Player::delCharm(Creature* creature) {
	etag    *cp, *prev;

	ASSERTLOG( creature );

	if(!hasCharm(creature->name))
		return(0);

	cp = first_charm;

	if(!strcmp(cp->enemy, creature->name)) {
		first_charm = cp->next_tag;
		delete cp;
		return(1);
	} else
		while(cp) {
			if(!strcmp(cp->enemy, creature->name)) {
				prev->next_tag = cp->next_tag;
				delete cp;
				return(1);
			}
			prev = cp;
			cp = cp->next_tag;
		}
	return(0);
}

//*********************************************************************
//						mobileEnter
//*********************************************************************

bool mobileEnter(Exit* exit) {
	return( !exit->flagIsSet(X_SECRET) &&
		!exit->flagIsSet(X_NO_SEE) &&
		!exit->flagIsSet(X_LOCKED) &&
		!exit->flagIsSet(X_PASSIVE_GUARD) &&
		!exit->flagIsSet(X_LOOK_ONLY) &&
		!exit->flagIsSet(X_NO_WANDER) &&
		!exit->isConcealed() &&
		!exit->flagIsSet(X_DESCRIPTION_ONLY) &&
		!exit->isWall("wall-of-fire") &&
		!exit->isWall("wall-of-force") &&
		!exit->isWall("wall-of-thorns")
	);
}

//*********************************************************************
//						mobile_crt
//*********************************************************************
// This function provides for self-moving monsters. If
// the M_MOBILE_MONSTER flag is set the creature will move around.

int Monster::mobileCrt() {
	BaseRoom *newRoom=0;
	UniqueRoom	*uRoom=0;
	AreaRoom* aRoom=0, *caRoom = area_room;
	xtag	*xp=0;
	int		i=0, num=0, ret=0;
	bool	mem=false;


	if(	!flagIsSet(M_MOBILE_MONSTER) ||
		flagIsSet(M_PERMENANT_MONSTER) ||
		flagIsSet(M_PASSIVE_EXIT_GUARD)
	)
		return(0);

	if(nearEnemy())
		return(0);

	xp = getRoom()->first_ext;
	while(xp) {
		// count up exits
		if(mobileEnter(xp->ext))
			i += 1;
		xp = xp->next_tag;
	}

	if(!i)
		return(0);

	num = mrand(1, i);
	i = 0;

	xp = getRoom()->first_ext;
	while(xp) {
		if(mobileEnter(xp->ext))
			i += 1;

		if(i == num) {

			// get us out of this room
			if(!Move::getRoom(this, xp->ext, &uRoom, &aRoom))
				return(0);
			if(aRoom) {
				mem = aRoom->getStayInMemory();
				aRoom->setStayInMemory(true);
				newRoom = aRoom;
			} else
				newRoom = uRoom;

			// make sure there are no problems with the new room
			if(!newRoom)
				return(0);
			if(newRoom->countCrt() >= newRoom->getMaxMobs())
				return(0);

			// no wandering between live/construction areas
			if(newRoom->isConstruction() != getRoom()->isConstruction())
				return(0);

			if(xp->ext->flagIsSet(X_CLOSED) && !xp->ext->flagIsSet(X_LOCKED)) {
				broadcast(NULL, getRoom(), "%M just opened the %s.", this, xp->ext->name);
				xp->ext->clearFlag(X_CLOSED);
			}

			if(flagIsSet(M_WILL_SNEAK) && flagIsSet(M_HIDDEN))
				setFlag(M_SNEAKING);

			if(	flagIsSet(M_SNEAKING) &&
				mrand (1,100) <= (3+dexterity.getCur())*3)
			{
				broadcast(::isStaff, getSock(), getRoom(), "*DM* %M just snuck to the %s.", this,xp->ext->name);
			} else {
			    Creature* lookingFor = NULL;
				if(flagIsSet(M_CHASING_SOMEONE) && hasEnemy() && ((lookingFor = getTarget(false)) != NULL) ) {

					broadcast(NULL, getRoom(), "%M %s to the %s^x, looking for %s.",
						this, Move::getString(this).c_str(), xp->ext->name, lookingFor->getName());
				}
				else
					broadcast(NULL, getRoom(), "%M just %s to the %s^x.",
						this, Move::getString(this).c_str(), xp->ext->name);

				clearFlag(M_SNEAKING);
			}


			// see if we can recycle this room
			deleteFromRoom();
			if(caRoom && aRoom == caRoom) {
				aRoom = Move::recycle(aRoom, xp->ext);
				newRoom = aRoom;
			}
			addToRoom(newRoom);

			// reset stayInMemory
			if(aRoom)
				aRoom->setStayInMemory(mem);

			lasttime[LT_MON_WANDER].ltime = time(0);
			lasttime[LT_MON_WANDER].interval = mrand(5,60);

			ret = 1;
			break;
		}
		xp = xp->next_tag;
	}

	if(mrand(1,100) > 80)
		clearFlag(M_MOBILE_MONSTER);

	return(ret);
}

//*********************************************************************
//						monsterCombat
//*********************************************************************
// This function should make monsters attack each other

void Monster::monsterCombat(Monster *target) {
	broadcast(NULL, getRoom(), "%M attacks %N.\n", this, target);

	addEnemy(target);
}

//*********************************************************************
//						mobWield
//*********************************************************************

int Monster::mobWield() {
	otag	*op=0;
	Object	*object=0;
	int		i=0, found=0;

	if(!first_obj)
		return(0);

	if(ready[WIELD - 1])
		return(0);

	op = first_obj;
	while(op) {
		for(i=0;i<10;i++) {
			if(carry[i].info == op->obj->info) {
				found=1;
				break;
			}
		}

		if(!found) {
			op = op->next_tag;
			continue;
		}

		if(op->obj->getWearflag() == WIELD) {
			if(	(op->obj->damage.getNumber() + op->obj->damage.getSides() + op->obj->damage.getPlus()) <
					(damage.getNumber() + damage.getSides() + damage.getPlus())/2 ||
				(op->obj->getShotsCur() < 1)
			) {
				op = op->next_tag;
				continue;
			}

			object = op->obj;
			break;
		}
		op = op->next_tag;
	}
	if(!op)
		return(0);
	if(!object)
		return(0);

	equip(object, WIELD);
	return(1);
}

//*********************************************************************
//						getMobSave
//*********************************************************************

void Monster::getMobSave() {
	int		lvl=0, cls=0, index=0;

	if(cClass == 0)
		cls = 4;	// All mobs default save as fighters.
	else
		cls = cClass;

	if((saves[POI].chance +
		 saves[DEA].chance +
		 saves[BRE].chance +
		 saves[MEN].chance +
		 saves[SPL].chance) == 0)
	{


		saves[POI].chance = 5;
		saves[DEA].chance = 5;
		saves[BRE].chance = 5;
		saves[MEN].chance = 5;
		saves[SPL].chance = 5;

		if(race) {
			saves[POI].chance += gConfig->getRace(race)->getSave(POI);
			saves[DEA].chance += gConfig->getRace(race)->getSave(DEA);
			saves[BRE].chance += gConfig->getRace(race)->getSave(BRE);
			saves[MEN].chance += gConfig->getRace(race)->getSave(MEN);
			saves[SPL].chance += gConfig->getRace(race)->getSave(SPL);
		}

		if(level > 1) {
			for(lvl = 2;  lvl <= level; lvl++) {
				index = (lvl - 2) % 10;
				switch (saving_throw_cycle[cls][index]) {
				case POI:
					saves[POI].chance += 6;
					break;
				case DEA:
					saves[DEA].chance += 6;
					break;
				case BRE:
					saves[BRE].chance += 6;
					break;
				case MEN:
					saves[MEN].chance += 6;
					break;
				case SPL:
					saves[SPL].chance += 6;
					break;
				}

			}
		}

		saves[LCK].chance = ((saves[POI].chance +
			saves[DEA].chance +
			saves[BRE].chance +
			saves[MEN].chance +
			saves[SPL].chance)/5);
	}
}

//*********************************************************************
//						castDelay
//*********************************************************************

void Creature::castDelay(long delay) {
	if(delay < 1)
		return;

	lasttime[LT_SPELL].ltime = time(0);
	lasttime[LT_SPELL].interval = delay;
}

//*********************************************************************
//						attackDelay
//*********************************************************************

void Creature::attackDelay(long delay) {
	if(delay < 1)
		return;

	getPlayer()->updateAttackTimer(true, delay*10);
}

//*********************************************************************
//						enm_in_group
//*********************************************************************

Creature *enm_in_group(Creature *target) {
	Creature *enemy=0;
	int		chosen=0;


	if(!target || mrand(1,100) <= 50)
		return(target);

	Group* group = target->getGroup();

	if(!group)
	    return(target);

	chosen = mrand(1, group->getSize());

	enemy = group->getMember(chosen);
	if(!enemy || !enemy->inSameRoom(target))
	    enemy = target;

	return(enemy);

}

//*********************************************************************
//						clearEnemyList
//*********************************************************************

void Monster::clearEnemyList() {
    threatTable->clear();
}

//*********************************************************************
//						clearMobInventory
//*********************************************************************

void Monster::clearMobInventory() {
	otag *op = first_obj;
	Object* object=0;

	while(op) {
		object = op->obj;
		op = op->next_tag;
		this->delObj(object, false, false, true, false);
		delete object;
	}
	checkDarkness();
}

//*********************************************************************
//						cleanMobForSaving
//*********************************************************************
// This function will clean up the mob so it is ready for saving to the database
// ie: it will clear the inventory, any flags that shouldn't be saved to the
// database, clear any following etc.

int Monster::cleanMobForSaving() {

	// No saving pets!
	if(isPet())
		return(-1);

	// Clear the inventory
	clearMobInventory();

	// Clear any flags that shouldn't be set
	clearFlag(M_WILL_BE_LOGGED);

//	// If the creature is possessed, clean that up
	if(flagIsSet(M_DM_FOLLOW)) {
		clearFlag(M_DM_FOLLOW);
		Player* master;
		if(getMaster() != NULL && (master = getMaster()->getPlayer()) != NULL) {
		    master->clearFlag(P_ALIASING);
		    master->getPlayer()->setAlias(0);
		    master->print("%1M's soul was saved.\n", this);
			removeFromGroup(false);
		}
	}

	// Success
	return(1);
}

//*********************************************************************
//						displayFlags
//*********************************************************************

int Creature::displayFlags() const {
	int flags = 0;
	if(isEffected("detect-invisible"))
		flags |= INV;
	if(isEffected("detect-magic"))
		flags |= MAG;
	if(isEffected("true-sight"))
		flags |= MIST;
	if(isPlayer()) {
		if(cClass == BUILDER)
			flags |= ISBD;
		if(cClass == CARETAKER)
			flags |= ISCT;
		if(isDm())
			flags |= ISDM;
		if(flagIsSet(P_NO_NUMBERS))
			flags |= NONUM;
	}
	return(flags);
}

//*********************************************************************
//						displayCreature
//*********************************************************************

int Player::displayCreature(Creature* target) const {
	int		percent=0, align=0, rank=0, chance=0, flags = displayFlags();
	Player	*pTarget = target->getPlayer();
	Monster	*mTarget = target->getMonster();
	std::ostringstream oStr;
	bstring str = "";
	bool space=false;

	if(mTarget) {
		oStr << "You see " << mTarget->getCrtStr(this, flags, 1) << ".\n";
		if(mTarget->getDescription() != "")
			oStr << mTarget->getDescription() << "\n";
		else
			oStr << "There is nothing special about " << mTarget->getCrtStr(this, flags, 0) << ".\n";

		if(mTarget->getMobTrade()) {
			rank = mTarget->getSkillLevel()/10;
			oStr << "^y" << mTarget->getCrtStr(this, flags | CAP, 0) << " is a " << get_trade_string(mTarget->getMobTrade())
				 << ". " << mTarget->upHisHer() << " skill level: " << get_skill_string(rank) << ".^x\n";
		}
	} else if(pTarget) {

		oStr << "You see " << pTarget->fullName() << " the "
			 << gConfig->getRace(pTarget->getDisplayRace())->getAdjective();
		// will they see through the illusion?
		if(willIgnoreIllusion() && pTarget->getDisplayRace() != pTarget->getRace())
			oStr << " (" << gConfig->getRace(pTarget->getRace())->getAdjective() << ")";
		oStr << " "
			 << pTarget->getTitle() << ".\n";

		if(gConfig->getCalendar()->isBirthday(pTarget)) {
			oStr << "^yToday is " << pTarget->name << "'s birthday! " << pTarget->upHeShe()
				 << " is " << pTarget->getAge() << " years old.^x\n";
		}

		if(pTarget->description != "")
			oStr << pTarget->description << "\n";
	}

	if(target->isEffected("vampirism")) {
		chance = intelligence.getCur()/10 + piety.getCur()/10;
		// vampires and werewolves can sense each other
		if(isEffected("vampirism") || isEffected("lycanthropy"))
			chance = 101;
		if(chance > mrand(0,100)) {
			switch(mrand(1,4)) {
			case 1:
				oStr << target->upHeShe() << " looks awfully pale.\n";
				break;
			case 2:
				oStr << target->upHeShe() << " has an unearthly presence.\n";
				break;
			case 3:
				oStr << target->upHeShe() << " has hypnotic eyes.\n";
				break;
			default:
				oStr << target->upHeShe() << " looks rather pale.\n";
			}
		}
	}

	if(target->isEffected("lycanthropy")) {
		chance = intelligence.getCur()/10 + piety.getCur()/10;
		// vampires and werewolves can sense each other
		if(isEffected("vampirism") || isEffected("lycanthropy"))
			chance = 101;
		if(chance > mrand(0,100)) {
			switch(mrand(1,3)) {
			case 1:
				oStr << target->upHeShe() << " looks awfully shaggy.\n";
				break;
			case 2:
				oStr << target->upHeShe() << " has a feral presence.\n";
				break;
			default:
				oStr << target->upHeShe() << " looks rather shaggy.\n";
			}
		}
	}

	if(target->isEffected("slow"))
		oStr << target->upHeShe() << " is moving very slowly.\n";
	else if(target->isEffected("haste"))
		oStr << target->upHeShe() << " is moving unnaturally quick.\n";


	if((cClass == CLERIC && deity == JAKAR && level >=7) || isCt())
		oStr << "^y" << target->getCrtStr(this, flags | CAP, 0 ) << " is carrying "
			 << target->coins[GOLD] << " gold coin"
			 << (target->coins[GOLD] != 1 ? "s" : "") << ".^x\n";

	if(isEffected("know-aura") || cClass==PALADIN) {
		space = true;
		oStr << target->getCrtStr(this, flags | CAP, 0) << " ";

		align = target->getAdjustedAlignment();

		switch(align) {
		case BLOODRED:
			oStr << "has a blood red aura.";
			break;
		case REDDISH:
			oStr << "has a reddish aura.";
			break;
		case PINKISH:
			oStr << "has a pinkish aura.";
			break;
		case NEUTRAL:
			oStr << "has a grey aura.";
			break;
		case LIGHTBLUE:
			oStr << "has a light blue aura.";
			break;
		case BLUISH:
			oStr << "has a bluish aura.";
			break;
		case ROYALBLUE:
			oStr << "has a royal blue aura.";
			break;
		default:
			oStr << "has a grey aura.";
			break;
		}
	}

	if(mTarget && mTarget->getRace()) {
		if(space)
			oStr << " ";
		space = true;
		oStr << mTarget->upHeShe() << " is ^W"
			 << gConfig->getRace(mTarget->getRace())->getAdjective().toLower() << "^x.";
	}
	if(target->getSize() != NO_SIZE) {
		if(space)
			oStr << " ";
		space = true;
		oStr << target->upHeShe() << " is ^W" << getSizeName(target->getSize()) << "^x.";
	}

	if(space)
		oStr << "\n";

	if(target->hp.getCur() > 0 && target->hp.getMax())
		percent = (100 * (target->hp.getCur())) / (target->hp.getMax());
	else
		percent = -1;

	if(!(mTarget && mTarget->flagIsSet(M_UNKILLABLE))) {
		oStr << target->upHeShe();
		if(percent >= 100 || !target->hp.getMax())
			oStr << " is in excellent condition.\n";
		else if(percent >= 90)
			oStr << " has a few scratches.\n";
		else if(percent >= 75)
			oStr << " has some small wounds and bruises.\n";
		else if(percent >= 60)
			oStr << " is wincing in pain.\n";
		else if(percent >= 35)
			oStr << " has quite a few wounds.\n";
		else if(percent >= 20)
			oStr << " has some big nasty wounds and scratches.\n";
		else if(percent >= 10)
			oStr << " is bleeding awfully from big wounds.\n";
		else if(percent >= 5)
			oStr << " is barely clinging to life.\n";
		else if(percent >= 0)
			oStr << " is nearly dead.\n";
	}

	if(pTarget) {
		if(pTarget->flagIsSet(P_MISTED)) {
			oStr << pTarget->upHeShe() << "%s is currently in mist form.\n";
			printColor("%s", oStr.str().c_str());
			return(0);
		}

		if(pTarget->flagIsSet(P_UNCONSCIOUS))
			oStr << pTarget->name << " is "
				 << (pTarget->flagIsSet(P_SLEEPING) ? "sleeping" : "unconscious") << ".\n";

		if(pTarget->isEffected("petrification"))
			oStr << pTarget->name << " is petrified.\n";

		if(pTarget->isBraindead())
			oStr << pTarget->name << "%M is currently brain dead.\n";
	} else {
		if(mTarget->isEnemy(this))
			oStr << mTarget->upHeShe() << " looks very angry at you.\n";
		else if(mTarget->getPrimeFaction() != "")
			oStr << mTarget->upHeShe() << " " << getFactionMessage(mTarget->getPrimeFaction()) << ".\n";

	    Creature* firstEnm = NULL;
		if((firstEnm = mTarget->getTarget(false)) != NULL) {
			if(firstEnm == this) {
				if(  !mTarget->flagIsSet(M_HIDDEN) &&
					!(mTarget->isInvisible() && isEffected("detect-invisible")))
					oStr << mTarget->upHeShe() << " is attacking you.\n";
			} else
				oStr << mTarget->upHeShe() << " is attacking " << firstEnm->getName() << ".\n";

			/// print all the enemies if a CT or DM is looking
			if(isCt())
			    oStr << mTarget->threatTable;
		}
		oStr << consider(mTarget);

		// pet code
		if(mTarget->isPet() && mTarget->getMaster() == this) {
			str = listObjects(this, mTarget->first_obj, true);
			oStr << mTarget->upHeShe() << " ";
			if(str == "")
				oStr << "isn't holding anything.\n";
			else
				oStr << "is carrying: " << str << ".\n";
		}
	}
	printColor("%s", oStr.str().c_str());
	target->printEquipList(this);
	return(0);
}

//*********************************************************************
//						findCrt
//*********************************************************************

int findCrt(Creature * player, ctag *first_ct, int findFlags, char *str, int val, int* match, Creature ** target ) {
	ctag *cp;
	int found=0;

	if(!player || !str || !first_ct)
		return(0);

	cp = first_ct;
	while(cp) {
		if(!cp->crt) {
			cp = cp->next_tag;
			continue;
		}
		if(!player->canSee(cp->crt)) {
			cp = cp->next_tag;
			continue;
		}

		if(keyTxtEqual(cp->crt, str)) {
			(*match)++;
			if(*match == val) {
				*target = cp->crt;
				found = 1;
				break;
			}
		}

		cp = cp->next_tag;
	}
	return(found);
}

//*********************************************************************
//						checkSpellWearoff
//*********************************************************************

void Monster::checkSpellWearoff() {
	long t = time(0);

	/*
	if(flagIsSet(M_WILL_MOVE_FOR_CASH)) {
		if(t > LT(this, LT_MOB_PASV_GUARD)) {
			// Been 30 secs, time to stand guard again
			setFlag(M_PASSIVE_EXIT_GUARD);
		}
	}
	 */

	if(t > LT(this, LT_CHARMED) && flagIsSet(M_CHARMED)) {
		printColor("^y%M's demeanor returns to normal.\n", this);
		clearFlag(M_CHARMED);
	}
}


//*********************************************************************
//						canFlee
//*********************************************************************

bool Creature::canFlee() const {
	bool	crtInRoom=false;
	long	t=0;
	int		i=0;
	ctag*	cp=0;

	if(isMonster()) {

		if(!flagIsSet(M_WILL_FLEE) || flagIsSet(M_DM_FOLLOW))
			return(false);

		if(hp.getCur() > hp.getMax()/5)
			return(false);

		if(flagIsSet(M_PERMENANT_MONSTER))
			return(false);

	} else {
		//Player* pThis = getPlayer();
		if(!ableToDoCommand())
			return(false);

		if(flagIsSet(P_SITTING)) {
			print("You have to stand up first.\n");
			return(false);
		}

		if(isEffected("hold-person")) {
			print("You are unable to move right now.\n");
			return(false);
		}


		if(	(cClass == BERSERKER || cClass == CLERIC) &&
			flagIsSet(P_BERSERKED)
		) {
			printColor("^rYour lust for battle prevents you from fleeing!\n");
			return(false);
		}

		if(!isEffected("fear") && !isStaff()) {
//			// Check attack timer first
//			pThis->modifyAttackDelay(30);
//			bool result = pThis->checkAttackTimer();
//			pThis->modifyAttackDelay(-30); // Set the timer back to whwere it was before
//			if(result == false)
//				return(false);

			t = time(0);
			i = MAX(getLTAttack(), MAX(lasttime[LT_SPELL].ltime,lasttime[LT_READ_SCROLL].ltime)) + 3L;
			if(t < i) {
				pleaseWait(i-t);
				return(false);
			}
		}

		// confusion and drunkenness overrides following checks
		if(isEffected("confusion") || isEffected("drunkenness"))
			return(true);


		// players can only flee if someone/something else is in the room
		cp = getRoom()->first_mon;
		while(cp && !crtInRoom) {
			if(!cp->crt->isPet())
				crtInRoom = true;
			cp = cp->next_tag;
		}

		cp = getRoom()->first_ply;
		while(cp && !crtInRoom) {
			if(cp->crt == this) {
				cp = cp->next_tag;
				continue;
			}
			if(!cp->crt->isStaff())
				crtInRoom = true;
			cp = cp->next_tag;
		}

		if(	!crtInRoom &&
			!checkStaff("There's nothing to flee from!\n")
		)
			return(false);
	}

	return(true);
}


//*********************************************************************
//						canFleeToExit
//*********************************************************************

bool Creature::canFleeToExit(const Exit *exit, bool skipScary, bool blinking) {
	Player	*pThis = getPlayer();

	// flags both players and mobs have to obey
	if(!canEnter(exit, false, blinking) ||

		exit->flagIsSet(X_SECRET) ||
		exit->isConcealed(this) ||
		exit->flagIsSet(X_DESCRIPTION_ONLY) ||

		exit->flagIsSet(X_NO_FLEE) ||

		exit->flagIsSet(X_TO_STORAGE_ROOM) ||
		exit->flagIsSet(X_TO_BOUND_ROOM) ||
		exit->flagIsSet(X_TOLL_TO_PASS) ||
		exit->flagIsSet(X_WATCHER_UNLOCK)
	)
		return(false);


	if(pThis) {

		if(
			((	getRoom()->flagIsSet(R_NO_FLEE) ||
				getRoom()->flagIsSet(R_LIMBO)
			) && inCombat()) ||
			(exit->flagIsSet(X_DIFFICULT_CLIMB) && !isEffected("levitate")) ||
			(
				(
					exit->flagIsSet(X_NEEDS_CLIMBING_GEAR) ||
					exit->flagIsSet(X_CLIMBING_GEAR_TO_REPEL) ||
					exit->flagIsSet(X_DIFFICULT_CLIMB)
				) &&
				!isEffected("levitate")
			)
		) return(false);

		int chance=0, *scary;

		// should we skip checking for scary exits?
		skipScary = skipScary || fd == -1 || exit->target.mapmarker.getArea() || isEffected("fear");

		// are they scared of going in that direction
		if(!skipScary && pThis->scared_of) {
			scary = pThis->scared_of;
			while(*scary) {
				if(exit->target.room.id && *scary == exit->target.room.id) {
					// oldPrint(fd, "Scared of going %s^x!\n", exit->name);

					// there is a chance we will flee to this exit anyway
					chance = 65 + bonus((int) dexterity.getCur()) * 5;

					if(getRoom()->flagIsSet(R_DIFFICULT_TO_MOVE) || getRoom()->flagIsSet(R_DIFFICULT_FLEE))
						chance /=2;

					if(mrand(1,100) < chance)
						return(false);

					// success; means that the player is now scared of this room
					if(parent_rom && fd > -1) {
						scary = pThis->scared_of;
						{
							int roomNum = parent_rom->info.id;
							if(scary) {
								int size = 0;
								while(*scary) {
									size++;
									if(*scary == roomNum)
										break;
									scary++;
								}
								if(!*scary) {
									// LEAK: Next line reported to be leaky: 10 count
									pThis->scared_of = (int *) realloc(pThis->scared_of, (size + 2) * sizeof(int));
									(pThis->scared_of)[size] = roomNum;
									(pThis->scared_of)[size + 1] = 0;
								}
							} else {
								// LEAK: Next line reported to be leaky: 10 count
								pThis->scared_of = (int *) malloc(sizeof(int) * 2);
								(pThis->scared_of)[0] = roomNum;
								(pThis->scared_of)[1] = 0;
							}
						}
					}
					return(true);
				}
				scary++;
			}
		}

	}

	return(true);
}


//*********************************************************************
//						getFleeableExit
//*********************************************************************

Exit* Creature::getFleeableExit() {
	xtag*	xp = getRoom()->first_ext;
	int		i=0, exit=0;
	bool	skipScary=false;

	// count exits so we can randomly pick one
	while(xp) {
		if(canFleeToExit(xp->ext))
			i++;
		xp = xp->next_tag;
	}

	if(i) {
		// pick a random exit
		exit = mrand(1, i);
	} else if(isPlayer()) {
		// force players to skip the scary list
		xp = getRoom()->first_ext;
		skipScary = true;
		i=0;

		while(xp) {
			if(canFleeToExit(xp->ext, true))
				i++;
			xp = xp->next_tag;
		}
		if(i)
			exit = mrand(1, i);
	}

	if(!exit)
		return(0);

	i=0;
	xp = getRoom()->first_ext;
	while(xp) {
		if(canFleeToExit(xp->ext, skipScary))
			i++;
		if(i == exit)
			return(xp->ext);
		xp = xp->next_tag;
	}

	return(0);
}


//*********************************************************************
//						getFleeableRoom
//*********************************************************************

BaseRoom* Creature::getFleeableRoom(Exit* exit) {
	AreaRoom* aRoom=0;
	UniqueRoom*	uRoom=0;
	if(!exit)
		return(0);
	// exit flags have already been taken care of above, so
	// feign teleporting so we don't recycle the room
	Move::getRoom(this, exit, &uRoom, &aRoom, false, 0, false);
	if(uRoom)
		return(uRoom);
	else if(aRoom)
		return(aRoom);
	return(0);
}


//*********************************************************************
//						flee
//*********************************************************************
// This function allows a player to flee from an enemy. If successful
// the player will drop their readied weapon and run through one of the
// visible exits, losing 10% or 1000 experience, whichever is less.

int Creature::flee(bool magicTerror) {
	Monster* mThis = getMonster();
	Player* pThis = getPlayer();
	BaseRoom* oldRoom = getRoom(), *newRoom=0;
	UniqueRoom*	uRoom=0;
	MapMarker* mapmarker=0;
	Exit*	exit=0;
	unsigned int n=0;

	if(!magicTerror && !canFlee())
		return(0);

	if(isEffected("fear"))
		magicTerror = true;

	exit = getFleeableExit();
	if(!exit) {
		printColor("You failed to escape!\n");
		return(0);
	}

	newRoom = getFleeableRoom(exit);
	if(!newRoom) {
		printColor("You failed to escape!\n");
		return(0);
	}


	switch(mrand(1,10)) {
	case 1:
		print("You run like a chicken.\n");
		break;
	case 2:
		print("You flee in terror.\n");
		break;
	case 3:
		print("You run screaming in horror.\n");
		break;
	case 4:
		print("You flee aimlessly in any direction you can.\n");
		break;
	case 5:
		print("You run screaming for your mommy.\n");
		break;
	case 6:
		print("You run as your life flashes before your eyes.\n");
		break;
	case 7:
		print("Your heart throbs as you attempt to escape death.\n");
		break;
	case 8:
		print("Colors and sounds mingle as you frantically flee.\n");
		break;
	case 9:
		print("Fear of death grips you as you flee in panic.\n");
		break;
	case 10:
		print("You run like a coward.\n");
		break;
	default:
		print("You run like a chicken.\n");
		break;
	}


	broadcast(getSock(), oldRoom, "%M flees to the %s^x.", this, exit->name);
	if(mThis) {
		mThis->diePermCrt();
		if(exit->doEffectDamage(this))
			return(2);
		mThis->deleteFromRoom();
		mThis->addToRoom(newRoom);
	} else if(pThis) {
		pThis->dropWeapons();
		pThis->checkDarkness();
		unhide();

		if(magicTerror)
			printColor("^rYou flee from unnatural fear!\n");

		if(area_room)
			mapmarker = &(area_room->mapmarker);
		Move::track(parent_rom, mapmarker, exit, pThis, false);

		if(pThis->flagIsSet(P_ALIASING)) {
			pThis->getAlias()->deleteFromRoom();
			pThis->getAlias()->addToRoom(newRoom);
		}

		Move::update(pThis);
		pThis->statistics.flee();

		if(cClass == PALADIN && deity != GRADIUS && !magicTerror && level >= 10) {
			n = level * 8;
			n = MIN(experience, n);
			print("You lose %d experience for your cowardly retreat.\n", n);
			experience -= n;
		}

		if(exit->doEffectDamage(this))
			return(2);
		pThis->deleteFromRoom();
		pThis->addToRoom(newRoom);

		for(Monster* pet : pThis->pets) {
			if(pet && inSameRoom(pThis))
				broadcast(getSock(), oldRoom, "%M flees to the %s^x with its master.", mThis, exit->name);
		}
	}
	exit->checkReLock(this, false);

	broadcast(getSock(), newRoom, "%M just fled rapidly into the room.", this);

	if(pThis) {
		pThis->doPetFollow();
		uRoom = newRoom->getUniqueRoom();
		if(uRoom)
			pThis->checkTraps(uRoom);

		if(Move::usePortal(pThis, oldRoom, exit))
			Move::deletePortal(oldRoom, exit);
	}

	return(1);
}
