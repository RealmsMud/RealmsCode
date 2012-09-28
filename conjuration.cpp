/*
 * conjuration.cpp
 *	 Conjuration spells
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
#include "effects.h"

//*********************************************************************
//						petTalkDesc
//*********************************************************************

void petTalkDesc(Monster* pet, Creature* owner) {
	bstring name = owner->name, desc = "";

	if(owner->flagIsSet(P_DM_INVIS))
		name = "Someone";

	desc = "It's wearing a tag that says, '";
	desc += name;
	desc += "'s, hands off!'.";
	pet->setDescription(desc);

	desc = "I serve only ";
	desc += name;
	desc += "!";
	pet->setTalk(desc);
	pet->escapeText();
}

//*********************************************************************
//						getPetTitle
//*********************************************************************

int getPetTitle(int how, int skLevel, bool weaker, bool undead) {
	int title=0, num=0;
	if(how == CAST || how == SKILL || how == WAND) {
		title = (skLevel + 2) / 3;
		if(weaker) {
			num = skLevel / 2;
			if(num < 1)
				num = 1;
			title = (num + 2) / 3;
		} else {
			title = (skLevel + 2) / 3;
		}

		if(title > 10)
			title = 10;
	} else {
		title = (undead ? 3 : 1);
	}
	return(MAX(1, title));
}


//*********************************************************************
//						conjure
//*********************************************************************
int conjureCmd(Player* player, cmd* cmnd) {
	SpellData data;
	data.set(SKILL, CONJURATION, NO_DOMAIN, 0, player);
	if(!data.check(player))
		return(0);
	return(conjure(player, cmnd, &data));
}

int conjure(Creature* player, cmd* cmnd, SpellData* spellData) {
	Player* pPlayer = player->getAsPlayer();
	if(!pPlayer)
		return(0);

	Monster *target=0;
	int		title=0, mp=0, realm=0, level=0, spells=0, chance=0, sRealm=0;
	int		buff=0, hp_percent=0, mp_percent=0, a=0, rnum=0, cClass=0, skLevel=0;
	int		interval=0, len=0, n=0, x=0, hplow=0,hphigh=0, mplow=0, mphigh=0;
	time_t	t, i;
	const char *delem;

	// BUG: Name is not long enough for "schizophrenic serial killer clown"
	char	*s, *p, name[80];

	delem = " ";

	if(!player->ableToDoCommand())
		return(0);

	if(noPotion(player, spellData))
		return(0);

	if(spellData->how == SKILL && !player->knowsSkill("conjure")) {
		player->print("The conjuring of elementals escapes you.\n");
		return(0);
	}

	/*
	if(spellData->how == SKILL) {
		skLevel = (int)player->getSkillLevel("conjure");
	} else {
		skLevel = player->getLevel();
	}
	*/
	// TODO: conjure/animate is no longer a skill until progression
	// has been fixed
	skLevel = player->getLevel();

	if(	spellData->how == SKILL &&
		player->getClass() == CLERIC &&
		player->getDeity() == GRADIUS &&
		(	player->getAdjustedAlignment() == BLOODRED ||
			player->getAdjustedAlignment() == ROYALBLUE
		) )
	{
		player->print("Your alignment is out of harmony.\n");
		return(0);
	}

	if(player->getClass() == BUILDER) {
		player->print("You cannot conjure pets.\n");
		return(0);
	}

	if(player->hasPet() && !player->checkStaff("Only one conjuration at a time!\n"))
		return(0);

	if(spellData->object) {
		level = spellData->object->getQuality()/10;
	} else {
		level = skLevel;
	}

	title = getPetTitle(spellData->how, level, spellData->how == WAND && player->getClass() != DRUID && !(player->getClass() == CLERIC && player->getDeity() == GRADIUS), false);
	mp = 4 * title;

	if(spellData->how == CAST && !player->checkMp(mp))
		return(0);

	t = time(0);
	i = LT(player, LT_INVOKE);
	if(!player->isCt()) {
		if( (i > t) && (spellData->how != WAND) ) {
			player->pleaseWait(i-t);
			return(0);
		}
		if(spellData->how == CAST) {
			if(spell_fail(player, spellData->how)) {
				player->subMp(mp);
				return(0);
			}
		}
	}

	if(	player->getClass() == DRUID ||
		(spellData->how == SKILL && !(player->getClass() == CLERIC && player->getDeity() == GRADIUS)) ||
		player->isDm() )
	{

		if(	(cmnd->num < 3 && (spellData->how == CAST || spellData->how == WAND)) ||
			(cmnd->num < 2 && spellData->how == SKILL)
		) {
			player->print("Conjure what kind of elemental (earth, air, water, fire, electricity, cold)?\n");
			return(0);
		}
		s = cmnd->str[spellData->how == SKILL ? 1 : 2];
		len = strlen(s);

		if(!strncasecmp(s, "earth", len))
			realm = EARTH;
		else if(!strncasecmp(s, "air", len))
			realm = WIND;
		else if(!strncasecmp(s, "fire", len))
			realm = FIRE;
		else if(!strncasecmp(s, "water", len))
			realm = WATER;
		else if(!strncasecmp(s, "cold", len))
			realm = COLD;
		else if(!strncasecmp(s, "electricity", len))
			realm = ELEC;

		else if(player->isDm() && !strncasecmp(s, "mage", len))
			realm = CONJUREMAGE;
		else if(player->isDm() && !strncasecmp(s, "bard", len))
			realm = CONJUREBARD;
		else {
			player->print("Conjure what kind of elemental (earth, air, water, fire, electricity, cold)?\n");
			return(0);
		}
	} else {
		if(player->getClass() == CLERIC && player->getDeity() == GRADIUS)
			realm = EARTH;
		else if(player->getClass() == BARD)
			realm = CONJUREBARD;
		else
			realm = CONJUREMAGE;
	}

	// 0 = weak, 1 = normal, 2 = buff
	buff = mrand(1,3) - 1;

	target = new Monster;
	if(!target) {
		player->print("Cannot allocate memory for target.\n");
		merror("conjure", NONFATAL);
		return(PROMPT);
	}

	// Only level 30 titles
	int titleIdx = MIN(29, title);
	if(realm == CONJUREBARD) {
		strncpy(target->name, bardConjureTitles[buff][titleIdx-1],79);
		strncpy(name,  bardConjureTitles[buff][titleIdx-1], 79);
	} else if(realm == CONJUREMAGE) {
		strncpy(target->name, mageConjureTitles[buff][titleIdx-1], 79);
		strncpy(name,  mageConjureTitles[buff][titleIdx-1], 79);
	} else {
		strncpy(target->name, conjureTitles[realm-1][buff][titleIdx-1], 79);
		strncpy(name, conjureTitles[realm-1][buff][titleIdx-1], 79);
	}
	target->name[79] = '\0';
	name[79] = '\0';

	if(spellData->object) {
		level = spellData->object->getQuality()/10;
	} else {
		level = skLevel;
	}

	switch(buff) {
		case 0:
			level -= 3;
			break;
		case 1:
			level -= 2;
			break;
		case 2:
			level -= mrand(0,1);
			break;
	}

	level = MAX(1, MIN(MAXALVL, level));

	p = strtok(name, delem);
	if(p)
		strncpy(target->key[0], p, 19);
	p = strtok(NULL, delem);
	if(p)
		strncpy(target->key[1], p, 19);
	p = strtok(NULL, delem);
	if(p)
		strncpy(target->key[2], p, 19);

