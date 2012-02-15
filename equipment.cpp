/*
 * equipment.cpp
 *   Functions dealing with players equipment/inventory
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
#include "commands.h"
#include "property.h"
#include "unique.h"
#include "calendar.h"
#include "quests.h"
#include "effects.h"
#include <sstream>


int cmdCompare(Player* player, cmd* cmnd) {

    if(!player->ableToDoCommand())
        return(0);

    if(cmnd->num < 2) {
        *player << "What would you like to compare?\n";
        return(0);
    }
    Object* toCompare = 0;
    Object* compareTo = 0;

    // Attempt to compare to something we're wearing
    toCompare = findObject(player, player->first_obj, cmnd);
    if(!toCompare) {
        *player << "You don't have that in your inventory.\n";
        return(0);
    }

    if(cmnd->num == 2) {
        for(int i=0; i<MAXWEAR; i++) {
            if( player->ready[i] ) {
                if( player->ready[i]->getType() == toCompare->getType()
                        && player->ready[i]->getWearflag() == toCompare->getWearflag())
                {
                    compareTo = player->ready[i];
                    break;
                }
            }
        }
        if(!compareTo) {
            *player << "You're not wearing anything to compare it with!\n";
            return(0);
        }
    } else {
        // cmnd > 2
        compareTo = findObject(player, player->first_obj, cmnd, 2);
        if(!compareTo) {
            *player << "You don't have that in your inventory to compare with!\n";
            return(0);
        }
    }
    if(toCompare->getWearflag() != compareTo->getWearflag() ||
            toCompare->getType() != compareTo->getType()) {
        *player << ColorOn << "You don't know how to compare " << toCompare << " to " << compareTo << "!\n" << ColorOff;
        return(0);
    }
    if(toCompare == compareTo) {
    	*player << ColorOn << "You compare " << compareTo << " to itself...it looks about the same.\n" << ColorOff;
    	return(0);
    }

    if(toCompare->getType() == WEAPON) {
        *player << ColorOn << setf(CAP) << toCompare << " seems ";
        if(toCompare->getDps() > compareTo->getDps()) {
            *player << ColorOn << "^gbetter^x than ";
        } else if(toCompare->getDps() < compareTo->getDps()) {
            *player << ColorOn << "^rworse^x than ";
        } else {
            *player << "about the same as " << ColorOff;
        }
        *player << compareTo << ".\n";
    } else if(toCompare->getType() == ARMOR) {
        *player << ColorOn << setf(CAP) << toCompare << " seems ";
        if(toCompare->getArmor() > compareTo->getArmor()) {
            *player << ColorOn << "^gbetter^x than ";
        } else if(toCompare->getArmor() < compareTo->getArmor()) {
            *player << ColorOn << "^rworse^x than ";
        } else {
            *player << "about the same as ";
        }
        *player << compareTo << ".\n" << ColorOff;
    } else {
        *player << ColorOn << "You don't know how to compare " << toCompare << " to " << compareTo << "!\n" << ColorOff;
    }
    return(0);

}
// Rules for creating a new ownership object for unique items
// 		no - pet gets
// 		yes -get from normal room
// 		yes -get from normal container in room
// 		no - get from storage room
// 		no - get from storage container in room
// 		no - get from own container

//*********************************************************************
//					equip
//*********************************************************************

bool Creature::equip(Object* object, bool showMessage, bool resetUniqueId) {
	Player* player = getAsPlayer();
	bool isWeapon = false;

	switch(object->getWearflag()) {
	case BODY:
	case ARMS:
	case LEGS:
	case HANDS:
	case HEAD:
	case FEET:
	case FACE:
	case HELD:
	case SHIELD:
	case NECK:
	case BELT:
		// handle armor
		ready[object->getWearflag()-1] = object;
		break;
	case FINGER:
		// handle rings
		for(int i=FINGER1; i<FINGER8+1; i++) {
			if(!ready[i-1]) {
				ready[i-1] = object;
				break;
			}
		}
		break;
	case WIELD:
		// handle weapons
		if(!ready[WIELD-1]) {
			// weapons going in the first hand
			if(showMessage) {
				printColor("You wield %1P.\n", object);
				broadcast(getSock(), getRoomParent(), "%M wields %1P.", this, object);
			}
			ready[WIELD-1] = object;
		} else if(!ready[HELD-1]) {
			// weapons going in the off hand
			if(showMessage) {
				printColor("You wield %1P in your off hand.\n", object);
				broadcast(getSock(), getRoomParent(), "%M wields %1P in %s off hand.",
					this, object, hisHer());
			}
			ready[HELD-1] = object;
		} else
			return(false);

		isWeapon = true;
		break;
	default:
		return(false);
	}

	// message for armor/rings
	if(!isWeapon && showMessage) {
		printColor("You wear %1P.\n", object);
		broadcast(getSock(), getRoomParent(), "%M wore %1P.", this, object);
	}


	if(showMessage && object->getType() != CONTAINER && object->use_output[0])
		printColor("%s\n", object->use_output);

	object->setFlag(O_WORN);

	delObj(object, false, false, true, false, true);
	if(resetUniqueId && player)
		player->setObjectId(object);

	if(object->flagIsSet(O_EQUIPPING_BESTOWS_EFFECT)) {
		if(Effect::objectCanBestowEffect(object->getEffect())) {
			if(!isEffected(object->getEffect())) {
				// passing keepApplier = true, which means this effect will continue to point to this object
				addEffect(object->getEffect(), object->getEffectDuration(), object->getEffectStrength(), object, true, this, true);
			}
		} else {
			object->clearFlag(O_EQUIPPING_BESTOWS_EFFECT);
			if(isStaff())
				printColor("^MClearing flag EquipEffect flag from %s^M.\n", object->name);
		}
	}

	return(true);
}

//*********************************************************************
//						unequip
//*********************************************************************

Object* Creature::unequip(int wearloc, UnequipAction action, bool darkness, bool showEffect) {
	wearloc--;
	Object* object = ready[wearloc];
	ready[wearloc] = 0;
	if(object) {
		object->clearFlag(O_WORN);
		
		if(object->flagIsSet(O_EQUIPPING_BESTOWS_EFFECT))
			removeEffect(object->getEffect(), showEffect, true, object);

		// don't run checkDarkness if this isnt a dark item
		if(!object->flagIsSet(O_DARKNESS))
			darkness = false;
		if(action == UNEQUIP_DELETE) {
			Limited::remove(getAsPlayer(), object);
			darkness = object->flagIsSet(O_DARKNESS);
			delete object;
			object = 0;
		} else if(action == UNEQUIP_ADD_TO_INVENTORY) {
			darkness = false;
			addObj(object);
		}
	}
	if(darkness)
		checkDarkness();
	return(object);
}

//*********************************************************************
//						cmdUse
//*********************************************************************
// This function allows a player to use an item without specifying the
// special command for its type. Use determines which type of item
// it is, and calls the appropriate functions.

int cmdUse(Player* player, cmd* cmnd) {
	Object	*object=0;
	BaseRoom* room = player->getRoomParent();
	unsigned long amt=0;

	player->clearFlag(P_AFK);


	if(!player->ableToDoCommand())
		return(0);

	if(cmnd->num < 2) {
		player->print("Use what?\n");
		return(0);
	}

	if(!strcmp(cmnd->str[1], "all"))
		return(cmdWear(player, cmnd));

	object = findObject(player, player->first_obj, cmnd);

	if(!object) {
		object = findObject(player, room->first_obj, cmnd);
		if(!object && player->ready[HELD-1]) {
			object = player->ready[HELD-1];
		} else if(!object || !object->flagIsSet(O_CAN_USE_FROM_FLOOR)) {
			player->print("Use what?\n");
			return(0);
		} else if(object->flagIsSet(O_CAN_USE_FROM_FLOOR)) {
			//onfloor = 1;
			//			cmnd->num = 2;
		}
	}

	if(object->flagIsSet(O_COIN_OPERATED_OBJECT)) {

		amt = object->getCoinCost();

		if(amt <= 0) {
			player->printColor("%O is currently out of order.\n", object);
			return(0);
		}

		if(amt > player->coins[GOLD]) {
			player->printColor("You don't have enough gold to use %P.\n", object);
			return(0);
		}

		player->printColor("You spend %ld gold coin%s on %P.\n", amt,
				amt != 1 ? "s" : "", object);
		broadcast(player->getSock(), room, "%M puts some coins in %P.", player, object);

		gServer->logGold(GOLD_OUT, player, Money(amt, GOLD), object, "CoinOperated");
		player->coins.sub(amt, GOLD);
	}

	player->unhide();


	if(object->flagIsSet(O_FISHING))
		return(cmdHold(player, cmnd));
	if(object->flagIsSet(O_EATABLE) || object->flagIsSet(O_DRINKABLE))
		return(cmdConsume(player, cmnd));

	switch(object->getType()) {
	case WEAPON:
		return(cmdReady(player, cmnd));
	case ARMOR:
		return(cmdWear(player, cmnd));
	case POTION:
		return(cmdConsume(player, cmnd));
	case SCROLL:
		return(cmdReadScroll(player, cmnd));
	case WAND:
		return(cmdUseWand(player, cmnd));
	case KEY:
		return(cmdUnlock(player, cmnd));
	case LIGHTSOURCE:
		return(cmdHold(player, cmnd));
	default:
		player->print("How does one use that?\n");
	}
	return(0);
}

//*********************************************************************
//						doWear
//*********************************************************************
// actual code to wear object

bool doWear(Player* player, cmd* cmnd, int id) {
	Object	*object=0;

	player->clearFlag(P_AFK);

	if(!player->ableToDoCommand())
		return(false);
	if(player->flagIsSet(P_SITTING)) {
		player->print("You must stand to do that.\n");
		return(false);
	}

	if(cmnd && cmnd->num < 2) {
		player->print("Wear what?\n");
		return(false);
	}

	player->unhide();

	if(cmnd && !strcmp(cmnd->str[1], "all")) {
		wearAll(player);
		return(false);
	}

	if(cmnd)
		object = findObject(player, player->first_obj, cmnd);
	else
		object = findObject(player, id);

	if(!object) {
		player->print("You don't have that.\n");
		return(false);
	}

	if(!player->canUse(object) || !player->canWear(object))
		return(false);

	if(object->getWearflag() == SHIELD && !player->canWield(object, SHIELDOBJ))
		return(false);

	object->clearFlag(O_JUST_BOUGHT);
	// if cmnd is set, we want to pass true to resetUniqueId
	player->equip(object, true, cmnd);
	player->computeAC();
	return(true);
}

//*********************************************************************
//						webWear
//*********************************************************************
// web interface for wearing objects

bool webWear(Player* player, int id) {
	return(doWear(player, 0, id));
}

//*********************************************************************
//						cmdWear
//*********************************************************************
// This function allows the player pointed to by the first parameter
// to wear an item specified in the pointer to the command structure
// in the second parameter. That is, if the item is wearable.

int cmdWear(Player* player, cmd* cmnd) {
	doWear(player, cmnd, 0);
	return(0);
}

//*********************************************************************
//						wearCursed
//*********************************************************************

void Player::wearCursed() {
	Object	*object=0;
	otag	*op = first_obj;

	while(op) {
		object = op->obj;
		op = op->next_tag;

		if(object->flagIsSet(O_CURSED) && object->flagIsSet(O_WORN)) {
			if(object->getShotsCur() < 1) {
				object->clearFlag(O_WORN);
			} else {

				if(	ready[WIELD-1] &&
					object->getType() == WEAPON &&
					object->flagIsSet(O_CURSED)
				) {

					if(ready[HELD-1]) {
						ready[HELD-1] = 0;
						object->clearFlag(O_WORN);
						addObj(object);
					}

					delObj(object, false, false, true, false, true);
					ready[HELD-1] = object;
					object->setFlag(O_WORN);


					if(object->getWeight() > ready[WIELD-1]->getWeight()) {
						ready[HELD-1] = ready[WIELD-1];
						ready[WIELD-1] = object;
					}

				} else {
					equip(object, false);
				}

			}
		}
	}
}

//*********************************************************************
//						wear_all
//*********************************************************************
// This function allows a player to wear everything in their inventory that
// they possibly can.

void wearAll(Player* player, bool login) {
	Object	*object=0;
	otag	*op=0;
	char	str[4096], str2[85];
	bool	found=false;
	str[0] = 0;

	if(!login) {
		if(!player->ableToDoCommand()) return;

		if(player->flagIsSet(P_SITTING)) {
			player->print("You must stand to do that.\n");
			return;
		}
	}

	op = player->first_obj;
	while(op) {

		object = op->obj;
		op = op->next_tag;

		if(object->getWearflag() && object->getWearflag() != HELD && object->getWearflag() != WIELD) {

			if(!player->canSee(object))
				continue;

			if(!player->canUse(object, true) || !player->canWear(object, true))
				continue;
			if(object->getWearflag() == SHIELD && player->ready[HELD-1] && !object->flagIsSet(O_SMALL_SHIELD))
				continue;

			if(object->getWearflag() == SHIELD && !player->canWield(object, SHIELDOBJ))
				continue;

			if(!login && object->use_output[0] && object->getType() != CONTAINER && object->getWearflag() != FINGER)
				player->printColor("%s\n", object->use_output);

			object->clearFlag(O_JUST_BOUGHT);
			if(!login) {
				sprintf(str2, "%s, ", object->getObjStr(NULL, 0, 1).c_str());
				strcat(str, str2);
			}
			player->equip(object, false);

			found = true;

		}
	}

	if(found) {
		player->computeAC();
		if(!login) {
			str[strlen(str)-2] = 0;
			player->printColor("You wear %s.\n", str);
			broadcast(player->getSock(), player->getParent(), "%M wears %s.", player, str);
		}
	} else
		if(!login)
			player->print("You have nothing you can wear.\n");

}

//*********************************************************************
//						doRemoveObj
//*********************************************************************
// This function does the heavy lifting of removing objects

bool doRemoveObj(Player* player, cmd* cmnd, int id) {
	Object	*object=0, *second=0;
	int		found=0, match=0, i=0, jumped=0;

	player->clearFlag(P_AFK);

	if(!player->ableToDoCommand())
		return(false);

	if(cmnd && cmnd->num < 2) {
		player->print("Remove what?\n");
		return(false);
	}

	player->unhide();

	i = 0;
	if(!cmnd || cmnd->num > 1) {

		if(cmnd && !strcmp(cmnd->str[1], "all")) {
			remove_all(player);
			return(false);
		}

		while(i<MAXWEAR) {
			object = player->ready[i];
			second = 0;
			jumped=0;
			if(!object) {
				i++;
				continue;
			}
			if(cmnd) {
				if(keyTxtEqual(object, cmnd->str[1])) {
					match++;
					if(match == cmnd->val[1]) {
						found = 1;
						break;
					}
				}
			} else {
				if(object->getUniqueId() == id) {
					found = 1;
					break;
				}
			}
			i++;
		}

		if(!found) {
			player->print("You aren't using that.\n");
			return(false);
		}

		if(	object->flagIsSet(O_CURSED) &&
			object->getShotsCur() > 0 &&
			!player->checkStaff("You can't. It's cursed!\n")
		)
			return(false);


		if(i == (WIELD-1)) {
			if(player->ready[HELD-1] && player->ready[HELD-1]->getType() == WEAPON) {
				second = player->ready[HELD-1];
				// if cmnd is set, pass true to resetUniqueId
				player->addObj(second, cmnd);
				second->clearFlag(O_WORN);
				player->ready[HELD-1] = 0;
			}
		}

		player->doRemove(i, cmnd);
		player->computeAC();
		player->computeAttackPower();

		if(	second &&
			second->flagIsSet(O_CURSED) &&
			second->flagIsSet(O_WORN) &&
			i == (WIELD-1)
		) {

			player->ready[HELD-1] = 0;
			player->ready[WIELD-1] = second;
			player->computeAC();
			player->computeAttackPower();

			if(second->flagIsSet(O_NO_PREFIX)) {
				player->printColor("%s jumped to your primary hand! It's cursed!\n", second->name);
				broadcast(player->getSock(), player->getParent(), "%s jumped to %N's primary hand! It's cursed!",
					second->name, player);
			} else {
				player->printColor("The %s jumped to your primary hand! It's cursed!\n", second->name);
				broadcast(player->getSock(), player->getParent(), "%M's %s jumped to %s primary hand! It's cursed!", player,
					second->name, player->hisHer());
			}

			jumped = 1;
		}

		player->printColor("You removed %1P.\n", object);
		broadcast(player->getSock(), player->getParent(), "%M removed %1P.", player, object);

		if(second && !jumped) {
			player->printColor("You removed %1P.\n", second);
			broadcast(player->getSock(), player->getParent(), "%M removed %1P.", player, second);
		}

		object->clearFlag(O_WORN);

	}

	return(true);
}

//*********************************************************************
//						webRemove
//*********************************************************************

bool webRemove(Player* player, int id) {
	return(doRemoveObj(player, 0, id));
}

//*********************************************************************
//						cmdRemoveObj
//*********************************************************************
// This function allows the player pointed to by the first parameter to/
// remove the item specified by the command structure in the second
// parameter from those things which they are wearing.

int cmdRemoveObj(Player* player, cmd* cmnd) {
	doRemoveObj(player, cmnd, 0);
	return(0);
}

//*********************************************************************
//						doRemove
//*********************************************************************

void Player::doRemove(int i, bool resetUniqueId) {
	i++;
	Object* object = unequip(i, UNEQUIP_NOTHING);
	addObj(object, resetUniqueId);

	if(i == FEET) {
		// Takes some time to remove boots in combat and then kick.
		if(inCombat()) {
			lasttime[LT_KICK].ltime = time(0);
			lasttime[LT_KICK].interval = (cClass == FIGHTER ? 12L:15L);
		}
	}
	if(cClass == FIGHTER && cClass2 == THIEF && object->isHeavyArmor()) {
		updateAttackTimer(true, 70);
		lasttime[LT_STEAL].ltime = time(0);
		lasttime[LT_STEAL].interval = 30;
		lasttime[LT_PEEK].ltime = time(0);
		lasttime[LT_PEEK].interval = 30;
	}
}

//*********************************************************************
//						remove_all
//*********************************************************************
// This function allows the player pointed to in the first parameter
// to remove everything they are wearing all at once.

void remove_all(Player* player) {
	char	str[4096], str2[85];
	int		i, found=0;

	str[0] = 0;
	if(!player->ableToDoCommand())
		return;

	if(player->flagIsSet(P_SITTING)) {
		player->print("You must stand to do that.\n");
		return;
	}

	for(i=0; i<MAXWEAR; i++) {
		if(player->ready[i] && (!(player->ready[i]->flagIsSet(O_CURSED) && player->ready[i]->getShotsCur() > 0))) {
			sprintf(str2,"%s, ", player->ready[i]->getObjStr(NULL, 0, 1).c_str());
			strcat(str, str2);
			player->ready[i]->clearFlag(O_WORN);
			player->doRemove(i);
			found = 1;
		}
	}

	if(!found) {
		player->print("You aren't wearing anything that can be removed.\n");
		return;
	}

	player->computeAC();
	player->computeAttackPower();

	str[strlen(str)-2] = 0;
	broadcast(player->getSock(), player->getParent(), "%M removes %s.", player, str);
	player->printColor("You remove %s.\n", str);

}

//*********************************************************************
//						cmdEquipment
//*********************************************************************
// This function outputs to the player all of the equipment that they are
// wearing/wielding/holding on their body.

int cmdEquipment(Player* player, cmd* cmnd) {
	Player	*target=0;
	int		i=0, found=0;

	player->clearFlag(P_AFK);

	if(!player->ableToDoCommand())
		return(0);

	if(player->isBlind()) {
		player->printColor("^CYou can't see anything! You're blind!\n");
		return(0);
	}

	if(player->isCt()) {
		if(cmnd->num > 1) {
			cmnd->str[1][0] = up(cmnd->str[1][0]);
			target = gServer->findPlayer(cmnd->str[1]);

			if(!target || !player->canSee(target)) {
				player->print("That player is not logged on.\n");
				return(0);
			}

			for(i=0; i<MAXWEAR; i++)
				if(target->ready[i])
					found = 1;

			if(!found) {
				player->print("%s isn't wearing anything.\n", target->upHeShe());
				return(0);
			}

			player->print("%s's worn equipment:\n", target->name);
			target->printEquipList(player);
			return(0);
		}
	}


	for(i=0; i<MAXWEAR; i++)
		if(player->ready[i])
			found = 1;

	if(!found) {
		player->print("You aren't wearing anything.\n");
		return(0);
	}
	player->printEquipList(player);
	return(0);

}

//*********************************************************************
//						printEquipList
//*********************************************************************

void Creature::printEquipList(const Player* viewer) {
	if(ready[BODY-1])
		viewer->printColor("On body:   %1P\n", ready[BODY-1]);
	if(ready[ARMS-1])
		viewer->printColor("On arms:   %1P\n", ready[ARMS-1]);
	if(ready[LEGS-1])
		viewer->printColor("On legs:   %1P\n", ready[LEGS-1]);
	if(ready[NECK-1])
		viewer->printColor("On neck:   %1P\n", ready[NECK-1]);
	if(ready[HANDS-1])
		viewer->printColor("On hands:  %1P\n", ready[HANDS-1]);
	if(ready[HEAD-1])
		viewer->printColor("On head:   %1P\n", ready[HEAD-1]);
	if(ready[BELT-1])
		viewer->printColor("On waist:  %1P\n", ready[BELT -1]);
	if(ready[FEET-1])
		viewer->printColor("On feet:   %1P\n", ready[FEET-1]);
	if(ready[FACE-1])
		viewer->printColor("On face:   %1P\n", ready[FACE-1]);
	if(ready[FINGER1-1])
		viewer->printColor("On finger: %1P\n", ready[FINGER1-1]);
	if(ready[FINGER2-1])
		viewer->printColor("On finger: %1P\n", ready[FINGER2-1]);
	if(ready[FINGER3-1])
		viewer->printColor("On finger: %1P\n", ready[FINGER3-1]);
	if(ready[FINGER4-1])
		viewer->printColor("On finger: %1P\n", ready[FINGER4-1]);
	if(ready[FINGER5-1])
		viewer->printColor("On finger: %1P\n", ready[FINGER5-1]);
	if(ready[FINGER6-1])
		viewer->printColor("On finger: %1P\n", ready[FINGER6-1]);
	if(ready[FINGER7-1])
		viewer->printColor("On finger: %1P\n", ready[FINGER7-1]);
	if(ready[FINGER8-1])
		viewer->printColor("On finger: %1P\n", ready[FINGER8-1]);
	// dual wield
	if(ready[HELD-1]) {
		if(ready[HELD-1]->getWearflag() != WIELD)
			viewer->printColor("Holding:   %1P\n", ready[HELD-1]);
		else if(ready[HELD-1]->getWearflag() == WIELD)
			viewer->printColor("In Hand:   %1P\n", ready[HELD-1]);
	}
	if(ready[SHIELD-1])
		viewer->printColor("Shield:    %1P\n", ready[SHIELD-1]);
	if(ready[WIELD-1])
		viewer->printColor("Wielded:   %1P\n", ready[WIELD-1]);
}

//*********************************************************************
//						cmdReady
//*********************************************************************
// This function allows the player pointed to by the first parameter to
// ready a weapon specified in the second, if it is a weapon.

bool doWield(Player* player, cmd* cmnd, int id) {
	Object	*object=0;

	player->clearFlag(P_AFK);


	if(cmnd && cmnd->num < 2) {
		player->print("Wield what?\n");
		return(false);
	}
	if(!player->ableToDoCommand())
		return(false);

	player->unhide();

	if(!cmnd || cmnd->num > 1) {

		if(cmnd)
			object = findObject(player, player->first_obj, cmnd);
		else
			object = findObject(player, id);

		if(!object) {
			player->print("You don't have that.\n");
			return(false);
		}

		if(object->getWearflag() != WIELD) {
			player->print("You can't wield that.\n");
			return(false);
		}
		if(player->ready[WIELD-1]) {
			player->print("You're already wielding something.\n");
			return(false);
		}
		if(player->ready[HELD-1] && player->ready[HELD-1]->getWearflag() == WIELD) {
			if(!player->isCt() && (object->getWeight() <= player->ready[HELD-1]->getWeight())) {
				player->print("Your primary weapon must be heavier then your secondary weapon.\n");
				return(false);
			}
		}

		// these functions print out their reasons for denial
		if(!player->canUse(object) || !player->canWield(object, WIELDOBJ))
			return(false);

		player->equip(object, true);
		player->computeAttackPower();

		object->clearFlag(O_JUST_BOUGHT);
	}

	return(true);
}

bool webWield(Player* player, int id) {
	return(doWield(player, 0, id));
}

int cmdReady(Player* player, cmd* cmnd) {
	doWield(player, cmnd, 0);
	return(0);
}

//*********************************************************************
//						cmdHold
//*********************************************************************
// This function allows a player to hold an item if it is designated
// as a hold-able item.

int cmdHold(Player* player, cmd* cmnd) {
	Object	*object=0;

	player->clearFlag(P_AFK);

	if(!player->ableToDoCommand())
		return(0);

	if(cmnd->num < 2) {
		player->print("Hold what?\n");
		return(0);
	}

	player->unhide();

	if(cmnd->num > 1) {

		object = findObject(player, player->first_obj, cmnd);

		if(!object) {
			player->print("You don't have that.\n");
			return(0);
		}
		if(object->getWearflag() != HELD) {
			player->print("You can't hold that.\n");
			return(0);
		}


		if(!player->canUse(object, false) || !player->canWield(object, HOLDOBJ))
			return(0);

		if(!player->willFit(object)) {
			player->printColor("%O isn't the right size for you.\n", object);
			return(0);
		}

		if(object->getMinStrength() > player->strength.getCur()) {
			player->printColor("You are currently not strong enough to hold %P.\n", object);
			return(0);
		}

		player->ready[HELD-1] = object;
		player->delObj(object, false, false, true, true, true);

		player->printColor("You hold %1P.\n", object);
		broadcast(player->getSock(), player->getParent(), "%M holds %1P.", player, object);
		if(object->use_output[0] && object->getType() != POTION && object->getType() != CONTAINER && object->getType() != WAND)
			player->printColor("%s\n", object->use_output);

		object->setFlag(O_WORN);
		object->clearFlag(O_JUST_BOUGHT);
	}

	return(0);
}


//*********************************************************************
//						canGetUnique
//*********************************************************************

bool canGetUnique(const Player* player, const Creature* creature, const Object* object, bool print) {

	if(player != creature) {
		// monsters
		if(Unique::is(object)) {
			if(print)
				player->print("%M cannot get that item.\n", creature);
			return(false);
		}
	} else {
		// player
		if(!Unique::canGet(player, object)) {
			if(print)
				player->print("You currently cannot take that limited object.\n");
			return(false);
		}
	}

	return(true);
}

//*********************************************************************
//						doGetObject
//*********************************************************************
// Parameters:	<object> The object bein taken
//				<player> The player getting the item
//				[doLimited=true] Limited item ownership
//				[noSplit=false] Don't split gold items
//				[noQuest=false] Don't do quest code
//				[noMessage=false] Don't show "You now have xx gold pieces"

// Handles getting of objects
// 0 = normal
// 1 = killMortalObjects is run
// 2 = money

int doGetObject(Object* object, Creature* creature, bool doLimited, bool noSplit, bool noQuest, bool noMessage, bool saveOnLimited) {
	// we're being sent either a player or a pet
	//	- creature will be the one getting the object
	//	- player will be the one getting the messages
	Player *player = creature->getPlayerMaster();
	int i = 0;

	if(!player) {
		creature->print("Unable to get item!\n");
		object->addToRoom(creature->getRoomParent());
		return(i);
	}

	if(doLimited) {
		if(Limited::addOwner(player, object) == 2)
			i = 1;
	}

	if(!noQuest)
		fulfillQuest(player, object);

	if(object->getType() == MONEY) {
	    Group* group = player->getGroup(true);

		if(group && group->flagIsSet(GROUP_SPLIT_GOLD) && !noSplit) {
			gServer->logGold(GOLD_IN, player, object->value, object, "GetObject-Split");
			if(!player->autosplit(object->value[GOLD])) {
				player->coins.add(object->value);
			}
		} else {
			player->coins.add(object->value);
			gServer->logGold(GOLD_IN, player, object->value, object, "GetObject");
		}

		delete object;
		if(!noMessage)
			player->print("You now have %s.\n", player->coins.str().c_str());
		player->bug("%s now has %s.\n", player->name, player->coins.str().c_str());
		return(2);

	} else {
		if(saveOnLimited && Limited::is(object))
			player->save(true);
		creature->addObj(object);
	}

	return(i);
}

//*********************************************************************
//						limitedInStorage
//*********************************************************************

bool limitedInStorage(const Player* player, const Object* object, const Property *p, bool print) {
	if(	object &&
		p &&
		Limited::is(object) &&
		!p->isOwner(player->name)
	) {
		if(print)
			player->print("You may only handle limited objects in a storage room if you are the primary owner.\n");
		return(false);
	}
	return(true);
}

//*********************************************************************
//						storageProperty
//*********************************************************************
// runs checks to see if the player is allowed to get/drop/throw

bool storageProperty(const Player* player, const Object* object, Property **p) {
	if(player->inUniqueRoom()) {
		(*p) = gConfig->getProperty(player->parent_rom->info);
		if(*p) {
			// currently, we only care about storage rooms. we don't log gets
			// in any other type of property
			if((*p)->getType() != PROP_STORAGE)
				(*p) = 0;

			// are they allowed to get anything?
			else if(!(*p)->isOwner(player->name) && !(*p)->isPartialOwner(player->name) &&
				!player->checkStaff("You aren't allowed to do that; this is not your storage room.\n")
			)
				return(false);

			if(!limitedInStorage(player, object, (*p), true))
				return(false);
		}
	}
	return(true);
}

//*********************************************************************
//						getPermObj
//*********************************************************************
// This function is called whenever someone picks up a permanent item
// from a room. The item's room-permanent flag is cleared, and the
// inventory-permanent flag is set. Also, the room's permanent
// time for that item is updated.

void getPermObj(Object* object) {
	std::map<int, crlasttime>::iterator it;
	crlasttime* crtm=0;
	Object	*temp_obj;
	UniqueRoom* room=0;
	long	t = time(0);

	object->setFlag(O_PERM_INV_ITEM);
	object->clearFlag(O_PERM_ITEM);

	room = object->parent_room->getAsUniqueRoom();
	if(!room)
		return;

	for(it = room->permObjects.begin(); it != room->permObjects.end() ; it++) {
		crtm = &(*it).second;
		if(!crtm->cr.id)
			continue;
		if(crtm->ltime + crtm->interval > t)
			continue;
		if(!loadObject(crtm->cr, &temp_obj))
			continue;
		if(!strcmp(temp_obj->name, object->name)) {
			crtm->ltime = t;
			delete temp_obj;
			break;
		}
		delete temp_obj;
	}
}

//*********************************************************************
//						getAllObj
//*********************************************************************
// This function allows a player to get the entire contents from a
// container object.  The player is pointed to by the first parameter and
// the container by the second.

void getAllObj(Creature* creature, Object *container) {
	Player	*player = creature->getPlayerMaster();
	Property *p=0;
	Object	*object=0, *last_obj=0;
	otag	*op=0;
	char	str[2048];
	const char *str2;
	int		n=1, found=0, heavy=0;
	bool	doUnique=false, saveLimited=false;
	str[0] = 0;

	// we're being sent either a player or a pet
	//	- creature will be the one getting the object
	//	- player will be the one getting the messages

	if(!player) {
		creature->print("Unable to get all!\n");
		return;
	}

	if(!player->ableToDoCommand())
		return;

	// are they allowed to put objects from this storage room?
	if(!storageProperty(player, 0, &p))
		return;
	doUnique = !container->parent_crt && !p;


	op = container->first_obj;
	while(op) {
		if(	!op->obj->flagIsSet(O_SCENERY) &&
			!op->obj->flagIsSet(O_NO_TAKE) &&
			!op->obj->flagIsSet(O_HIDDEN) &&
			player->canSee(op->obj)
		) {
			object = op->obj;
			op = op->next_tag;
			found++;

			if(player->getWeight() + object->getActualWeight() > player->maxWeight()) {
				heavy++;
				continue;
			}

			if(player->tooBulky(object->getActualBulk())) {
				heavy++;
				continue;
			}

			if(doUnique && !Lore::canHave(player, object)) {
				heavy++;
				continue;
			}

			// on ground and quest
			if(!container->parent_crt && object->getQuestnum()) {
				if(player->questIsSet(object->getQuestnum()-1)) {
					heavy++;
					continue;
				} else
					fulfillQuest(player, object);
			}

			if(!p && !canGetUnique(player, creature, object, false)) {
				heavy++;
				continue;
			}

			if(object->flagIsSet(O_TEMP_PERM)) {
				object->clearFlag(O_PERM_INV_ITEM);
				object->clearFlag(O_TEMP_PERM);
			}
			if(object->flagIsSet(O_PERM_ITEM))
				getPermObj(object);
			del_obj_obj(object, container);
			if(last_obj && last_obj->showAsSame(player, object))
				n++;
			else if(last_obj) {
				str2 = last_obj->getObjStr(NULL, 0, n).c_str();
				if(strlen(str2)+strlen(str) < 2040) {
					strcat(str, str2);
					strcat(str, ", ");
					n = 1;
				}
			}

			if(object->getType() == MONEY) {
				last_obj = 0;
			} else {
				last_obj = object;
			}

			// don't let doGetObject save the player, handle it right here
			saveLimited = saveLimited || Limited::is(object);
			// Ignore quest items
			doGetObject(object, creature, doUnique, false, true, false, false);
		} else
			op = op->next_tag;
	}

	if(found && last_obj) {
		str2 = object->getObjStr(NULL, 0, n).c_str();
		if(strlen(str2)+strlen(str) < 2040)
			strcat(str, str2);
	} else if(!found) {
		player->print("There's nothing in it.\n");
		return;
	}

	if(heavy) {
		if(creature == player)
			player->print("You weren't able to carry everything.\n");
		else
			player->print("%M wasn't able to carry everything.\n", creature);
		if(heavy == found)
			return;
	}

	if(!strlen(str))
		return;

	broadcast(player->getSock(), player->getParent(), "%M gets %s from %1P.", creature, str, container);

	player->bug("%s%s gets %s from %s.\n", player->name, player != creature ? "'s pet" : "", str, container->name);
	if(player == creature)
		player->printColor("You get %s from %1P.\n", str, container);
	else
		player->printColor("%M gets %s from %1P.\n", creature, str, container);

	if(p)
		p->appendLog(player->name, "%s gets %s.", player->name, str);

	if(saveLimited)
		player->save(true);
}

//*********************************************************************
//						get_all_rom
//*********************************************************************
// This function will cause the creature pointed to by the first parameter
// to get everything they are able to see in the room.

void get_all_rom(Creature* creature, char *item) {
	Player	*player = creature->getPlayerMaster();
	BaseRoom* room = creature->getRoomParent();
	Object	*object=0, *last_obj=0;
	otag	*op=0;
	char	str[2048];
	const char *str2;
	int 	n=1, found=0, heavy=0, dogoldmsg=0;

	// we're being sent either a player or a pet
	//	- creature will be the one getting the object
	//	- player will be the one getting the messages

	if(!player) {
		creature->print("Unable to get all!\n");
		return;
	}


	if(!player->ableToDoCommand())
		return;

	last_obj = 0;
	str[0] = 0;

	if(!player->isStaff()) {
	    for(Player* ply : room->players) {
			if(	ply != player &&
				player->canSee(ply) &&
				!ply->flagIsSet(P_HIDDEN) &&
				!player->inSameGroup(ply) &&
				!ply->isStaff())
			{
				player->print("You cannot do that when someone else is in the room.\n");
				return;
			}
		}
	}


	op = room->first_obj;
	while(op && strlen(str)< 2040) {
		if(	!op->obj->flagIsSet(O_SCENERY) &&
			!op->obj->flagIsSet(O_NO_TAKE) &&
			!op->obj->flagIsSet(O_HIDDEN) &&
			player->canSee(op->obj)
		) {
			found++;
			object = op->obj;
			op = op->next_tag;

			if(item && !keyTxtEqual(object, item))
				continue;

			if(player->getWeight() + object->getActualWeight() > player->maxWeight()) {
				heavy++;
				continue;
			}
			if(player->tooBulky(object->getActualBulk())) {
				heavy++;
				continue;
			}

			if(!Lore::canHave(player, object)) {
				heavy++;
				continue;
			}

			if(object->getQuestnum() && player->questIsSet(object->getQuestnum()-1)) {
				heavy++;
				continue;
			}

			if(!canGetUnique(player, creature, object, false)) {
				heavy++;
				continue;
			}


			if(object->flagIsSet(O_TEMP_PERM)) {
				object->clearFlag(O_PERM_INV_ITEM);
				object->clearFlag(O_TEMP_PERM);
			}
			if(object->flagIsSet(O_PERM_ITEM))
				getPermObj(object);
			object->clearFlag(O_HIDDEN);

			object->deleteFromRoom();
			if(last_obj && last_obj->showAsSame(player, object))
				n++;
			else if(last_obj) {
				bstring oStr = last_obj->getObjStr(NULL, 0, n);
				str2 = oStr.c_str();
				if(strlen(str2)+strlen(str) < 2040) {
					strcat(str, str2);
					strcat(str, ", ");
					n=1;
				}
			}
			if(object->getType() == MONEY) {
				bstring strtmp = object->getObjStr(NULL, 0, 1);
				str2 = strtmp.c_str();
				if(strlen(str2)+strlen(str) < 2040) {
					strcat(str, str2);
					strcat(str, ", ");
				}
//				if(player->flagIsSet(P_GOLD_SPLIT)) {
//					if(!player->autosplit(object->value[2]))
//						player->coins[GOLD] += object->value[2];
//				} else
//					player->coins[GOLD] += object->value[2];
//
//				delete object;
				last_obj = 0;
				dogoldmsg = 1;
			} else {
//				player->addObj(object);
				last_obj = object;
			}

			// Don't show "You now have xxx message
			// if killMortalObjects is run, we need to use the previous tag
			// because op might be pointing to invalid memory

			if(doGetObject(object, creature, true, false, false, true) == 1) {
				if(player == creature)
					player->print("You are interrupted from getting anything else.\n");
				else
					player->print("%M was interrupted from getting anything else.\n", creature);
				op = 0;
				break;
			}
		} else {
			op = op->next_tag;
		}
	}

	if(found && last_obj) {
		bstring objStr = last_obj->getObjStr(NULL, 0, n);
		str2 = objStr.c_str();
		if(strlen(str2)+strlen(str) < 2040)
			strcat(str, str2);
	} else if(!found) {
		player->print("There's nothing here.\n");
		return;
	}

	if(dogoldmsg && !last_obj)
		str[strlen(str)-2] = 0;

	if(heavy) {
		if(creature == player)
			player->print("You weren't able to carry everything.\n");
		else
			player->print("%M wasn't able to carry everything.\n", creature);
		if(heavy == found)
			return;
	}

	if(!strlen(str))
		return;

	broadcast(player->getSock(), room, "%M gets %s.", creature, str);

	player->bug("%s%s gets %s.\n", player->name, player != creature ? "'s pet" : "", str);

	if(player == creature)
		player->printColor("You get %s.\n", str);
	else
		player->printColor("%M gets %s.\n", creature, str);

	if(dogoldmsg)
		player->print("You now have %ld gold pieces.\n", player->coins[GOLD]);
}

//*********************************************************************
//						cmdGet
//*********************************************************************
// This function allows players or pets to get things lying on the floor or
// inside another object.

int cmdGet(Creature* creature, cmd* cmnd) {
	Player	*player = creature->getPlayerMaster();
	BaseRoom* room = creature->getRoomParent();
	Object	*object=0, *container=0;
	otag	*cop=0;
	int		 n=0, match=0, ground=0;
	Monster *pet=0;
	Property* p=0;

	// we're being sent either a player or a pet
	//	- creature will be the one getting the object
	//	- player will be the one getting the messages

	if(!player) {
		creature->print("Unable to get!\n");
		return(0);
	}

	if(!player->ableToDoCommand())
		return(0);

	player->clearFlag(P_AFK);

	if(player->getClass() == BUILDER) {
		if(!player->canBuildObjects()) {
			player->print("You are not allowed get items.\n");
			return(0);
		}
		if(!player->checkBuilder(player->parent_rom)) {
			player->print("Error: Room number not inside any of your alotted ranges.\n");
			return(0);
		}
	}

	if(cmnd->num < 2) {
		player->print("Get what?\n");
		return(0);
	}

	player->unhide();



	if(cmnd->num == 2) {

		if(!player->isStaff() && room->flagIsSet(R_SHOP_STORAGE)) {
			player->print("You cannot get anything here.\n");
			return(0);
		}

		if(isGuardLoot(room, player, "%M won't let you take anything.\n"))
			return(0);

		if(player->isBlind()) {
			player->printColor("^CYou can't see to do that!\n");
			return(0);
		}
		if(!strcmp(cmnd->str[1], "all")) {
			if(cmnd->num == 3)
				get_all_rom(creature, cmnd->str[2]);
			else
				get_all_rom(creature, 0);

			return(0);
		}

		object = findObject(player, room->first_obj, cmnd);

		if(!object) {
			player->print("That isn't here.\n");
			return(0);
		}

		if(	!player->canSee(object) &&
			!player->checkStaff("That isn't here.\n") )
			return(0);

		if( (object->flagIsSet(O_NO_TAKE) || object->flagIsSet(O_SCENERY)) &&
			!player->checkStaff("You can't take that!\n") )
			return(0);


		// are they allowed to get objects from this storage room?
		if(!storageProperty(player, object, &p))
			return(0);
		if(!canGetUnique(player, creature, object, true))
			return(0);
		if(!Lore::canHave(player, object, false)) {
			player->print("You already have enough of those.\nYou cannot currently have any more.\n");
			return(0);
		}
		if(!Lore::canHave(player, object, true)) {
			player->print("You cannot get that item.\nIt contains a limited item that you cannot carry.\n");
			return(0);
		}


		if(player->getLevel() < object->getLevel() && object->getQuestnum() > 0 &&
			!player->checkStaff("You are not high enough level to pick that up yet.\n")
		)
			return(0);

		if( (player->getWeight() + object->getActualWeight()) > player->maxWeight() &&
			!player->checkStaff("You can't carry anymore.\n")
		)
			return(0);

		if( player->tooBulky(object->getActualBulk()) &&
			!player->checkStaff("That is too bulky to fit in your inventory right now.\n")
		)
			return(0);

		if( object->getQuestnum() &&
			player->questIsSet(object->getQuestnum()-1) &&
			!player->checkStaff("You may not take that. You have already fulfilled that quest.\n")
		)
			return(0);



		cop = object->first_obj;
		while(cop) {
			if( cop->obj->getQuestnum() && player->questIsSet(cop->obj->getQuestnum()-1) &&
				!player->checkStaff("You may not take that.\nIt contains an item for a quest you have already completed.\n"))
				return(0);
			cop = cop->next_tag;
		}

		if(object->flagIsSet(O_TEMP_PERM)) {
			object->clearFlag(O_PERM_INV_ITEM);
			object->clearFlag(O_TEMP_PERM);
		}

		if(object->flagIsSet(O_PERM_ITEM))
			getPermObj(object);


		if(player == creature && !player->isStaff()) {
			// The following keeps misted vampires from
			// stealing loot off the ground from people. -TC
			player->unmist();

			// Mist timer always reset when getting something
			if(player->isEffected("vampirism")) {
				player->lasttime[LT_MIST].ltime = time(0);
				player->lasttime[LT_MIST].interval = 10L;
			}
		}


		object->clearFlag(O_HIDDEN);

		if(player == creature) {
			player->printColor("You get %1P.\n", object);
		} else
			player->printColor("%M gets %1P.\n", creature, object);

		player->bug("%s%s gets %s.\n", player->name, player != creature ? "'s pet" : "", object->name);

		if(object->flagIsSet(O_SILVER_OBJECT) && player->isEffected("lycanthropy")) {
			if(player == creature)
				player->printColor("%O burns you and you drop it!\n", object);
			else
				player->printColor("%O burns %N and %s drop it!\n", object, creature, creature->heShe());


			broadcast(player->getSock(), room, "%M is burned by %P.", creature, object);

			player->bug("%s%s burned %s and %s dropped it.\n",
				player->name, player != creature ? "'s pet" : "", object->name, creature->heShe());
			//player->delObj(object, false, true);
			//object->addToRoom(room);
			return(0);
		}

		object->deleteFromRoom();

		broadcast(player->getSock(), room, "%M gets %1P.", creature, object);

		doGetObject(object, creature);

	} else {

		container = findObject(player, creature->first_obj, cmnd, 2);
		if(!container) {
			container = findObject(player, room->first_obj, cmnd, 2);
			if(container) {
				if(!player->isStaff() && room->flagIsSet(R_SHOP_STORAGE)) {
					player->print("You cannot get anything here.\n");
					return(0);
				}
				ground = true;
			}
		}

		// only check wear for players
		if(player == creature && (!container || !cmnd->val[2])) {
			for(n=0; n < MAXWEAR; n++) {
				if(!player->ready[n])
					continue;
				if(keyTxtEqual(player->ready[n], cmnd->str[2]))
					match++;
				else
					continue;
				if(match == cmnd->val[2] || !cmnd->val[2]) {
					container = player->ready[n];
					break;
				}
			}
		}

		object = 0;
		pet = 0;

		if(!container && player == creature) {
			// check for pets
			pet = room->findMonster(player, cmnd, 2);
			if(pet) {
				if(!player->findPet(pet)) {
					player->print("%M is not your pet!\n", pet);
					return(0);
				}
				object = findObject(pet, pet->first_obj, cmnd);
				if(!object) {
					player->print("%M doesn't have %s.\n", pet, mrand(0, 1) ? "that" : "one of those");
					return(0);
				}
			}
		}

		if(!object) {
			pet = 0;
			if(!container) {
				player->print("That isn't here.\n");
				return(0);
			}

			if(container->getType() != CONTAINER) {
				player->print("That isn't a container.\n");
				return(0);
			}

			if(ground && isGuardLoot(room, player, "%M won't let you take anything.\n"))
				return(0);


			if(!strcmp(cmnd->str[1], "all")) {
				if(player->flagIsSet(P_NO_GET_ALL)) {
					player->print("You have lost the privilage to get all for now.\n");
					return(0);
				}

				getAllObj(creature, container);
				return(0);
			}

			object = findObject(player, container->first_obj, cmnd);
			if(!object) {
				player->print("That isn't in there.\n");
				return(0);
			}

			// are they allowed to put objects from this storage room?
			if(!storageProperty(player, object, &p))
				return(0);
		}

		bool doUnique = !p && !pet && ground;

		if(doUnique && !canGetUnique(player, creature, object, true))
			return(0);

		// if the container is on the ground, we have to prevent them from
		// getting quest items
		if(ground && object->getQuestnum()) {
			if(player->questIsSet(object->getQuestnum()-1)) {
				if(!player->checkStaff("You may not take that. You have already fulfilled that quest.\n"))
					return(0);
			} else {
				fulfillQuest(player, object);
			}
		}


// TODO: Dom: add monster weight check so we don't have monsters carrying tons of items!
// right now they are forced to obey player's weight/bulk rules

		// Weight has already been taken into account if it's in a container in the inventory,
		// but not if it is in the room or on the pet
		if( player->getWeight() + object->getActualWeight() > player->maxWeight() &&
			((container && container->parent_room) || pet) &&
			!player->checkStaff("You can't carry anymore.\n")
		) return(0);

		// Make sure it's not too bulky to fit in the player's inventory
		if(player->tooBulky(object->getActualBulk()) &&
			!player->checkStaff("That is too bulky to fit in your inventory right now.\n"))
			return(0);

		if(ground && !pet && !p) {
			if(!Lore::canHave(player, object, false)) {
				player->print("You already have enough of those.\nYou cannot currently have any more.\n");
				return(0);
			}
			if(!Lore::canHave(player, object, true)) {
				player->print("You cannot get that item.\nIt contains a limited item that you cannot carry.\n");
				return(0);
			}
		}

		if(object->flagIsSet(O_PERM_ITEM))
			getPermObj(object);

		if(pet) {
			pet->delObj(object);
			player->printColor("You get %1P from %N.\n", object, pet);
			broadcast(player->getSock(), room, "%M gets %1P from %N.", player, object, pet);
		} else {
			del_obj_obj(object, container);
			if(player == creature)
				player->printColor("You get %1P from %1P.\n", object, container);
			else
				player->printColor("%M gets %1P from %1P.\n", creature, object, container);
			broadcast(player->getSock(), room, "%M gets %1P from %1P.", creature, object, container);

			player->bug("%s%s get's %s from %s.\n", player->name, player != creature ? "'s pet" : "", object->name, container->name);
		}
		if(p)
			p->appendLog(player->name, "%s gets %s.", player->name, object->getObjStr(player, player->displayFlags(), 1).c_str());

		// Don't ignore split, Do ignore quest
		doGetObject(object, creature, doUnique, false, true);
	}
	return(0);
}

//*********************************************************************
//						cmdInventory
//*********************************************************************
// This function outputs the contents of a player's inventory.

int cmdInventory(Player* player, cmd* cmnd) {
	Player	*target = player;
	otag	*op=0;
	int		m=0, n=0, flags = player->displayFlags();
	bool	online=true;
	std::ostringstream oStr;


	player->clearFlag(P_AFK);

	if(!player->ableToDoCommand())
		return(0);

	if(player->isCt()) {
		if(cmnd->num>1) {
			cmnd->str[1][0] = up(cmnd->str[1][0]);
			target = gServer->findPlayer(cmnd->str[1]);

			if(!target) {
				online = false;
				if(!loadPlayer(cmnd->str[1], &target)) {
					player->print("Player does not exist.\n");
					return(0);
				}
			}

			if(target->isDm() && !player->isDm()) {
				player->print("You are not allowed to do that.\n");
				if(!online)
					free_crt(target);
				return(0);
			}

			if(cmnd->num > 2) {
				peek_bag(player, target, cmnd, 1);
				if(!online)
					free_crt(target);
				return(0);
			}
		}
	}

	if(target != player && player->isBlind()) {
		player->printColor("^CYou can't do that! You're blind!\n");
		return(0);
	}

	if(player == target)
		oStr << "You have: ";
	else
		oStr << target->name << "'s inventory: ";

	op = target->first_obj;

	while(op) {
		if(player->canSee(op->obj)) {
			m = 1;
			while(op->next_tag) {
				if(op->obj->showAsSame(player, op->next_tag->obj)) {
					m++;
					op = op->next_tag;
				} else
					break;
			}

			if(n)
				oStr << ", ";
			oStr << op->obj->getObjStr(player, flags, m);

			if(op->obj->flagIsSet(O_KEEP))
				oStr << "(K)";
			if(op->obj->flagIsSet(O_BEING_PREPARED))
				oStr << "(P)";
			n++;
		}
		op = op->next_tag;
	}


	if(!n)
		oStr << "nothing";

	oStr << ".\n";
	player->printColor("%s", oStr.str().c_str());

	if(!online)
		free_crt(target);

	/*
		strcpy(prepstr, "");
		pp = player->first_obj;
		while(pp)
		{
			if(pp->obj->flagIsSet(O_BEING_PREPARED)) {
				strcat(prepstr, pp->obj->name);
				strcat(prepstr, ", ");
				pcount++;
			}
			pp = pp->next_tag;
		}
		prepstr[strlen(prepstr) - 2] = '.';
		prepstr[strlen(prepstr) - 1] = 0;

		if(pcount)
			player->print("\nYou are preparing: %s\n", prepstr);
			*/

	return(0);
}

