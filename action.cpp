/*
 * action.cpp
 *	 Contains the routines necessary to achieve action commands.
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
//#include "structs.h"
//#include "mextern.h"
#include "commands.h"


//*********************************************************************
//						socialHooks
//*********************************************************************

void socialHooks(Creature *creature, MudObject* target, bstring action, bstring result) {
	Hooks::run(creature, "doSocial", target, "receiveSocial", action, result);
}

void socialHooks(Creature *target, bstring action, bstring result) {
	if(!target->getRoom())
		return;
	Hooks::run<Monster*,MonsterPtrLess>(target->getRoom()->monsters, target, "roomSocial", action, result);
}

//*********************************************************************
//						actionShow
//*********************************************************************

bool actionShow(Player* pTarget, Creature* creature) {
	return(pTarget && !pTarget->isGagging(creature->name) && !pTarget->flagIsSet(P_UNCONSCIOUS));
}

// prototype for actionFail
int actionFail(Creature* player, bstring action, bstring second = "m");

#define ACTION_SHOW

// update for oroom

#define OUT(a,b)	sock->print(a); \
					broadcast(sock, room, b, creature); \
					socialHooks(creature, str);
#define OUT_HISHER(a,b)	sock->print(a); \
						broadcast(sock, room, b, creature, creature->hisHer()); \
						socialHooks(creature, str);
#define OUT_HESHE(a,b)	sock->print(a); \
						broadcast(sock, room, b, creature, creature->heShe()); \
						socialHooks(creature, str);
#define OUT4(a,b,c) creature->print((a), target); \
					if(actionShow(pTarget, creature)) \
						{pTarget->print((b), creature);} \
					broadcast(sock, target->getSock(), room, (c), creature, target); \
					socialHooks(creature, target, str);
#define OUT_HIMHER(a,b)	sock->print(a); \
						broadcast(sock, room, b, creature, creature->himHer()); \
						socialHooks(creature, str);



//*********************************************************************
//						orderPet
//*********************************************************************

int orderPet(Player* player, cmd* cmnd) {
	int x=0;
	bool allPets = true;

	if(!player->hasPet()) {
		player->print("You don't have a pet to command.\n");
		return(0);
	}

	if(cmnd->num < 2) {
	    player->displayPets();
        return(0);
    }


	if(cmnd->num >= 3 && strlen(cmnd->str[1]) >= 3) {
	    // You need to type at least 3 characters to indicate what pet you want, otherwise we assume it's just a command

	    //cmnd->str[0] = pet
	    //cmnd->str[1] = <pet>
	    //cmnd->str[2] = <action>

        // See if they're trying to order a specific pet

        Creature* pet = player->findPet(cmnd->str[1], cmnd->val[1]);
        if(pet) {
            allPets = false;
            for(; x<cmnd->num-2; x++)
                strcpy(cmnd->str[x], cmnd->str[x+2]);
            cmnd->num--;
            strcpy(cmnd->str[cmnd->num], "");

            // chop the "pet <pet> " off
            cmnd->fullstr = getFullstrText(cmnd->fullstr, 2);
            cmnd->myCommand = 0;

            cmdProcess(player, cmnd, pet);

        }
	}

	if(allPets) {
	    // the player is done with their cmnd, let the pet use it
        for(; x<cmnd->num-1; x++)
            strcpy(cmnd->str[x], cmnd->str[x+1]);
        cmnd->num--;
        strcpy(cmnd->str[cmnd->num], "");

        // chop the "pet " off
        cmnd->fullstr = getFullstrText(cmnd->fullstr, 1);

	    for(Creature* pet : player->pets) {
            if(!player->inSameRoom(pet)) {
                player->print("%M can't hear you.\n", pet);
                continue;
            }
            cmnd->myCommand = 0;

            cmdProcess(player, cmnd, pet);
	    }
	}
	return(0);
}


//*********************************************************************
//						plyAction
//*********************************************************************
// this function is a wrapper - it allows us to put some action
// commands in the player command array, all while still using the
// same action function to do our work.

int plyAction(Player* player, cmd* cmnd) {
	return(cmdAction(player, cmnd));
}


//*********************************************************************
//						cmdAction
//*********************************************************************
// writeSocialFile in cmd.cpp will automatically create a helpfile (socials.txt)
// with any command that calls this function or plyAction. Go edit that function
// if you create a new social and wish to exclude it from the socials helpfile.

int cmdAction(Creature* creature, cmd* cmnd) {
	BaseRoom* room = creature->getRoom();
	Player	*player=0, *pTarget=0;
	Creature* target=0;
	Object	*object=0;
	Exit	*exit=0;
	int		n=0, num=0, num1=0, num2=0;
	char	temp[12];
	bstring str = "";

	ASSERTLOG( creature );
	ASSERTLOG( cmnd );

	// some actions are creature-only; we will need this variable to use them
	player = creature->getPlayer();

	Socket* sock = NULL;
	if(player)
		sock = player->getSock();

	if(!creature->ableToDoCommand(cmnd))
		return(0);

	str = cmnd->myCommand->getName();

	if(player) {
		creature->clearFlag(P_AFK);

		if(str != "sleep")
			player->unhide();

		// if they have a pet, they use orderPet instead
		if(str == "pet" && player->hasPet()) {
			return(orderPet(player, cmnd));
		}
	}


	if(cmnd->num == 2) {
		target = room->findCreature(creature, cmnd->str[1], cmnd->val[1], true, true);
		if(target)
			pTarget = target->getPlayer();
		if( (!target || target == creature) &&
			str != "point" &&
			str != "show" &&
			str != "mark" &&
			str != "dismiss"
		) {
			creature->print("That is not here.\n");
			return(player ? 0 : 1);
		}
	}


	// will this social wake anyone up?
	if(pTarget && pTarget->flagIsSet(P_SLEEPING)) {
		if(	str == "shake" || str == "anvil" || str == "knee" || str == "pounce" ||
			str == "tickle" || str == "kic" || str == "tackle" || str == "poke" ||
			str == "copulate" || str == "dance" || str == "cstr" || str == "suck" ||
			str == "hammer" || str == "slap" || str == "dirt" || str == "grab" ||
			str == "shove" || str == "high5" || str == "thank" || str == "punch" ||
			str == "lick" || str == "choke" || str == "zip" || str == "bhand" ||
			str == "beg" || str == "fondle" || str == "grope" || str == "pants" ||
			str == "noogie" || str == "nudge" || str == "pinch" || str == "pummel" ||
			str == "trip" || str == "pester" || str == "ruffle" || str == "slobber" ||
			str == "smack" || str == "spank" || str == "squeeze" || str == "swat" ||
			str == "thwack" || str == "tip" || str == "tug" || str == "whip" ||
			str == "drool" || str == "congrats" || str == "bonk" || str == "defenestrate" ||
			str == "whine" || str == "sneeze" || str == "pan" || str == "squeeze" ||
			str == "wedgy" || str == "bitchslap" || str == "hump"
		) {
			pTarget->wake("You awaken suddenly!");
		} else if( str == "smootch" || str == "french" || str == "rub" ||
			str == "massage" || str == "peck" || str == "nibble" || str == "hug" ||
			str == "comfort"
		) {
			pTarget->wake("You wake up.");
		}
	}

	// noisy socials
	if(	str == "stomp" || str == "hail" || str == "bark" || str == "rage" ||
		str == "ahem" || str == "roar" || str == "scream" || str == "spaz" ||
		str == "squeal" || str == "tantrum" || str == "wail" || str == "yodel" ||
		str == "crow" || str == "cornholio"
	) {
		room->wake("You awaken suddenly!", true);
	}



	if(str == "relax") {
		OUT("You breath deeply.\n", "%M takes a deep breath and sighs comfortably.");
	} else if(str == "stab") {
		if(pTarget && player && creature->isDm() ) {
			OUT4("You stab %N in the eye. OUCH!\n", "%M stabs you in the eye.\n",
				"%M stabs %N in the eye.");
			pTarget->addEffect("blindness", 120, 1, player, true, player);
		} else {
			if(	player &&
				(	!player->ready[WIELD-1] ||
					player->ready[WIELD-1]->getWeaponCategory() != "piercing"
				)
			) {
				creature->print("You must be wielding a piercing weapon in order to stab yourself.\n");
			} else {
				OUT_HIMHER("You stab yourself in the eye. OUCH!\n", "%M stabs %sself in the eye.");
				if(	mrand(1,100) <= 50 &&
					!creature->isEffected("blindness"))
				{
					creature->addEffect("blindness", creature->strength.getCur(), 1, player, true, player);
				}
			}
		}
	} else if(str == "vangogh") {

		if(	player &&
			(	!player->ready[WIELD-1] ||
				!(player->ready[WIELD-1]->getWeaponCategory() == "slashing" || player->ready[WIELD-1]->getWeaponCategory() == "chopping")
			) )
		{
			creature->print("You must be wielding a slashing or chopping weapon in order to cut yourself.\n");
		} else {

			if(!creature->checkAttackTimer())
				return(0);
			creature->updateAttackTimer();

			OUT_HISHER("You slice your ear!\n", "%M slices %s ear.");
			// don't let them die from it
			n = mrand(1,4);
			if(creature->hp.getCur() - n > 0) {
				creature->hp.decrease(n);
				creature->printColor("You take ^R%d^x damage.\n", n);
				broadcastGroup(false, creature, "%M slices %sself for *CC:DAMAGE*%d^x damage, %s%s\n",
					creature, creature->himHer(), n, creature->heShe(), creature->getStatusStr());
			}
		}

	} else if(str == "sleep") {

		if(player) {
			if(creature->flagIsSet(P_SLEEPING)) {
				sock->print("You are already asleep.\n");
				return(0);
			}
			if(creature->flagIsSet(P_UNCONSCIOUS)) {
				sock->print("You are already unconscious.\n");
				return(0);
			}

			if(creature->isUndead()) {
				// vampire undead MAY sleep at night
				if(creature->isEffected("vampirism")) {
					if(!room->vampCanSleep(creature->getSock()))
						return(0);
				} else {
					sock->print("The undead do not sleep.\n");
					return(0);
				}
			}

			if(creature->flagIsSet(P_MISTED)) {
				sock->print("You can't do that while misted.\n");
				return(0);
			}
			if(room->isCombat()) {
				sock->print("There is too much commotion to sleep.\n");
				return(0);
			}

			if(creature->hp.getCur() >= creature->hp.getMax() && creature->mp.getCur() >= creature->mp.getMax()) {
				sock->print("You aren't tired at the moment.\n");
				return(0);
			}

			player->interruptDelayedActions();
			player->setFlag(P_SLEEPING);
			player->setFlag(P_UNCONSCIOUS);
			player->clearFlag(P_SITTING);
		}

		sock->print("You take a nap.\n");
		if(player && player->flagIsSet(P_HIDDEN)) {
			broadcast(isCt, sock, room, "*DM* %M takes a nap.", creature);
		} else {
			broadcast(sock, room, "%M takes a nap.", creature);
			socialHooks(creature, str);
		}

	} else if(str == "wake") {
		if(target) {
			if(!pTarget || !pTarget->flagIsSet(P_SLEEPING)) {
				sock->print("%M isn't sleeping.", target);
			} else {
				OUT4("You wake %N up.\n", "%M wakes you up.\n",
					"%M wakes %N up.");

				target->wake();
			}
		} else {
			if(player) {
				if(!player->flagIsSet(P_SLEEPING)) {
					sock->print("You aren't sleeping!\n");
					return(0);
				}
				player->wake();
			}

			OUT("You wake up.\n", "%M wakes up.");
		}
	} else if(str == "masturbate") {
		OUT("You masturbate to orgasm.\n", "%M masturbates to orgasm.");

		if( player &&
			mrand(1,100) <= 5 &&
			!player->isEffected("blindness")
		) {
			player->addEffect("blindness", 300 - player->constitution.getCur(), 1, player, true, player);
		}

	} else if(str == "chest") {
		if(target) {
			sock->print("You beat your fists to your chest in praise of %N.\n", target);
			target->print("%M beats %s fists to %s chest in praise of you.\n",
				creature, creature->hisHer(), creature->hisHer());
			broadcast(sock, target->getSock(), room, "%M beats %s fists to %s chest in praise of %N.",
				creature, creature->hisHer(), creature->hisHer(), target);
			socialHooks(creature, target, str);
		} else {
			sock->print("You beat your fists to your chest in celebration.\n");
			broadcast(sock, room, "%M beats %s fists to %s chest in celebration.",
				creature, creature->hisHer(), creature->hisHer());
			socialHooks(creature, str);
		}
	} else if(str == "bfart" && player && !strcmp(creature->name, "Bane")) {
		if(pTarget) {
			OUT4("You fart on %N.\n", "%M farts on you.\n",
				"%M farts on %N.");
			OUT4("You knocked %N unconscious!\n", "%M's fart reeks so much you are knocked unconscious!\n",
				"%M's fart knocks %N unconscious!");
			pTarget->knockUnconscious(30);
		} else {
			OUT("You let rip a nasty one.\n", "%M lets out a deadly fart.");
			sock->print("You knock everyone in the room unconscious!\n");
			for(Player* ply : room->players) {
				if(	ply != player &&
					!ply->flagIsSet(P_DM_INVIS) &&
					!ply->isUnconscious() &&
					!ply->isDm() &&
					!ply->inCombat()
				) {
					ply->print("%M's fart knocks you unconscious!\n", creature);
					ply->knockUnconscious(30);
					broadcast(sock, ply->getSock(), room,
						"%M falls to the ground unconscious.", ply);
				}
			}

		}
	}  else if(str == "tag") {
		if(target) {
			if(target->isStaff() && !creature->isDm()) {
				sock->print("Staff doesn't play tag with mortals.\n");
				return(0);
			}
			target->wake("You awaken suddenly!");
			sock->print("You tag %N. %s's now it!\n", target, target->upHeShe());
			target->print("%M tags you. You're now it!\n", creature);
			broadcast(sock, target->getSock(), room, "%M just tagged %N! %s's it!",
				creature, target, target->upHeShe());
			socialHooks(creature, target, str);

		} else {
			OUT("You feel like playing tag.\n", "%M is in the mood to play tag.");
		}
	}  else if(str == "dream" && player) {
		if(!player->flagIsSet(P_SLEEPING)) {
			player->print("You aren't sleeping!\n");
		} else {
			OUT("You dream of pleasant things.\n", "%M has pleasant dreams.");
		}
	} else if(str == "rollover" && player) {
		if(!player->flagIsSet(P_SLEEPING)) {
			player->print("You aren't sleeping!\n");
		} else {
			OUT_HISHER("You roll over in your sleep.\n", "%M rolls over in %s sleep.");
		}
	} else if(str == "hum") {
		if(mrand(0,10) || player->getClass() == BARD || player->isStaff()) {
			OUT("You hum a little tune.\n", "%M hums a little tune.");
		} else {
			OUT("You hum off-key.\n", "%M hums off-key.");
		}
	}  else if((str == "defecate" || str == "def" || str == "defe") && player) {

		if(player->inCombat()) {
			sock->print("You can't do that now! You are in combat!\n");
			sock->print("Do you want to shit all over yourself?\n");
			return(0);
		}

		if(player->isPoisoned() || player->isDiseased()) {
			sock->print("You don't feel well enough to go right now.\n");
			return(0);
		}

		if(	!dec_daily(&player->daily[DL_DEFEC]) ||
			player->getLevel() < 4 ||
			player->getClass() == BUILDER
		) {
			sock->print("You don't have to go.\n");
			return(0);
		}


		OUT("You squat down and shit.\n", "%M squats down and shits.");

		sock->print("You feel SOOO much better now.\n");

		// Daily restore from defecate.
		player->hp.restore();
		player->mp.restore();


		if(loadObject(SHIT_OBJ, &object)) {
			sprintf(object->name, "piece of %s's shit",
				(player->flagIsSet(P_ALIASING) ? player->getAlias()->name : player->name));

			finishDropObject(object, room, player);
		}

	} else if(str == "mark" && player) {

		if(player->inCombat()) {
			sock->print("And piss all over yourself?!\n");
			return(0);
		}


		if(	!player->isEffected("lycanthropy") &&
			player->getLevel() >= 13 &&
			!player->isCt()
		) {
			sock->print("Only werewolves of high level can mark their territory.\n");
			return(0);
		}

		if(!player->isCt()) {
			if(!dec_daily(&player->daily[DL_DEFEC]) || player->getLevel() < 4) {
				sock->print("You don't have to go.\n");
				return(0);
			}
		}



		if(loadObject(42, &object)) {
			sprintf(object->name, "puddle of %s's piss",
				(player->flagIsSet(P_ALIASING) ? player->getAlias()->name : player->name));
			object->plural = "puddles of ";
			object->plural += (player->flagIsSet(P_ALIASING) ? player->getAlias()->name : player->name);
			object->plural += "'s piss";
			object->addToRoom(room);
		}

		if(target) {
			target->wake("You awaken suddenly!");
			OUT4("You piss on %N.\n", "%M pisses on you.\n",
				"%M pisses on %N.");
		} else {
			object = findObject(player, room->first_obj, cmnd);
			if(object) {
				if(!(object->flagIsSet(O_NO_PREFIX) && (object->flagIsSet(O_NO_TAKE) || object->flagIsSet(O_SCENERY)))) {
					sock->printColor("You piss on %P.\n", object);
					broadcast(player->getSock(), player->getRoom(), "%M pisses on %P.", player, object);
				} else {
					sock->printColor("You piss on the %P.\n", object);
					broadcast(player->getSock(), player->getRoom(), "%M pisses on the %P.", player, object);
				}

				socialHooks(creature, object, str);
			} else {
				lowercize(cmnd->str[1], 0);
				exit = findExit(creature, cmnd);

				if(exit && !exit->flagIsSet(X_DESCRIPTION_ONLY) && !exit->isConcealed(player)) {
					sock->printColor("You piss on the %s^x.\n", exit->name);
					broadcast(player->getSock(), player->getRoom(), "%M pisses on the %s^x.", player, exit->name);

					socialHooks(creature, exit, str);
				} else {
					OUT_HISHER("You mark your territory.\n", "%M marks %s territory.");
				}
			}
		}

		player->hp.restore();
		player->mp.restore();
	} else if(str == "bless") {
		if(target) {
			creature->print("You give %N your blessing.\n", target);
			if(actionShow(pTarget, creature))
				pTarget->print("%M gives you %s blessing.\n", creature, creature->hisHer());
			broadcast(sock, target->getSock(), room, "%M gives %N %s blessing.", creature, target, creature->hisHer());
			socialHooks(creature, target, str);
		} else
			actionFail(creature, str);
	} else if(str == "pat") {
		if(target && !target->isDm()) {
			OUT4("You pat %N on the head.\n", "%M pats you on the head.\n",
				"%M pats %N on the head.");
		} else
			actionFail(creature, str);
	} else if(str == "pet") {
		if(target) {
			OUT4("You pet %N on the back of the head.\n",
				"%M pets you on the back of the head.\n",
				"%M pets %N on the back of the head.");
		} else
			actionFail(creature, str);
	} else if(str == "dismiss") {

		if(target) {
			if(target->isPet() && target->getMaster() == creature) {
				player->dismissPet(target->getMonster());
			} else {
				OUT4("You curtly dismiss %N.\n", "%M dismisses you curtly.\n",
					"%M curtly dismisses %N .");
			}
		} else {
			if(player->hasPet() && !strcmp(cmnd->str[1], "all")) {
				player->dismissAll();
				return(0);
			} else {
				sock->print("Dismiss who?\n");
				return(0);
			}
		}

	} else if(str == "sit") {

		if(player) {
			if(player->flagIsSet(P_SITTING)) {
				sock->print("You are already sitting!\n");
				return(0);
			}
			if(player->flagIsSet(P_MISTED)) {
				sock->print("How does a mist sit down?\n");
				return(0);
			}
			if(player->inCombat()) {
				sock->print("Don't sit in the middle of combat!\n");
				sock->print("Are you daft?\n");
				return(0);
			}

			player->unhide();
			player->interruptDelayedActions();
			player->setFlag(P_SITTING);
		}

		OUT("You sit down.\n", "%M takes a seat.");

	} else if(str == "stand") {

		creature->stand();

	} else if(str == "flip") {
		bstring result = (mrand(0,1) ? "heads" : "tails");

		if(creature->getClass() == CLERIC && creature->getDeity() == KAMIRA) {
			sock->print("You flip a coin. It lands on its side!\n");
			broadcast(sock, room, "%M flips a coin. It lands on its side!", creature);
			socialHooks(creature, str, "side");
		} else if(target) {
			sock->print("You flip a coin and show it to %N: %s.\n", target, result.c_str());
			target->print("%M flips a coin and shows it to you: %s.\n", creature, result.c_str());
			broadcast(sock, target->getSock(), room, "%M flips a coin and shows it to %N: %s.", creature, target, result.c_str());
			socialHooks(creature, target, str, result);
		} else {
			sock->print("You flip a coin: %s.\n", result.c_str());
			broadcast(sock, room, "%M flips a coin: %s.", creature, result.c_str());
			socialHooks(creature, str, result);
		}
	} else if(str == "rps") {
		if(target) {
			OUT4("You offer rock-paper-scissors to %N.\n",
				"%M wants to do rock-paper-scissors with you.\n",
				"%M offers to do rock-paper-scissors with %N.");
		} else {
			num = mrand(1,100);

			if(num <= 33)
				strcpy(temp, "rock");
			else if(num > 33 && num <= 66)
				strcpy(temp, "paper");
			else
				strcpy(temp, "scissors");

			sock->print("One, two, three: You chose %s.\n", temp);
			broadcast(sock, room, "Rock-paper-scissors: %M chose %s.", creature, temp);
			socialHooks(creature, str, temp);
		}
	} else if(str == "dice") {
		if(target) {
			OUT4("You challenge %N to a game of dice.\n",
				"%M challenges you to a game of dice.\n",
				"%M challenges %N to a game of dice.");
		} else {
			num1 = mrand(1,6);
			num2 = mrand(1,6);
			//	sock->print("num: %d.", num);

			num = ((num1 + num2));

			if(creature->getClass() == CLERIC && creature->getDeity() == KAMIRA)
				num = (mrand(0,1) ? 2 : 12);

			sock->print("You roll 2d6\n: %d\n", num);
			broadcast(sock, room, "(Dice 2d6): %M got %d.", creature, num);
			socialHooks(creature, str, (bstring)num);
		}
	}  else if(str == "lobotomy") {

		if(!target) {
			OUT("You contemplate lobotomizing someone.\n", "%M contemplates a performing a lobotomy on someone.");
		} else {
			if(!player || !player->isCt() || !pTarget || pTarget->isDm()) {
				OUT4("You give %N a lobotomy.\n",
					"%M gives you a lobotomy.\n",
					"%M just gave %N a frontal lobotomy.");
			} else {
				pTarget->wake("You awaken suddenly!");
				if(pTarget->flagIsSet(P_BRAINDEAD)) {
					pTarget->clearFlag(P_BRAINDEAD);
					pTarget->print("You are no longer lobotomized.\n");
					sock->print("%M is no longer lobotomized.\n", pTarget);
					if(!creature->isDm())
						log_immort(false, player, "%s removes %s's lobotomy in room %s.\n", player->name, target->name,
							room->fullName().c_str());
					return(0);
				} else {
					target->setFlag(P_BRAINDEAD);

					OUT4("You give %N a lobotomy.\n",
						"%M gives you a lobotomy.\n",
						"%M just gave %N a frontal lobotomy.");
					if(!creature->isDm())
						log_immort(false, player, "%s lobotomizes %s in room %s.\n", player->name, target->name,
							room->fullName().c_str());
				}
			}
		}
	} else if(str == "point") {

		if(target) {
			OUT4("You point at %N.\n",
				"%M points at you.\n",
				"%M points at %N.");
		} else {
			object = findObject(creature, room->first_obj, cmnd);
			if(object) {
				if(!(object->flagIsSet(O_NO_PREFIX) && (object->flagIsSet(O_NO_TAKE) || object->flagIsSet(O_SCENERY)))) {
					sock->printColor("You point at %P.\n", object);
					broadcast(sock, room, "%M points at %P.", creature, object);
				} else {
					sock->printColor("You point at the %P.\n", object);
					broadcast(sock, room, "%M points at the %P.", creature, object);
				}

				socialHooks(creature, object, str);
			} else {
				lowercize(cmnd->str[1], 0);
				exit = findExit(creature, cmnd);

				if(exit && !exit->flagIsSet(X_DESCRIPTION_ONLY) && !exit->isConcealed(player)) {
					sock->printColor("You point to the %s^x.\n", exit->name);
					broadcast(sock, room, "%M points to the %s^x.", creature, exit->name);

					socialHooks(creature, exit, str);
				} else {
					OUT_HIMHER("You point at yourself.\n", "%M points at %sself.");
				}
			}
		}

	}   else if(str == "narrow") {
		if(target) {
			sock->print("You narrow your eyes at %N.\n", target);
			target->print("%M narrows %s eyes at you.\n", creature, creature->hisHer());
			broadcast(sock, target->getSock(), room, "%M narrows %s eyes at %N.", creature, creature->hisHer(), target);
			socialHooks(creature, target, str);
		} else {
			OUT_HISHER("You narrow your eyes.\n", "%M narrows %s eyes evilly.");
		}
	}  else if(str == "tnl" && player) {

		if(player->getActualLevel() > player->getLevel()) {
			broadcast(player->getSock(), player->getRoom(), "%M needs %s experience to relevel.", player, player->expToLevel(false).c_str());
			sock->print("You need %s experience to relevel.\n", player->expToLevel(false).c_str());
		} else {
			broadcast(player->getSock(), player->getRoom(), "%M needs %s experience to level.", player, player->expToLevel(false).c_str());
			sock->print("You need %s experience to level.\n", player->expToLevel(false).c_str());
		}

	} else if(str == "hunger") {

		if(target) {
			if(	!creature->isEffected("vampirism") &&
				!creature->isEffected("lycanthropy") &&
				creature->getDisplayRace() != GOBLIN &&
				creature->getDisplayRace() != TROLL &&
				creature->getDisplayRace() != OGRE &&
				creature->getDisplayRace() != ORC &&
				creature->getDisplayRace() != MINOTAUR &&
				pTarget
			) {
				sock->print("%s doesn't make you hungry.\n", target->upHeShe());
				return(0);
			}

			OUT4("You stare hungrily at %N.\n",
				"%M stares at you hungrily.\n",
				"%M stares hungrily at %N.");
		} else {
			OUT("You tell everyone you are dying of hunger.\n", "%M is extremely hungry - starving in fact.");
		}
	}  else if(str == "show") {


		if(cmnd->num < 2) {
			sock->print("Show what to whom?\n");
			return(0);
		}

		target = room->findCreature(creature, cmnd->str[2], cmnd->val[2], false);
		object = findObject(creature, creature->first_obj, cmnd);

		if(target) {

			if(!object) {
				sock->print("You don't have that out in your inventory.\n");
				return(0);
			}

			sock->printColor("You proudly show %N %P.\n", target, object);
			target->printColor("%M proudly shows you %P.\n", creature, object);
			broadcast(sock, target->getSock(), room, "%M proudly shows %N %P.", creature, target, object);
			socialHooks(creature, target, str);
			socialHooks(creature, object, str);
		} else {

			if(!object) {
				sock->print("You don't have that out in your inventory.\n");
				return(0);
			}

			sock->printColor("You proudly show %P to everyone in the room.\n", object);
			broadcast(sock, room, "%M proudly shows %P to everyone in the room.", creature, object);
			socialHooks(creature, object, str);
		}
	}  else if(str == "ballz") {
		if(!creature->isEffected("lycanthropy") && !creature->isCt()) {
			sock->print("Don't do that! That's disgusting!\n");
			return(0);
		}

		if(creature->getSex() != SEX_MALE) {
			sock->print("You can't do that! You don't have the right anatomy!\n");
			return(0);
		}

		OUT_HISHER("You sit down and lick your balls.\n", "%M is licking %s balls.");
	} else if(str == "wag") {
		if(!creature->isEffected("lycanthropy") && !creature->isCt()) {
			sock->print("You don't have a tail to wag.\n");
			return(0);
		}

		OUT_HISHER("You wag your tail happily.\n", "%M wags %s tail in excitement.");
	}  else if(str == "murmur") {
		if(player && player->flagIsSet(P_SLEEPING)) {
			OUT_HISHER("You murmur.\n", "%M murmurs something in %s sleep.");
		} else {
			OUT("You murmur.\n", "%M murmurs something.");
		}
	} else if(str == "snore") {
		if(player && !player->flagIsSet(P_SLEEPING) && !player->isStaff()) {
			player->print("You aren't sleeping!\n");
		} else {
			OUT("You snore loudly.\n", "%M snores loudly.");
		}
	}  else if(!player)
		return(2);
	return(0);
}

//*********************************************************************
//						isBadSocial
//*********************************************************************

bool isBadSocial(const bstring& str) {
	return(	str == "anvil" || str == "knee" || str == "kic" || str == "spit" || str == "mark" || str == "cstr" || str == "hammer" ||
			str == "slap" || str == "bird" || str == "moon" || str == "punch" || str == "choke" || str == "bhand" || str == "pants" ||
			str == "noogie" || str == "pummel" || str == "" || str == "trip" || str == "smack" || str == "swat" || str == "whip" ||
			str == "defenestrate" || str == "fangs" || str == "hiss" || str == "" || str == "gnaw" || str == "pan" || str == "wedgy" ||
			str == "" || str == "bitchslap");
}

//*********************************************************************
//						isBadSocial
//*********************************************************************

bool isSemiBadSocial(const bstring& str) {
	return(	str == "hump" || str == "ogle" || str == "fart" || str == "" || str == "expose" || str == "goose" || str == "copulate" ||
			str == "suck" || str == "glare" || str == "growl" || str == "dirt" || str == "cough" || str == "shove" || str == "taunt" ||
			str == "eye" || str == "narrow" || str == "disgust" || str == "fondle" || str == "grope" || str == "loom" || str == "pester" ||
			str == "boo" || str == "scowl" || str == "snarl" || str == "sneer" || str == "spank" || str == "tease" || str == "tip" ||
			str == "warn" || str == "frown" || str == "drool" || str == "grumble" || str == "mutter" || str == "bark" || str == "damn" ||
			str == "fist" || str == "whine" || str == "fun" || str == "french" || str == "evil" || str == "bonk");
}

//*********************************************************************
//						isBadSocial
//*********************************************************************

bool isGoodSocial(const bstring& str) {
	return(	str == "cheer" || str == "dance" || str == "comfort" || str == "kiss" || str == "bless" || str == "hbeer" || str == "bow" ||
			str == "curtsey" || str == "high5" || str == "worship" || str == "blow" || str == "caress" || str == "cuddle" || str == "embrace" ||
			str == "grovel" || str == "massage" || str == "salute" || str == "snuggle" || str == "thumb" || str == "congrats" || str == "thank" ||
			str == "smile" || str == "warm" || str == "hug");
}

//*********************************************************************
//						actionFail
//*********************************************************************
// print the error message to the right person

int actionFail(Creature* player, bstring action, bstring second) {
	action.at(0) = up(action.at(0));
	player->print("%s who%s?\n", action.c_str(), second.c_str());
	return(0);
}

//*********************************************************************
//						canDefecate
//*********************************************************************

bool Player::canDefecate() const {
	if(inCombat())
		return(false);
	if(isPoisoned() || isDiseased())
		return(false);
	if(daily[DL_DEFEC].cur < 1 || getLevel() < 4)
		return(false);
	return(true);
}

//*********************************************************************
//						stand
//*********************************************************************

void Creature::stand() {
	Player* player = getPlayer();

	if(player && player->flagIsSet(P_SITTING)) {
		player->unhide();
		player->interruptDelayedActions();
		player->clearFlag(P_SITTING);
	}

	print("You stand up.\n");
	if(getRoom()) {
		broadcast(getSock(), getRoom(), "%M stands up.", this);
		socialHooks(this, "stand");
	}
}


//*********************************************************************
//						wake
//*********************************************************************

void Creature::wake(bstring str, bool noise) {
	if(isMonster())
		return;

	if(!flagIsSet(P_SLEEPING))
		return;

	// can't wake sleeping people with noise
	if(noise && isEffected("deafness"))
		return;

	if(str != "") {
		printColor("%s\n", str.c_str());
		broadcast(getSock(), getRoom(), "%M wakes up.", this);
	}

	clearFlag(P_SLEEPING);
	clearFlag(P_UNCONSCIOUS);
	unhide();

	socialHooks(this, "wake");
}