//	if(realm == CONJUREBARD) {
//		level = title * 12 / 7 + mrand(1,4) - 2;
//		level = MAX(1,MIN(skLevel-3, level));
//	} else if(realm == CONJUREMAGE) {
//		level = title * 6 / 4 + mrand(1,4) - 2;
//	} else {
//		level = title * 2 + mrand(1,4) - 2;
//	}

	target->setType(MONSTER);
	bool combatPet = false;

	if(realm == CONJUREBARD) {
		chance = mrand(1,100);
		if(chance > 50)
			cClass = mrand(1,13);

		if(cClass) {
			switch(cClass)	{
			case 1:
				target->setClass(ASSASSIN);
				break;
			case 2:
				target->setClass(BERSERKER);
				combatPet = true;
				break;
			case 3:
				target->setClass(CLERIC);
				break;
			case 4:
				target->setClass(FIGHTER);
				combatPet = true;
				break;
			case 5:
				target->setClass(MAGE);
				break;
			case 6:
				target->setClass(PALADIN);
				break;
			case 7:
				target->setClass(RANGER);
				break;
			case 8:
				target->setClass(THIEF);
				break;
			case 9:
				target->setClass(PUREBLOOD);
				break;
			case 10:
				target->setClass(MONK);
				break;
			case 11:
				target->setClass(DEATHKNIGHT);
				break;
			case 12:
				target->setClass(WEREWOLF);
				break;
			case 13:
				target->setClass(ROGUE);
				break;
			}
		}
		//if(target->getClass() == CLERIC && mrand(1,100) > 50)
		//	target->learnSpell(S_HEAL);

		if(target->getClass() == MAGE && mrand(1,100) > 50)
			target->learnSpell(S_RESIST_MAGIC);
	}


	target->setLevel(MIN(level, skLevel+1));
	level--;
	target->strength.setInitial(conjureStats[buff][level].str*10);
	target->dexterity.setInitial(conjureStats[buff][level].dex*10);
	target->constitution.setInitial(conjureStats[buff][level].con*10);
	target->intelligence.setInitial(conjureStats[buff][level].intel*10);
	target->piety.setInitial(conjureStats[buff][level].pie*10);

	target->strength.restore();
	target->dexterity.restore();
	target->constitution.restore();
	target->intelligence.restore();
	target->piety.restore();

	// This will be adjusted in 2.50, for now just str*2
	target->setAttackPower(target->strength.getCur()*2);

	// There are as many variations of elementals on the elemental
	// planes as there are people on the Prime Material. Therefore,
	// the elementals summoned have varying hp and mp stats. -TC
	if(player->getClass() == CLERIC && player->getDeity() == GRADIUS) {
		hp_percent = 8;
		mp_percent = 4;
	} else {
		hp_percent = 6;
		mp_percent = 8;
	}

	hphigh = conjureStats[buff][level].hp;
	hplow = (conjureStats[buff][level].hp * hp_percent) / 10;
	target->hp.setInitial(mrand(hplow, hphigh));
	target->hp.restore();

	if(!combatPet) {
		mphigh = conjureStats[buff][level].mp;
		mplow = (conjureStats[buff][level].mp * mp_percent) / 10;
		target->mp.setInitial(MAX(10,mrand(mplow, mphigh)));
		target->mp.restore();
	}

	//target->hp.getMax() = target->hp.getCur() = conjureStats[buff][level].hp;
	// target->mp.getMax() = target->mp.getCur() = conjureStats[buff][level].mp;
	target->setArmor(conjureStats[buff][level].armor);

	int skill = ((target->getLevel() - (combatPet ? 0 : 1)) * 10) + (buff * 5);

	target->setDefenseSkill(skill);
	target->setWeaponSkill(skill);

	target->setExperience(0);
	target->damage.setNumber(conjureStats[buff][level].ndice);
	target->damage.setSides(conjureStats[buff][level].sdice);
	target->damage.setPlus(conjureStats[buff][level].pdice);
	target->first_tlk = 0;
	target->setParent(NULL);

	for(n=0; n<20; n++)
		target->ready[n] = 0;

	if(!combatPet) {
		for(Realm r = MIN_REALM; r<MAX_REALM; r = (Realm)((int)r + 1))
			target->setRealm(mrand((conjureStats[buff][level].realms*3)/4, conjureStats[buff][level].realms), r);
	}


	target->lasttime[LT_TICK].ltime =
	target->lasttime[LT_TICK_SECONDARY].ltime =
	target->lasttime[LT_TICK_HARMFUL].ltime = time(0);

	target->lasttime[LT_TICK].interval  =
	target->lasttime[LT_TICK_SECONDARY].interval = 60;
	target->lasttime[LT_TICK_HARMFUL].interval = 30;

	target->getMobSave();

	if(!combatPet) {
		if(player->getDeity() == GRADIUS || realm == CONJUREBARD)
			target->setCastChance(mrand(5,10));	// cast precent
		else
			target->setCastChance(mrand(20,50));	// cast precent
		target->proficiency[1] = realm;
		target->setFlag(M_CAST_PRECENT);
		target->setFlag(M_CAN_CAST);

		target->learnSpell(S_VIGOR);
		if(target->getLevel() > 10 && player->getDeity() != GRADIUS)
			target->learnSpell(S_MEND_WOUNDS);
		if(target->getLevel() > 7 && player->getDeity() != GRADIUS && player->getClass() != DRUID)
			target->learnSpell(S_CURE_POISON);


		switch(buff) {
		case 0:
			// Wimpy mob -- get 1-2 spells
			spells = mrand(1, 2);
			break;
		case 1:
			// Medium mob -- get 1-4 spells
			spells = mrand(2, 4);
			break;
		case 2:
			// Buff mob -- get 2-5 spells
			spells = mrand(2,5);
			break;
		}

		// Give higher level pets higher chance for more spells
		if(target->getLevel() > 25)
			spells += 2;
		else if(target->getLevel() > 16)
			spells += 1;

		if(target->getLevel() < 3)
			spells = 1;
		else if(target->getLevel() < 7)
			spells = MIN(spells, 2);
		else if(target->getLevel() < 12)
			spells = MIN(spells, 3);
		else if(target->getLevel() < 16)
			spells = MIN(spells, 4);
		else
			spells = MIN(spells, 5);

	}

