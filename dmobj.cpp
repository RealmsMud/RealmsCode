/*
 * dmobj.cpp
 *	 Staff functions related to objects
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 *	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include "mud.h"
#include "commands.h"
#include "dm.h"
#include "craft.h"
#include "unique.h"
#include "effects.h"
#include <iomanip>

//*********************************************************************
//						dmCreateObj
//*********************************************************************
// This function allows a DM to create an object that will appear
// in their inventory.

int dmCreateObj(Player* player, cmd* cmnd) {
	Object	*object=0;

	CatRef	cr;
	getCatRef(getFullstrText(cmnd->fullstr, 1), &cr, player);

	if(!player->checkBuilder(cr)) {
		player->print("Error: %s out of your allowed range.\n", cr.str().c_str());
		return(0);
	}

	if(!loadObject(cr, &object)) {
		player->print("Error (%s)\n", cr.str().c_str());
		return(0);
	}
	if(!object->name[0] || object->name[0] == ' ') {
		player->printColor("Error (%s)\n", cr.str().c_str());
		delete object;
		return(0);
	}
	if(object->flagIsSet(O_RANDOM_ENCHANT))
		object->randomEnchant();
	player->printColor("%s^x added to your inventory.\n", object->name);

	if(!player->isDm())
		log_immort(false,player, "%s added %s to %s inventory.\n", player->name, object->name, player->hisHer());

	object->setDroppedBy(player, "DmCreate");
	player->addObj(object);
	return(0);
}

//*********************************************************************
//						statObj
//*********************************************************************
//  Display information on object given to player given.

bstring Object::statObj(int statFlags) {
	std::ostringstream objStr;
	Object* object=0;
	bstring str = "";
	bstring objName = name;
	bstring objPlural = plural;
	
	objName.Replace("^", "^^");
	objPlural.Replace("^", "^^");

	if(objPlural == "") {
		objStr << "Name: " << objName << "^x\n";
	} else {
		objStr << "Name:   " << objName << "^x\n"
			   << "Plural: " << objPlural << "^x\n";
	}

	objStr << "Id: " << getId() << "\n";
	const Unique* unique = gConfig->getUnique(this);
	if(unique)
		objStr << "^y - Unique this (In Game: ^Y" << unique->getInGame()
			   << "^y, Global Limit: ^Y" << unique->getGlobalLimit()<< "^y)^x\n";

	objStr << "Index: " << info.str();
	if(statFlags & ISDM && !droppedBy.name.empty())
		objStr << "\nDropped By: " << droppedBy << "\n";

	if(statFlags & ISCT)
		objStr << "\nLast modified by: " << lastMod;
	objStr << "\n";

	objStr << "Desc: " << description.c_str() << "\n";

	if(type == WEAPON)
		objStr << "Weapon type: " << getWeaponType().c_str() << "\n";
	else if(type == ARMOR)
		objStr << "Armor type: " << getArmorType().c_str() << "\n";

	objStr << "Use:  " << use_output << "\n";
	objStr << "Keys: [" << key[0] << "] [" << key[1] << "] [" << key[2] << "]\n\n";

	if(use_attack[0])
		objStr << "Attack string: " << use_attack << "\n";

	if(deed.high || deed.low.id)
		objStr << "Deed Room Range: " << deed.str() << "\n";

	if(type != BANDAGE)
		objStr << "Hit: " << damage.str();
	else
		objStr << "Heals: " << damage.str() << " hit points";

	if(adjustment != 0) {
		objStr << " (";
		if(adjustment > 0)
			objStr << "+";
		objStr << adjustment << ")\n";
	} else
		objStr << "\n";

	if(type == WEAPON || (type == ARMOR && wearflag == FEET) || type == BANDAGE) {
		if(type != BANDAGE)
			objStr << "Damage: ";
		else
			objStr << "Healing: ";

		objStr << "low " << damage.low() << ", high " << damage.high()
				<< "  average: " << damage.average() << "\n";
		if(type == WEAPON) {
			short num = numAttacks;
			if(numAttacks)
				objStr << "NumAttacks: " << numAttacks << "\n";
			else
				num = 1;

			if(delay > 0) // TODO: This will need adjusting (formatting)
				objStr << "Delay: " << (delay/10.0) << " second(s)\n";

			// Note: this equation is actually used backwards in Object::adjustWeapon
			// to compute weapon quality.
			objStr << "Damage per Second: " <<
				( (damage.average() + adjustment) * (1+num) / 2)
					/ (getWeaponDelay()/10.0) << "\n";
		}
	}

	
	objStr << "Shots: " << shotsCur << "/" <<  shotsMax << " ";
	if(chargesMax != 0)
	    objStr << " Charges: " << chargesCur << "/" << chargesMax;
	objStr << "\n";

	if(type == LIGHTSOURCE)
		objStr << "^WThis light source will last for approximately " << (MAX(1, shotsCur) * 20 - 10) << " seconds.^x\n";

	if(compass)
		objStr << "Compass: ^y" << compass->str() << "^x\n";
	if(recipe)
		objStr << "Recipe: ^y#" << recipe << "^x\n";

	if(minStrength > 0)
		objStr << "^WMinimum " << minStrength << " strength required to use.^x\n";

	objStr << "Type: " << obj_type(type) << "    ";

	if(type == WEAPON) {
		if(magicpower > 0 && magicpower < MAXSPELL && flagIsSet(O_WEAPON_CASTS))
			objStr << "Casts: " << magicpower << "(" << get_spell_name(magicpower - 1) << ")\n";
	} else {
		switch (type) {
		case ARMOR:
			break;
		case POTION:
		case SCROLL:
		case WAND:
			if(magicpower > 0 && magicpower <= MAXSPELL) {
				objStr << "Spell: " << magicpower << "(" << get_spell_name(magicpower - 1) << ")";
				if(magicpower-1 == S_ILLUSION) {
					const RaceData* race = 0;
					if(getExtra() > 0 && getExtra() < RACE_COUNT)
						race = gConfig->getRace(getExtra());
					if(!race)
						objStr << "\n^R*set o " << key[0] << " extra <race>^x to choose a race for this potion.";
					else
						objStr << " (" << race->getName() << ")";
				}
				objStr << "\n";
			}
			break;
		case CONTAINER:
			objStr << "    MaxBulk: ";
			if(maxbulk != 0)
				objStr << maxbulk;
			else objStr << bulk;
			objStr << "\n";
			break;
		case KEY:
			objStr << " (" << getKey() << ")\n";
			break;
		case LIGHTSOURCE:
		case MISC:
		case POISON:
		case BANDAGE:
			objStr << "\n";
			break;
		case LOTTERYTICKET:
			objStr << "Lottery Cycle: " << lotteryCycle << "  Ticket Info: " <<
				  lotteryNumbers[0] << " " << lotteryNumbers[1] << " " << lotteryNumbers[2] << " " <<
				  lotteryNumbers[3] << " " << lotteryNumbers[4] << "  " << lotteryNumbers[5] << "\n";
			break;
		case SONGSCROLL:
			if(magicpower > 0 && magicpower < gConfig->getMaxSong())
				objStr << "Song #" << magicpower << "(Song of " << get_song_name(magicpower-1) << ")\n";
			break;
		}
	}

	if(type == ARMOR || type == WEAPON) {
		objStr << "Wear location: " << getWearName() << "(" << wearflag << ").\n";
		if(type == WEAPON && wearflag != WIELD)
			objStr << "^rThis object is a weapon and is not wieldable.^x\n";
		if(type == ARMOR && (!wearflag || wearflag == HELD || wearflag == WIELD))
			objStr << "^rThis object is a armor and is not wearable.^x\n";
	}

	if(size)
		objStr << "Size: ^y" << getSizeName(size) << "^x\n";
	if(material)
		objStr << "Material: ^g" << getMaterialName(material) << "^x\n";

	if(type == ARMOR)
		objStr << "AC: " << armor;


	objStr << "   Value(gp): " << value[GOLD];
	objStr << "   Weight: " << weight;
	objStr << "   Bulk: " << bulk << " (" << getActualBulk() << ")";

	if(type == CONTAINER) {
		objStr << "\nLoadable inside: [" << in_bag[0].str() << "] [" << in_bag[1].str()
				<< "] [" << in_bag[2].str() << "]\n";
	}

	if(flagIsSet(O_COIN_OPERATED_OBJECT) && coinCost)
		objStr << "\n\nCoin Operated Item: " << coinCost << "gp per use.\n";

	if(shopValue)
		objStr << "\nShop Value: $" << shopValue;

	if(questnum && questnum <= numQuests) {
		objStr << "   Quest: " << questnum << " - (";
		if(gConfig->questTable[questnum-1]->name[0] != '\0')
			objStr << get_quest_name(questnum-1);
		else
			objStr << "Unknown";
		objStr << ")\n";
	} else
		objStr << "\n";
	if(level)
		objStr << "Level: ^y" << level << "^x  ";
	if(quality)
		objStr << "Quality: ^y" << (quality/10.0) << "^x  ";
	if(requiredSkill)
		objStr << "Minimum Skill: ^y" << requiredSkill << "^x  ";
	if(level || requiredSkill || quality > 0)
		objStr << "\n";

	if(special) {
		objStr << "Special: ";
		switch (special) {
		case SP_MAPSC:
			objStr << "Map or Scroll.\n";
			break;
		case SP_COMBO:
			objStr << "Combo Lock.\n";
			break;
		default:
			objStr << "Unknown.\n";
		}
	}

	if(effect != "") {
		objStr << "^yEffect: " << effect << "^x\n"
			   << "  Duration: ";

		if(effect == "poison" && !effectDuration)
			objStr << "Standard poison duratin.\n";
		else
			objStr << effectDuration << " seconds.\n";

		objStr << "  Strength: " << effectStrength << ".\n";
		if(flagIsSet(O_ENVENOMED)) {
			objStr << "Time remaining: " <<
				timestr(MAX(0,(lasttime[LT_ENVEN].ltime+lasttime[LT_ENVEN].interval-time(0))))
				<< "\n";
		}
	}

	if(flagIsSet(O_EQUIPPING_BESTOWS_EFFECT) && !Effect::objectCanBestowEffect(effect))
		objStr << "^yObject is flagged as bestowing effect \"" << effect << "\" that cannot be bestowed.^x\n";


	objStr << "Flags set: ";

	int loop = 0, numFlags = 0;

	for(loop=0; loop<MAX_OBJECT_FLAGS; loop++) {
		if(flagIsSet(loop)) {
			if(numFlags++ != 0)
				objStr << ", ";
			objStr << get_oflag(loop) << "(" << loop+1 << ")";
		}
	}

	if(numFlags == 0)
		objStr << "None.";
	else
		objStr << ".";

 	objStr << "\n";

 	if(randomObjects.size()) {
 		objStr << "^WRandomObjects:^x this item will turn into these random objects:\n";
 		std::list<CatRef>::const_iterator it;
 		for(it = randomObjects.begin(); it != randomObjects.end(); it++) {
 			loadObject(*it, &object);

 			objStr << "    " << std::setw(14) << (*it).str("", 'y') << " ^y::^x "
				   << (object ? object->name : "") << "\n";

 			if(object) {
 				delete object;
 				object = 0;
 			}
 		}
 	}

	if(increase) {
		objStr << "^cWhen studied, this object will increase:\n";
		if(increase->type == SkillIncrease) {
			const SkillInfo* skill = gConfig->getSkill(increase->increase);
			if(!skill)
				objStr << "^rThe skill set on this object is not a valid skill.^x\n";
			else
				objStr << "^W  Skill:^x     " << skill->getDisplayName() << "\n";
			objStr << "^W  Can Add If Not Known:^x " << (increase->canAddIfNotKnown ? "Yes" : "No") << "\n";
		} else if(increase->type == LanguageIncrease) {
			int lang = atoi(increase->increase.c_str());
			if(lang < 1 || lang > LANGUAGE_COUNT)
				objStr << "^rThe language set on this object is not a valid language.^x\n";
			else
				objStr << "^W  Language:^x  " << get_language_adj(lang-1) << "\n";
		} else {
			objStr << "^rThe increase type set on this object is not a valid increase type.^x\n";
		}

		if(increase->type != LanguageIncrease)
			objStr << "^W  Amount:^x    " << increase->amount << "\n";

		objStr << "^W  Only Once:^x " << (increase->onlyOnce ? "Yes" : "No") << "\n";
	}

 	if(questOwner != "")
 		objStr << "^gOwner:^x " << questOwner << "\n";
 	if(made) {
		str = ctime(&made);
		str.trim();
 		objStr << "Made: " << str << "\n";
 	}

	objStr << showAlchemyEffects()
			<< hooks.display();

	bstring toReturn = objStr.str();
	return(toReturn);
}


int stat_obj(Player* player, Object* object) {
	if(!player->canBuildObjects())
		return(cmdNoAuth(player));

	if(object->info.id && !player->checkBuilder(object->info)) {
		player->print("Error: object not in any of your alotted ranges.\n");
		return(0);
	}

	if(!player->isDm())
		log_immort(false,player, "%s statted object %s(%s).\n",
			player->name, object->name, object->info.str().c_str());

    int statFlags = 0;
    if(player->isCt())
		statFlags |= ISCT;
    if(player->isDm())
    	statFlags |= ISDM;
    player->printColor("%s\n", object->statObj(statFlags).c_str());



	return(0);
}

//*********************************************************************
//						setWhich
//*********************************************************************

int setWhich(const Player* player, bstring options) {
	options.Replace(",", "^x,^W");
	player->printColor("Which do you wish to set: ^W%s^x?\n", options.c_str());
	return(0);
}

//*********************************************************************
//						dmSetObj
//*********************************************************************

int dmSetObj(Player* player, cmd* cmnd) {
	Creature* creature=0;
	Monster* mTarget=0;
	BaseRoom* room = player->getRoomParent();
	Object	*object=0;
	int		n=0, match=0, test=0;
	long	num=0;
	double	dNum=0.0;
	char	flags[25], objname[200]; //temp3[80], objfile[80], ;
	//FILE	*filename;

	if(!player->canBuildObjects())
		return(cmdNoAuth(player));

	if(cmnd->num < 4) {
		player->print("Syntax: *set o <name> [<crt>] <ad|ar|dn|ds|dp|f|m|sm|s|t|v|wg|wr|q> [<value>]\n");
		return(0);
	}

	strcpy(objname, "");

	// make an exception when we're setting certain flags
	if(	cmnd->num == 4 ||
		(cmnd->num == 5 && (
			!strcmp(cmnd->str[3], "size") ||
			!strcmp(cmnd->str[3], "sub") ||
			!strcmp(cmnd->str[3], "ra") ||
			!strcmp(cmnd->str[3], "material") ||
			!strcmp(cmnd->str[3], "effect") ||
			!strcmp(cmnd->str[3], "eff") ||
			!strcmp(cmnd->str[3], "wear")
		) )
	) {
		strcpy(flags, cmnd->str[3]);
		num = cmnd->val[3];
		dNum = atof(getFullstrText(cmnd->fullstr, 4).c_str());

		object = player->findObject(player, cmnd, 2);
		if(!object)
			object = room->findObject(player, cmnd, 2);
		if(!object) {
			player->print("Object not found.\n");
			return(0);
		}
		if(Unique::isUnique(object))
			player->printColor("^yCaution:^x this will remove the object's unique status.\n");

	} else if(cmnd->num == 5) {
		strncpy(flags, cmnd->str[4], 24);
		num = cmnd->val[4];
		dNum = atof(getFullstrText(cmnd->fullstr, 5).c_str());

		cmnd->str[3][0] = up(cmnd->str[3][0]);
		creature = gServer->findPlayer(cmnd->str[3]);
		if(!creature) {
			cmnd->str[3][0] = low(cmnd->str[3][0]);
			creature = room->findCreature(player, cmnd, 3);
		}
		if(!creature || !player->canSee(creature)) {
			player->print("Creature not found.\n");
			return(0);
		}

		object = creature->findObject(player, cmnd, 2);
		if(!object || !cmnd->val[2]) {
			for(n = 0; n < MAXWEAR; n++) {
				if(!creature->ready[n])
					continue;
				if(keyTxtEqual(creature->ready[n], cmnd->str[2]))
					match++;
				else
					continue;
				if(cmnd->val[1] == match || !cmnd->val[2]) {
					object = creature->ready[n];
					break;
				}
			}
		}
		if(!object) {
			player->print("Object not found.\n");
			return(0);
		}

		if(Unique::isUnique(object)) {
			player->printColor("^yCannot change object %s^y owned by %s.\n", object->name, creature->name);
			player->print("This will remove the object's uniqueness and distort the object count.\n");
			player->printColor("Use ^W*take^x or ^Wsteal^x to remove the object from their inventory first.\n");
			return(0);
		}
		if(Lore::isLore(object)) {
			player->printColor("^yCannot change object %s^y owned by %s.\n", object->name, creature->name);
			player->print("This will remove the object's loreness and distort the object count.\n");
			player->printColor("Use ^W*take^x or ^Wsteal^x to remove the object from their inventory first.\n");
			return(0);
		}

		sprintf(objname, "%s's ", creature->name);
	}

	// because float variables suck
	dNum += 0.0001;

	if(!object) {
		player->print("Object not found.\n");
		return(0);
	}
	// make the green broadcast stay green
	strcat(objname, object->name);
	if(player->isDm() || player->isCt())
		strcat(objname, "^g");
	else
		strcat(objname, "^G");

	if(player->getClass() == BUILDER) {
		if(creature) {
			if(!player->canBuildMonsters()) {
				player->print("Error: you do not have authorization to modify monsters.\n");
				return(PROMPT);
			}
			mTarget = creature->getAsMonster();
			if(!mTarget) {
				player->print("Error: you are not allowed to modify players.\n");
				return(0);
			} else if(mTarget->info.id && !player->checkBuilder(mTarget->info)) {
				player->print("Creature number is not in any of your alotted ranges.\n");
				return(0);
			}
		}
		if(object->info.id && !player->checkBuilder(object->info, false)) {
			player->print("Error: object not in any of your alotted ranges.\n");
			return(0);
		}
	}

	short result = 0;
	bstring resultTxt = "";
	bstring setType = "";

	// Save this object as a full object until *saved
	object->setFlag(O_SAVE_FULL);

	switch (flags[0]) {
	case 'a':
		switch (flags[1]) {
		case 'd':
			object->setAdjustment(num);
			result = object->getAdjustment();
			setType = "Adjustment";
			break;
		case 'r':
			if(object->getType() != ARMOR) {
				player->printColor("Please set %P to type 6(armor) first.\n", object);
				return(PROMPT);
			}

			object->setArmor(num);
			result = object->getArmor();
			setType = "Armor";
			break;
		default:
			return(setWhich(player, "adjustment, armor"));
		}
		break;

	case 'b':
		if(flags[1] == 'u' || !flags[1]) {
			if(num > 100 || num < 0) {
				player->print("Bulk must be greater than or equal to 0 and less than or equal to 100.\n");
				return(PROMPT);
			}

			object->setBulk(num);
			result = object->getBulk();
			setType = "Bulk";
		} else {
			return(setWhich(player, "bulk"));
		}
		break;
	case 'c':

		if(flags[1] == 'o') {
			if(	flags[2] == 's' ||
				flags[2] == 'i'
			) {

				if(num > 100000 || num < 0) {
					player->print("Nothing is going to cost more than 100000gp.\n");
					return(PROMPT);
				}

				object->setCoinCost(num);
				result = object->getCoinCost();
				setType = "Coin Use Cost";

			} else if(flags[2] == 'm') {

				MapMarker mapmarker;
				CatRef	cr;
				getDestination(getFullstrText(cmnd->fullstr, cmnd->num+cmnd->val[2]-1), &mapmarker, &cr, player);

				Area *area=0;
				if(mapmarker.getArea())
					area = gConfig->getArea(mapmarker.getArea());

				if(!area) {
					if(object->compass) {
						player->print("Deleting compass.\n");
						delete object->compass;
						object->compass = 0;
						log_immort(2, player, "%s cleared %s's %s.\n",
							player->name, objname, "compass");
					} else
						player->print("That's not a valid location.\n");
				} else {

					if(!object->compass)
						object->compass = new MapMarker;
					*object->compass = *&mapmarker;

					player->print("Compass set to location %s.\n", mapmarker.str().c_str());
					log_immort(2, player, "%s set %s's %s to %s.\n",
						player->name, objname, "compass", mapmarker.str().c_str());

				}
			} else {
				return(setWhich(player, "coin cost, compass"));
			}
		} else {
			return(setWhich(player, "coin cost, compass"));
		}
		break;

	case 'd':
		switch(flags[1]) {
		case 'a':
		case 'l':
		case 'h':
			player->printColor("Use the following commands to set deed range:\n");
			player->printColor("  ^W*set o <object> ra <area>\n");
			player->printColor("  ^W*set o <object> rl <low #>\n");
			player->printColor("  ^W*set o <object> rh <high #>\n");
			break;
		case 'e':
			{
				if(dNum < 0.0) {
					player->print("Attack delay cannot be negative.\n");
					return(0);
				}

				object->setDelay((int)(dNum * 10.0));
				player->print("Delay set to %.1f.\n", object->getDelay()/10.0);

				log_immort(2, player, "%s set %s's %s to %.1f.\n",
					player->name, objname, "Delay", object->getDelay()/10.0);
				if(object->adjustWeapon() != -1)
					player->print("Weapon has been auto adusted to a mean damage of %.2f.\n", object->damage.getMean());
			}
			break;
		case 'n':
			object->damage.setNumber(num);
			result = object->damage.getNumber();
			setType = "Dice Number";
			break;
		case 's':
			object->damage.setSides(num);
			result = object->damage.getSides();
			setType = "Dice Sides";
			break;
		case 'p':
			object->damage.setPlus(num);
			result = object->damage.getPlus();
			setType = "Dice Plus";
			break;
		default:
			return(setWhich(player, "delay, dn, ds, dp"));
		}
		break;
	case 'e':
		switch(flags[1]) {
		case 'x':
			object->setExtra(num);
			result = object->getExtra();
			setType = "Extra";
			break;
		case 'f':
		{
			if(cmnd->num < 5) {
				player->print("Set what effect to what?\n");
				return(0);
			}

			long duration = -1;
			int strength = 1;

			bstring txt = getFullstrText(cmnd->fullstr, 5);
			if(txt != "")
				duration = atoi(txt.c_str());
			txt = getFullstrText(cmnd->fullstr, 6);
			if(txt != "")
				strength = atoi(txt.c_str());

			if(duration > EFFECT_MAX_DURATION || duration < -1) {
				player->print("Duration must be between -1 and %d.\n", EFFECT_MAX_DURATION);
				return(0);
			}

			if(strength < 0 || strength > EFFECT_MAX_STRENGTH) {
				player->print("Strength must be between 0 and %d.\n", EFFECT_MAX_STRENGTH);
				return(0);
			}

			bstring effectStr = cmnd->str[4];

			if(duration == 0) {
				player->print("Effect '%s' removed.\n", object->getEffect().c_str());
				object->clearEffect();
				log_immort(2, player, "%s cleared %s's effect.\n",
					player->name, objname);
			} else {
				object->setEffect(effectStr);
				object->setEffectDuration(duration);
				object->setEffectStrength(strength);
				player->print("Effect '%s' added with duration %d and strength %d.\n", effectStr.c_str(), duration, strength);
				log_immort(2, player, "%s set %s's effect to %s with duration %d and strength %d.\n",
					player->name, objname, effectStr.c_str(), duration, strength);
			}
		}
		break;
		default:
			return(setWhich(player, "extra, effect"));
		}
		break;
	case 'f':
		if(flags[1] == 'l' || !flags[1]) {
			if(num < 1 || num > MAX_OBJECT_FLAGS) {
				player->print("Error: flag out of range.\n");
				return(PROMPT);
			}
			if(num-1 == O_UNIQUE) {
				if(player->getClass() == BUILDER)
					player->printColor("You will need to get a CT to set this object as unique using the ^y*unique^x command.\n");
				else
					player->printColor("Use the ^y*unique^x command to modify the uniqueness of this object.\n");
				return(0);
			}
			if(num-1 == O_LORE) {
				if(player->getClass() == BUILDER)
					player->printColor("You may have a CT use ^y*set o [object] lore #^x to specify how many of this object may be in game.\n");
				else
					player->printColor("You may use ^y*set o [object] lore #^x to specify how many of this object may be in game.\n");
			}
			if(object->flagIsSet(num - 1)) {
				object->clearFlag(num - 1);
				player->printColor("%O's flag #%d off.\n", object, num);
				test=0;
			} else {
				object->setFlag(num - 1);
				player->printColor("%O's flag #%d on.\n", object, num);
				test=1;
			}
			log_immort(2, player, "%s turned %s's flag %ld(%s) %s.\n",
				player->name, objname, num, get_oflag(num-1), test == 1 ? "On" : "Off");
		} else {
			return(setWhich(player, "flag"));
		}
		break;
	case 'k':
		if(flags[1] == 'e' || !flags[1]) {
			if(object->getType() != KEY) {
				player->print("Object is not a key. *set to object type 12.\n");
				return(PROMPT);
			}

			if(num < 0 || num > 256) {
				player->print("Error: key out of range.\n");
				return(PROMPT);
			}

			object->setKey(num);
			result = object->getKey();
			setType = "Key Value";
		} else {
			return(setWhich(player, "key"));
		}
		break;

	case 'l':
		if(flags[1] == 'e') {
			if(num < 0 || num > MAXALVL) {
				player->print("Error: level out of range.\n");
				return(PROMPT);
			}
			
			if(num > 0 && (object->getType() == ARMOR || object->getType() == WEAPON))
				player->printColor("^yConsider using a minimum-skill requirement instead of a level requirement.\n");

			object->setLevel(num);
			result = object->getLevel();
			setType = "Level";

		} else if(flags[1] == 'o') {

			if(!object->info.id) {
				player->print("Object must be saved to an index to set lore amount on it.\n");
				return(0);
			}

			num = MIN(50, MAX(1, num));

			if(num == 1)
				gConfig->delLore(object->info);
			else {
				Lore* l = gConfig->getLore(object->info);
				if(l)
					l->setLimit(num);
				else
					gConfig->addLore(object->info, num);
			}
			gConfig->saveLimited();

			player->printColor("Object lore set to ^y%d^x.\n", num);
			if(num == 1)
				player->print("To remove lore, clear the object's lore flag and resave.\n");
			else
				player->print("To remove lore, set lore amount to 1 and clear the object's lore flag and resave.\n");
			return(0);

		} else {
			return(setWhich(player, "level, lore"));
		}
		break;
	case 'm':
		if(flags[1] == 'a' && flags[2] == 't') {
			if(num > (MAX_MATERIAL-1) || num < NO_MATERIAL) {
				player->print("Error: material out of range.\n");
				return(PROMPT);
			}

			object->setMaterial((Material)num);
			result = object->getMaterial();
			resultTxt = getMaterialName(object->getMaterial());
			setType = "Material";
			
		} else if(flags[1] == 'b') {
			if(num > 100 || num < 0) {
				player->print("Max bulk must be greater than or equal to 0 and less than or equal to 100.\n");
				return(0);
			}

			object->setMaxbulk(num);
			result = object->getMaxbulk();
			setType = "Max Bulk";

		} else if(flags[1] == 'a') {

			if(num < 0 || num > MAXSPELL) {
				player->print("Error: spell out of range.\n");
				return(0);
			}

			object->setMagicpower(num);
			if(object->getMagicpower()) {
				if(object->getType() == SONGSCROLL) {
					player->print("Magic power set to %d - (Song of %s).\n", num, get_song_name(num-1));
					log_immort(2, player, "%s set %s's %s to %ld - (Song of %s).\n",
							player->name, objname, "Song", num, get_song_name(num-1));
				} else {
					player->print("Magic power set to %d - (%s).\n",num, get_spell_name(num-1));
					log_immort(2, player, "%s set %s's %s to %ld - (%s).\n",
							player->name, objname, "Spell", num,get_spell_name(num-1));
				}
			} else {
					player->print("Magic power cleared.\n");
					log_immort(2, player, "%s cleared %s's %s.\n",
							player->name, objname, "Spell");
			}
		} else {
			return(setWhich(player, "material, maxbulk, magic"));
		}

		break;
	case 'n':
		if(flags[1] == 'u' || !flags[1]) {
			if(object->getType() != WEAPON) {
				player->print("Error: numAttacks can only be set on weapons.\n");
				return(PROMPT);
			}
			if(num > 100 || num < 1) {
				player->print("Number of attacks must be greater than 0 and less than or equal to 100.\n");
				return(0);
			}

			object->setNumAttacks(num);
			result = object->getNumAttacks();
			setType = "numAttacks";

			if(object->adjustWeapon() != -1)
				player->print("Weapon has been auto adusted to a mean damage of %.2f.\n", object->damage.getMean());

		} else {
			return(setWhich(player, "numAttacks"));
		}
		break;
	case 'o':
		if(flags[1] == 'b' || !flags[1]) {
			if(object->getType() != CONTAINER) {
				player->print("Object must be a container.\n");
				return(0);
			}
			num = atoi(&cmnd->str[3][1]);

			object->in_bag[num-1].id = cmnd->val[3];

			object->setShotsMax(MAX(num, object->getShotsMax()));
			player->print("Loadable container object %s set to item number %s.\n",
				object->info.str().c_str(), object->in_bag[num-1].str().c_str());
			log_immort(2, player, "%s set container %s(%s) to load object %s.\n",
				player->name, objname, object->info.str().c_str(), object->in_bag[num-1].str().c_str());

		} else {
			return(setWhich(player, "objects"));
		}
		break;

	case 'q':
		if(flags[1] == 'u' && flags[2] == 'a') {
			if(object->getType() == ARMOR || object->getType() == WEAPON) {
				if(dNum < 0.0 || dNum > MAXALVL+0.0001) {
					player->print("Error: quality out of range. (0-%d)\n", MAXALVL);
					return(PROMPT);
				}
			} else {
				if(dNum < 0.0 || dNum > 1000.0) {
					player->print("Error: quality out of range.(0-1000)\n");
					return(PROMPT);
				}
			}

			object->setQuality(dNum*10.0);
			player->print("Quality set to %.1f.\n", dNum);

			log_immort(2, player, "%s set %s's %s to %.1f.\n", player->name, objname, "Quality", dNum);
			if(object->adjustArmor() != -1)
				player->print("Armor has been auto adusted to %d.\n", object->getArmor());
			if(object->adjustWeapon() != -1)
				player->print("Weapon has been auto adusted to a mean damage of %.2f.\n", object->damage.getMean());
			break;
		} else if(flags[1] == 'u' && flags[2] == 'e') {
			if(num < 0 || num > 64) {
				player->print("Quest number must be greater than or equal to 0 and less than or equal to 64.\n");
				return(0);
			}

			object->setQuestnum(num);
			result = object->getQuestnum();
			resultTxt = get_quest_name(object->getQuestnum() - 1);
			setType = "Quest";
		} else {
			return(setWhich(player, "quality, quest"));
		}
		break;
	case 'r':
		if(low(cmnd->str[3][1]) == 'l') {
			object->deed.low.id = (int)cmnd->val[3];
			result = object->deed.low.id;
			setType = "Low Deed Room boundary";
		} else if(low(cmnd->str[3][1]) == 'h') {
			object->deed.high = (int)cmnd->val[3];
			result = object->deed.high;
			setType = "High Deed Room boundary";
		} else if(low(cmnd->str[3][1]) == 'a') {
			object->deed.low.setArea(cmnd->str[4]);
			player->print("Deed Area: %s\n", object->deed.low.area.c_str());
			log_immort(2, player, "%s set %s's %s to %s.\n",
				player->name, objname, "Deed Area", object->deed.low.area.c_str());
		} else if(low(cmnd->str[3][1]) == 'e') {
			Recipe* recipe = gConfig->getRecipe((int)cmnd->val[3]);
			if(!recipe) {
				player->print("That recipe does not exist.\n");
				return(0);
			}
			if(!recipe->canBeEdittedBy(player)) {
				player->print("You are not allowed to set that recipe on an object.\n");
				return(0);
			}

			object->setRecipe(recipe->getId());
			result = object->getRecipe();
			resultTxt = recipe->getResultName();
			setType = "Recipe";
		} else {
			return(setWhich(player, "rl (room low), rh (room high), ra (room area), recipe"));
		}
		break;
	case 's':
		if(flags[1] == 'i') {
			object->setSize(getSize(cmnd->str[4]));

			player->print("Size set to %s.\n", getSizeName(object->getSize()).c_str());
			log_immort(2, player, "%s set %s's %s to %s.\n",
				player->name, objname, "Size", getSizeName(object->getSize()).c_str());
		} else if(flags[1] == 'm') {
			num = MAX(0, MIN(num,5000));
			
			object->setShotsMax(num);
			result = object->getShotsMax();
			setType = "Max Shots";
		} else if(flags[1] == 'p') {
			num=MAX(0, MIN(num, MAX_SP));

			object->setSpecial(num);
			result = object->getSpecial();
			setType = "Special";
		} else if(flags[1] == 't') {
			num=MAX(0, MIN(num, 280));

			object->setMinStrength(num);
			result = object->getMinStrength();
			setType = "MinStrength";
		} else if(flags[1] == 'u') {
			bstring subType = cmnd->str[4];
			if(object->getType() == WEAPON)
				object->setWeaponType(subType);
			else if(object->getType() == ARMOR) {
				object->setArmorType(subType);
				if(object->adjustArmor() != -1)
					player->print("Armor has been auto adusted to %d.\n", object->getArmor());
			} else {
				object->setSubType(subType);
			}
			player->print("Object subtype set to %s.\n", object->getSubType().c_str());
			log_immort(2, player, "%s set %s's %s to %s.\n",
				player->name, objname, "Subtype", object->getSubType().c_str());
		} else if(flags[1] == 'h' || flags[1] == 'v') {
			object->setShopValue(num);
			player->print("Shop value set.\n");
			log_immort(2, player, "%s set %s's %s to %ld.\n",
				player->name, objname, "shopValue", object->getShopValue());
		} else if(flags[1] == 'k') {
			if(num < 0 || num > MAXALVL*10) {
				player->print("Error: required skill out of range.\n");
				return(PROMPT);
			}

			object->setRequiredSkill(num);
			result = object->getRequiredSkill();
			setType = "Required Skill";
		} else if(flags[1] == 'c' || !flags[1]) {
			num=MAX(0, MIN(num, 5000));

			object->setShotsCur(num);
			result = object->getShotsCur();
			setType = "Current Shots";
		} else {
			return(setWhich(player, "size, shots, sm (shots max), special, strength, subtype, shopvalue, skill"));
		}

		break;
	case 't':
		if(flags[1] == 'y' || !flags[1]) {
			if((num < 0 || num > MAX_OBJ_TYPE) && !player->isDm())
				return(PROMPT);

			if(num > MAX_OBJ_TYPE) {
				object->setType(MISC);
			} else {
				object->setType(num);
			}

			result = object->getType();
			resultTxt = obj_type(num);
			setType = "Type";
		} else {
			return(setWhich(player, "type"));
		}
		break;
	case 'v':
		if(flags[1] == 'a' || !flags[1]) {
			if(!player->isDm())
				num=MAX(0, MIN(num, 500000));
			else
				num=MAX(0, MIN(num, 20000000));

			object->value.set(num, GOLD);
			result = num;
			setType = "Value";
		} else {
			return(setWhich(player, "value"));
		}
		break;
	case 'w':
		if(flags[1] == 'g' || (flags[1] == 'e' && flags[2] == 'i')) {
			num=MAX(0, MIN(num, 5000));

			object->setWeight(num);
			result = object->getWeight();
			setType = "Weight";
		} else if(flags[1] == 'r' || (flags[1] == 'e' && flags[2] == 'a')) {
			bstring wear = cmnd->str[4];
			if(!isdigit(cmnd->str[4][0])) {
				if(wear == "body")
					num = 1;
				else if(wear == "arm" || wear == "arms" || wear == "sleeve" || wear == "sleeves")
					num = 2;
				else if(wear == "leg" || wear == "legs" || wear == "pant" || wear == "pants")
					num = 3;
				else if(wear == "neck")
					num = 4;
				else if(wear == "belt" || wear == "waist")
					num = 5;
				else if(wear == "hand" || wear == "hands" || wear == "glove" || wear == "gloves")
					num = 6;
				else if(wear == "head" || wear == "helmet" || wear == "hat")
					num = 7;
				else if(wear == "feet" || wear == "shoe" || wear == "shoes" || wear == "boot" || wear == "boots")
					num = 8;
				else if(wear == "finger" || wear == "ring")
					num = 9;
				else if(wear == "held" || wear == "hold")
					num = 17;
				else if(wear == "shield")
					num = 18;
				else if(wear == "face" || wear == "mask" || wear == "visor")
					num = 19;
				else if(wear == "wield" || wear == "weapon")
					num = 20;
			}
			if(num < 0 || num > 64) {
				player->print("Use a valid wear location.\n");
				break;
			}
			if((num > 9) && (num < 16)) {
				player->print("Use wear location 9 for rings.\n");
				break;
			}

			object->setWearflag(num);
			result = object->getWearflag();
			resultTxt = object->getWearName();
			setType = "Wear Location";

			if(object->adjustArmor() != -1)
				player->print("Armor has been auto adusted to %d.\n", object->getArmor());
		} else {
			return(setWhich(player, "wear, weight"));
		}
		
		break;
	default:
		player->print("Invalid option.\n");
		return(PROMPT);
	}

	
	if(setType != "") {
		if(resultTxt != "") {
			player->printColor("%s set to %ld(%s).\n", setType.c_str(), result, resultTxt.c_str());
			log_immort(2, player, "%s set %s's %s to %ld(%s^g).\n",
				player->name, objname, setType.c_str(), result, resultTxt.c_str());
		} else {
			player->print("%s set to %ld.\n", setType.c_str(), result);
			log_immort(2, player, "%s set %s's %s to %ld.\n",
				player->name, objname, setType.c_str(), result);
		}
	}


	if(Unique::isUnique(object)) {
		player->printColor("^yUnique status of %s^y has been removed.\n", object->name);
		player->print("If the object is saved to a new index, the unique flag will be removed.\n");
		player->print("If the object is resaved to the unique range, it will become unique again.\n");
		object->clearFlag(O_UNIQUE);
	}

	if(Lore::isLore(object)) {
		player->printColor("^yLore status of %s^y has been removed.\n", object->name);
		player->print("If the object is resaved to any index, it will become lore again.\n");
	}

	// Reset object index
	object->info.id = 0;
	object->escapeText();
	object->track(player);

	return(0);
}



//*********************************************************************
//						dmObjName
//*********************************************************************
// the dmObjName command allows a dm/ caretaker to modify an already
// existing object's name, description, wield description, and key
// words. This command does not save the changes to the object to the
// object data base.  This is used to edit an object that may or may
// not be saved to the database using the dmSaveObj function.

int dmObjName(Player* player, cmd* cmnd) {
	Object	*object=0;
	int		i=0, num=0;
	char	which=0;
	bstring text = "";

	if(cmnd->num < 2) {
		player->print("\nRename what object to what?\n");
		player->print("*oname <object> [#] [-adok] <name>\n");
		return(PROMPT);
	}

	// parse the full command string for the start of the description
	// (pass  command, object, obj #, and possible flag).   The object
	// number has to be interpreted separately, and with the normal
	// command parse (cmnd->val), due to problems caused having
	// the object number followed by a "-"

	while(isspace(cmnd->fullstr[i]))
		i++;
	while(!isspace(cmnd->fullstr[i]))
		i++;
	while(isspace(cmnd->fullstr[i]))
		i++;
	while(isalpha(cmnd->fullstr[i]))
		i++;
	while(isspace(cmnd->fullstr[i]))
		i++;

	cmnd->val[1]= 1;
	if(isdigit(cmnd->fullstr[i]))
		cmnd->val[1] = atoi(&cmnd->fullstr[i]);

	object = player->findObject(player, cmnd, 1);
	if(!object)
		object = player->getRoomParent()->findObject(player, cmnd, 1);
	if(!object) {
		player->print("Item not found.\n");
		return(0);
	}

	while(isdigit(cmnd->fullstr[i]))
		i++;
	while(isspace(cmnd->fullstr[i]))
		i++;

	// parse flag
	if(cmnd->fullstr[i] == '-') {
		if(cmnd->fullstr[i+1] == 'd') {
			which =1;
			i += 2;
		} else if(cmnd->fullstr[i+1] == 'o') {
			which =2;
			i += 2;
		} else if(cmnd->fullstr[i+1] == 'a') {
			which =3;
			i += 2;
		} else if(cmnd->fullstr[i+1] == 'k') {
			i += 2;
			which = 4;
			num = atoi(&cmnd->fullstr[i]);
			if(num <1 || num > 3)
				num = 0;
			while(isdigit(cmnd->fullstr[i]))
				i++;
		}
		while(isspace(cmnd->fullstr[i]))
			i++;
	}

	// no description given
	if(cmnd->fullstr[i] == 0)
		return(PROMPT);

	text = &cmnd->fullstr[i];
	// handle object modification

	switch(which) {
	case 0:
		if(text.getLength() > 79)
			text = text.left(79);
		if(Pueblo::is(text)) {
			player->print("That object name is not allowed.\n");
			return(0);
		}
		strcpy(object->name, text.c_str());
		player->print("\nName ");
		break;
	case 1:
		if(text == "0" && object->description != "") {
			object->description = "";
			player->print("Item description cleared.\n");
			return(0);
		} else if(Pueblo::is(text)) {
			player->print("That object description is not allowed.\n");
			return(0);
		} else {
			object->description = text;
			object->description.Replace("*CR*", "\n");
		}
		player->print("\nDescription ");
		break;
	case 2:
		if(text == "0" && object->use_output[0]) {
			zero(object->use_output, sizeof(object->use_output));
			player->print("Item output string cleared.\n");
			return(0);
		} else if(Pueblo::is(text)) {
			player->print("That object output string is not allowed.\n");
			return(0);
		} else {
			if(text.getLength() > 79)
				text = text.left(79);
			strcpy(object->use_output, text.c_str());
			player->print("\nOutput String ");
		}
		break;
	case 3:
		if(text == "0" && object->use_attack[0]) {
			zero(object->use_attack, sizeof(object->use_attack));
			player->print("Item attack string cleared.\n");
			return(0);
		} else {
			if(text.getLength() > 49)
				text = text.left(49);
			strcpy(object->use_attack, text.c_str());
			player->print("\nAttack String ");
		}
		break;
	case 4:
		if(num) {
			if(text == "0" && object->key[num-1][0]) {
				zero(object->key[num-1], sizeof(object->key[num-1]));
				player->print("Key #%d string cleared.\n", num);
				return(0);
			} else {
				if(text.getLength() > OBJ_KEY_LENGTH-1)
					text = text.left(OBJ_KEY_LENGTH-1);
				strcpy(object->key[num-1], text.c_str());
				player->print("\nKey ");
			}
		}
		break;
	}
	object->escapeText();
	player->print("done.\n");
	return(0);
}

//*********************************************************************
//						dmAddObj
//*********************************************************************

int dmAddObj(Player* player, cmd* cmnd) {
	Object    *newObj=0;

	if(!player->canBuildObjects())
		return(cmdNoAuth(player));

	if(cmnd->val[1] < 1) {
		player->print("Add what?\n");
		return(0);
	}

	newObj = new Object;
	if(!newObj) {
		merror("dmAddObj", NONFATAL);
		player->print("Cannot allocate object.\n");
		return(0);
	}
	log_immort(true, player, "%s made a new Object!\n", player->name);

	strcpy(newObj->name, "small clay ball");
	strcpy(newObj->key[0], "ball");
	strcpy(newObj->key[1], "clay");
	newObj->setWearflag(HELD);
	newObj->setType(MISC);
	newObj->setWeight(1);

	newObj->setFlag(O_SAVE_FULL);

	player->addObj(newObj);
	player->print("Generic object added to your inventory.\n");
	return(0);
}

//*********************************************************************
//						dmSaveObj
//*********************************************************************

void dmSaveObj(Player* player, cmd* cmnd, CatRef cr) {
	char	file[80];
	Object* object=0;

	if(!player->canBuildObjects()) {
		cmdNoAuth(player);
		return;
	}

	object = player->findObject(player, cmnd->str[2], 1);
	if(!object)
		object = player->getRoomParent()->findObject(player, cmnd->str[2], 1);
	if(!object) {
		player->print("Object not found.\n");
		return;
	}

	if(!validObjId(cr)) {
		player->print("Index error: object number invalid.\n");
		return;
	}

	if(object->info.id && !player->checkBuilder(object->info, false)) {
		player->print("Error: %s out of your allowed range.\n", object->info.str().c_str());
		return;
	}
	if(!player->checkBuilder(cr, false)) {
		player->print("Error: %s out of your allowed range.\n", cr.str().c_str());
		return;
	}

	object->clearFlag(O_BEING_PREPARED);
	object->clearFlag(O_SAVE_FULL);
	logn("log.bane", "%s saved %s to %s.\n", player->name, object->name, cr.str().c_str());

	object->info = cr;

	sprintf(file, "%s", objectPath(object->info));
	if(file_exists(file))
		player->print("Object might already exist.\n");

	dmResaveObject(player, object);
}

//*********************************************************************
//						dmResaveObject
//*********************************************************************

void dmResaveObject(const Player* player, Object* object, bool flush) {
	Container* cont = object->getParent();
	int		val = object->getShopValue();
	DroppedBy droppedBy = object->droppedBy;

	// TODO: Clear out items inside before saving
	object->first_obj = 0;

	object->setParent(0);
	object->setShopValue(0);
	object->droppedBy.clear();


	if(object->saveToFile() != 0) {
		loge("Error saving object in dmResaveObject()");
		player->print("Error: object was not saved.\n");
	} else
		player->print("Object %s updated.\n", object->info.str().c_str());


	// swap this new Object if its in the queue
	if(flush || player->flagIsSet(P_NO_FLUSHCRTOBJ))
		gConfig->replaceObjectInQueue(object->info, object);

	object->droppedBy = droppedBy;
	object->setParent(cont);
	object->setShopValue(val);
}

//*********************************************************************
//						dmSize
//*********************************************************************

int dmSize(Player* player, cmd* cmnd) {
	if(cmnd->num < 2) {
		player->print("Syntax: *size <object> <size>\n");
		return(0);
	}
	std::ostringstream oStr;
	oStr << "*set o " << cmnd->str[1] << " " << cmnd->val[1] << " size " << cmnd->str[2];
	cmnd->fullstr = oStr.str();
	parse(cmnd->fullstr, cmnd);

	return(dmSetObj(player, cmnd));
}

//*********************************************************************
//						makeWeapon
//*********************************************************************

void makeWeapon(Player *player, CatRef* cr, Object* object, Object *random, const bstring& sub, const bstring& descBase, const bstring& descAll, bool twoHanded, int weight, double value, int bulk, short numAttacks) {
	Object* newObj = 0;
	bool addToInventory = true;

	if(sub == object->getSubType()) {
		// the object already exists
		newObj = object;
		addToInventory = false;
	} else {
		// create one if it doesn't
		newObj = new Object;
		if(!newObj) {
			merror("makeWeapon", NONFATAL);
			player->print("Cannot allocate object.\n");
			return;
		}
		*newObj = *object;
	}


	if(!newObj->key[2][0]) {
		strcpy(newObj->key[1], sub.c_str());
		sprintf(newObj->name, "%s %s", newObj->key[0], sub.c_str());
	} else {
		strcpy(newObj->key[2], sub.c_str());
		sprintf(newObj->name, "%s %s %s", newObj->key[0], newObj->key[1], sub.c_str());
	}

	newObj->setWeight(MAX(1, weight));
	newObj->setBulk(MAX(1, bulk));
	newObj->setNumAttacks(numAttacks);
	//newObj->setBulk(bulk);
	value = MAX(1, value);
	newObj->value.set((long)value, GOLD);
	newObj->setWearflag(WIELD);

	// The random item should represent the total value, weight, and bulk of the item lists
	// this is useful in trading - if they can carry the random object, then they can carry
	// all of the objects in the list.
	random->value.set(random->value[GOLD] + newObj->value[GOLD], GOLD);
	random->setWeight(random->getWeight() + newObj->getActualWeight());
	random->setBulk(random->getBulk() + newObj->getActualBulk());

	newObj->description = descBase + " " + descAll;
	newObj->setWeaponType(sub);

	newObj->adjustWeapon();

	newObj->info = *cr;
	random->randomObjects.push_back(newObj->info);

	if(twoHanded)
		newObj->setFlag(O_TWO_HANDED);
	else
		newObj->clearFlag(O_TWO_HANDED);

	player->printColor("Saving %s to %s... ", newObj->name, cr->str("", 'W').c_str());
	dmResaveObject(player, newObj);
	log_immort(2, player, "%s cloned %s into %s.\n",
		player->name, newObj->name, cr->str().c_str());

	if(addToInventory)
		player->addObj(newObj);

	cr->id++;
}

//*********************************************************************
//						makeArmor
//*********************************************************************

void makeArmor(Player *player, CatRef* cr, Object* object, Object *random, int wear, const bstring& base, const bstring& descBase, const bstring& descAll, long value, int weight, int bulk, bool somePrefix=false) {
	Object* newObj = 0;
	bool addToInventory = true;

	if(wear == object->getWearflag()) {
		// the object already exists
		newObj = object;
		addToInventory = false;
	} else {
		// create one if it doesn't
		newObj = new Object;
		if(!newObj) {
			merror("makeArmor", NONFATAL);
			player->print("Cannot allocate object.\n");
			return;
		}
		*newObj = *object;
	}


	if(!newObj->key[2][0]) {
		strcpy(newObj->key[1], base.c_str());
		sprintf(newObj->name, "%s %s", newObj->key[0], base.c_str());
	} else {
		strcpy(newObj->key[2], base.c_str());
		sprintf(newObj->name, "%s %s %s", newObj->key[0], newObj->key[1], base.c_str());
	}

	if(somePrefix)
		newObj->setFlag(O_SOME_PREFIX);
	else
		newObj->clearFlag(O_SOME_PREFIX);

	newObj->setWeight(weight);
	newObj->setBulk(bulk);
	newObj->value.set(value, GOLD);
	newObj->setWearflag(wear);

	// The random item should represent the total value, weight, and bulk of the item lists
	// this is useful in trading - if they can carry the random object, then they can carry
	// all of the objects in the list.
	random->value.set(random->value[GOLD] + value, GOLD);
	random->setWeight(random->getWeight() + weight);
	random->setBulk(random->getBulk() + bulk);

	newObj->adjustArmor();

	newObj->description = descBase + " " + descAll;

	newObj->info = *cr;
	random->randomObjects.push_back(newObj->info);

	player->printColor("Saving %s to %s... ", newObj->name, cr->str("", 'W').c_str());
	dmResaveObject(player, newObj);
	log_immort(2, player, "%s cloned %s into %s.\n",
		player->name, newObj->name, cr->str().c_str());

	if(addToInventory)
		player->addObj(newObj);

	cr->id++;
}

//*********************************************************************
//						dmClone
//*********************************************************************
// given an object, this code will create a set of armor

int dmClone(Player* player, cmd* cmnd) {
	Object* object=0, *random=0;
	bool isArmor=false, isWeapon = false;

	if(cmnd->num == 1) {
		player->printColor("^WArmor:^x This command will take a single piece of armor and create a set of armor out of it.\n");
		player->print("       It will save the items starting at <area.id> and will use between 10 and 11 spaces.\n");
		player->printColor("^WWeapons:^x This command will take a weapon and create one weapon for every weapon type.\n");
		player->print("         It will save the items starting at <area.id> and will use 19 spaces.\n");
		player->print("The final item can be used to randomly become any of the items.\n");
		player->printColor("Syntax: ^c*clone <object> <area.id> <partial desc>\n");
		player->print("The partial description should complete the sentence, \"It's a <item> _____\".\n");
		return(0);
	}

	object = findObject(player, player->first_obj, cmnd->str[1], 1);
	if(!object) {
		player->print("Object not found.\n");
		player->printColor("Syntax: ^c*clone <object> <area.id> <partial desc>\n");
		return(0);
	}

	player->printColor("^yCloning %s.\n", object->name);

	if(object->info.id && !player->checkBuilder(object->info, false)) {
		player->print("Error: %s out of your allowed range.\n", object->info.str().c_str());
		return(0);
	}

	bstring sub = object->getSubType();
	if(sub == "cloth" || sub == "leather" || sub == "chain" || sub == "plate") {
		isArmor = true;
	} else if(sub == "sword" || sub == "great-sword" || sub == "polearm" || sub == "whip" ||
		sub == "axe" || sub == "great-axe" || sub == "rapier" || sub == "spear" ||
		sub == "dagger" || sub == "staff" || sub == "mace" || sub == "great-mace" ||
		sub == "club" || sub == "hammer" || sub == "great-hammer" || sub == "bow" ||
		sub == "crossbow" || sub == "thrown"
	) {
		isWeapon = true;
	} else {
		player->print("The object subtype \"%s\" is not an armor or a weapon subtype!\n", sub.c_str());
		return(0);
	}

	CatRef	cr;
	getCatRef(getFullstrText(cmnd->fullstr, 2), &cr, player);

	if(!validObjId(cr)) {
		player->print("Index error: object number invalid.\n");
		player->printColor("Syntax: ^c*clone <object> <area.id> <partial desc>\n");
		return(0);
	}

	if(!player->checkBuilder(cr, false)) {
		player->print("Error: %s out of your allowed range.\n", cr.str().c_str());
		return(0);
	}

	bstring desc = getFullstrText(cmnd->fullstr, 3);
	if(desc == "") {
		player->printColor("Syntax: ^c*clone <object> <area.id> <partial desc>\n");
		player->print("The partial description should complete the sentence, \"It's a <item> _____\".\n");
		return(0);
	}

	// create a random object container for this suit of armor
	random = new Object;
	if(!random) {
		merror("dmClone", NONFATAL);
		player->print("Cannot allocate object.\n");
		return(0);
	}

	strcpy(random->key[0], "random");
	strcpy(random->key[1], object->key[0]);

	if(!object->key[2][0] && !isWeapon) {
		sprintf(random->name, "random %s", random->key[1]);
	} else {
		if(!object->key[2][0])
			strcpy(random->key[2], "weapon");
		else
			strcpy(random->key[2], object->key[1]);
		sprintf(random->name, "random %s %s", random->key[1], random->key[2]);
	}



	if(isArmor) {
		long value = object->value[GOLD];

		// makeArmor expects the value to be the body armor's value. If we're cloning
		// a non-body item, modify the value
		switch(object->getWearflag()) {
		case 17:
			value *= 2;
			break;
		case 3:
		case 19:
			value *= 3;
			break;
		case 2:
		case 5:
		case 6:
		case 7:
		case 8:
			value *= 4;
			break;
		case 4:
			value *= 5;
			break;
		default:
			break;
		}

		if(sub == "cloth") {
			object->setMaterial(CLOTH);
			makeArmor(player, &cr, object, random, BODY, "tunic", "It's a tunic", desc, value, 3, 5);
			makeArmor(player, &cr, object, random, ARMS, "sleeves", "They're some sleeves", desc, value/4, 1, 3, true);
			makeArmor(player, &cr, object, random, LEGS, "pants", "They're some pants", desc, value/3, 2, 4, true);
			makeArmor(player, &cr, object, random, NECK, "scarf", "It's a scarf", desc, value/5, 1, 3);
			makeArmor(player, &cr, object, random, BELT, "belt", "It's a belt", desc, value/4, 1, 3);
			makeArmor(player, &cr, object, random, HANDS, "gloves", "They're some gloves", desc, value/4, 1, 3, true);
			makeArmor(player, &cr, object, random, HEAD, "hat", "It's a hat", desc, value/4, 1, 3);
			makeArmor(player, &cr, object, random, FEET, "shoes", "They're some shoes", desc, value/4, 2, 4, true);
			makeArmor(player, &cr, object, random, FACE, "mask", "It's a mask", desc, value/3, 1, 3);
		} else if(sub == "leather") {
			object->setMaterial(LEATHER);
			makeArmor(player, &cr, object, random, BODY, "armor", "It's armor", desc, value, 5, 8, true);
			makeArmor(player, &cr, object, random, ARMS, "sleeves", "They're some sleeves", desc, value/4, 3, 5, true);
			makeArmor(player, &cr, object, random, LEGS, "leggings", "They're some leggings", desc, value/3, 4, 6, true);
			makeArmor(player, &cr, object, random, NECK, "neckguard", "It's a neckguard", desc, value/5, 3, 5);
			makeArmor(player, &cr, object, random, BELT, "belt", "It's a belt", desc, value/4, 3, 5);
			makeArmor(player, &cr, object, random, HANDS, "gloves", "They're some gloves", desc, value/4, 3, 5, true);
			makeArmor(player, &cr, object, random, HEAD, "cap", "It's a cap", desc, value/4, 3, 5);
			makeArmor(player, &cr, object, random, FEET, "boots", "They're some boots", desc, value/4, 3, 5, true);
			makeArmor(player, &cr, object, random, FACE, "faceguard", "It's a faceguard", desc, value/3, 2, 4);
			makeArmor(player, &cr, object, random, SHIELD, "buckler", "It's a buckler", desc, value/3, 4, 6);
		} else if(sub == "chain") {
			makeArmor(player, &cr, object, random, BODY, "armor", "It's armor", desc, value, 7, 9, true);
			makeArmor(player, &cr, object, random, ARMS, "sleeves", "They're some sleeves", desc, value/4, 5, 7, true);
			makeArmor(player, &cr, object, random, LEGS, "leggings", "They're some leggings", desc, value/3, 5, 7, true);
			makeArmor(player, &cr, object, random, NECK, "neckguard", "It's a neckguard", desc, value/5, 4, 6);
			makeArmor(player, &cr, object, random, BELT, "waistguard", "It's a waistguard", desc, value/4, 4, 6);
			makeArmor(player, &cr, object, random, HANDS, "gloves", "They're some gloves", desc, value/4, 4, 6, true);
			makeArmor(player, &cr, object, random, HEAD, "hood", "It's a hood", desc, value/4, 4, 6);
			makeArmor(player, &cr, object, random, FEET, "boots", "They're some boots", desc, value/4, 4, 6, true);
			makeArmor(player, &cr, object, random, FACE, "faceguard", "It's a faceguard", desc, value/3, 3, 5);
			makeArmor(player, &cr, object, random, SHIELD, "shield", "It's a shield", desc, value/3, 6, 8);
		} else if(sub == "plate") {
			makeArmor(player, &cr, object, random, BODY, "chestplate", "It's armor", desc, value, 10, 15);
			makeArmor(player, &cr, object, random, ARMS, "sleeves", "They're some sleeves", desc, value/4, 6, 8, true);
			makeArmor(player, &cr, object, random, LEGS, "leggings", "They're some leggings", desc, value/3, 6, 8, true);
			makeArmor(player, &cr, object, random, NECK, "neckguard", "It's a neckguard", desc, value/5, 5, 7);
			makeArmor(player, &cr, object, random, BELT, "waistguard", "It's a waistguard", desc, value/4, 5, 7);
			makeArmor(player, &cr, object, random, HANDS, "gauntlets", "They're some gauntlets", desc, value/4, 5, 7, true);
			makeArmor(player, &cr, object, random, HEAD, "helmet", "It's a helmet", desc, value/4, 5, 7);
			makeArmor(player, &cr, object, random, FEET, "boots", "They're some boots", desc, value/4, 5, 7, true);
			makeArmor(player, &cr, object, random, FACE, "faceguard", "It's a faceguard", desc, value/3, 4, 6);
			makeArmor(player, &cr, object, random, SHIELD, "shield", "It's a shield", desc, value/3, 10, 15);
		}
	} else if(isWeapon) {
		double value = object->value[GOLD];
		int baseWeight = object->getActualWeight();
		short numAttacks = object->getNumAttacks();

		if(numAttacks < 1)
			numAttacks = 1;
		// bows get more attacks, so lower it for everyone else
		if(object->getSubType() == "bow" && numAttacks == 2)
			numAttacks = 1;

		if(	sub == "great-sword" || sub == "polearm" || sub == "great-axe" ||
			sub == "great-mace" || sub == "great-hammer"
		) {
			value *= 0.8;
			baseWeight -= 2;
		} else if(sub == "whip" || sub == "dagger") {
			value *= 1.2;
			baseWeight += 2;
		} else if(sub == "bow" || sub == "crossbow") {
			baseWeight += 1;
		}

		makeWeapon(player, &cr, object, random, "sword", "It's a sword", desc, false, baseWeight, value, 4, numAttacks);
		makeWeapon(player, &cr, object, random, "great-sword", "It's a great sword", desc, true, baseWeight+2, value/0.8, 6, numAttacks);
		makeWeapon(player, &cr, object, random, "polearm", "It's a polearm", desc, true, baseWeight+2, value/0.8, 6, numAttacks);
		makeWeapon(player, &cr, object, random, "whip", "It's a whip", desc, false, baseWeight-2, value/1.2, 2, numAttacks);
		makeWeapon(player, &cr, object, random, "axe", "It's an axe", desc, false, baseWeight, value, 4, numAttacks);
		makeWeapon(player, &cr, object, random, "great-axe", "It's a great axe", desc, true, baseWeight+2, value/0.8, 6, numAttacks);
		makeWeapon(player, &cr, object, random, "rapier", "It's a rapier", desc, false, baseWeight, value, 3, numAttacks);
		makeWeapon(player, &cr, object, random, "spear", "It's a spear", desc, true, baseWeight, value, 5, numAttacks);
		makeWeapon(player, &cr, object, random, "dagger", "It's a dagger", desc, false, baseWeight-2, value/1.2, 2, numAttacks);
		makeWeapon(player, &cr, object, random, "staff", "It's a staff", desc, true, baseWeight, value, 5, numAttacks);
		makeWeapon(player, &cr, object, random, "mace", "It's a mace", desc, false, baseWeight, value, 4, numAttacks);
		makeWeapon(player, &cr, object, random, "great-mace", "It's a great mace", desc, true, baseWeight+2, value/0.8, 6, numAttacks);
		makeWeapon(player, &cr, object, random, "club", "It's a club", desc, false, baseWeight, value, 4, numAttacks);
		makeWeapon(player, &cr, object, random, "hammer", "It's a hammer", desc, false, baseWeight, value, 4, numAttacks);
		makeWeapon(player, &cr, object, random, "great-hammer", "It's a great hammer", desc, true, baseWeight+2, value/0.8, 6, numAttacks);
		makeWeapon(player, &cr, object, random, "bow", "It's a bow", desc, true, baseWeight-1, value, 6, numAttacks == 1 ? 2 : numAttacks);
		makeWeapon(player, &cr, object, random, "crossbow", "It's a crossbow", desc, false, baseWeight-1, value, 4, numAttacks);
	}

	random->info = cr;
	random->setType(MISC);
	random->setAdjustment(object->getAdjustment());

	player->printColor("Saving %s to %s... ", random->name, cr.str("", 'W').c_str());
	dmResaveObject(player, random);
	log_immort(2, player, "%s cloned %s into %s.\n",
		player->name, random->name, cr.str().c_str());

	player->addObj(random);

	player->printColor("^YDone!\n");
	return(0);
}