//*********************************************************************
//						canDrop
//*********************************************************************

bool canDrop(const Player* player, const Object* object, const Property* p) {
	otag	*op=0;

	if(!limitedInStorage(player, object, p, false))
		return(false);
	if(object->flagIsSet(O_NO_DROP) && !player->isStaff())
		return(false);
	if(object->flagIsSet(O_KEEP))
		return(false);

	op = object->first_obj;
	while(op) {
		if(!canDrop(player, op->obj, p))
			return(false);
		op = op->next_tag;
	}
	return(true);
}

//*********************************************************************
//						delete_drop_obj
//*********************************************************************

bool delete_drop_obj(const BaseRoom* room, const Object* object, bool factionCanRecycle) {

	if(room->flagIsSet(R_DUMP_ROOM) && factionCanRecycle)
		return(true);

	if(object->flagIsSet(O_BREAK_ON_DROP)) {
		broadcast(NULL, room, "%O shattered and turned to dust!", object);
		return(true);
	}

	return(false);
}

//*********************************************************************
//						dropAllRoom
//*********************************************************************

void dropAllRoom(Creature* creature, Player *player, bool factionCanRecycle) {
	Player	*pCreature = creature->getAsPlayer();
	int		money=0, flags=0, m=0, n=0;
	otag	*op=0, *oprev=0;
	BaseRoom* room = creature->getRoomParent();
	Object	*object=0;
	Property* p=0;
	bool	first=false;
	bstring txt = "";

	// we're being sent either a player or a pet
	//	- creature will be the one dropping the object
	//	- player will be the one getting the messages
	if(!player) {
		creature->print("Unable to drop all!\n");
		return;
	}
	flags = player->displayFlags();

	if(!storageProperty(player, 0, &p))
		return;

	if(room->isDropDestroy()) {
		if(player == creature)
			player->print("Surely you would lose your entire inventory if you did that here!\n");
		else
			player->print("Surely %N would lose %s entire inventory if %s did that here!\n",
				creature, creature->hisHer(), creature->heShe());
		return;
	}

	if(room->flagIsSet(R_DUMP_ROOM) && !factionCanRecycle) {
		player->print("The shopkeeper refuses to recycle those for you.\n");
		return;
	}

	op = creature->first_obj;
	while(op) {
		if(!canDrop(player, op->obj, p)) {
			op = op->next_tag;
			continue;
		}
		if(player->canSee(op->obj)) {
			m=1;
			while(op->next_tag) {
				if(op->obj->showAsSame(player, op->next_tag->obj) && canDrop(player, op->obj, p)) {
					m++;
					op = op->next_tag;
				} else
					break;
			}
			if(first)
				txt += ", ";
			first = true;
			txt += op->obj->getObjStr(player, flags, m);
			n++;
		}
		op = op->next_tag;
	}

	if(n) {
		txt += ".";
	} else {
		if(player == creature)
			player->print("You don't have anything you can drop.\n");
		else
			player->print("%M doesn't have anything %s can drop.\n", creature, creature->heShe());
		return;
	}


	op = creature->first_obj;
	while(op) {
		oprev = op;
		op = op->next_tag;
		object = oprev->obj;

		if(!canDrop(player, object, p))
			continue;

		if(delete_drop_obj(room, object, factionCanRecycle)) {
			if(room->flagIsSet(R_DUMP_ROOM))
				money += 5;

			creature->delObj(object, true, false, true, false);
			// players save last pawn, monsters delete right away
			if(pCreature)
				pCreature->setLastPawn(object);
			else
				delete object;
		} else {
			// if in storage room, keep ownership
			creature->delObj(object, false, true, true, false);
			object->addToRoom(room);
		}
	}
	creature->checkDarkness();

	if(player == creature)
		player->printColor("You drop: %s\n", txt.c_str());
	else
		player->printColor("%M drops: %s\n", creature, txt.c_str());
	broadcast(player->getSock(), room, "%M drops %s", creature, txt.c_str());

	if(money) {
		player->coins.add(money, GOLD);
		player->print("Thank you for recycling!\nYou now have %s.\n", player->coins.str().c_str());
		gServer->logGold(GOLD_IN, player, Money(money, GOLD), NULL, "RecycleAll");
	}

	player->bug("%s%s dropped %s.\n", player->name, player != creature ? "'s pet" : "", txt.c_str());

	if(!player->isDm()) {
		log_immort(false, player, "%s%s dropped %s in room %s\n", player->name,
			player != creature ? "'s pet" : "", txt.c_str(), room->fullName().c_str());
	}

	player->save(true);
}