//	// Druid and gradius pets only
//	if(realm != CONJUREBARD && realm != CONJUREMAGE) {
//
//		// TODO: Add realm based attacks
//		// ie: earth - smother, water - ?? fire - ??
//	}
	//Gradius Earth Pets
	if(player->getDeity() == GRADIUS) {
		int enchantedOnlyChance = 0;
		int bashChance = 0;
		int circleChance = 0;

		if(target->getLevel() <= 12)
			enchantedOnlyChance = 50;
		else
			enchantedOnlyChance = 101;

		if(target->getLevel() >= 10)
			bashChance = 50;

		if(target->getLevel() >= 16)
			circleChance = 75;
		else if(target->getLevel() >= 13)
			circleChance = 50;

		if(combatPet) {
			bashChance += 25;
			circleChance += 25;
		}

		if(mrand(1,100) <= enchantedOnlyChance)
			target->setFlag(M_ENCHANTED_WEAPONS_ONLY);

		if(mrand(1, 100) <= bashChance)
			target->addSpecial("bash");

		if(mrand(1, 100) <= circleChance)
			target->addSpecial("circle");


		if(target->getLevel() >= 10) {

			int numResist = mrand(1,3);
			for(a=0;a<numResist;a++) {
				rnum = mrand(1,5);
				switch(rnum) {
				case 1:
					target->addEffect("resist-slashing");
					break;
				case 2:
					target->addEffect("resist-piercing");
					break;
				case 3:
					target->addEffect("resist-crushing");
					break;
				case 4:
					target->addEffect("resist-ranged");
					break;
				case 5:
					target->addEffect("resist-chopping");
					break;
				}
			}
		}

		if(target->getLevel() >= 13) {
			if(mrand(1,100) <= 50) {
				target->setFlag(M_PLUS_TWO);
				target->clearFlag(M_ENCHANTED_WEAPONS_ONLY);
			}
			target->setFlag(M_REGENERATES);
			if(mrand(1,100) <= 15)
				target->addSpecial("smother");
		}

		if(target->getLevel() >= 16) {
			target->setFlag(M_PLUS_TWO);
			if(mrand(1,100) <= 20) {
				target->setFlag(M_PLUS_THREE);
				target->clearFlag(M_PLUS_TWO);
				target->clearFlag(M_ENCHANTED_WEAPONS_ONLY);
			}
			target->setFlag(M_REGENERATES);
			if(mrand(1,100) <= 30)
				target->addSpecial("smother");

			if(mrand(1,100) <= 15)
				target->addSpecial("trample");

			target->addPermEffect("immune-earth");
		}

		if(target->getLevel() >= 19) {
			target->setFlag(M_PLUS_TWO);
			if(mrand(1,100) <= 40) {
				target->setFlag(M_PLUS_THREE);
				target->clearFlag(M_PLUS_TWO);
				target->clearFlag(M_ENCHANTED_WEAPONS_ONLY);
			}
			target->setFlag(M_REGENERATES);
			if(mrand(1,100) <= 40)
				target->addSpecial("smother");

			if(mrand(1,100) <= 25)
				target->addSpecial("trample");

			target->addPermEffect("immune-earth");
			target->setFlag(M_NO_LEVEL_ONE);
			target->setFlag(M_RESIST_STUN_SPELL);
		}

	}

	if(!combatPet) {
		sRealm = realm;
		for(x=0;x<spells;x++) {
			if(realm == CONJUREBARD || realm == CONJUREMAGE)
				sRealm = getRandomRealm();
			target->learnSpell(getOffensiveSpell((Realm)sRealm, x)); // TODO: fix bad cast
		}
	}

	player->print("You conjure a %s.\n", target->name);
	player->checkImprove("conjure", true);
	broadcast(player->getSock(), player->getParent(), "%M conjures a %s.", player, target->name);


	if(!combatPet) {
		chance = mrand(1,100);
		if(chance > 50)
			target->learnSpell(S_DETECT_INVISIBILITY);
		if(chance > 90)
			target->learnSpell(S_TRUE_SIGHT);
	}

	// add it to the room, make it active, and make it follow the summoner
	target->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
    target->lasttime[LT_MOB_THIEF].ltime = t;
	target->addToRoom(player->getRoomParent());
	gServer->addActive(target);

	player->addPet(target);
