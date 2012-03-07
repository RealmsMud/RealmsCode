/*
 * magic1.cpp
 *	 User routines dealing with magic spells. Potions, wands, scrolls, and casting are all covered.
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
#include "move.h"
#include "commands.h"
#include "craft.h"
#include "unique.h"
#include "effects.h"
#include "alchemy.h"

//*********************************************************************
//						spellShortcut
//*********************************************************************

void spellShortcut(char *spell) {
	if(!strcasecmp(spell, "m"))
		strcpy(spell, "mend");
	if(!strcasecmp(spell, "v"))
		strcpy(spell, "vigor");

	if(!strcasecmp(spell, "d-i"))
		strcpy(spell, "detect-invis");
	if(!strcasecmp(spell, "d-m"))
		strcpy(spell, "detect-magic");
}

//*********************************************************************
//						doMpCheck
//*********************************************************************

int doMpCheck(Creature* target, int splno) {
	// If required MP is set to -1, the spell function is reponsible for verifying
	// the caster has enough MP, otherwise it is checked here
	int reqMp = getSpellMp(splno);

	if(reqMp != -1) {
		if(!target->checkMp(reqMp))
			return(0);
		// Handle spell fail here now if the spell doesn't handle mp on it's own
		if(!(splno == S_VIGOR || splno == S_MEND_WOUNDS) && spell_fail(target, CAST)) {
			// Reduced spell fails to half mp
			target->subMp(MAX(1,reqMp/2));
			return(0);
		}
	}
	return(reqMp);
}

//*********************************************************************
//						cmdCast
//*********************************************************************
// wrapper for doCast because doCast returns more information than we want

int cmdCast(Creature* creature, cmd* cmnd) {
	doCast(creature, cmnd);
	return(0);
}


void doCastPython(MudObject* caster, Creature* target, bstring spell, int strength) {
	std::cout << caster->getName() << " is casting " << spell << " on " << target->getName() << std::endl;
}
//*********************************************************************
//						cmdCast
//*********************************************************************
// This function allows a creature to cast a magical spell. It looks at
// the second parsed word to find out if the spell-name is valid, and
// then calls the appropriate spell function.

CastResult doCast(Creature* creature, cmd* cmnd) {
	long	i=0, t=0;
	int		(*fn)(SpellFn);
	int		c=0, match=0, n=0, num=0, lvl=0, reqMp=0;
	Player* player = creature->getPlayerMaster();
	bool	offensive=false, self = (!player || player == creature);
	Creature* listen = (player ? player : creature);
	SpellData data;


	if(player) {
		player->clearFlag(P_AFK);
		player->interruptDelayedActions();
		if(!player->ableToDoCommand())
			return(CAST_RESULT_FAILURE);
	}

	if(creature->getClass() == BERSERKER) {
		listen->print("Cast? BAH! Magic is for the weak! Bash and cut instead!\n");
		return(CAST_RESULT_FAILURE);
	}

	if(player && player->getClass() == FIGHTER && !player->getSecondClass() && player->flagIsSet(P_PTESTER)) {
		listen->print("You lack training in the arcane arts.\n");
		return(CAST_RESULT_FAILURE);
	}

	if(cmnd->num < 2) {
		if(self)
			listen->print("Cast what?\n");
		else
			listen->print("Tell %N to cast what?\n", creature);
		return(CAST_RESULT_FAILURE);
	}

	// always let staff cast
	if(!player || !player->isStaff()) {
		if(creature->isEffected("confusion") || creature->isEffected("drunkenness")) {
			if(self)
				listen->print("Your mind is too clouded for that right now.\n");
			else
				listen->print("%M's mind is too clouded for that right now.\n", creature);
			return(CAST_RESULT_CURRENT_FAILURE);
		}
		if(player && player->isBlind()) {
			listen->printColor("^CYou can't see to direct the incantation!\n");
			return(CAST_RESULT_CURRENT_FAILURE);
		}
		if(creature->isBlind()) {
			listen->printColor("^C%M can't see to direct the incantation!\n", creature);
			return(CAST_RESULT_CURRENT_FAILURE);
		}
		if(player && !player->canSpeak()) {
			listen->printColor("^yYou can't speak the incantation!\n");
			return(CAST_RESULT_CURRENT_FAILURE);
		}
		if(!creature->canSpeak()) {
			listen->printColor("^y%M can't speak the incantation!\n", creature);
			return(CAST_RESULT_CURRENT_FAILURE);
		}
	}


	spellShortcut(cmnd->str[1]);


	do {
		if(!strcmp(cmnd->str[1], get_spell_name(c))) {
			match = 1;
			data.splno = c;
			break;
		} else if(!strncmp(cmnd->str[1], get_spell_name(c), strlen(cmnd->str[1]))) {
			match++;
			data.splno = c;
		}
		c++;
	} while(get_spell_num(c) != -1);

	if(match == 0) {
		listen->print("That spell does not exist.\n");
		return(CAST_RESULT_FAILURE);
	} else if(match > 1) {
		listen->print("Spell name is not unique.\n");
		return(CAST_RESULT_FAILURE);
	}

	if(!creature->spellIsKnown(get_spell_num(data.splno))) {
		if(self)
			listen->print("You don't know that spell.\n");
		else
			listen->print("%M doesn't know that spell.\n", creature);
		return(CAST_RESULT_FAILURE);
	}


	if(!creature->isStaff()) {
		switch(creature->getCastingType()) {
			case Divine:
				if(data.domain == DOMAIN_CANNOT_CAST) {
					listen->print("Divine spellcasters may not cast that spell.\n");
					return(CAST_RESULT_FAILURE);
				}
				break;
			case Arcane:
				if(data.school == SCHOOL_CANNOT_CAST) {
					listen->print("Arcane spellcasters may not cast that spell.\n");
					return(CAST_RESULT_FAILURE);
				}
				break;
			default:
				if(data.school == SCHOOL_CANNOT_CAST || data.domain == DOMAIN_CANNOT_CAST) {
					listen->print("Your class may not cast that spell.\n");
					return(CAST_RESULT_FAILURE);
				}
				break;
		}
	}
	

	if(	creature->getRoomParent()->flagIsSet(R_NO_MAGIC) &&
		!creature->checkStaff("Nothing happens.\n")
	)
		return(CAST_RESULT_FAILURE);


	if(	player == creature &&
		player->flagIsSet(P_SITTING) &&
		data.splno != S_VIGOR &&
		data.splno != S_MEND_WOUNDS &&
		data.splno != S_BLESS &&
		data.splno != S_PROTECTION &&
		data.splno != S_HEAL
	) {
		listen->print("You cannot cast that while sitting.\n");
		return(CAST_RESULT_FAILURE);
	}


	if(player == creature)
		i = MAX(LT(creature, LT_SPELL), creature->lasttime[LT_READ_SCROLL].interval + 2);
	else
		i = LT(creature, LT_SPELL);
	t = time(0);

	if(t < i && i != MAXINT) {
		listen->pleaseWait(i-t);
		return(CAST_RESULT_CURRENT_FAILURE);
	}


	num = get_spell_lvl(data.splno);

	if(num == 3)
		lvl = 5;
	else if(num == 4)
		lvl = 10;
	else if(num == 5)
		lvl = 16;
	else if(num == 6)
		lvl = 16;
	else
		lvl = 0;

	if(player == creature && !player->isCt() && num > 0) {
		if( (num == 3 && player->getLevel() < 5)  ||
			(num == 4 && player->getLevel() < 10) ||
			(num == 5 && player->getLevel() < 16) ||
			(num == 6 && player->getLevel() < 16)
		) {
			listen->print("You must be at least level %d to cast that spell.\n", lvl);
			return(CAST_RESULT_FAILURE);
		}
	}


	if(!player || !player->isStaff()) {
	    if(creature->getRoomParent()->checkAntiMagic()) {
            if(self)
                listen->printColor("^yYour spell fails.\n");
            else
                listen->printColor("^y%M's spell fails.\n", creature);
            return(CAST_RESULT_SPELL_FAILURE);
	    }
	}

	reqMp = doMpCheck(creature, data.splno);
	if(!reqMp)
		return(CAST_RESULT_CURRENT_FAILURE);
	fn = get_spell_function(data.splno);


	data.set(CAST, get_spell_school(data.splno), get_spell_domain(data.splno), 0, creature);
	if(!data.check(creature))
		return(CAST_RESULT_FAILURE);


	offensive = (int(*)(SpellFn, char*, osp_t*))fn == splOffensive ||
		(int(*)(SpellFn, char*, osp_t*))fn == splMultiOffensive;


	if(offensive) {
		for(c=0; ospell[c].splno != get_spell_num(data.splno); c++)
			if(ospell[c].splno == -1)
				return(CAST_RESULT_FAILURE);
		n = ((int(*)(SpellFn, const char*, osp_t*))*fn) (creature, cmnd, &data, get_spell_name(data.splno), &ospell[c]);
	} else {
		n = ((int(*)(SpellFn))*fn) (creature, cmnd, &data);
	}


	// Spell failed or didn't cast
	if(!n)
		return(CAST_RESULT_SPELL_FAILURE);

	// If reqMp is valid, subtract the mp here
	if(reqMp != -1)
		creature->subMp(reqMp);

	if(player) {
		player->statistics.cast();
		player->unhide();
	}
	if(player != creature)
		creature->unhide();
	creature->lasttime[LT_SPELL].ltime = t;


	if(player != creature) {
		creature->lasttime[LT_SPELL].interval = 4;
		creature->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
		return(CAST_RESULT_SUCCESS);
	}

	switch(player->getClass()) {
	case DUNGEONMASTER:
	case CARETAKER:
		player->lasttime[LT_SPELL].interval = 0;
		break;
	case MAGE:
		if(player->getSecondClass() == THIEF || player->getSecondClass() == ASSASSIN)
			player->lasttime[LT_SPELL].interval = 4;
		else
			player->lasttime[LT_SPELL].interval = 3;
		break;
	case BUILDER:
	case LICH:
		player->lasttime[LT_SPELL].interval = 3;
		break;
	case BARD:
	case VAMPIRE:
	case PALADIN:
	case DRUID:
		player->lasttime[LT_SPELL].interval = 4;
		break;
	case CLERIC:
		if(player->getSecondClass() == ASSASSIN)
			player->lasttime[LT_SPELL].interval = 5;
		else
			player->lasttime[LT_SPELL].interval = 4;
		break;
	case FIGHTER:
		if(player->getSecondClass() == MAGE) {
			bool heavy = false;
			for(int z = 0 ; z < MAXWEAR ; z++) {
				if(player->ready[z] && (player->ready[z]->isHeavyArmor() || player->ready[z]->isMediumArmor())) {
					player->printColor("^W%P^x hinders your movement!\n",player->ready[z]);
					heavy = true;
					break;
				}
			}
			if(heavy)
				player->lasttime[LT_SPELL].interval = 5;
			else
				player->lasttime[LT_SPELL].interval = 4;
		}
		else
			player->lasttime[LT_SPELL].interval = 5;
		break;
	case THIEF:
		if(player->getSecondClass() == MAGE)
			player->lasttime[LT_SPELL].interval = 4;
		else
			player->lasttime[LT_SPELL].interval = 5;
		break;
	default:
		player->lasttime[LT_SPELL].interval = 5;
		break;
	}

	// casting time settings for some specific spells override standard
	// casting delays for classes. -TC
	switch(data.splno) {
	case S_REJUVENATE:
		player->lasttime[LT_SPELL].interval = 20L;
		break;
	}

	// improve schools/domains of magic
	// Can't improve evocation/necromancy/destruction spells by casting on yourself
	MagicType castingType = player->getCastingType();
	if(data.skill != "" && (!self || (
			(castingType == Arcane && data.school != EVOCATION) ||
			(castingType == Arcane && data.school != NECROMANCY) ||
			(castingType == Divine && data.domain != DESTRUCTION)
		)
	))
		player->checkImprove(data.skill, true);
	return(CAST_RESULT_SUCCESS);
}

//*********************************************************************
//						cmdTeach
//*********************************************************************
// This function allowsspell-casters to teach other players basic spells

int cmdTeach(Player* player, cmd* cmnd) {
	Creature* target=0;
	int		 splno=0, c=0, match=0;
	bstring skill = "";

	if(!player->ableToDoCommand())
		return(0);

	if(cmnd->num < 3) {
		player->print("Teach whom what?\n");
		return(0);
	}
	if(player->isBlind()) {
		player->printColor("^CYou're blind!\n");
		return(0);
	}
	if(!player->canSpeak()) {
		player->printColor("^yYou can't speak to do that!\n");
		return(0);
	}

	if(!player->isStaff()) {
		if((player->getClass() != MAGE && player->getClass() != CLERIC)) {
			player->print("Only mages and clerics may teach spells.\n");
			return(0);
		}
		if(player->getSecondClass()) {
			player->print("Only true mages and clerics are able to teach.\n");
			return(0);
		}
	} else if(player->getClass() == BUILDER) {
		if(!player->canBuildMonsters())
			return(cmdNoAuth(player));
		if(!player->checkBuilder(player->getUniqueRoomParent())) {
			player->print("Error: room number not in any of your alotted ranges.\n");
			return(0);
		}
	}

	target = player->getParent()->findCreature(player, cmnd->str[1], cmnd->val[1], false);

	if(!target || target == player) {
		player->print("You don't see that person here.\n");
		return(0);
	}

	if(target->pFlagIsSet(P_LINKDEAD)) {
		player->print("%M doesn't want to be taught right now.\n", target);
		return(0);
	}


	spellShortcut(cmnd->str[2]);


	do {
		if(!strcmp(cmnd->str[2], get_spell_name(c))) {
			match = 1;
			splno = c;
			break;
		} else if(!strncmp(cmnd->str[2], get_spell_name(c), strlen(cmnd->str[2]))) {
			match++;
			splno = c;
		}
		c++;
	} while(get_spell_num(c) != -1);

	if(match == 0) {
		player->print("That spell does not exist.\n");
		return(0);
	}
	else if(match > 1) {
		player->print("Spell name is not unique.\n");
		return(0);
	}

	if(!player->spellIsKnown(get_spell_num(splno))) {
		player->print("You don't know that spell.\n");
		return(0);
	}


	if(!player->isStaff()) {
		if(player->getClass() == CLERIC && get_spell_num(splno) != 0 && get_spell_num(splno) != 3 && get_spell_num(splno) != 4) {
			player->print("You may not teach that spell to anyone.\n");
			return(0);
		}
		if(player->getClass() == CLERIC && (get_spell_num(splno) == 3) && target->getClass() != CLERIC && target->getClass() != PALADIN) {
			player->print("You may only teach that spell to clerics and paladins.\n");
			return(0);
		}
		if(	player->getClass() == MAGE &&
			get_spell_num(splno) != 2 &&
			get_spell_num(splno) != 5 &&
			get_spell_num(splno) != 27 &&
			get_spell_num(splno) != 28
		) {
			player->print("You may not teach that spell to anyone.\n");
			return(0);
		}
		if(target->isMonster()) {
			player->print("You may not teach spells to monsters.\n");
			return(0);
		}
	} else if(player->getClass() == BUILDER) {
		if(target->isPlayer()) {
			player->print("You may not teach spells to players.\n");
			return(0);
		}
	}

	if(	target->isPlayer() &&
		!player->isDm() &&
		get_spell_num(splno) != 2 &&
		get_spell_num(splno) != 5 &&
		get_spell_num(splno) != 27 &&
		get_spell_num(splno) != 28 &&
		get_spell_num(splno) != 0 &&
		get_spell_num(splno) != 3 &&
		get_spell_num(splno) != 4
	) {
		player->print("You may not teach those spells to anyone.\n");
		return(0);
	}

	player->unhide();

	if(target->spellIsKnown(get_spell_num(splno)) && target->isMonster() && player->isStaff()) {
		logn("log.teach", "%s caused %s to forget %s.\n", player->name, target->name, get_spell_name(splno));
		player->print("Spell \"%s\" removed from %N's memory.\n", get_spell_name(splno), target);
		target->forgetSpell(get_spell_num(splno));
	} else if(target->spellIsKnown(get_spell_num(splno)) && target->isPlayer() && player->isDm()) {
		logn("log.teach", "%s caused %s to forget %s.\n", player->name, target->name, get_spell_name(splno));
		player->print("Spell \"%s\" removed from %N's memory.\n", get_spell_name(splno), target);
		target->forgetSpell(get_spell_num(splno));
	} else {
		if(target->isPlayer()) {
			skill = spellSkill(get_spell_school(splno));
			if(skill != "") {
				if(!player->knowsSkill(skill)) {
					player->print("You are unable to teach %s spells.\n", gConfig->getSkillDisplayName(skill).c_str());
					return(0);
				}
				if(	!target->knowsSkill(skill) &&
					player->checkStaff("%M is unable to learn %s spells.\n", target, gConfig->getSkillDisplayName(skill).c_str())
				)
					return(0);
			}
		}

		target->learnSpell(get_spell_num(splno));
		target->print("%M teaches you the %s spell.\n", player,
		      get_spell_name(splno));
		player->print("Spell \"%s\" taught to %N.\n", get_spell_name(splno), target);
		if(!player->flagIsSet(P_DM_INVIS) && !player->flagIsSet(P_INCOGNITO)) {
			broadcast(player->getSock(), target->getSock(), player->getParent(),
				"%M taught %N the %s spell.", player, target, get_spell_name(splno));
		}
	}

	return(0);
}


//*********************************************************************
//						studyFindObject
//*********************************************************************
// This function finds the object the player is trying to study
// and determines if they can study it.

Object* studyFindObject(Player* player, const cmd* cmnd) {
	Object	*object=0;

	if(cmnd->num < 2) {
		player->print("Study what?\n");
		return(0);
	}

	if(!player->ableToDoCommand())
		return(0);

	if(player->isBlind()) {
		player->printColor("^CYou're blind!\n");
		return(0);
	}

	object = findObject(player, player->first_obj, cmnd);
	if(!object) {
		player->print("You don't have that.\n");
		return(0);
	}

	if(object->increase) {
		//
		// If this object can increase the player's skills or languages
		//
		// make sure we respect onlyOnce
		if(object->increase->onlyOnce) {
			std::list<CatRef>::const_iterator it;

			for(it = player->objIncrease.begin() ; it != player->objIncrease.end() ; it++) {
				if(	*it == object->info &&
					!player->checkStaff("You can gain no further understanding from %P.\n", object))
					return(0);
			}
		}

		// handle objects that increase skills
		if(object->increase->type == SkillIncrease) {
			Skill* crtSkill = player->getSkill(object->increase->increase);
			const SkillInfo *skill = gConfig->getSkill(object->increase->increase);

			if(!skill) {
				player->printColor("The skill set on this object is not a valid skill.\n");
				return(0);
			}
			if(!crtSkill && !object->increase->canAddIfNotKnown) {
				player->printColor("You lack the training to understand %P properly.\n", object);
				return(0);
			}

			// don't improve skills that don't need to improve
			if(crtSkill && skill->isKnownOnly()) {
				player->printColor("You can gain no further understanding from %P.\n", object);
				return(0);
			}

			// 10 skill points per level possible, can't go over that
			if(crtSkill && (crtSkill->getGained() + object->increase->amount) >= (player->getLevel()*10)) {
				player->printColor("You can currently gain no further understanding from %P.\n", object);
				return(0);
			}

		} else if(object->increase->type == LanguageIncrease) {
			int lang = atoi(object->increase->increase.c_str());

			if(lang < 1 || lang > LANGUAGE_COUNT) {
				player->printColor("The language set on this object is not a valid language.\n");
				return(0);
			}

			if(player->languageIsKnown(LUNKNOWN+lang-1)) {
				player->printColor("You already know how to speak %s!\n", get_language_adj(lang-1));
				return(0);
			}

		} else {
			player->printColor("You don't understand how to use %P properly.\n", object);
			return(0);
		}
		
	} else if(object->getRecipe()) {

		//
		// If this object can teach the player a recipe
		//
		Recipe* recipe = gConfig->getRecipe(object->getRecipe());

		if(!recipe) {
			player->print("Error: Bad Recipe.\n");
			broadcast(isCt, "^y%s has a bad recipe!^x\n", player->name);
			loge("Study: %s has a bad recipe.\n", player->name);
			return(0);
		}
		
		player->checkFreeSkills(recipe->getSkill());
		if(recipe->getSkill() != "" && !player->knowsSkill(recipe->getSkill())) {
			player->print("Sorry, you don't have enough training in %s to learn that recipe.\n", recipe->getSkill().c_str());
			return(0);
		}

	} else if(object->getType() == SCROLL) {

		//
		// If this object can teach the player a spell
		//

		if(object->getMagicpower() - 1 < 0 || object->getMagicpower() - 1 > MAXSPELL) {
			player->print("%O is unintelligible.\n", object);
			broadcast(isCt, "^y%s has a bad scroll: %s.\n", player->name, object->getName());
			loge("Study: %s has a bad scroll: %s.\n", player->name, object->getName());
			return(0);
		}


		bstring skill = "";
		if(player->getCastingType() == Divine)
			skill = spellSkill(get_spell_domain(object->getMagicpower() - 1));
		else
			skill = spellSkill(get_spell_school(object->getMagicpower() - 1));
		if(skill != "" && !player->knowsSkill(skill)) {
			player->print("You are unable to learn %s spells.\n", gConfig->getSkillDisplayName(skill).c_str());
			return(0);
		}
		
	} else if(object->getType() == SONGSCROLL) {

		//
		// If this object can teach the player a song
		//

	} else {

		player->print("You can't study that!\n");
		return(0);

	}

	if(object->doRestrict(player, true))
		return(0);

	return(object);
}

//*********************************************************************
//						doStudy
//*********************************************************************
// This function does the grunt work of studying the object.

void doStudy(Player* player, Object* object, bool immediate) {
	player->unhide();
	
	if(object->increase) {
		//
		// If this object can increase the player's skills or languages
		//

		// handle objects that increase skills
		if(object->increase->type == SkillIncrease) {

			Skill* crtSkill = player->getSkill(object->increase->increase);
			const SkillInfo *skill = gConfig->getSkill(object->increase->increase);


			// improve the skill
			bstring output = skill->getDisplayName();
			player->printColor("Your understanding of ^W%s^x has improved.\n", output.c_str());

			if(!crtSkill)
				player->addSkill(object->increase->increase, object->increase->amount);
			else
				crtSkill->improve(object->increase->amount);

		} else if(object->increase->type == LanguageIncrease) {
			
			int lang = atoi(object->increase->increase.c_str());

			player->printColor("You learn know how to speak ^W%s^x!\n", get_language_adj(lang-1));
			player->learnLanguage(lang-1);

		}

		// we only get here if it increases successfully
		if(object->increase->onlyOnce)
			player->objIncrease.push_back(object->info);

	} else if(object->getRecipe()) {

		//
		// If this object can teach the player a recipe
		//
		Recipe* recipe = gConfig->getRecipe(object->getRecipe());
		
		player->printColor("You learn the art of making ^W%s^x.\n", recipe->getResultName().c_str());
		player->learnRecipe(recipe);

	} else if(object->getType() == SCROLL) {

		//
		// If this object can teach the player a spell
		//

		player->printColor("You learn the ^W%s^x spell.\n", get_spell_name(object->getMagicpower() - 1));
		player->learnSpell(object->getMagicpower() - 1);

	} else if(object->getType() == SONGSCROLL) {

		//
		// If this object can teach the player a song
		//
		
		player->printColor("You learn the song of ^W%s^x.\n", get_song_name(object->getMagicpower() - 1));
		player->learnSong(object->getMagicpower() - 1);
		
	}

	player->printColor("%O disintegrates!\n", object);
	if(immediate)
		broadcast(player->getSock(), player->getParent(), "%M studies %1P.", player, object);
	else
		broadcast(player->getSock(), player->getParent(), "%M finishes studiying %1P.", player, object);

	player->delObj(object, true);
	delete object;
}

// this function is called when we are ready to finish studying the object
void doStudy(const DelayedAction* action) {
	Player* player = action->target->getAsPlayer();
	Object* object = studyFindObject(player, &action->cmnd);

	// nothing to study?
	if(!object)
		return;

	player->printColor("You finish studying %P.\n", object);
	doStudy(player, object, false);
}

//*********************************************************************
//						cmdStudy
//*********************************************************************
// This function allows a player to study a scroll, and learn the
// spell / recipe / skill / language / song that is on it.

int cmdStudy(Player* player, cmd* cmnd) {
	Object* object = studyFindObject(player, cmnd);

	// nothing to study?
	if(!object)
		return(0);

	// these types of objects take a while to study
	if(object->increase) {
		int delay;

		switch(object->increase->type) {
			case SkillIncrease:
				delay = 15;
				break;
			case LanguageIncrease:
				delay = 60;
				break;
			default:
				delay = 60;
				break;
		}

		// doStudy calls unhide, no need to do it twice
		player->unhide();

		player->printColor("You begin studying %P.\n", object);
		broadcast(player->getSock(), player->getParent(), "%M begins studying %1P.", player, object);
		gServer->addDelayedAction(doStudy, player, cmnd, ActionStudy, delay);
	} else {
		doStudy(player, object, true);
	}

	return(0);
}

//*********************************************************************
//						cmdReadScroll
//*********************************************************************
// This function allows a player to read a scroll and cause its magical
// spell to be cast. If a third word is used in the command, then that
// player or monster will be the target of the spell.

int cmdReadScroll(Player* player, cmd* cmnd) {
	Object	*object=0;
	int		(*fn)(SpellFn);
	long	i=0, t=0;
	int		n=0, match=0, c=0;
	bool	dimensionalFailure=false;
	SpellData data;

	fn = 0;
	if(!player->ableToDoCommand())
		return(0);

	if(cmnd->num < 2) {
		player->print("Read what?\n");
		return(0);
	}
	if(player->isBlind()) {
		player->printColor("^CYou're blind!\n");
		return(0);
	}
	object = findObject(player, player->first_obj, cmnd);

	if(!object || !cmnd->val[1]) {
		for(n = 0; n < MAXWEAR; n++) {
			if(!player->ready[n])
				continue;
			if(keyTxtEqual(player->ready[n], cmnd->str[1]))
				match++;
			else
				continue;
			if(match == cmnd->val[1] || !cmnd->val[1]) {
				object = player->ready[n];
				break;
			}
		}
	}

	if(!object) {
		player->print("You don't have that.\n");
		return(0);
	}

	if(object->getSpecial()) {
		n = object->doSpecial(player);
		if(n != -2)
			return(MAX(0, n));
	}

	if(object->getType() != SCROLL) {
		player->print("That's not a scroll.\n");
		return(0);
	}


	if(object->doRestrict(player, true))
		return(0);


	if( (player->getRoomParent()->flagIsSet(R_NO_MAGIC)) ||
		(object->getMagicpower() - 1 < 0)
	) {
		player->print("Nothing happens.\n");
		return(0);
	}

	i = MAX(LT(player, LT_READ_SCROLL), player->lasttime[LT_SPELL].ltime + 2);
	t = time(0);

	if(i > t) {
		player->pleaseWait(i - t);
		return(0);
	}

	if(player->getRoomParent()->checkAntiMagic()) {
	    player->print("Nothing happens.\n");
	    return(0);
	}

	player->unhide();

	player->lasttime[LT_READ_SCROLL].ltime = t;
	player->lasttime[LT_READ_SCROLL].interval = 3;

	if(LT(player, LT_PLAYER_BITE) <= t) {
		player->lasttime[LT_PLAYER_BITE].ltime = t;
		player->lasttime[LT_PLAYER_BITE].interval = 3L;
	}

	if(object->getMagicpower() - 1 < 0 || object->getMagicpower() - 1 > MAXSPELL) {
		player->print("Error: Bad Scroll.\n");
		broadcast(isCt, "^y%s has a bad scroll!\n", player->name);
		loge("Readscroll: %s has a bad scroll.\n", player->name);
		return(0);
	}

	if(spell_fail(player, SCROLL)) {
		player->printColor("%O disintegrates.\n", object);
		player->delObj(object, true);
		delete object;
		return(0);
	}

	data.splno = object->getMagicpower() - 1;
	fn = get_spell_function(data.splno);

	data.set(SCROLL, get_spell_school(data.splno), get_spell_domain(data.splno), object, player);
	if(!data.check(player))
		return(0);

	// check for dimensional anchor
	if(hinderedByDimensionalAnchor(data.splno) && player->checkDimensionalAnchor())
		dimensionalFailure = true;

	// if the spell failed due to dimensional anchor, don't even run this
	if(!dimensionalFailure) {

		if((int(*)(SpellFn, char*, osp_t*))fn == splOffensive ||
			(int(*)(SpellFn, char*, osp_t*))fn == splMultiOffensive) {
			for(c = 0; ospell[c].splno != get_spell_num(data.splno); c++)
				if(ospell[c].splno == -1)
					return(0);
			n = ((int(*)(SpellFn, const char*, osp_t*))*fn) (player, cmnd, &data, get_spell_name(data.splno), &ospell[c]);
		} else {
			n = ((int(*)(SpellFn))*fn) (player, cmnd, &data);
		}
	}

	if(n || dimensionalFailure) {
		if(object->use_output[0] && !dimensionalFailure)
			player->printColor("%s\n", object->use_output);

		player->printColor("%O disintegrates.\n", object);
		player->delObj(object, true);
		delete object;
	}

	return(0);

}

//*********************************************************************
//						consume
//*********************************************************************
// This function allows players to drink potions / eat food, thereby casting any
// spell it was meant to contain.

// return 0 on failure, 1 on drink, 2 on deleted object
// lagprot means we can bypass

int endConsume(Object* object, Player* player, bool forceDelete=false) {
	if(object->flagIsSet(O_EATABLE)) {
		player->print("Food eaten.\n");
		broadcast(player->getSock(), player->getParent(), "%M eats %1P.", player, object);
	} else {
		player->print("Potion drank.\n");
		broadcast(player->getSock(), player->getParent(), "%M drinks %1P.", player, object);
	}

	if(!forceDelete)
		object->decShotsCur();
	if(forceDelete || object->getShotsCur() < 1) {
		if(object->flagIsSet(O_EATABLE))
			player->printColor("You ate all of %P.\n", object);
		else
			player->printColor("You drank all of %P.\n", object);

		player->delObj(object, true);
		delete object;
		return(2);
	}
	return(1);
}

int consume(Player* player, Object* object, cmd* cmnd) {
	int		c=0, splno=0, n=0;
	int		(*fn)(SpellFn);
	bool	dimensionalFailure=false;
	bool	eat = object->flagIsSet(O_EATABLE), drink=object->flagIsSet(O_DRINKABLE) || object->getType() == POTION;
	//const bstring& effect = object->getEffect();
	fn = 0;

	if(player->isStaff() && object->getType() != POTION && !strcmp(cmnd->str[0], "eat")) {
		broadcast(player->getSock(), player->getParent(), "%M eats %1P.", player, object);
		player->printColor("You ate %P.\n", object);

		player->delObj(object);
		delete object;
		return(0);
	}

	if(!strcmp(cmnd->str[0], "eat")) {
		if(!eat && !player->checkStaff("You may not eat that.\n"))
			return(0);
	} else {
		if(!drink && !player->checkStaff("You may not drink that.\n"))
			return(0);
	}


	if(object->doRestrict(player, true))
		return(0);

	// Handle Alchemy Potions
	if(object->isAlchemyPotion()) {
		if(object->consumeAlchemyPotion(player))
			return(endConsume(object,player));
		else
			return(0);
	}

	// they are eating a non-potion object
	if(	object->getShotsCur() < 1 ||
		(object->getMagicpower() - 1 < 0) ||
		object->getType() != POTION
	) {
		player->unhide();

		if(object->use_output[0])
			player->printColor("%s\n", object->use_output);

		// some food can heal you
		if(object->flagIsSet(O_CONSUME_HEAL) && !player->isUndead())
			player->hp.increase(object->damage.roll());

		return(endConsume(object, player, true));
	}

	if(	player->getRoomParent()->flagIsSet(R_NO_POTION) ||
		player->getRoomParent()->flagIsSet(R_LIMBO)
	) {
		if(!player->checkStaff("%O starts to %s before you %s it.\n",
			object, eat ? "get moldy" : "evaporate", eat ? "eat" : "drink"))
			return(0);
	}

	player->unhide();

	if(object->getMagicpower() - 1 < 0 || object->getMagicpower() - 1 > MAXSPELL) {
		player->print("Error: Bad Potion.\n");
		broadcast(isCt, "^y%s has a bad potion!\n", player->name);
		loge("Quaff: %s has a bad potion.\n", player->name);
		return(0);
	}
	splno = object->getMagicpower() - 1;
	fn = get_spell_function(splno);

	// check for dimensional anchor
	if(hinderedByDimensionalAnchor(splno) && player->checkDimensionalAnchor())
		dimensionalFailure = true;

	// if the spell failed due to dimensional anchor, don't even run this
	if(!dimensionalFailure) {
		SpellData data;
		data.splno = splno;
		data.set(POTION, get_spell_school(data.splno), get_spell_domain(data.splno), object, player);

		if(	(int(*)(SpellFn, char*, osp_t*))fn == splOffensive ||
			(int(*)(SpellFn, char*, osp_t*))fn == splMultiOffensive
		) {
			for(c = 0; ospell[c].splno != get_spell_num(data.splno); c++)
				if(ospell[c].splno == -1)
					return(0);
			n = ((int(*)(SpellFn, const char*, osp_t*))*fn)
			(player, cmnd, &data, get_spell_name(data.splno), &ospell[c]);
		} else {
			n = ((int(*)(SpellFn))*fn) (player, cmnd, &data);
		}
	}

	if(n || dimensionalFailure) {
		player->statistics.potion();
		if(object->use_output[0] && !dimensionalFailure)
			player->printColor("%s\n", object->use_output);
		return(endConsume(object, player));
	}
	return(0);
}

// drink wrapper

int cmdConsume(Player* player, cmd* cmnd) {
	Object  *object=0;
	int	     n=0, match=0;

	if(!player->ableToDoCommand())
		return(0);

	if(cmnd->num < 2) {
	    if(strcmp(cmnd->str[0], "eat")) {
		    player->print("Drink what?\n");
	    } else {
		    player->print("Eat what?\n");
	    }
	    return(0);
	}

	object = findObject(player, player->first_obj, cmnd);

	if(!object || !cmnd->val[1]) {
		for(n = 0; n < MAXWEAR; n++) {
			if(!player->ready[n])
				continue;
			if(keyTxtEqual(player->ready[n], cmnd->str[1]))
				match++;
			else
				continue;
			if(match == cmnd->val[1] || !cmnd->val[1]) {
				object = player->ready[n];
				break;
			}
		}
	}

	if(!object) {
		player->print("You don't have that.\n");
		return(0);
	}

	consume(player, object, cmnd);
	return(0);
}

//*********************************************************************
//						cmdUseWand
//*********************************************************************
// This function allows players to zap a wand or staff at another player
// or monster.

int cmdUseWand(Player* player, cmd* cmnd) {
	Object	*object=0;
	long	i=0, t=0;
	int		(*fn)(SpellFn);
	bool	dimensionalFailure=false;
	int		match=0, n=0, c=0;
	SpellData data;

	if(!player->ableToDoCommand())
		return(0);

	if(cmnd->num < 2) {
		player->print("Use what?\n");
		return(0);
	}

	if(player->isBlind()) {
		player->printColor("^CYou're blind!\n");
		return(0);
	}

	object = findObject(player, player->first_obj, cmnd);

	if(!object || !cmnd->val[1]) {
		for(n = 0; n < MAXWEAR; n++) {
			if(!player->ready[n])
				continue;
			if(keyTxtEqual(player->ready[n], cmnd->str[1]))
				match++;
			else
				continue;
			if(match == cmnd->val[1] || !cmnd->val[1]) {
				object = player->ready[n];
				break;
			}
		}
	}

	if(!object) {
		object = findObject(player, player->getRoomParent()->first_obj, cmnd);
		if(object && !object->flagIsSet(O_CAN_USE_FROM_FLOOR)) {
			player->print("You don't have that.\n");
			return(0);
		}
	}

	if(!object) {
		player->print("You don't have that.\n");
		return(0);
	}

	if(object->getType() != WAND) {
		player->print("That's not a wand or staff.\n");
		return(0);
	}

	if(object->doRestrict(player, true))
		return(0);

	if(object->getShotsCur() < 1) {
		player->print("It's used up.\n");
		return(0);
	}

	if(	(player->getRoomParent()->flagIsSet(R_NO_MAGIC)) ||
		(object->getMagicpower() < 1)
	) {
		player->print("Nothing happens.\n");
		return(0);
	}

	i = MAX(LT(player, LT_SPELL), player->lasttime[LT_READ_SCROLL].interval + 2);
	t = time(0);

	if(!player->isCt() && i > t) {
		player->pleaseWait(i - t);
		return(0);
	}

	player->unhide();

	if(player->getRoomParent()->checkAntiMagic()) {
        player->printColor("%O sputters and smokes.\n", object);
        return(0);
	}

	data.splno = object->getMagicpower() - 1;
	if(data.splno < 0 || data.splno > MAXSPELL) {
		player->print("Error: Bad Wand.\n");
		broadcast(isCt, "^y%s has a bad wand!\n", player->name);
		loge("Study: %s has a bad wand.\n", player->name);
		return(0);
	}

	data.set(WAND, get_spell_school(data.splno), get_spell_domain(data.splno), object, player);
	if(!data.check(player, true))
		return(0);

	player->lasttime[LT_SPELL].ltime = t;
	player->lasttime[LT_SPELL].interval = 3;
	player->statistics.wand();

	if(spell_fail(player, WAND)) {
		if(!object->flagIsSet(O_CAN_USE_FROM_FLOOR))
			object->decShotsCur();
		if(object->getShotsCur() < 1 && Unique::isUnique(object)) {
			player->delObj(object, true);
			delete object;
		}
		return(0);
	}

	fn = get_spell_function(data.splno);
	n = 0;

	// check for dimensional anchor
	if(hinderedByDimensionalAnchor(data.splno) && player->checkDimensionalAnchor())
		dimensionalFailure = true;

	// if the spell failed due to dimensional anchor, don't even run this
	if(!dimensionalFailure) {

		if(	(int(*)(SpellFn, char*, osp_t*))fn == splOffensive ||
			(int(*)(SpellFn, char*, osp_t*))fn == splMultiOffensive
		) {
			for(c = 0; ospell[c].splno != get_spell_num(data.splno); c++)
				if(ospell[c].splno == -1)
					return(0);
			n = ((int(*)(SpellFn, const char*, osp_t*))*fn) (player, cmnd, &data, get_spell_name(data.splno), &ospell[c]);
		} else { // if(!object->flagIsSet(ODDICE)) // Flag isn't used best i can tell
			n = ((int(*)(SpellFn))*fn) (player, cmnd, &data);
		}
//		else
//			n = (*fn) (player, cmnd, WAND, object);
	}

	if(n || dimensionalFailure) {
		if(object->use_output[0] && !dimensionalFailure)
			player->printColor("%s\n", object->use_output);

		if(!object->flagIsSet(O_CAN_USE_FROM_FLOOR))
			object->decShotsCur();

		if(object->getShotsCur() < 1 && Unique::isUnique(object)) {
			player->delObj(object, true);
			delete object;
		}

	}

	return(0);
}

//*********************************************************************
//						cmdRecall
//*********************************************************************
// wrapper for the useRecallPotion function

int cmdRecall(Player* player, cmd* cmnd) {
	if((player->getLevel() <= 7 && !player->inCombat()) || player->isStaff())
		player->doRecall();
    else
		useRecallPotion(player, 1, 0);

	return(0);
}

//*********************************************************************
//						recallLog
//*********************************************************************

// does the logging for a lag-protect hazy

void recallLog(Player* player, bstring name, bstring cname, bstring room) {
	std::ostringstream log;

	log << "### " << player->name << "(L" << player->getLevel() << ") hazied (" << name << ") ";
	if(cname != "")
		log << "(out of bag: " << cname << ") ";
	log << "due to lag protection. HP: " << player->hp.getCur() << "/" << player->hp.getMax()
		<< ". Room: " << room << " to " << player->getRoomParent()->fullName() << ".";

	broadcast(isWatcher, "^C%s", log.str().c_str());
	logn("log.lprotect", "%s", log.str().c_str());
}

//*********************************************************************
//						recallCheckBag
//*********************************************************************
// This handles searching through a bag for hazy, since using a hazy
// inside a bag means we need to reduce its shots. Plus, del_obj_crt
// used from consume doesnt handle deleting it properly

int recallCheckBag(Player* player, Object *cont, cmd* cmnd, int show, int log) {
	Object	*object=NULL;
	otag	*cop=0, *prev=0;
	int		drank=0, first=1;
	bstring room = player->getRoomParent()->fullName(), name = "";

	prev = cont->first_obj;
	cop = cont->first_obj;
	while(cop) {
		object = cop->obj;
		cop = cop->next_tag;
		name = object->name;

		if(object->getMagicpower() == (S_WORD_OF_RECALL+1) && object->getType() == POTION) {
			if(show)
				player->printColor("Recall potion found in %s: %s. Initiating auto-recall.\n", cont->name, name.c_str());
			drank = consume(player, object, cmnd);
			if(drank) {
				if(log)
					recallLog(player, name, cont->name, room);
				return(1);
			}
			if(show)
				player->print("Unable to use potion. Continuing search.\n");
		}
		// don't move prev if we're at the first object
		if(!first)
			prev = prev->next_tag;
		first=0;
	}
	return(0);
}

//*********************************************************************
//						useRecallPotion
//*********************************************************************
// used for recall command and lagprotect

int useRecallPotion(Player* player, int show, int log) {
	cmd 	*cmnd;
	Object	*object=NULL;
	otag	*op=0;
	int		i=0;
	bstring room = player->getRoomParent()->fullName(), name = "";

	if(player->flagIsSet(P_ANCHOR)) {
		player->print("%s will not work while you are protected by a dimensional anchor.\n",
			log ? "Automatic recall" : "The recall command");
		return(0);
	}

	// the functions we call want a command...
	// so we'll give them a fake one
	cmnd = new cmd;
	cmnd->num = 2;

	player->print("Beginning recall sequence.\n");

	// do they have anything on their person?
	for(i=0; i<MAXWEAR; i++) {
		object = player->ready[i];
		if(object) {
			name = object->name;
			if(object->getMagicpower() == (S_WORD_OF_RECALL+1) && object->getType() == POTION) {
				if(show)
					player->printColor("Recall potion found: %s. Initiating auto-recall.\n", name.c_str());
				if(consume(player, object, cmnd)) {
					if(log)
						recallLog(player, name, "", room);
					return(1);
				}
				if(show)
					player->print("Unable to use potion. Continuing search.\n");
			}

			if(object->getType() == CONTAINER)
				if(recallCheckBag(player, object, cmnd, show, log))
					return(1);
		}
	}

	// check through their inventory
	op = player->first_obj;
	while(op) {
		object = op->obj;
		op = op->next_tag;
		name = object->name;

		// is this object it?
		if(object->getMagicpower() == (S_WORD_OF_RECALL+1) && object->getType() == POTION) {
			if(show)
				player->printColor("Recall potion found: %s. Initiating auto-recall.\n", name.c_str());
			if(consume(player, object, cmnd)) {
				if(log)
					recallLog(player, name, "", room);
				return(1);
			}
			if(show)
				player->print("Unable to use potion. Continuing search.\n");
		}

		// in a container?
		if(object->getType() == CONTAINER) {
			if(recallCheckBag(player, object, cmnd, show, log))
				return(1);
		}
	}

	player->print("No recall potions found!\n");
	return(0);
}

//*********************************************************************
//						logCast
//*********************************************************************

void logCast(Creature* caster, Creature* target, bstring spell, bool dmToo) {
	Player* pCaster = caster->getAsPlayer();
	if(!pCaster || !pCaster->isStaff() || (!dmToo && pCaster->isDm()))
		return;
	log_immort(true, pCaster, "%s cast an %s spell on %s in room %s.\n",
		caster->name, spell.c_str(), target ? target->name : "self",
		caster->getRoomParent()->fullName().c_str());
}

//*********************************************************************
//						noCastUndead
//*********************************************************************

bool noCastUndead(const bstring& effect) {
	return(effect == "regeneration");
}

//*********************************************************************
//						splGeneric
//*********************************************************************

int splGeneric(Creature* player, cmd* cmnd, SpellData* spellData, const char* article, const char* spell, bstring effect, int strength, long duration) {
	Creature* target=0;

	if(cmnd->num == 2) {
		target = player;

		if(	noCastUndead(effect) &&
			target->isUndead() &&
			!player->checkStaff("You cannot cast that spell on yourself.\n")
		)
			return(0);

		if(spellData->how == CAST) {
			player->print("You cast %s %s spell.\n", article, spell);
			broadcast(player->getSock(), player->getParent(), "%M casts %s %s spell.", player, article, spell);
		}
	} else {
		if(noPotion(player, spellData))
			return(0);

		cmnd->str[2][0] = up(cmnd->str[2][0]);
		target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);

		if(!target) {
			player->print("You don't see that player here.\n");
			return(0);
		}

		if(	noCastUndead(effect) &&
			target->isUndead() &&
			!player->checkStaff("You cannot cast that spell on the undead.\n")
		)
			return(0);

		if(checkRefusingMagic(player, target))
			return(0);

		if((effect == "drain-shield" || effect == "undead-ward") && target->isUndead()) {
			player->print("The spell fizzles.\n%M naturally resisted your spell.\n", target);
			return(0);
		}

		broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts %s %s spell on %N.",
			player, article, spell, target);
		target->print("%M casts %s on you.\n", player, spell);
		player->print("You cast %s %s spell on %N.\n", article, spell, target);
	}

	if(target->inCombat(false))
		player->smashInvis();

	if(	target->hasPermEffect(effect) ||
		(effect == "heat-protection" && target->hasPermEffect("alwayswarm")) ||
		(effect == "warmth" && target->hasPermEffect("alwayscold"))
	) {
		player->print("The spell didn't take hold.\n");
		return(0);
	}

	// these spells will counteract porphyria
	if((effect == "drain-shield" || effect == "undead-ward") && target->isEffected("porphyria"))
		target->removeEffect("porphyria");



	if(spellData->how == CAST) {
		if(player->getRoomParent()->magicBonus())
			player->print("The room's magical properties increase the power of your spell.\n");
		if(!target->addEffect(effect, duration, strength, player, true))
			return(0);
	} else {
		target->addEffect(effect, duration, strength, NULL, true);
	}

	return(1);
}

//*********************************************************************
//						cmdTransmute
//*********************************************************************
// This allows a mage to transmute gold into magical charges for a wand

int cmdTransmute(Player* player, cmd* cmnd) {
	Object	*object=0;
	unsigned long cost=0;

	if(!player->ableToDoCommand())
		return(0);

	if(!player->isCt()) {
		if(!player->knowsSkill("transmute")) {
			player->print("You haven't a clue as to how to transmute anything!\n");
			return(0);
		}
		if(player->isBlind()) {
			player->printColor("^CYou're blind!\n");
			return(0);
		}
		if(player->ready[WIELD-1]) {
			player->print("Your hands are too full to do that.\n");
			return(0);
		}
	}
	if(!player->ready[HELD-1] || player->ready[HELD-1]->getType() != WAND) {
		player->print("You must hold the wand you wish to recharge.\n");
		return(0);
	}
	object = player->ready[HELD-1];

	if(object->getShotsCur()) {
		player->printColor("That %P still has magic in it.\n", object);
		return(0);
	}
	if(!player->spellIsKnown(object->getMagicpower() - 1)) {
		player->print("You don't know the spell of this wand.\n");
		return(0);
	}

	if(object->getMagicpower() == (S_TELEPORT+1) || object->getMagicpower() == (S_WORD_OF_RECALL+1)) {
		player->print("Your transmutation fails.\n");
		return(0);
	}

	// TODO: SKILLS: throw in a skill level check
	cost = (unsigned)(1000 * object->getShotsMax());
	if(object->flagIsSet(O_NO_FIX))
		cost *= 2;
	if(player->coins[GOLD] < cost || cost < 1) {
		player->print("You don't have enough gold.\n");
		return(0);
	}
	player->coins.sub(cost, GOLD);
	gServer->logGold(GOLD_OUT, player, Money(cost, GOLD), object, "Transmuate");

	if(!player->isCt() && !dec_daily(&player->daily[DL_RCHRG])) {
		player->print("You have recharged enough wands for one day.\n");
		return(0);
	}

	if(spell_fail(player, CAST)) {
		int dmg = 0;
		player->print("The wand glows bright red and explodes!\n");
		broadcast(player->getSock(), player->getParent(), "A wand explodes in %s's hand!\n", player->name);

		if(player->chkSave(SPL, player, -1)) {
			dmg = mrand(5, 10);
			player->print("You managed to avoid most of the explosion.\n");
		} else
			dmg = mrand(10, 20);


		player->doDamage(player, dmg, CHECK_DIE);
		player->checkImprove("transmute", false);
		player->unequip(HELD, UNEQUIP_DELETE);
		return(0);
	}

	// success!
	object->setShotsCur(object->getShotsMax());
	player->printColor("You successfully recharge the %s.\n", object->name);
	player->checkImprove("transmute", true);
	player->statistics.transmute();
	return(0);
}

//*********************************************************************
//						splBlind
//*********************************************************************
// The blind spell prevents a player or monster from seeing. The spell
// results in a penalty on attacks, and an inability look at objects
// players, rooms, or inventory. Also a player or monster cannot read.

int splBlind(Creature* player, cmd* cmnd, SpellData* spellData) {
	Creature* target=0;


	if(player->getClass() == BUILDER) {
		player->print("You cannot cast this spell.\n");
		return(0);
	}

	// blind self
	if(cmnd->num == 2) {

		target = player;
		if(spellData->how == CAST || spellData->how == SCROLL || spellData->how == WAND) {
			player->print("You are blind and can no longer see.\n");
			broadcast(player->getSock(), player->getParent(), "%M casts blindness on %sself.", player, player->himHer());
		} else if(spellData->how == POTION)
			player->print("Everything goes dark.\n");

	// blind a monster or player
	} else {
		if(noPotion(player, spellData))
			return(0);

		target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);

		if(!target || target == player) {
			player->print("That's not here.\n");
			return(0);
		}

		if(!player->canAttack(target))
			return(0);

		player->smashInvis();
		target->wake("Terrible nightmares disturb your sleep!");

		if(target->chkSave(SPL, player, 0) && !player->isCt()) {
			target->print("%M tried to cast a blind spell on you!\n", player);
			broadcast(player->getSock(), target->getSock(), player->getParent(), "%M tried to cast a blind spell on %N!", player, target);
			player->print("Your spell fizzles.\n");
			return(0);
		}

		if(spellData->how == CAST || spellData->how == SCROLL || spellData->how == WAND) {
			player->print("Blindness spell cast on %s.\n", target->name);
			broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts a blindness spell on %N.", player, target);
			target->print("%M casts a blindness spell on you.\n", player);
		}

		if(target->isMonster()) {
			target->getAsMonster()->addEnemy(player);
		}
	}
	if(target->isPlayer()) {
		if(player->isCt())
			target->setFlag(P_DM_BLINDED);
	}
	if(spellData->how == CAST && player->isPlayer())
		player->getAsPlayer()->statistics.offensiveCast();
	if(!player->isCt())
		target->addEffect("blindness", 180 - (target->constitution.getCur()/10), 1, player, true, player);
	else
		target->addEffect("blindness", 600, 1, player);

	return(1);
}


//*********************************************************************
//						spell_fail
//*********************************************************************
// This function returns 1 if the casting of a spell fails, and 0 if it is
// sucessful.

int spell_fail(Creature* player, int how) {
	Player	*pPlayer = player->getAsPlayer();
	int		chance=0, n=0;

	if(how == POTION)
		return(0);

	if(!pPlayer)
		return(0);

	n = mrand(1, 100);
	if(pPlayer)
		pPlayer->computeLuck();

	switch (pPlayer->getClass()) {

	case ASSASSIN:
		chance = ((pPlayer->getLevel() + bonus((int) pPlayer->intelligence.getCur())) * 5) + 30;
		break;
	case BERSERKER:
		chance = ((pPlayer->getLevel() + bonus((int) pPlayer->intelligence.getCur())) * 5);
		break;
	case VAMPIRE:
		chance = ((pPlayer->getLevel() + bonus((int) pPlayer->intelligence.getCur())) * 5) + 60;
		break;
	case CLERIC:
	case DRUID:
	case BARD:
		chance = ((pPlayer->getLevel() + bonus((int) pPlayer->intelligence.getCur())) * 5) + 65;
		break;
	case FIGHTER:
		if(pPlayer->getSecondClass() == MAGE)
			chance = ((pPlayer->getLevel() + bonus((int) pPlayer->intelligence.getCur())) * 5) + 75;
		else
			chance = ((pPlayer->getLevel() + bonus((int) pPlayer->intelligence.getCur())) * 5) + 10;
		break;
	case MAGE:
	case LICH:
		chance = ((pPlayer->getLevel() + bonus((int) pPlayer->intelligence.getCur())) * 5) + 75;
		break;
	case MONK:
	case WEREWOLF:
		chance = ((pPlayer->getLevel() + bonus((int) pPlayer->intelligence.getCur())) * 6) + 25;
		break;
	case PALADIN:
	case DEATHKNIGHT:
		chance = ((pPlayer->getLevel() + bonus((int) pPlayer->intelligence.getCur())) * 5) + 50;
		break;
	case RANGER:
		chance = ((pPlayer->getLevel() + bonus((int) pPlayer->intelligence.getCur())) * 4) + 56;
		break;
	case THIEF:
		if(pPlayer->getSecondClass() == MAGE)
			chance = ((pPlayer->getLevel() + bonus((int) pPlayer->intelligence.getCur())) * 5) + 75;
		else
			chance = ((pPlayer->getLevel() + bonus((int) pPlayer->intelligence.getCur())) * 6) + 22;
		break;
	case ROGUE:
		chance = ((pPlayer->getLevel() + bonus((int) pPlayer->intelligence.getCur())) * 6) + 22;
		break;
	default:
		return(0);
	}
	chance = chance * (pPlayer->getLuck() / 30);
	if(n > chance) {
		player->printColor("^yYour spell fails.\n");
		return(1);
	} else
		return(0);
}



//*********************************************************************
//						splJudgement
//*********************************************************************
// This function allows a DM to teleport themself or another player to jail

int splJudgement(Creature* player, cmd* cmnd, SpellData* spellData) {
	Player	*target=0;
	UniqueRoom	*new_rom=0;

	if(!player->isPlayer() || (!player->isCt() && spellData->how == CAST)) {
		player->print("That spell does not exist.\n");
		return(0);
	}

	if(!player->spellIsKnown(S_JUDGEMENT) && spellData->how == CAST) {
		player->print("You are unable to pass judgement at this time.\n");
		return(0);
	}

	CatRef 	cr;
	cr.setArea("jail");
	cr.id = 1;
	if(!loadRoom(cr, &new_rom)) {
		player->print("Spell failure.\n");
		return(0);
	}

	// Cast judgement on yourself
	if(cmnd->num == 2) {
		target = player->getAsPlayer();
		if(spellData->how == CAST || spellData->how == SCROLL || spellData->how == WAND) {
			player->print("You pass judgement upon yourself.\n");
			broadcast(player->getSock(), player->getParent(), "%M casts word of Judgement on %sself.", player, player->himHer());
		} else if(spellData->how == POTION) {
			player->print("You find yourself elsewhere.\n");
			broadcast(player->getSock(), player->getParent(), "%M drinks a potion of judgement and disapears.", player, player->himHer());
		}
	// Cast word of judgement on another player
	} else {
		if(noPotion(player, spellData))
			return(0);

		cmnd->str[2][0] = up(cmnd->str[2][0]);
		target = player->getParent()->findPlayer(player, cmnd, 2);
		if(!target) {
			player->print("That player is not here.\n");
			return(0);
		}


		if(spellData->how == CAST || spellData->how == SCROLL || spellData->how == WAND) {
			player->print("You pass judgemet on %N.\n", target);
			target->print("%M passes judgement on you.\nYou have been judged.\n", player);
			broadcast(player->getSock(), target->getSock(), player->getParent(), "%M passes judgement on %N.", player, target);

			logCast(player, target, "judgement", true);

			broadcast("The skies grow dark, and a fog begins to cover the ground.\nA shrill voice proclaims, \"Judgement has been passed upon %N\".\n", target);
		}
	}

	target->deleteFromRoom();
	target->addToRoom(new_rom);

	return(1);
}

//*********************************************************************
//						cmdBarkskin
//*********************************************************************

int cmdBarkskin(Player *player, cmd *cmnd) {
	long	i=0, t = time(0);
	int		adjustment=0;

	player->clearFlag(P_AFK);

	if(!player->ableToDoCommand())
		return(0);

	if(!player->knowsSkill("barkskin")) {
		player->print("You don't know how to turn your skin to bark.\n");
		return(0);
	}
	if(player->getRoomParent()->flagIsSet(R_ETHEREAL_PLANE)) {
		player->print("That is not possible here.\n");
		return(0);
	}


	if(player->flagIsSet(P_OUTLAW)) {
		player->print("You are unable to do that right now.\n");
		return(0);
	}

	if(player->isEffected("barkskin")) {
		player->print("Your skin is already made of bark.\n");
		return(0);
	}

	if(player->inCombat(false)) {
		player->print("Not in the middle of combat.\n");
		return(0);
	}

	i = player->lasttime[LT_BARKSKIN].ltime + player->lasttime[LT_BARKSKIN].interval;
	if(i>t && !player->isCt()) {
		player->pleaseWait(i-t);
		return(0);
	}

	// TODO: SKILLS: add skill level check
	player->smashInvis();
	player->unhide();

	broadcast(player->getSock(), player->getParent(), "%M's skin turns to bark.", player);

	adjustment = mrand(3,6);
	player->addEffect("barkskin", 120, adjustment, player, true, player);
	player->printColor("^yYour skin turns to bark.\n");
	player->checkImprove("barkskin", true);

	player->lasttime[LT_BARKSKIN].ltime = t;
	player->lasttime[LT_BARKSKIN].interval = 600;

	player->computeAC();
	return(0);
}


//*********************************************************************
//						cmdCommune
//*********************************************************************

int cmdCommune(Player *player, cmd *cmnd) {
	long	i=0, t = time(0), first_exit=0;
	int		chance=0;
	BaseRoom* newRoom=0;

	player->clearFlag(P_AFK);


	if(!player->ableToDoCommand())
		return(0);

	if(!player->knowsSkill("commune")) {
		player->print("You don't know how to commune with nature.\n");
		return(0);
	}

	if(!player->getRoomParent()->isOutdoors()) {
		player->print("You can only commune with nature while outdoors.\n");
		return(0);
	}

	i = player->lasttime[LT_PRAY].ltime;


	if(t - i < 45L && !player->isCt()) {
		player->pleaseWait(45L-t+i);
		return(0);
	}
	int level = (int)player->getSkillLevel(("commune"));

	chance = MIN(85, level * 20 + bonus((int) player->piety.getCur()));

	if(player->isStaff())
		chance = 100;

	if(mrand(1, 100) <= chance) {

		player->print("You successfully commune with nature.\n");
		player->print("You sense any living creatures in your surroundings.\n");

		first_exit = 1;
		for(Exit* ext : player->getRoomParent()->exits) {
			if(	ext->flagIsSet(X_DESCRIPTION_ONLY) ||
				ext->flagIsSet(X_SECRET) ||
				ext->isConcealed(player) ||
				ext->flagIsSet(X_STAFF_ONLY) ||
				ext->flagIsSet(X_LOOK_ONLY) ||
				ext->flagIsSet(X_CLOSED) ||
				ext->flagIsSet(X_NO_SEE) )
			{
				continue;
			}

			if(!first_exit)
				player->print("\n");
			player->print("%s:\n", ext->name);
			first_exit = 0;

			if(!Move::getRoom(player, ext, &newRoom, true)) {
				continue;
			}

			PlayerSet::iterator pIt;
			PlayerSet::iterator pEnd;
			pIt = newRoom->players.begin();
			pEnd = newRoom->players.end();
			Player* ply;
			while(pIt != pEnd) {
			    ply = (*pIt++);
				if(ply->isUndead() && !player->isCt())
					continue;

				if(ply->flagIsSet(P_DM_INVIS) && !player->isDm())
					continue;

				if(ply->isInvisible() && !player->isEffected("detect-invisible") && !player->isCt())
					continue;

				player->print("   %M\n", ply);
			}

			MonsterSet::iterator mIt;
			MonsterSet::iterator mEnd;
			mIt = newRoom->monsters.begin();
			mEnd = newRoom->monsters.end();
			Monster* mons;
			while(mIt != mEnd) {
			    mons = (*mIt++);
				if(mons->isUndead() && !player->isCt())
					continue;

				if(mons->isPet() && !player->isCt())
					continue;

				if(mons->isUndead() && !player->isCt())
					continue;

				if(mons->isInvisible() && !player->isEffected("detect-invisible") && !player->isCt())
					continue;

				player->print("   %s\n", mons->name);
			}
		}
		player->checkImprove("commune", true);
		player->lasttime[LT_PRAY].interval = 60L;
	} else {
		player->print("You failed to commune with nature.\n");
		broadcast(player->getSock(), player->getParent(), "%M tries to commune with nature.", player);
		player->checkImprove("commune", false);
		player->lasttime[LT_PRAY].ltime = 10L;
	}

	player->lasttime[LT_PRAY].ltime = t;
	return(0);
}

//*********************************************************************
//						isMageLich
//*********************************************************************

bool isMageLich(const Creature* creature) {
	if(	creature->getClass() != MAGE &&
		(creature->isPlayer() && creature->getAsConstPlayer()->getSecondClass() != MAGE) &&
		creature->getClass() != LICH &&
		!creature->isCt()
	) {
		creature->print("The arcane nature of that spell eludes you.\n");
		return(false);
	}
	return(true);
}

//*********************************************************************
//						noPotion
//*********************************************************************

bool noPotion(Creature* player, SpellData* spellData) {
	if(spellData->how == POTION) {
		player->print("You can only use a potion on yourself.\n");
		return(1);
	}
	return(0);
}