//*********************************************************************
//						canDropAllObj
//*********************************************************************

bool canDropAllObj(Object* object, Object* container) {
	if(!container->flagIsSet(O_BULKLESS_CONTAINER)) {
		if(!container->getMaxbulk()) {
			if(object->getActualBulk() > container->getActualBulk())
				return(false);
		} else {
			if(object->getActualBulk() > container->getMaxbulk())
				return(false);
		}
	}

	if(object->getType() == CONTAINER)
		return(false);

	if(object->getSize() && container->getSize() && object->getSize() > container->getSize())
		return(false);

	if(container->getShotsCur() >= container->getShotsMax())
		return(false);

	// cannot put NODROP items into normal bags
	if(!container->flagIsSet(O_DEVOURS_ITEMS) && object->flagIsSet(O_NO_DROP))
		return(false);

	return(true);
}

//*********************************************************************
//						dropAllObj
//*********************************************************************
// This function drops all the items in a player's inventory into a
// container object, if possible.  The player is pointed to by the first
// parameter, and the container by the second.

void dropAllObj(Creature* creature, Object *container, Property *p) {
	Player	*player = creature->getPlayerMaster();
	Object	*object=0, *last=0;
	BaseRoom* room = creature->getRoomParent();
	otag	*op=0;
	int		n=1, found=0, full=0;
	bstring txt = "";
	bool	removeUnique = !container->parent_crt && !p;

	// we're being sent either a player or a pet
	//	- creature will be the one getting the object
	//	- player will be the one getting the messages

	if(!player) {
		creature->print("Unable to drop all!\n");
		return;
	}

	if(!player->ableToDoCommand())
		return;

	op = creature->first_obj;
	while(op) {
		if(creature->canSee(op->obj) && op->obj != container) {
			found++;
			object = op->obj;
			op = op->next_tag;


			if(!canDropAllObj(object, container)) {
				full++;
				continue;
			}


			container->incShotsCur();
			creature->delObj(object, false, false, true, false);

			// broadcast for devouring items
			if(container->flagIsSet(O_DEVOURS_ITEMS))
				Limited::remove(player, object, false);
			else if(removeUnique)
				Limited::deleteOwner(player, object, false, true);

			container->addObj(object);
			if(last && last->showAsSame(player, object))
				n++;
			else if(last) {
				txt += last->getObjStr(player, 0, n);
				txt += ", ";
				n = 1;
			}
			last = object;
		} else
			op = op->next_tag;
	}
	player->checkDarkness();

	if(found && last) {
		txt += last->getObjStr(player, 0, n);
	} else {
		if(player == creature)
			player->print("You don't have anything to put into it.\n");
		else
			player->print("%M doesn't have anything to put into it.\n", creature);
		return;
	}

	if(full)
		player->printColor("%O couldn't hold everything.\n", container);

	if(full == found)
		return;


	if(container->flagIsSet(O_DEVOURS_ITEMS)) {
		otag	*cop=0;
		op = container->first_obj;
		while(op) {
			delete op->obj;
			cop = op;
			op = op->next_tag;
			delete cop;
		}
	}


	container->clearFlag(O_BEING_PREPARED);
	broadcast(player->getSock(), room, "%M put %s into %1P.", creature, txt.c_str(), container);

	player->bug("%s%s dropped %s into %s.\n",
		player->name, player != creature ? "'s pet" : "", txt.c_str(), container->name);

	if(player == creature)
		player->printColor("You put %s into %1P.\n", txt.c_str(), container);
	else
		player->printColor("%M put %s into %1P.\n", creature, txt.c_str(), container);
	if(p)
		p->appendLog(player->name, "%s stores %s.", player->name, txt.c_str());

	if(container->flagIsSet(O_DEVOURS_ITEMS))
		broadcast(NULL, room, "%O devours everything!", container);

	player->save(true);
}