//	addFollower(player, target, FALSE);

	petTalkDesc(target, player);
	target->setFlag(M_PET);

	if(!combatPet) {
		if(realm < MAX_REALM)
			target->setBaseRealm((Realm)realm); // TODO: fix bad cast
	}

	// find out how long it's going to last and create all the timeouts
	x = player->piety.getCur();
	if(player->getClass() == DRUID && player->constitution.getCur() > player->piety.getCur())
		x = player->constitution.getCur();
	x = bonus((int) x)*60L;

	interval = (60L*mrand(2,4)) + (x >= 0 ? x :  0); //+ 60 * title;

	target->lasttime[LT_INVOKE].ltime = t;
	target->lasttime[LT_INVOKE].interval = interval;
	player->lasttime[LT_INVOKE].ltime = t;
	player->lasttime[LT_INVOKE].interval = 600L;
	// pets use LT_SPELL
	target->lasttime[LT_SPELL].ltime = t;
	target->lasttime[LT_SPELL].interval = 3;

	if(player->isCt())
		player->lasttime[LT_INVOKE].interval = 6L;

	if(spellData->how == CAST)
		player->mp.decrease(mp);

	if(spellData->how == SKILL)
		return(PROMPT);
	else
		return(1);
}

//*********************************************************************
//						splDenseFog
//*********************************************************************

int splDenseFog(Creature* player, cmd* cmnd, SpellData* spellData) {
	int strength = 10;
	long duration = 600;

	if(noPotion(player, spellData))
		return(0);

	if(!player->isCt()) {
		if(player->getRoomParent()->isUnderwater()) {
			player->print("Water currents prevent you from casting that spell.\n");
			return(0);
		}
	}

	player->print("You cast a dense fog spell.\n");
	broadcast(player->getSock(), player->getParent(), "%M casts a dense fog spell.", player);

	if(player->getRoomParent()->hasPermEffect("dense-fog")) {
		player->print("The spell didn't take hold.\n");
		return(0);
	}

	if(spellData->how == CAST) {
		if(player->getRoomParent()->magicBonus())
			player->print("The room's magical properties increase the power of your spell.\n");
	}

	player->getRoomParent()->addEffect("dense-fog", duration, strength, player, true, player);
	return(1);
}

//*********************************************************************
//						splToxicCloud
//*********************************************************************

int splToxicCloud(Creature* player, cmd* cmnd, SpellData* spellData) {
	int strength = spellData->level;
	long duration = 600;

	if(noPotion(player, spellData))
		return(0);

	if(!player->isCt()) {
		if(player->getRoomParent()->isPkSafe()) {
			player->print("That spell is not allowed here.\n");
			return(0);
		}
		if(player->getRoomParent()->isUnderwater()) {
			player->print("Water currents prevent you from casting that spell.\n");
			return(0);
		}
	}

	player->print("You cast a toxic cloud spell.\n");
	broadcast(player->getSock(), player->getParent(), "%M casts a toxic cloud spell.", player);

	if(player->getRoomParent()->hasPermEffect("toxic-cloud")) {
		player->print("The spell didn't take hold.\n");
		return(0);
	}

	if(spellData->how == CAST) {
		if(player->getRoomParent()->magicBonus())
			player->print("The room's magical properties increase the power of your spell.\n");
	}

	player->getRoomParent()->addEffect("toxic-cloud", duration, strength, player, true, player);
	return(1);
}

//*********************************************************************
//						splWallOfFire
//*********************************************************************