//*********************************************************************
//						finishDropObject
//*********************************************************************

void finishDropObject(Object* object, BaseRoom* room, Creature* player, bool cash, bool printPlayer, bool printRoom) {
	Socket* sock = 0;
	if(player)
		sock = player->getSock();

	if(room->isDropDestroy()) {
		const AreaRoom* aRoom = room->getAsConstAreaRoom();

		if(room->flagIsSet(R_EARTH_BONUS)) {
			if(printPlayer) {
				if(!cash)
					player->printColor("%O was swallowed by the earth!!\n", object);
				else
					player->print("Your gold was swallowed by the earth!!\n", object);
			}
			if(printRoom)
				broadcast(0, room, "The earth swallowed it!");
		} else if(room->flagIsSet(R_AIR_BONUS)) {
			if(printPlayer) {
				if(!cash)
					player->printColor("%O was frozen solid and destroyed!!\n", object);
				else
					player->print("Your gold frozen solid and destroyed!!\n", object);
			}
			if(printRoom)
				broadcast(0, room, "It froze completely and was destroyed!");
		} else if(room->flagIsSet(R_FIRE_BONUS)) {
			if(printPlayer) {
				if(!cash)
					player->printColor("%O was engulfed in flames and incinerated!!\n", object);
				else
					player->print("Your gold was engulfed in flames and melted away!!\n", object);
			}
			if(printRoom)
				broadcast(0, room, "It was incinerated by flames!");
		} else if(room->flagIsSet(R_WATER_BONUS) || (aRoom && aRoom->isWater())) {
			if(printPlayer) {
				if(!cash)
					player->printColor("%O was swept away in the strong current!!\n", object);
				else
					player->print("Your gold is swept away by the strong current!!\n", object);
			}
			if(printRoom)
				broadcast(0, room, "The current swept it away!");
		} else if(room->flagIsSet(R_ELEC_BONUS)) {
			if(printPlayer) {
				if(!cash)
					player->printColor("%O was destroyed by electricity!!\n", object);
				else
					player->print("Your gold is melted by electricity!!\n", object);
			}
			if(printRoom)
				broadcast(0, room, "It was destroyed by electricity!");
		} else if(room->flagIsSet(R_COLD_BONUS) || room->isWinter()) {
			if(printPlayer) {
				if(!cash)
					player->printColor("%O freezes solid and shatters!!\n", object);
				else
					player->print("Your gold freezes solid and shatters!!\n", object);
			}
			if(printRoom)
				broadcast(0, room, "It froze solid and shattered!");
		} else {
			if(printPlayer) {
				if(!cash)
					player->printColor("%O shatters and turns to dust!\n", object);
				else
					player->print("Your gold shatters and turns to dust!\n", object);
			}
			if(printRoom)
				broadcast(0, room, "It shattered and turned to dust!");
		}

		if(player)
			Limited::remove(player->getAsPlayer(), object);
		delete object;
	} else if(object->flagIsSet(O_BREAK_ON_DROP) && (!player || !player->isStaff())) {
		if(printPlayer)
			player->printColor("%O shatters and turns to dust!\n", object);
		if(printRoom)
			broadcast(sock, room, "It shattered and turned to dust!");

		if(player)
			Limited::remove(player->getAsPlayer(), object);
		delete object;
	} else {
		if(player) {
			if(!player->isPet())
				Limited::deleteOwner(player->getAsPlayer(), object);
			else
				Lore::remove(player->getAsPlayer(), object, true);
		}

		object->addToRoom(room);
	}
}

//*********************************************************************
//						containerOutput
//*********************************************************************

void containerOutput(const Player* player, const Object* container, const Object* object) {
	if(!container->use_output[0])
		return;
	bstring output = container->use_output;

	output.Replace("*OBJECT-UPPER*", object->getObjStr(NULL, CAP, 0).c_str());
	output.Replace("*OBJECT*", object->getObjStr(NULL, 0, 0).c_str());

	player->printColor("%s\n", output.c_str());
}

//*********************************************************************
//						cmdDrop
//*********************************************************************
// This function allows the player pointed to by the first parameter
// to drop an object in the room at which they are located.

int cmdDrop(Creature* creature, cmd* cmnd) {
	Player	*player = creature->getPlayerMaster();
	BaseRoom* room = creature->getRoomParent();
	Object	*object=0, *container=0;
	otag	*op=0;
	int		n=0, match=0, in_room=0;
	unsigned long cash=0;
	bool	dmAlias=false, is_pet=false, factionCanRecycle=true, created=false;
	Property *p=0;

// TODO: check to see if this extra aliasing check is even needed - won't
// print handle it?

	// we're being sent either a player or a pet
	//	- creature will be the one getting the object
	//	- player will be the one getting the messages

	if(!player) {
		creature->print("Unable to drop!\n");
		return(0);
	}

	if(!player->ableToDoCommand())
		return(0);

	player->clearFlag(P_AFK);


	if(cmnd->num < 2) {
		player->print("Drop what?\n");
		return(0);
	}

	if(player->getClass() == BUILDER) {
		if(!player->canBuildObjects()) {
			player->print("You are not allowed drop items.\n");
			return(0);
		}
		if(!player->checkBuilder(player->parent_rom)) {
			player->print("Error: Room number not inside any of your alotted ranges.\n");
			return(0);
		}
	}

	// a dm possessing a monster - only if they arent using a pet
	if(player != creature && player->flagIsSet(P_ALIASING)) {
		dmAlias = true;
		creature = player->getAlias();
	}

	// with an extra rule, let's simplify the check to see if a pet is doing it
	is_pet = !dmAlias && player != creature;


	player->unhide();
	factionCanRecycle = !player->parent_rom || Faction::willDoBusinessWith(player, player->parent_rom->getFaction());

	if(cmnd->num == 2) {

		if(!player->isStaff() && room->flagIsSet(R_SHOP_STORAGE)) {
			player->print("You cannot drop anything here.\n");
			return(0);
		}

		if(!strcmp(cmnd->str[1], "all")) {
			dropAllRoom(creature, player, factionCanRecycle);
			return(0);
		}

		// drop gold
		if(cmnd->str[1][0] == '$') {

			// pets don't carry gold, so they can't drop any
			if(is_pet) {
				player->print("%M isn't carrying any gold!\n", creature);
				return(0);
			}

			cash = strtoul(cmnd->str[1]+1, 0, 0);
			if(cash > 0 && cash <= player->coins[GOLD]) {
				loadObject(MONEY_OBJ, &object);
				object->nameCoin("gold", cash);
				object->value.set(cash, GOLD);
				player->coins.sub(cash, GOLD);
				gServer->logGold(GOLD_OUT, player, Money(cash, GOLD), NULL, "DropGold");
				object->setDroppedBy(player, "DropGold");
				created = true;
			} else {
				player->print("You don't have that much!\n");
				return(0);
			}
		} else
			object = findObject(player, is_pet ? creature->first_obj : player->first_obj, cmnd);

		if(!object) {
			player->print("You don't have that.\n");
			return(0);
		}

		if(object->flagIsSet(O_KEEP)) {
			player->printColor("%O is currently in safe keeping.\nYou must unkeep it to drop it.\n", object);
			if(created)
				delete object;
			return(0);
		}

		if(object->getType() == CONTAINER && room->flagIsSet(R_DUMP_ROOM) && !cantDropInBag(object)) {
			if(object->first_obj) {
				player->print("You don't want to drop that here!\nThere's something inside it!\n");
				if(created)
					delete object;
				return(0);
			}
		}

		if(!storageProperty(player, object, &p)) {
			if(created)
				delete object;
			return(0);
		}

		if(!room->flagIsSet(R_DUMP_ROOM)) {
			// no dropping uniques on the ground or it will reset
			// the decay timer
			if(	object->flagIsSet(O_NO_DROP) &&
				!player->checkStaff("You cannot drop that.\n")
			) {
				if(created)
					delete object;
				return(0);
			}

			op = object->first_obj;
			while(op) {
				if(	op->obj->flagIsSet(O_NO_DROP) &&
					!player->checkStaff("You cannot drop that. It contains %P.\n", op->obj)
				) {
					if(created)
						delete object;
					return(0);
				}
				op = op->next_tag;
			}
		}

		if(room->flagIsSet(R_DUMP_ROOM) && !factionCanRecycle) {
			player->print("The shopkeeper refuses to recycle that for you.\n");
			if(created)
				delete object;
			return(0);
		}

		if(!cash) {
			if(!is_pet) {
				player->delObj(object);
			} else {
				creature->delObj(object);
			}
		}

		if(!is_pet)
			player->printColor("You drop %1P.\n", object);
		else
			player->printColor("%M drops %1P.\n", creature, object);



		broadcast(player->getSock(), room, "%M dropped %1P.", creature, object);

		player->bug("%s%s dropped %s.\n", player, is_pet ? "'s pet" : "", object);

		if(!player->isDm())
			log_immort(false,player, "%s%s dropped %s in room %s\n", player->name,
				is_pet ? "'s pet" : "", object->name, room->fullName().c_str());

		if(!room->flagIsSet(R_DUMP_ROOM)) {

			finishDropObject(object, room, player, cash, true, true);

		} else {
			object->refund.zero();
			// can't pawn starting objects for money
			if(!object->flagIsSet(O_STARTING)) {
				if(created || object->flagIsSet(O_NO_PAWN) || (object->flagIsSet(O_BREAK_ON_DROP) && object->getType() != BANDAGE && object->getType() != LOTTERYTICKET)) {
					player->coins.add(object->value);
					gServer->logGold(GOLD_IN, player, object->value, object, "Recycle");
					object->refund = object->value;
				} else {
					player->coins.add(5, GOLD);
					gServer->logGold(GOLD_IN, player, Money(5, GOLD), object, "Recycle");
					object->refund.add(5, GOLD);
				}
			}

			player->print("Thanks for recycling.\n");
			if(!object->flagIsSet(O_STARTING))
				player->print("You have %s.\n", player->coins.str().c_str());

			Limited::remove(player, object);
			player->setLastPawn(object);
		}

	} else {

		container = findObject(player, is_pet ? creature->first_obj : player->first_obj, cmnd, 2);

		if(!container) {
			container = findObject(player, room->first_obj, cmnd, 2);
			if(container) {
				if(!player->isStaff() && room->flagIsSet(R_SHOP_STORAGE)) {
					player->print("You cannot drop anything here.\n");
					return(0);
				}
				// are they allowed to put objects from this storage room?
				if(!storageProperty(player, object, &p))
					return(0);
				in_room = true;
			}
		}

		if(!is_pet && (!container || !cmnd->val[2])) {
			for(n=0; n<MAXWEAR; n++) {
				if(!player->ready[n])
					continue;
				if(keyTxtEqual(player->ready[n], cmnd->str[2]))
					match++;
				else
					continue;
				if(match == cmnd->val[2] || !cmnd->val[2]) {
					container = player->ready[n];
					break;
				}
			}
		}

		if(!container) {
			player->print("You don't see that here.\n");
			return(0);
		}

		if(container->getType() != CONTAINER) {
			player->print("That isn't a container.\n");
			return(0);
		}



		if(!strcmp(cmnd->str[1], "all")) {
			dropAllObj(player, container, p);
			return(0);
		}

		object = findObject(player, is_pet ? creature->first_obj : player->first_obj, cmnd);

		if(!object) {
			player->print("You don't have that.\n");
			return(0);
		}

		if(object == container) {
			player->print("You can't put something in itself!\n");
			return(0);
		}

		if(container->getShotsCur() >= container->getShotsMax()) {
			player->printColor("%O can't hold anymore.\n", container);
			return(0);
		}

		if(object->getSize() && container->getSize() && object->getSize() > container->getSize()) {
			player->print("That won't fit in there!\n");
			return(0);
		}

		if(object->getType() == CONTAINER) {
			player->print("You can't put containers into containers.\n");
			return(0);
		}

		// no putting 2 items with the same quest in a bag
		if(object->getQuestnum()) {
			op = container->first_obj;
			while(op) {
				if(	op->obj->getQuestnum() == object->getQuestnum() &&
					!player->checkStaff("You cannot put that in there.\nIt already contains %P.\n", op->obj)
				)
					return(0);
				op = op->next_tag;
			}
		}

		if(container->getSubType() == "mortar") {
			if(object->getType() != HERB && !player->checkStaff("You can only put herbs in there!\n"))
				return(0);

			op = container->first_obj;
			while(op) {
				if(!strcmp(object->name, op->obj->name) &&
				   !player->checkStaff("You can only put one %s in there.\n", object->name))
					return(0);

				op = op->next_tag;
			}
		}
		if(	object->flagIsSet(O_NO_DROP) &&
			in_room &&
			!player->checkStaff("You cannot put %P in there.\n", object)
		)
			return(0);

		if(container->flagIsSet(O_DEVOURS_ITEMS)) {
			if(container->use_output[0])
				containerOutput(player, container, object);
			else
				player->printColor("%1O is devoured by %1P!\n", object, container);
			broadcast(player->getSock(), room, "%M put %1P in %1P.", creature, object, container);

			if(container->info.id == 2636 && container->info.isArea("misc"))
				broadcast(NULL, room, "Flush!");


			if(	object->flagIsSet(O_NO_DROP) &&
				in_room &&
				!player->checkStaff("You cannot drop that.\n")
			)
				return(0);


			if(is_pet) {
				creature->delObj(object);
			} else {
				player->delObj(object, true);
			}

			delete object;
			return(0);
		}

		if(!container->flagIsSet(O_BULKLESS_CONTAINER)) {
			if(!container->getMaxbulk()) {
				if(object->getActualBulk() > container->getActualBulk()) {
					player->printColor("%O is too bulky to fit inside %P.\n", object, container);
					return(0);
				}
			} else {
				if(object->getActualBulk() > container->getMaxbulk()) {
					player->printColor("%O is too bulky to fit inside %P.\n", object, container);
					return(0);
				}
			}
		}

		if(is_pet)
			creature->delObj(object, false, false, true, true, !in_room);
		else
			// if in storage room, keep ownership
			player->delObj(object, false, in_room && !p, true, true, !in_room);

		container->addObj(object);
		container->incShotsCur();
		container->clearFlag(O_BEING_PREPARED);

		if(!is_pet)
			player->printColor("You put %1P in %1P.\n", object, container);
		else
			player->printColor("%M put %1P in %1P.\n", creature, object, container);
		if(container->use_output[0])
			containerOutput(player, container, object);
		broadcast(player->getSock(), room, "%M put %1P in %1P.",
			creature, object, container);

		player->bug("%s%s put %s in %s.\n", player->name, !is_pet ? "'s pet" : "",
			object->name, container->name);
		if(p)
			p->appendLog(player->name, "%s stores %s.", player->name, object->getObjStr(player, player->displayFlags(), 1).c_str());
	}

	player->save(true);
	return(0);
}