int splWallOfFire(Creature* player, cmd* cmnd, SpellData* spellData) {
	Exit *exit=0;
	int strength = spellData->level;
	long duration = 300;

	if(noPotion(player, spellData))
		return(0);

	if(!player->isCt()) {
		if(player->getRoomParent()->isPkSafe()) {
			player->print("That spell is not allowed here.\n");
			return(0);
		}
		if(player->getRoomParent()->isUnderwater()) {
			player->print("Water currents prevent you from casting that spell.\n");
			return(0);
		}
	}

	if(cmnd->num > 2)
		exit = findExit(player, cmnd, 2);
	if(!exit) {
		player->print("Cast a wall of fire on which exit?\n");
		return(0);
	}

	player->printColor("You cast a wall of fire spell on the %s^x.\n", exit->name);
	broadcast(player->getSock(), player->getParent(), "%M casts a wall of fire spell on the %s^x.",
		player, exit->name);

	if(exit->hasPermEffect("wall-of-fire")) {
		player->print("The spell didn't take hold.\n");
		return(0);
	}

	if(spellData->how == CAST) {
		if(player->getRoomParent()->magicBonus())
			player->print("The room's magical properties increase the power of your spell.\n");
	}

	exit->addEffectReturnExit("wall-of-fire", duration, strength, player);
	return(1);
}

//*********************************************************************
//						splWallOfForce
//*********************************************************************

int splWallOfForce(Creature* player, cmd* cmnd, SpellData* spellData) {
	Exit *exit=0;
	int strength = spellData->level;
	long duration = 300;

	if(noPotion(player, spellData))
		return(0);

	if(cmnd->num > 2)
		exit = findExit(player, cmnd, 2);
	if(!exit) {
		player->print("Cast a wall of force on which exit?\n");
		return(0);
	}

	player->printColor("You cast a wall of force spell on the %s^x.\n", exit->name);
	broadcast(player->getSock(), player->getParent(), "%M casts a wall of force spell on the %s^x.",
		player, exit->name);

	if(exit->hasPermEffect("wall-of-force")) {
		player->print("The spell didn't take hold.\n");
		return(0);
	}

	if(spellData->how == CAST) {
		if(player->getRoomParent()->magicBonus())
			player->print("The room's magical properties increase the power of your spell.\n");
	}

	exit->addEffectReturnExit("wall-of-force", duration, strength, player);
	return(1);
}

//*********************************************************************
//						splWallOfThorns
//*********************************************************************

int splWallOfThorns(Creature* player, cmd* cmnd, SpellData* spellData) {
	Exit *exit=0;
	int strength = spellData->level;
	long duration = 300;

	if(noPotion(player, spellData))
		return(0);

	if(player->getRoomParent()->isPkSafe() && !player->isCt()) {
		player->print("That spell is not allowed here.\n");
		return(0);
	}

	if(cmnd->num > 2)
		exit = findExit(player, cmnd, 2);
	if(!exit) {
		player->print("Cast a wall of thorns on which exit?\n");
		return(0);
	}

	player->printColor("You cast a wall of thorns spell on the %s^x.\n", exit->name);
	broadcast(player->getSock(), player->getParent(), "%M casts a wall of thorns spell on the %s^x.",
		player, exit->name);

	if(exit->hasPermEffect("wall-of-thorns")) {
		player->print("The spell didn't take hold.\n");
		return(0);
	}

	if(spellData->how == CAST) {
		if(player->getRoomParent()->magicBonus())
			player->print("The room's magical properties increase the power of your spell.\n");
	}

	exit->addEffectReturnExit("wall-of-thorns", duration, strength, player);
	return(1);
}

//*********************************************************************
//						bringDownTheWall
//*********************************************************************

void bringDownTheWall(EffectInfo* effect, BaseRoom* room, Exit* exit) {
	if(!effect)
		return;

	BaseRoom* targetRoom=0;
	bstring name = effect->getName();

	if(effect->isPermanent()) {
		// fake being removed
		Effect* ef = effect->getEffect();
		room->effectEcho(ef->getRoomAddStr(), exit);

		// extra of 2 means a 2 pulse (21-40 seconds) duration
		effect->setExtra(2);
		exit = exit->getReturnExit(room, &targetRoom);
		if(exit) {
			effect = exit->getEffect(name);
			if(effect) {
				targetRoom->effectEcho(ef->getRoomAddStr(), exit);
				effect->setExtra(2);
			}
		}
	} else {
		exit->removeEffectReturnExit(name, room);
	}
}