//*********************************************************************
//						canGiveTransport
//*********************************************************************
// does some simple checks to see if we can give item away

int canGiveTransport(Creature* creature, Creature* target, Object* object, bool give) {
	Player	*player=0, *pTarget=0, *pMaster=0;
	otag	*op=0;

	// we're being sent either a player or a pet
	//	- creature will be the one getting the object
	//	- player will be the one getting the messages
	player = creature->getPlayerMaster();
	pMaster = target->getPlayerMaster();
	pTarget = target->getAsPlayer();

	if(!player) {
		creature->print("Unable to give!\n");
		return(0);
	}

	// for transport, target will always be a player
	// but for give, target might be a monster

	if(pMaster && pMaster != player) {
		if(!Lore::canHave(pMaster, object, false)) {
			player->print("%M cannot have that item.\n%s cannot currently have any more.\n", target, target->upHeShe());
			return(0);
		}
		if(!Lore::canHave(pMaster, object, true)) {
			player->print("%M cannot have that item.\nIt contains a limited item that %s cannot carry.\n", target, target->heShe());
			return(0);
		}
	}

	if(pTarget) {

		if(pTarget->flagIsSet(P_LINKDEAD)) {
			player->print("%s doesn't want that right now.\n", target->name);
			return(0);
		}

		if(pTarget->isEffected("petrification")) {
			player->print("%s is petrified right now! You can't do that!\n", target->name);
			return(0);
		}

		if(pTarget->isRefusing(player->name)) {
			player->print("%M is refusing items from you right now.\n", target);
			return(0);
		}

		if(!pTarget->isStaff()) {
			if(pTarget->getWeight() + object->getActualWeight() > pTarget->maxWeight()) {
				player->print("%s can't hold anymore.\n", pTarget->name);
				return(0);
			}
			if(target->tooBulky(object->getActualBulk())) {
				player->print("%M cannot carry that much.\n", pTarget);
				return(0);
			}
		}

		if(!Unique::canGet(pTarget, object, true)){
			player->print("%M cannot carry that limited object.\n", pTarget);
			return(0);
		}

	} else {

		if(	Unique::is(object) &&
			!player->checkStaff("%M cannot have that item.\n", target)
		)
			return(false);

	}

	if(object->flagIsSet(O_KEEP)) {
		player->printColor("%O is currently in safe keeping.\nYou must unkeep it to %s.\n", object, give ? "give it away" : "transport it");
		return(0);
	}

	if( object->getQuestnum() &&
		!player->checkStaff("You can't %s a quest object.\n", give ? "give away" : "transport")
	) return(0);

	if( object->flagIsSet(O_NO_DROP) &&
		!player->checkStaff("You cannot %s that object.\n", give ? "give away" : "transport")
	) return(0);

	if(object->getType() == CONTAINER) {
		op = object->first_obj;
		while(op) {
			if(	op->obj->flagIsSet(O_NO_DROP) &&
				!player->checkStaff("You must remove %P first.\n", op->obj)
			)
				return(0);
			if(op->obj->getQuestnum() && !player->checkStaff("You can't %s a quest object.\nYou must remove %P first.\n", give ? "give away" : "transport", op->obj))
				return(0);
			op = op->next_tag;
		}
	}

	return(1);
}

//*********************************************************************
//						cmdGive
//*********************************************************************
// This function allows a player to give an item in their inventory to
// another player or monster.

int cmdGive(Creature* creature, cmd* cmnd) {
	Player	*player = creature->getPlayerMaster();
	Object	*object=0;
	Creature* target=0;
	BaseRoom* room = creature->getRoomParent();

	// we're being sent either a player or a pet
	//	- creature will be the one giving the object
	//	- player will be the one getting the messages

	if(!player) {
		creature->print("Unable to give!\n");
		return(0);
	}

	player->clearFlag(P_AFK);

	if(!player->ableToDoCommand())
		return(0);

	if(cmnd->num < 3) {
		player->print("Give what to whom?\n");
		return(0);
	}

	if(player == creature && cmnd->str[1][0] == '$') {
		give_money(player, cmnd);
		return(0);
	}

	object = findObject(player, creature->first_obj, cmnd);

	if(!object) {
		if(player == creature)
			player->print("You don't have that.\n");
		else
			player->print("%M doesn't have that.\n", creature);
		return(0);
	}

	player->unhide();

	cmnd->str[2][0] = up(cmnd->str[2][0]);
	target = room->findCreature(player, cmnd->str[2], cmnd->val[2], false);

	if(!target || target == creature) {
		if(player == creature)
			player->print("You don't see that person here.\n");
		else
			player->print("%M doesn't see that person here.\n", creature);
		return(0);
	}


	if(player->getClass() == BUILDER) {
		// can they give their item away?
		if(!player->canBuildObjects() && !target->isCt()) {
			player->print("You are not allowed to give items to anyone.\n");
			return(0);
		}
		if(!target->isStaff()) {
			player->print("You are not allowed to give items to players.\n");
			return(0);
		}
		if(!target->getAsPlayer()->canBuildObjects()) {
			player->print("%s are not allowed to receive items.\n", target->upHeShe());
			return(0);
		}

		// they can give it, but can the builder *st/modify it?
		if(	target->getClass()==BUILDER &&
			target->getAsPlayer()->checkRangeRestrict(object->info)
		) {
			player->printColor("That item is outside their range. Please save to the ^ytest^x range.\n");
			return(0);
		}
	}



	if(target->isPlayer()) {
		if(	target->flagIsSet(P_MISTED) &&
			!player->checkStaff("How can you give something to a misted vampire?\n")
		)
			return(0);
	} else {
		if(target->flagIsSet(M_WILL_WIELD) && object->getType() <= 5) {
			player->print("%M doesn't want that.\n", target);
			return(0);
		}

		if(!Faction::willDoBusinessWith(player, target->getAsMonster()->getPrimeFaction())) {
			player->print("%M refuses to accept that from you.\n", target);
			return(0);
		}

		// only worry about trade/give mixup if giver is a player
		if(player == creature && target->flagIsSet(M_TRADES))
			return(cmdTrade(player, cmnd));
	}

	// most basic checks to see if they can give item away or not
	if(!canGiveTransport(creature, target, object, true))
		return(0);


	/*
	if(target->isPlayer() && !player->isCt()) {		// checks idle to keep jackasses from loading
	t=time(0);														// up someone's inventory with crap. -TC
	idle = t - Ply[target->getSock()].io->ltime;

	if(idle > 10 || target->flagIsSet(P_AFK)) {
	player->print("You can't give that to %N right now.\n", target);
	return(0);
	}
	}
	*/

	if(target->pFlagIsSet(P_AFK) && !player->isCt()) {
		player->print("You can't give that to %N when %s is afk.\n", target, target->heShe());
		return(0);
	}

	if(player == creature)
		player->printColor("You give %1P to %N.\n", object, target);
	else if(player != target)
		player->printColor("%M gives %1P to %N.\n", creature, object, target);

	target->printColor("%M gave %1P to you.\n", creature, object);
	broadcast(player->getSock(), target->getSock(), room, "%M gave %1P to %N.", creature, object, target);

	creature->delObj(object);
	Limited::transferOwner(player, target->getPlayerMaster(), object);
	target->addObj(object);

	if(!player->isDm())
		log_immort(false, player, "%s%s gave %s to %s.\n", player->name, player != creature ? "'s pet" : "", object->name, target->name);

	if(object->flagIsSet(O_SILVER_OBJECT) && target->isEffected("lycanthropy")) {
		target->printColor("%O burns you and you drop it!\n", object);
		broadcast(player->getSock(), room, "%M is burned by %P and %s drops it.",
			target, object, target->heShe());
		target->delObj(object, false, true);
		object->addToRoom(room);
		return(0);
	}

	player->save(true);
	return(0);
}

//*********************************************************************
//						give_money
//*********************************************************************
// This function allows a player to give gold to another player. Gold
// is interpreted as gold if it is preceded by a dollar sign.

void give_money(Player* player, cmd* cmnd) {
	Creature* target=0, *master=0;
	Monster* mTarget=0;
	BaseRoom* room = player->getRoomParent();
	unsigned long amt = strtoul(&cmnd->str[1][1], 0, 0);

	if(!player->ableToDoCommand()) return;
	if(amt < 1 || amt > player->coins[GOLD]) {
		player->print("You don't have that much gold.\n");
		return;
	}

	cmnd->str[2][0] = up(cmnd->str[2][0]);
	target = room->findCreature(player, cmnd->str[2], cmnd->val[2], false);
	if(!target || target == player) {
		player->print("Who do you want to give it to?\n");
		return;
	}

	if(target->pFlagIsSet(P_MISTED)) {
		player->print("How can you give money to a mist?\n");
		return;
	}

	if(!target) {
		player->print("That person is not here.\n");
		return;
	}

	master = target;
	if(target->isPet())
		master = target->getMaster();

	if(master->flagIsSet(P_LINKDEAD)) {
		player->print("%s doesn't want that right now!\n", target->name);
		return;
	}
	if(master->isEffected("petrification")) {
		player->print("%s is petrified right now! You can't do that!\n", master->name);
		return;
	}

	master->coins.add(amt, GOLD);
	player->coins.sub(amt, GOLD);

	player->print("Ok.\n");

	master->print("%M gave you %ld gold pieces.\n", player, amt);
	broadcast(player->getSock(), target->getSock(), room, "%M gave %N %ld gold pieces.", player, target, amt);

	if(!player->isDm())
		log_immort(true, player, "%s gave %d gold to %s.\n", player->name, amt, master->name);

	player->save(true);
	mTarget = target->getAsMonster();

	if(mTarget && !mTarget->isPet()) {
		bstring pay = "$pay " + (bstring)amt;
		bool match = false;
		bool hasPay = false;
		unsigned long cost=0;

		for(TalkResponse * talkResponse : mTarget->responses) {
			for(const bstring& keyword : talkResponse->keywords) {
				if(keyword.left(4) == "$pay")
					hasPay = true;

				// match if this is a flat-rate
				match = keyword == pay;
				// investigate level-based cost
				if(!match && keyword.left(10) == "$pay level") {
					cost = atoi(keyword.right(keyword.getLength()-11).c_str());
					match = cost * player->getLevel() == amt;
				}
				if(match) {
					if(talkResponse->response != "")
						mTarget->sayTo(player, talkResponse->response);

					if(talkResponse->action != "") {
						if(!mTarget->doTalkAction(player, talkResponse->action)) {
							mTarget->coins.sub(amt, GOLD);
							player->coins.add(amt, GOLD);

							player->print("%M returns your money.\n", mTarget);
							broadcast(player->getSock(), mTarget->getSock(), room, "%M returns the money.", mTarget);
							player->save(true);
				            return;
						}
					}
					gServer->logGold(GOLD_OUT, player, Money(amt, GOLD), mTarget, "GiveGold(TalkResponse)");
					return;
				}
			}
		}

		// if we get this far, the monster has a $pay string, but the player didn't give
		// them the right amount
		if(hasPay) {
			mTarget->sayTo(player, "That's not the right amount!");

			mTarget->coins.sub(amt, GOLD);
			player->coins.add(amt, GOLD);

			player->print("%M returns your money.\n", mTarget);
			broadcast(player->getSock(), mTarget->getSock(), room, "%M returns the money.", mTarget);
			player->save(true);
			return;
		}
		gServer->logGold(GOLD_OUT, player, Money(amt, GOLD), mTarget, "GiveGold");
	}
}

//*********************************************************************
//						getRepairCost
//*********************************************************************

unsigned long getRepairCost(Player* player, Monster* smithy, Object* object) {
	unsigned long cost=0;
	float	mult=0.0, rcost=0.0;
	int		alignDiff=0, skill=0;

	alignDiff = getAlignDiff(player, smithy);

	cost = object->value[GOLD] / 2;
	cost += (cost/10)*alignDiff;

	skill = smithy->getSkillLevel();

	if(skill < 10)			/* 0-9 */
	{
		mult = 0.1296;
	} else if(skill < 20)	/* 10-19 */
	{
		mult = 0.216;
	} else if(skill < 30)	/* 20-29 */
	{
		mult = 0.36;
	} else if(skill < 40)	/* 30-39 */
	{
		mult = 0.6;
	} else if(skill < 50)	/* 40-49 */
	{
		mult = 1.0;
	} else if(skill < 60)	/* 50-59 */
	{
		mult = 1.5;
	} else if(skill < 70)	/* 60-69 */
	{
		mult = 2.25;
	} else if(skill < 80)	/* 70-79 */
	{
		mult = 3.375;
	} else if(skill < 90)	/* 80-89 */
	{
		mult = 5.0625;
	} else if(skill < 100)	/* 90-99 */
	{
		mult = 7.59375;
	} else if(skill >= 100)	/* 100+ */
	{
		mult = 11.390625;
	}

	// 1.5 moves it down a category in cost
	rcost = (float)cost * (mult / 1.5);
	cost = (long)rcost;

	cost = (MAX(1,cost));

	Money money;
	money.set(cost, GOLD);
	money = Faction::adjustPrice(player, smithy->getPrimeFaction(), money, true);
	cost = money[GOLD];

	return(cost);
}

//*********************************************************************
//						cmdCost
//*********************************************************************

int cmdCost(Player* player, cmd* cmnd) {
	int		alignDiff=0;
	unsigned long cost=0;
	Monster	*smithy=0;
	Object	*object=0;


	if(!player->ableToDoCommand())
		return(0);

	if(cmnd->num < 3) {
		player->print("Syntax: cost (object) (smithy)\n");
		return(0);
	}

	object = findObject(player, player->first_obj, cmnd);

	if(!object) {
		player->print("You don't have that item.\n");
		return(0);
	}

	smithy = player->getParent()->findMonster(player, cmnd, 2);
	if(!smithy) {
		player->print("That smithy is not here.\n");
		return(0);
	}

	if(!Faction::willDoBusinessWith(player, smithy->getPrimeFaction())) {
		player->print("%M refuses to do business with you.\n", smithy);
		return(0);
	}

	if(smithy->getMobTrade() != SMITHY) {
		player->print("%M is not a smithy.\n", smithy);
		return(0);
	}

	if(object->flagIsSet(O_NO_FIX)) {
		player->print("%M says, \"Unfortunately, that cannot be repaired.\"\n", smithy);
		return(0);
	}

	alignDiff = getAlignDiff(player, smithy);
	if(Faction::getAttitude(player->getFactionStanding(smithy->getPrimeFaction())) < Faction::INDIFFERENT) {
		switch(alignDiff) {
		case 2:
			player->print("%M sighs in irritation.\n", smithy);
			break;
		case 3:
			player->print("%M eyes you suspiciously.\n", smithy);
			break;
		case 4:
			player->print("%M mutters obscenities about you.\n", smithy);
			break;
		case 5:
			player->print("%M sneers at you in disgust.\n", smithy);
			break;
		case 6:
			player->print("%M spits on you.\n", smithy);
			break;
		default:
			break;
		}
	}
	cost = getRepairCost(player, smithy, object);

	player->print("%M says, \"It'll cost you %ld gold for me to fix that.\"\n", smithy, cost);
	return(0);
}

//*********************************************************************
//						cmdRepair
//*********************************************************************
// This function allows a player to repair a weapon or armor if they are
// in a repair shop. There is a chance of breakage.

int cmdRepair(Player* player, cmd* cmnd) {
	Object	*object=0;
	Monster	*smithy=0;
	unsigned long cost=0;
	int		skill=0, alignDiff=0, repairChance=0, repairMod=0, roll=0, plusRoll=0, keepPlus=0;



	player->clearFlag(P_AFK);

	if(!player->ableToDoCommand())
		return(0);

	/*if((player->getClass() == CLERIC && player->getDeity() == JAKAR))
	{
		return(jakarRepair(player, cmnd));
	}*/

	if(cmnd->num < 3) {
		player->print("Syntax: repair (object) (smithy)\n");
		return(0);
	}

	object = findObject(player, player->first_obj, cmnd);
	if(!object) {
		player->print("You don't have that.\n");
		return(0);
	}

	smithy = player->getParent()->findMonster(player, cmnd, 2);
	if(!smithy) {
		player->print("That smithy is not here!\n");
		return(0);
	}

	if(!Faction::willDoBusinessWith(player, smithy->getPrimeFaction())) {
		player->print("%M refuses to do business with you.\n", smithy);
		return(0);
	}

	if(smithy->getMobTrade() != SMITHY) {
		player->print("%M is not a smithy.\n", smithy);
		return(0);
	}

	player->unhide();

	if(object->flagIsSet(O_NO_FIX) || object->flagIsSet(O_CUSTOM_OBJ)) {
		player->print("%M says, \"Sorry, I just can't fix that!\"\n", smithy);
		return(0);
	}

	if(object->getType() != WEAPON && object->getType() != ARMOR) {
		player->print("%M says, \"Sorry, I just can't fix that!\"\n", smithy);
		return(0);
	}

	if(object->getShotsCur() > MAX(3, object->getShotsMax() / 10)) {
		player->print("%M says, \"I've always said if it ain't broke, don't fix it!\"\n", smithy);
		return(0);
	}


	cost = getRepairCost(player, smithy, object);

	if(player->coins[GOLD] < cost) {
		player->print("%M eyes you suspiciously.\n%M says, \"That'll be $%ld gold and not a coin less!\n", smithy, smithy, cost);
		return(0);
	}

	player->print("%M says, \"That'll be $%ld.\"\n%M takes the gold from you.\n", smithy, cost, smithy);
	player->coins.sub(cost, GOLD);

	gServer->logGold(GOLD_OUT, player, Money(cost, GOLD), object, "Repair");

	broadcast(player->getSock(), player->getParent(), "%M has %N repair %1P.", player, smithy, object);


	alignDiff = getAlignDiff(player, smithy);


	switch(alignDiff) {
	case 2:
		player->print("%M gives you sidelong glances and is working slow on purpose.", smithy);
		broadcast(player->getSock(), player->getParent(),
			"%M gives %N sidelong glances as %s works and is working slow on purpose.", smithy, player, smithy->heShe());
		break;

	case 3:
		player->print("%M openly glares at you as %s works.\n", smithy, smithy->heShe());
		broadcast(player->getSock(), player->getParent(),
			"%M openly glares at %N as %s works.\n", smithy, player, smithy->heShe());
		break;
	case 4:
		player->print("%M scowls and curses about you as %s works.\n", smithy, smithy->heShe());
		broadcast(player->getSock(), player->getParent(),
			"%M scowls and curses about %N as %s works.\n", smithy, player, smithy->heShe());
		break;
	case 5:
		player->print("%M sneers at you in disgust as %s works.\n", smithy, smithy->heShe());
		broadcast(player->getSock(), player->getParent(),
			"%M sneers at %N in disgust as %s works.\n", smithy, player, smithy->heShe());
		break;
	case 6:
		player->print("As %N works, %s makes no bones about openly despising you.\n", smithy, smithy->heShe());
		broadcast(player->getSock(), player->getParent(),
			"As %N works, %s makes no bones about openly despising %N.\n", smithy, smithy->heShe(), player);
		break;
	}

	skill = smithy->getSkillLevel();
	repairMod = -1 * (50-skill);
	repairChance = 50 + repairMod + bonus((int) player->piety.getCur());

	if(object->getAdjustment()) {
		if(skill < 30)		/* skill levels 0-29 cannot repair magical or high quality weapons */
			repairChance -= 100;
		else if(skill < 80) {
			switch(object->getAdjustment()) {
			case 1:
				repairChance -= 5;
				break;
			case 2:
				repairChance -= 10;
				break;
			case 3:
				repairChance -= 15;
				break;
			case 4:
				repairChance -= 20;
				break;
			default:
				repairChance -= 25;
				break;
			}
		}
	}



	repairChance -= alignDiff*10;

	if(skill < 30)			/* 00-29 */
	{
		keepPlus = 0;
	} else if(skill < 40)	/* 30-39 */
	{
		keepPlus = 5;
	} else if(skill < 50)	/* 40-49 */
	{
		keepPlus = 10;
	} else if(skill < 60)	/* 50-59 */
	{
		keepPlus = 15;
	} else if(skill < 70)	/* 60-69 */
	{
		keepPlus = 20;
	} else if(skill < 80)	/* 70-79 */
	{
		keepPlus = 50;
	} else if(skill < 90)	/* 80-89 */
	{
		keepPlus = 75;
	} else if(skill < 100)	/* 90-99 */
	{
		keepPlus = 85;
	} else if(skill >= 100)	/* 100+ */
	{
		keepPlus = 100;
	}

	roll = mrand(1,100);
	/*if(player->isDm()) {
		player->print("Chance: %d\n", repairChance);
		player->print("AlignDiff: %d\n", alignDiff);
		player->print("Roll: %d\n", roll);
	}*/

	if(roll < repairChance) {

		if(object->getAdjustment()) {
			plusRoll = mrand(1,100);
			if(plusRoll > keepPlus && object->getAdjustment() > 1) {
				if(plusRoll - keepPlus <= 5) {
					object->setAdjustment(object->getAdjustment() - 1);
					player->print("%M says, \"Sorry, but it seems to have lost some of its %s.\"\n",
					      smithy, object->getAdjustment() > 0 ? "fine glow" : "fine quality");
				} else {
					player->print("%M says, \"Sorry, but it seems to have lost its %s.\"\n",
					      smithy, object->getAdjustment() > 0 ? "fine glow" : "fine quality");
					object->setAdjustment(0);
				}
			}
		}

		object->setShotsMax(object->getShotsMax() * 9 / 10);
		object->setShotsCur(object->getShotsMax());


		player->printColor("%M hands %P back to you, almost good as new.\n", smithy, object);

	} else {

		player->print("\"Bah!\", %N shouts, \"Broke it. Sorry.\"\n", smithy);
		broadcast(player->getSock(), player->getParent(), "Oops! %s broke it.", smithy->heShe());

		if(object->getShotsCur() > 0 && !alignDiff) {
			player->print("%M says, \"Well, here's your cash back.\"\n", smithy);
			player->coins.add(cost, GOLD);
			gServer->logGold(GOLD_IN, player, Money(cost, GOLD), object, "RepairRefund");
		}

		player->delObj(object, true);
		delete object;
	}
	return(0);
}

//*********************************************************************
//						checkDarkness
//*********************************************************************
// checks the creature for a darkness item. if they have one, flag them

void Creature::checkDarkness() {
	clearFlag(isPlayer() ? P_DARKNESS : M_DARKNESS);
	for(int i=0; i<MAXWEAR; i++) {
		if(ready[i] && ready[i]->flagIsSet(O_DARKNESS)) {
			setFlag(isPlayer() ? P_DARKNESS : M_DARKNESS);
			return;
		}
	}
	otag* op = first_obj;
	while(op) {
		if(op->obj->flagIsSet(O_DARKNESS)) {
			setFlag(isPlayer() ? P_DARKNESS : M_DARKNESS);
			return;
		}
		op = op->next_tag;
	}
}
