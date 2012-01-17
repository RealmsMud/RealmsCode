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

void socialHooks(Creature *creature, MudObject* target, bstring action, bstring result = "") {
	Hooks::run(creature, "doSocial", target, "receiveSocial", action, result);
}

void socialHooks(Creature *target, bstring action, bstring result = "") {
	if(!target->getRoom())
		return;
	Hooks::run(target->getRoom()->first_mon, target, "roomSocial", action, result);
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
	Monster *pet = player->getPet();
	int x=0;

	if(!pet) {
		player->print("You don't have a pet to command.\n");
		return(0);
	}
	if(!player->inSameRoom(pet)) {
		player->print("%M can't hear you.\n", pet);
		return(0);
	}
	if(cmnd->num < 2) {
		player->print("Tell %N to do what?\n", pet);
		return(0);
	}

	// the player is done with their cmnd, let the pet use it
	cmnd->myCommand = 0;
	
	for(; x<cmnd->num-1; x++)
		strcpy(cmnd->str[x], cmnd->str[x+1]);
	cmnd->num--;
	strcpy(cmnd->str[cmnd->num], "");

	// chop the "pet " off
	strcpy(cmnd->fullstr, getFullstrText(cmnd->fullstr, 1).c_str());

	cmdProcess(player, cmnd, true);
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
	ctag	*cp=0;
	Player	*player=0, *pTarget=0;
	Monster	*monster=0;
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
	monster = creature->getMonster();

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
		if(str == "pet" && player->getPet())
			return(orderPet(player, cmnd));
	}


	if(cmnd->num == 2) {
		target = room->findCreature(creature, cmnd->str[1], cmnd->val[1], true, true);
		if(target)
			pTarget = target->getPlayer();
		if( (!target || target == creature) &&
			str != "point" &&
			str != "show" &&
			str != "mark"
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
					!creature->isEffected("blindness")
				) {
					creature->addEffect("blindness", creature->strength.getCur(), 1, player, true, player);
				}
			}
		}
	} else if(str == "vangogh") {

		if(	player &&
			(	!player->ready[WIELD-1] ||
				!(player->ready[WIELD-1]->getWeaponCategory() == "slashing" || player->ready[WIELD-1]->getWeaponCategory() == "chopping")
			)
		) {
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

	} else if(str == "smoke") {
		OUT("You take a puff.\n", "%M puffs on a pipe.");
	} else if(str == "puke") {
		OUT("You blow chunks.\n", "%M vomits on the ground.");
	} else if(str == "hump") {
		if(target) {
			OUT4("You hump %N's leg.\n", "%M humps your leg.\n",
				"%M humps %N's leg.");
		} else {
			actionFail(creature, "hump");
		}
	} else if(str == "burp") {
		OUT("You belch loudly.\n", "%M belches rudely.");
	} else if(str == "ponder") {
		OUT("You ponder the situation.\n", "%M ponders the situation.");
	} else if(str == "think") {
		OUT("You think carefully.\n", "%M thinks carefully.");
	} else if(str == "ack") {
		OUT("You ack.\n", "%M acks.");
	} else if(str == "nervous") {
		OUT("You titter nervously.\n", "%M titters nervously.");
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
	} else if(str == "shake") {
		if(target) {
			OUT4("You shake %N's hand.\n", "%M shakes your hand.\n",
				"%M shakes %N's hand.");
		} else {
			OUT_HISHER("You shake your head.\n", "%M shakes %s head.");
		}
	} else if(str == "anvil") {
		if(target) {
			OUT4("You drop an anvil on %N's head.\n", "%M drops an anvil on your head!\n",
				"%M drops an anvil on %N's head.");
		} else
			actionFail(creature, "drop an anvil on");
	} else if(str == "knee") {
		if(target) {
			OUT4("You knee %N in the crotch.\n", "%M painfully knees you in the crotch.\n",
				"%M painfully knees %N.");
		} else
			actionFail(creature, str);
	} else if(str == "pounce") {
		if(target) {
			OUT4("You pounce on %N.\n", "%M pounces on you.\n", "%M pounces on %N.");
		} else
			actionFail(creature, "pounce on");
	} else if(str == "tickle") {
		if(target) {
			OUT4("You tickle %N mercilessly.\n", "%M tickles you mercilessly.\n",
				"%M tickles %N mercilessly.");
		} else
			actionFail(creature, str);
	} else if(str == "kic") {
		if(target) {
			OUT4("You kick %N.\n", "%M kicks you.\n", "%M kicks %N.");
		} else
			actionFail(creature, "kick");
	} else if(str == "tackle") {
		if(target) {
			OUT4("You tackle %N to the ground.\n", "%M tackles you to the ground.\n",
				"%M tackles %N to the ground.");
		} else
			actionFail(creature, str);
	} else if(str == "frustrate") {
		OUT_HISHER("You pull your hair out.\n", "%M pulls %s hair out in frustration.");
	} else if(str == "livejournal") {
		OUT_HISHER("You sell your soul to capitalism.\n", "%M sells %s soul to capitalism.");
	} else if(str == "tap") {
		OUT_HISHER("You tap your foot impatiently.\n", "%M taps %s foot impatiently.");
	} else if(str == "cheer") {
		if(target) {
			OUT4("You cheer for %N.\n", "%M cheers for you.\n", "%M cheers for %N.");
		} else {
			OUT("You cheer.\n", "%M yells like a cheerleader.");
		}
	} else if(str == "poke") {
		if(target) {
			OUT4("You poke %N.\n", "%M pokes you.\n", "%M pokes %N.");
		} else {
			OUT("You poke everyone.\n", "%M pokes you.");
		}
	} else if(str == "ogle") {
		if(target) {
			OUT4("You ogle %N with carnal intent.\n", "%M ogles you salaciously.\n",
				"%M ogles %N salaciously.");
		} else
			actionFail(creature, str);
	} else if(str == "fart") {
		if(target) {
			OUT4("You fart on %N.\n", "%M farts on you.\n", "%M farts on %N.");
		} else {
			OUT("You fart.\n", "%M breaks wind.");
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
			cp = room->first_ply;
			while(cp) {
				if(	cp->crt != player &&
					!cp->crt->flagIsSet(P_DM_INVIS) &&
					!cp->crt->isUnconscious() &&
					!cp->crt->isDm() &&
					!cp->crt->inCombat()
				) {
					cp->crt->print("%M's fart knocks you unconscious!\n", creature);
					cp->crt->knockUnconscious(30);
					broadcast(sock, cp->crt->getSock(), room,
						"%M falls to the ground unconscious.", cp->crt);
				}
				cp = cp->next_tag;
			}

		}
	} else if(str == "spit") {
		if(target) {
			OUT4("You spit on %N.\n", "%M spits on you.\n",
				"%M spits on %N.");
		} else {
			OUT("You spit.\n", "%M spits.");
		}
	} else if(str == "tag") {
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
	} else if(str == "expose") {
		if(target) {
			OUT4("You expose yourself in front of %N.\n", "%M gets naked in front of you.\n",
				"%M gets naked in front of %N.");
		} else {
			OUT_HIMHER("You expose yourself.\n", "%M exposes %sself.");
		}
	} else if(str == "wink") {
		if(target) {
			OUT4("You wink at %N.\n", "%M winks at you.\n",
				"%M winks at %N.");
		} else {
			OUT("You wink.\n", "%M winks.");
		}
	} else if(str == "wave") {
		if(target) {
			OUT4("You wave to %N.\n", "%M waves to you.\n",
				"%M waves to %N.");
		} else {
			OUT("You wave happily.\n", "%M waves happily.");
		}
	} else if(str == "dream" && player) {
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
	} else if(str == "cackle") {
		OUT("You cackle gleefully.\n", "%M cackles out loud with glee.");
	} else if(str == "chuckle") {
		OUT("You chuckle.\n", "%M chuckles.");
	} else if(str == "woot") {
		OUT("You jump and woot loudly.\n", "%M jumps and yells, \"Can I get a WOOT! WOOT!\".");
	} else if(str == "sigh") {
		OUT("You sigh sadly.\n", "%M lets out a long, sad sigh.");
	} else if(str == "bounce") {
		OUT_HESHE("You bounce around wildly!\n", "%M is so excited, %s can hardly keep still!");
	} else if(str == "shrug") {
		OUT("You shrug your shoulders.\n", "%M shrugs helplessly.");
	} else if(str == "twiddle") {
		OUT_HISHER("You twiddle your thumbs.\n", "%M twiddles %s thumbs.");
	} else if(str == "yawn") {
		OUT("You yawn loudly.\n", "%M yawns out loud.");
	} else if(str == "hum") {
		if(mrand(0,10) || player->getClass() == BARD || player->isStaff()) {
			OUT("You hum a little tune.\n", "%M hums a little tune.");
		} else {
			OUT("You hum off-key.\n", "%M hums off-key.");
		}
	} else if(str == "snap") {
		OUT_HISHER("You snap your fingers.\n", "%M snaps %s fingers.");
	} else if(str == "jump") {
		OUT("You jump for joy.\n", "%M jumps for joy.");
	} else if(str == "skip") {
		OUT("You skip like a girl.\n", "%M skips like a girl.");
	} else if(str == "cry") {
		OUT("You burst into tears.\n", "%M bursts into tears.");
	} else if(str == "bleed") {
		OUT("You bleed profusely.\n", "%M bleeds profusely.");
	} else if(str == "sniff") {
		OUT("You sniff the air.\n", "%M sniffs the air.");
	} else if(str == "whimper") {
		OUT("You whimper like a beat dog.\n", "%M whimpers like a beat dog.");
	} else if(str == "cringe") {
		OUT("You cringe fearfully.\n", "%M cringes fearfully.");
	} else if(str == "gasp") {
		OUT("Your jaw drops.\n", "%M gasps in amazement.");
	} else if(str == "grunt") {
		OUT("You grunt.\n", "%M grunts agonizingly.");
	} else if(str == "flex") {
		OUT_HISHER("You flex your muscles.\n", "%M flexes %s muscles.");
	} else if(str == "blush") {
		OUT("You blush.\n", "%M turns beet-red.");
	} else if(str == "stomp") {
		OUT("You stomp around.\n", "%M stomps around ostentatiously.");
	} else if(str == "usagi") {
		OUT("You hop like a bunny!\n", "%M hops like a bunny!");
	} else if(str == "fume") {
		OUT("You fume.\n", "%M fumes. You can almost see the steam.");
	} else if(str == "rage") {
		OUT("You rage like a madman.\n", "%M rages likes a madman.");
	} else if((str == "defecate" || str == "def" || str == "defe") && player) {

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
	} else if(str == "pout") {
		OUT("You pout.\n", "%M pouts like a child.");
	} else if(str == "faint") {
		OUT("You faint.\n", "%M faints.");
	} else if(str == "goose") {
		if(target) {
			OUT4("You goose %N.\n", "%M gooses you.\n", "%M gooses %N.");
		} else
			actionFail(creature, str);
	} else if(str == "copulate") {
		if(target) {
			OUT4("You copulate with %N.\n", "%M copulates with you.\n",
				"%M copulates with %N.");
		} else
			actionFail(creature, "copulate with");
	} else if(str == "dance") {
		if(target) {
			OUT4("You dance with %N.\n", "%M dances with you.\n",
				"%M dances with %N.");
		} else {
			OUT("You dance about the room.\n", "%M dances about the room.");
		}
	} else if(str == "cstr") {
		if(target) {
			OUT4("You castrate %N.\n", "%M castrates you.\n", "%M castrates %N.");
		} else
			actionFail(creature, "castrate");
	} else if(str == "hug") {
		if(target) {
			OUT4("You hug %N.\n", "%M hugs you close.\n", "%M hugs %N close.");
		} else
			actionFail(creature, str);
	} else if(str == "comfort") {
		if(target) {
			OUT4("You comfort %N.\n", "%M comforts you.\n", "%M comforts %N.");
		} else
			actionFail(creature, str);
	} else if(str == "suck") {
		if(target) {
			OUT4("You suck %N.\n", "%M sucks you.\n", "%M sucks %N.");
		} else
			actionFail(creature, str);
	} else if(str == "kiss") {
		if(target) {
			OUT4("You kiss %N gently.\n", "%M kisses you gently.\n", "%M kisses %N.");
		} else
			actionFail(creature, str);
	} else if(str == "hammer") {
		if(target) {
			OUT4("You hammer %N on the head.\n",
				"%M hammers you on the head. Ouch, that hurt!\n",
				"%M hammers %N on the head.");
		} else
			actionFail(creature, str);
	} else if(str == "bless") {
		if(target) {
			creature->print("You give %N your blessing.\n", target);
			if(actionShow(pTarget, creature))
				pTarget->print("%M gives you %s blessing.\n", creature, creature->hisHer());
			broadcast(sock, target->getSock(), room, "%M gives %N %s blessing.", creature, target, creature->hisHer());
			socialHooks(creature, target, str);
		} else
			actionFail(creature, str);
	} else if(str == "hbeer") {
		if(target) {
			OUT4("You drink a beer in %N's honor.\n",
				"%M drinks a beer in your honor.\n",
				"%M drinks a beer in %N's honor.");
		} else
			actionFail(creature, "drink a beer in", "se honor");
	} else if(str == "slap") {
		if(target) {
			OUT4("You slap %N.\n", "%M slaps you across the face.\n",
				"%M slaps %N across the face.");
		} else
			actionFail(creature, str);
	} else if(str == "glare") {
		if(target) {
			OUT4("You glare menacingly at %N.\n", "%M glares menacingly at you.\n",
				"%M glares menacingly at %N.");
		} else
			actionFail(creature, "glare at");
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
	} else if(str == "bow") {
		if(target) {
			OUT4("You bow before %N.\n", "%M bows before you.\n",
				"%M bows before %N.");
		} else {
			OUT("You make a full-sweeping bow.\n", "%M makes a full-sweeping bow.");
		}
	} else if(str == "curtsy") {
		if(target) {
			OUT4("You curtsy before %N.\n", "%M curtsies before you.\n",
				"%M curtsies before %N.");
		} else {
			OUT("You curtsy rather daintely.\n", "%M curtsies.");
		}
	} else if(str == "confused") {
		OUT("You look bewildered.\n", "%M looks bewildered.");
	} else if(str == "hiccup") {
		OUT("You hiccup.\n", "%M hiccups noisily.");
	} else if(str == "scratch") {
		OUT_HISHER("You scratch your head cluelessly.\n",
			"%M scratches %s head cluelessly.");
	} else if(str == "strut") {
		OUT("You strut around vainly.\n", "%M struts around vainly.");
	} else if(str == "maniac") {
		OUT("You laugh maniacally.\n", "%M laughs maniacally.");
	} else if(str == "sulk") {
		OUT("You sulk.\n", "%M sulks in dejection.");
	} else if(str == "satisfied") {
		OUT("You smile with satisfaction.\n", "%M smiles with satisfaction.");
	} else if(str == "dismiss") {

		if(target) {
			if(pTarget || !player) {
				OUT4("You curtly dismiss %N.\n", "%M dismisses you curtly.\n",
					"%M curtly dismisses %N .");
			} else {
				if(target->isPet() && target->following == creature) {
					sock->print("You dismiss %N.\n", target);
					broadcast(sock, room, "%M dismisses %N.", creature, target);

					if(target->isUndead())
						broadcast(NULL, room, "%M wanders away.", target);
					else
						broadcast(NULL, target->getRoom(), "%M fades away.", target);
					target->die(target->following);
					gServer->delActive(monster);
				}
			}
		} else {
			sock->print("Dismiss who?\n");
			return(0);
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

	} else if(str == "wince") {
		OUT("You wince painfully.\n", "%M winces painfully.");
	} else if(str == "roll") {
		OUT_HISHER("You roll your eyes in exasperation.\n",
			"%M rolls %s eyes in exasperation.");
	} else if(str == "raise") {
		OUT("You raise an eyebrow questioningly.\n",
			"%M raises an eyebrow questioningly.");
	} else if(str == "grr") {
		if(target) {
			OUT4("You stare menacingly at %N.\n",
				"%M threatens you with a look of death.\n",
				"%M stares at %N with a look of hatred.");
		} else {
			OUT("You start looking angry.\n", "%M looks angrily around.");
		}
	} else if(str == "growl") {
		if(target) {
			OUT4("You growl at %N.\n", "%M threatens you with a growl.\n",
				"%M growls at %N threateningly.");
		} else {
			OUT("You growl.\n", "%M growls threateningly.");
		}
	} else if(str == "dirt") {
		if(target) {
			OUT4("You kick dirt on %N.\n", "%M kicks dirt on you.\n",
				"%M kicks dirt on %N.");
		} else {
			OUT("You kick the dirt.\n", "%M kicks the dirt.");
		}
	} else if(str == "cough") {
		if(target) {
			OUT4("You cough on %N.\n", "%M coughs on you.\n", "%M coughs on %N.");
		} else {
			OUT("You cough politely.\n", "%M coughs politely.");
		}
	} else if(str == "bird") {
		if(target) {
			OUT4("You flip off %N.\n", "%M flips you the bird.\n",
				"%M flips off %N.");
		} else {
			OUT("You gesture indignantly.\n",
				"%M gestures indignantly.");
		}
	} else if(str == "grab") {
		if(target) {
			OUT4("You pull on %N.\n", "%M pulls on you eagerly.\n",
				"%M pulls on %N eagerly.");
		} else
			actionFail(creature, str);

	} else if(str == "shove") {
		if(target) {
			OUT4("You push %N away.\n", "%M pushes you away.\n",
				"%M shoves %N away.");
		} else
			actionFail(creature, str);
	} else if(str == "high5") {
		if(target) {
			OUT4("You slap %N a triumphant highfive.\n",
				"%M slaps you a triumphant highfive.\n",
				"%M slaps %N a triumphant highfive.");
		} else
			actionFail(creature, "highfive");
	} else if(str == "moon") {
		if(target) {
			OUT4("You moon %N.\n",
				"%M moons you. It's a full moon tonight!\n",
				"%M moons %N. It's a full moon tonight!");
		} else {
			OUT_HISHER("You moon the world.\n",
				"%M drops %s pants and moons the world.");
		}
	} else if(str == "purr") {
		if(target) {
			OUT4("You purr at %N.\n",
				"%M purrs provocatively at you.\n",
				"%M purrs provocatively at %N.");
		} else {
			OUT("You purr provocatively.\n",
				"%M purrs provocatively.");
		}
	} else if(str == "taunt") {
		if(target) {
			OUT4("You taunt and jeer at %N.\n",
				"%M taunts and jeers at you.\n",
				"%M taunts and jeers at %N.");
		} else
			actionFail(creature, str);
	} else if(str == "eye") {
		if(target) {
			OUT4("You eye %N suspiciously.\n",
				"%M eyes you suspiciously.\n",
				"%M eyes %N suspiciously.");
		} else
			actionFail(creature, str);
	} else if(str == "worship") {
		if(target) {
			OUT4("You worship %N.\n",
				"%M bows down in worship of you.\n",
				"%M bows down and worships %N.");
		} else
			actionFail(creature, str);
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
	} else if(str == "meiko") {
		if(target) {
			OUT4("You meiko %N lasciviously.\n",
				"%M meikos you lasciviously.\n",
				"%M meikos %N lasciviously.");
		} else {
			OUT("You meiko lustfully.\n", "%M meikos lustfully.");
		}
	} else if(str == "hail") {
		if(target) {
			OUT4("You hail %N.\n",
				"%M hails you.\n",
				"%M says, \"Hail, %N.\"");
		} else {
			OUT("You hail everyone.\n", "%M says, \"Hail!\"");
		}
	} else if(str == "lobotomy") {

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

	} else if(str == "punch") {
		if(target) {
			OUT4("You punch %N.\n",
				"%M punches you.\n",
				"%M punches %N.");
		} else
			actionFail(creature, str);
	} else if(str == "nibble") {
		if(target) {
			OUT4("You nibble %N's ear.\n",
				"%M nibbles your ear.\n",
				"%M nibbles %N's ear.");
		} else
			actionFail(creature, str);
	} else if(str == "lick") {
		if(target) {
			OUT4("You lick %N all over.\n",
				"%M licks you all over.\n",
				"%M licks %N all over.");
		} else {
			OUT_HISHER("You lick your wounds.\n", "%M licks %s wounds.");
		}
	} else if(str == "choke") {
		if(target) {
			OUT4("You choke %N.\n",
				"%M chokes you.\n",
				"%M chokes %N.");
		} else {
			OUT("You choke.\n", "%M chokes.");
		}
	} else if(str == "psycho") {
		if(target) {
			OUT4("You grin psychotically at %N.\n",
				"%M grins at you psychotically.\n",
				"%M grins psychotically at %N.");
		} else {
			OUT("You grin like a psychopath.\n", "%M grins like a psychopath.");
		}
	} else if(str == "zip") {
		if(target) {
			OUT4("You zip %N's lips.\n",
				"%M zips your lips.\n",
				"%M zips %N's lips.");
		} else {
			OUT_HISHER("You zip your lips.\n", "%M zips %s lips.");
		}
	} else if(str == "behind") {
		if(target) {
			OUT4("You hide behind %N for protection.\n",
				"%M runs behind you and hides.\n",
				"%M hides behind %N for protection.");
		} else {
			OUT("You look for something to hide behind.\n", "%M looks for something to hide behind.");
		}
	} else if(str == "narrow") {
		if(target) {
			sock->print("You narrow your eyes at %N.\n", target);
			target->print("%M narrows %s eyes at you.\n", creature, creature->hisHer());
			broadcast(sock, target->getSock(), room, "%M narrows %s eyes at %N.", creature, creature->hisHer(), target);
			socialHooks(creature, target, str);
		} else {
			OUT_HISHER("You narrow your eyes.\n", "%M narrows %s eyes evilly.");
		}
	} else if(str == "meow") {
		if(target) {
			OUT4("You meow seductively at %N.\n",
				"%M meows at you seductively.\n",
				"%M meows seductively at %N.");
		} else {
			OUT("You meow like a cat.\n", "%M meows like a cat.");
		}
	} else if(str == "bhand") {
		if(target) {
			OUT4("You backhand %N.\n",
				"%M backhands you.\n",
				"%M backhands %N.");
		} else
			actionFail(creature, "backhand");
	} else if(str == "beg") {
		if(target) {
			OUT4("You beg to %N.\n",
				"%M begs pathetically to you.\n",
				"%M begs pathetically to %N.");
		} else {
			OUT("You beg pathetically.\n", "%M begs pathetically.");
		}
	} else if(str == "blow") {
		if(target) {
			OUT4("You blow %N a kiss.\n",
				"%M blows you a kiss.\n",
				"%M blows %N a kiss.");
		} else
			actionFail(creature, "blow a kiss to");
	} else if(str == "caress") {
		if(target) {
			OUT4("You caress %N's body softly.\n",
				"%M caresses your body softly.\n",
				"%M caresses %N's body softly.");
		} else {
			OUT_HIMHER("You caress yourself.\n", "%M caresses %sself all over.");
		}
	} else if(str == "cower") {
		if(target) {
			OUT4("You cower indignantly before %N.\n",
				"%M cowers idignantly before you.\n",
				"%M cowers indignantly before %N.");
		} else {
			OUT("You cower indignantly.\n", "%M cowers indignantly.");
		}
	} else if(str == "cuddle") {
		if(target) {
			OUT4("You cuddle with %N.\n",
				"%M cuddles up with you.\n",
				"%M cuddles with %N.");
		} else
			actionFail(creature, "cuddle with");
	} else if(str == "disgust") {
		if(target) {
			OUT4("You look disgustedly at %N.\n",
				"%M gives you a disgusted look.\n",
				"%M looks disgustedly at %N.");
		} else {
			OUT("You look disgusted.\n", "%M looks disgusted.");
		}
	} else if(str == "embrace") {
		if(target) {
			OUT4("You embrace %N lovingly.\n",
				"%M embraces you lovingly.\n",
				"%M embraces %N lovingly.");
		} else
			actionFail(creature, str);
	} else if(str == "fondle") {
		if(target) {
			OUT4("You fondle %N.\n",
				"%M fondles you.\n",
				"%M fondles %N.");
		} else {
			OUT_HIMHER("You fondle yourself.\n", "%M fondles %sself.");
		}
	} else if(str == "grope") {
		if(target) {
			OUT4("You grope %N all over.\n",
				"%M gropes you all over.\n",
				"%M gropes %N.");
		} else {
			OUT_HIMHER("You grope yourself.\n", "%M gropes %sself.");
		}
	} else if(str == "grovel") {
		if(target) {
			OUT4("You grovel obsequiously before %N.\n",
				"%M grovels obsequiously before you.\n",
				"%M grovels obsequiously before %N.");
		} else
			actionFail(creature, "grovel before");
	} else if(str == "listen") {
		if(target) {
			OUT4("You listen to %N carefully.\n",
				"%M listens carefully to you.\n",
				"%M listens to %N carefully.");
		} else {
			OUT("You listen carefully.\n", "%M listens carefully.");
		}
	} else if(str == "peck") {
		if(target) {
			OUT4("You peck %N on the cheek.\n",
				"%M pecks you on the cheek.\n",
				"%M pecks %N on the cheek.");
		} else {
			OUT("You peck the ground like a chicken.\n", "%M pecks the ground like a chicken.");
		}
	} else if(str == "loom") {
		if(target) {
			OUT4("You loom menacingly over %N.\n",
				"%M looms menacingly over you.\n",
				"%M looms menacingly over %N.");
		} else
			actionFail(creature, "loom over");
	} else if(str == "pants") {
		if(target) {
			OUT4("You depants %N!\n",
				"%M yanks down your pants! How embarrasing!\n",
				"%M yanked down %N's pants! DOH!");
		} else
			actionFail(creature, "yank down", "se pants");
	} else if(str == "lust") {
		if(target) {
			OUT4("You eye %N lustfully.\n",
				"%M eyes you lustfully.\n",
				"%M eyes %N lustfully.");
		} else
			actionFail(creature, "lust after");
	} else if(str == "massage") {
		if(target) {
			OUT4("You give %N a sensual massage.\n",
				"%M massages you sensually.\n",
				"%M massages %N sensually.");
		} else {
			OUT_HIMHER("You massage yourself.\n", "%M massages %sself.");
		}
	} else if(str == "noogie") {
		if(target) {
			OUT4("You give %N a death noogie!\n",
				"%M gives you a death noogie!\n",
				"%M gives %N a death noogie!");
		} else
			actionFail(creature, str);
	} else if(str == "nudge") {
		if(target) {
			OUT4("You nudge %N annoyingly.\n",
				"%M nudges you annoyingly.\n",
				"%M nudges %N annoyingly.");
		} else
			actionFail(creature, str);
	} else if(str == "pinch") {
		if(target) {
			OUT4("You pinch %N teasingly.\n",
				"%M pinches you teasingly.\n",
				"%M pinches %N teasingly.");
		} else {
			OUT_HIMHER("You pinch yourself. Ow!\n", "%M pinched %sself. It looks like it hurt.");
		}
	} else if(str == "pummel") {
		if(target) {
			OUT4("You pummel %N thouroughly.\n",
				"%M pummels you thouroughly.\n",
				"%M thouroughly pummels %N.");
		} else
			actionFail(creature, str);
	} else if(str == "rag") {
		if(target) {
			OUT4("You offer %N a drool and slobber rag.\n",
				"%M gives you a drool and slobber rag.\n",
				"%M gives %N a drool and slobber rag.");
		} else {
			OUT_HISHER("You wipe your mouth with a rag.\n", "%M wipes %s mouth with a rag.");
		}
	} else if(str == "trip") {
		if(target) {
			OUT4("You trip %N.\n",
				"%M trips you off your feet.\n",
				"%M trips %N.");
		} else {
			OUT_HISHER("You trip all over yourself.\n", "%M trips all over %sself.");
		}
	} else if(str == "pester") {
		if(target) {
			OUT4("You pester %N.\n",
				"%M pesters you persistently.\n",
				"%M pesters %N persistently.");
		} else
			actionFail(creature, str);
	} else if(str == "rub") {
		if(target) {
			OUT4("You rub %N all over.\n",
				"%M rubs you all over.\n",
				"%M rubs %N all over.");
		} else {
			OUT_HIMHER("You rub yourself.\n", "%M rubs %sself.");
		}
	} else if(str == "boo") {
		if(target) {
			OUT4("You boo and hiss at %N.\n",
				"%M boos and hisses at you.\n",
				"%M boos and hisses at %N.");
		} else {
			OUT("You boo and hiss in disgust.\n", "%M boos and hisses in disgust.");
		}
	} else if(str == "oink") {
		if(target) {
			OUT4("You oink at %N.\n",
				"%M oinks at you.\n",
				"%M oinks at %N.");
		} else {
			OUT("You oink like a pig.\n", "%M oinks like a pig.");
		}
	} else if(str == "shush") {
		if(target) {
			OUT4("You shush %N.\n",
				"%M tells you to shhh!\n",
				"%M shushes %N.");
		} else
			actionFail(creature, str);
	} else if(str == "ruffle") {
		if(target) {
			OUT4("You ruffle %N's hair.\n",
				"%M ruffles your hair.\n",
				"%M ruffles %N's hair.");
		} else {
			OUT_HISHER("You ruffle your hair.\n", "%M ruffles %s hair.");
		}
	} else if(str == "pose") {
		if(target) {
			OUT4("You strike a pose for %N.\n",
				"%M strikes a triumpant pose for you.\n",
				"%M strikes a triumphant pose before %N.");
		} else {
			OUT("You strike a triumphant pose.\n", "%M strikes a triumphant pose.");
		}
	} else if(str == "blink") {
		if(target) {
			OUT4("You blink at %N.\n",
				"%M blinks at you.\n",
				"%M blinks at %N.");
		} else {
			OUT("You blink.\n", "%M blinks.");
		}
	} else if(str == "salute") {
		if(target) {
			OUT4("You give %N a salute.\n",
				"%M salutes you.\n",
				"%M gives %N a salute.");
		} else
			actionFail(creature, str);
	} else if(str == "scowl") {
		if(target) {
			OUT4("You scowl hatefully at %N.\n",
				"%M scowls hatefully at you.\n",
				"%M scowls hatefully at %N.");
		} else {
			OUT("You scowl.\n", "%M scowls.");
		}
	} else if(str == "seduce") {
		if(target) {
			OUT4("You moan seductively at %N.\n",
				"%M seductively moans to you.\n",
				"%M moans seductively at %N.");
		} else
			actionFail(creature, str);
	} else if(str == "slobber") {
		if(target) {
			OUT4("You slobber all over %N.\n",
				"%M slobbers all over you.\n",
				"%M slobbers all over %N.");
		} else {
			OUT("You slobber all over the place.\n", "%M slobbers all over the place.");
		}
	} else if(str == "smack") {
		if(target) {
			OUT4("You smack %N upside the head.\n",
				"%M smacks you upside the head.\n",
				"%M smacks %N upside the head.");
		} else
			actionFail(creature, str);
	} else if(str == "smell") {
		if(target) {
			OUT4("You smell %N inquisitively.\n",
				"%M smells you inquisitively.\n",
				"%M smells %N inquisitively.");
		} else {
			OUT("You smell the air.\n", "%M smells the air.");
		}
	} else if(str == "snarl") {
		if(target) {
			OUT4("You snarl odiously at %N.\n",
				"%M snarls odiously at you.\n",
				"%M snarls odiously at %N.");
		} else {
			OUT("You snarl odiously.\n", "%M snarls odiously.");
		}
	} else if(str == "sneer") {
		if(target) {
			OUT4("You sneer contemptuously at %N.\n",
				"%M sneers at you contemptuously.\n",
				"%M sneers contemptuously at %N.");
		} else {
			OUT("You sneer contemptuously.\n", "%M sneers contemptuously.");
		}
	} else if(str == "snuggle") {
		if(target) {
			OUT4("You snuggle with %N.\n",
				"%M snuggles with you.\n",
				"%M snuggles with %N.");
		} else
			actionFail(creature, "snuggle with");
	} else if(str == "spank") {
		if(target) {
			OUT4("You spank %N on the behind.\n",
				"%M spanks you on the behind.\n",
				"%M spanks %N on the behind.");
		} else
			actionFail(creature, str);
	} else if(str == "squeeze") {
		if(target) {
			OUT4("You squeeze %N tightly.\n",
				"%M squeezes you tightly.\n",
				"%M squeezes %N tightly.");
		} else
			actionFail(creature, str);
	} else if(str == "squint") {
		if(target) {
			OUT4("You squint dubiously at %N.\n",
				"%M squints dubiously at you.\n",
				"%M squints dubiously at %N.");
		} else {
			OUT("You squint.\n", "%M squints.");
		}
	} else if(str == "haha") {
		if(target) {
			OUT4("You burst out laughing at %N.\n",
				"%M bursts out laughing at you.\n",
				"%M bursts out laughing at %N.");
		} else {
			OUT("You burst out laughing.\n", "%M bursts out laughing.");
		}
	} else if(str == "hysterical") {
		if(target) {
			OUT4("You laugh hysterically at %N.\n",
				"%M laughs hysterically at you.\n",
				"%M laughs hysterically at %N.");
		} else {
			OUT("You laugh hysterically.\n", "%M laughs hysterically.");
		}
	} else if(str == "swat") {
		if(target) {
			OUT4("You swat %N like a bug.\n",
				"%M swats you like a bug.\n",
				"%M swats %N like a bug.");
		} else {
			OUT("You swat at flies in the air.\n", "%M swats at flies in the air.");
		}
	} else if(str == "stare") {
		if(target) {
			OUT4("You stare incredulously at %N.\n",
				"%M stares at you incredulously.\n",
				"%M stares incredulously at %N.");
		} else {
			OUT("You stare emptily into space.\n", "%M stares emptily into space.");
		}
	} else if(str == "thumb") {
		if(target) {
			OUT4("You give %N a thumbs up.\n",
				"%M gives you a thumbs up.\n",
				"%M gives %N a thumbs up.");
		} else {
			OUT("You give a thumbs up.\n", "%M gives a thumbs up.");
		}
	} else if(str == "thwack") {
		if(target) {
			OUT4("You thwack %N on the back.\n",
				"%M thwacks you on the back.\n",
				"%M thwacks %N on the back.");
		} else
			actionFail(creature, str);
	} else if(str == "tip") {
		if(target) {
			OUT4("You tip %N like a cow.\n",
				"%M tips you like a cow.\n",
				"%M tips %N like a cow.");
		} else
			actionFail(creature, str);
	} else if(str == "tease") {
		if(target) {
			OUT4("You tease %N mischievously.\n",
				"%M teases you mischievously.\n",
				"%M teases %N mischievously.");
		} else
			actionFail(creature, str);
	} else if(str == "tug") {
		if(target) {
			OUT4("You tug %N's shirt relentlessly.\n",
				"%M tugs your shirt relentlessly.\n",
				"%M tugs %N's shirt relentlessly.");
		} else
			actionFail(creature, "tug on");
	} else if(str == "warn") {
		if(target) {
			OUT4("You give %N a glance of warning.\n",
				"%M gives you a glance of warning.\n",
				"%M gives %N a glance of warning.");
		} else
			actionFail(creature, str);
	} else if(str == "laugh") {
		if(target) {
			OUT4("You laugh at %N.\n",
				"%M laughs at you.\n",
				"%M laughs at %N.");
		} else {
			OUT("You fall down laughing.\n", "%M falls down laughing.");
		}
	} else if(str == "snicker") {
		if(target) {
			OUT4("You snicker at %N.\n",
				"%M snickers at you.\n",
				"%M snickers at %N.");
		} else {
			OUT("You snicker.\n", "%M snickers.");
		}
	} else if(str == "clap") {
		if(target) {
			OUT4("You clap for %N.\n",
				"%M claps for you.\n",
				"%M claps for %N.");
		} else {
			OUT_HISHER("You clap your hands.\n", "%M claps %s hands.");
		}
	} else if(str == "nod") {
		if(target) {
			OUT4("You nod to %N.\n",
				"%M nods to you.\n",
				"%M nods to %N.");
		} else {
			OUT("You nod.\n", "%M nods.");
		}
	} else if(str == "wicked") {
		if(target) {
			OUT4("You laugh wickedly at %N.\n",
				"%M laughs at you wickedly.\n",
				"%M laughs wickedly at %N.");
		} else {
			OUT("You laugh wickedly.\n", "%M laughs wickedly.");
		}
	} else if(str == "tnl" && player) {

		if(player->getActualLevel() > player->getLevel()) {
			broadcast(player->getSock(), player->getRoom(), "%M needs %s experience to relevel.", player, player->expToLevel(false).c_str());
			sock->print("You need %s experience to relevel.\n", player->expToLevel(false).c_str());
		} else {
			broadcast(player->getSock(), player->getRoom(), "%M needs %s experience to level.", player, player->expToLevel(false).c_str());
			sock->print("You need %s experience to level.\n", player->expToLevel(false).c_str());
		}

	} else if(str == "smile") {
		if(target) {
			OUT4("You smile happily to %N.\n",
				"%M smiles happily to you.\n",
				"%M smiles happily at %N.");
		} else {
			OUT("You smile happily.\n", "%M smiles happily.");
		}
	} else if(str == "grin") {
		if(target) {
			OUT4("You grin evilly at %N.\n", "%M grins at you evilly.\n",
				"%M grins at %N evilly.");
		} else {
			OUT("You grin evilly.\n", "%M grins evilly.");
		}
	} else if(str == "whip") {
		if(target) {
			OUT4("You whip %N savagely.\n",
				"%M whips you savagely.\n",
				"%M whips %N into submission.");
		} else
			actionFail(creature, str);
	} else if(str == "frown") {
		if(target) {
			OUT4("You frown at %N.\n",
				"%M frowns at you.\n",
				"%M frowns at %N.");
		} else {
			OUT("You frown.\n", "%M frowns.");
		}
	} else if(str == "giggle") {
		if(target) {
			OUT4("You giggle at %N.\n",
				"%M giggles at you.\n",
				"%M giggles at %N.");
		} else {
			OUT("You giggle inanely.\n", "%M giggles inanely.");
		}
	} else if(str == "whistle") {
		if(target) {
			OUT4("You whistle at %N.\n",
				"%M whistles at you.\n",
				"%M whistles at %N.");
		} else {
			OUT("You whistle a tune.\n", "%M whistles a tune.");
		}
	} else if(str == "smirk") {
		if(target) {
			OUT4("You smirk wryly at %N.\n",
				"%M smirks at you wryly.\n",
				"%M smirks wryly at %N.");
		} else {
			OUT("You smirk wryly.\n", "%M smirks wryly.");
		}
	} else if(str == "drool") {
		if(target) {
			OUT4("You drool on %N.\n",
				"%M drools all over you.\n",
				"%M drools all over %N.");
		} else {
			OUT_HIMHER("You drool.\n", "%M drools all over %sself.");
		}
	} else if(str == "grumble") {
		if(target) {
			OUT4("You grumble darkly at %N.\n",
				"%M grumbles darkly at you.\n",
				"%M grumbles darkly at %N.");
		} else {
			OUT("You grumble darkly.\n", "%M grumbles darkly.");
		}
	} else if(str == "mutter") {
		if(target) {
			OUT4("You mutter obscenities about %N.\n",
				"%M mutters obscenities about you.\n",
				"%M mutters obscenities about %N.");
		} else {
			OUT_HISHER("You mutter.\n", "%M mutters obscenities under %s breath.")
		}
	} else if(str == "congrats") {
		if(target) {
			OUT4("You congratulate %N.\n",
				"%M congratulates you.\n",
				"%M congratulates %N.");
		} else {
			OUT_HIMHER("You congratulate yourself.\n", "%M congratulates %sself.")
		}
	} else if(str == "imitate") {
		if(target) {
			OUT4("You imitate %N.\n",
				"%M imitates you.\n",
				"%M imitates %N.");
		} else
			actionFail(creature, str);
	} else if(str == "bark") {
		if(target) {
			OUT4("You bark and growl at %N.\n",
				"%M barks and growls at you.\n",
				"%M barks and growls at %N.");
		} else {
			OUT("You bark and growl.\n", "%M barks and growls.")
		}
	} else if(str == "paranoid") {
		if(target) {
			OUT4("You look warily at %N.\n",
				"%M gives you a paranoid look.\n",
				"%M looks at %N with paranoia.");
		} else
			actionFail(creature, "be paranoid about");
	} else if(str == "damn") {
		if(target) {
			OUT4("You damn %N to the Abyss.\n",
				"%M damns you to the Abyss.\n",
				"%M damns %N to the Abyss.");
		} else {
			OUT("You damn the gods.\n", "%M damns the gods.");
		}
	} else if(str == "fist") {
		if(target) {
			OUT4("You shake your fist at %N.\n",
				"%M shakes a fist at you in rage.\n",
				"%M shakes a fist at %N in rage.");
		} else {
			OUT_HISHER("You shake your fist at the gods.\n", "%M shakes %s fist at the gods.");
		}
	} else if(str == "defenestrate") {
		if(target) {
			OUT4("You toss %N out the nearest window.\n",
				"%M tosses you out the nearest window.\n",
				"%M tosses %N out the nearest window.");
		} else {
			OUT("You dive out of the nearest window.\n", "%M dives out the nearest window.");
		}
	} else if(str == "vomjom") {
		if(target) {
			OUT4("You challenge %N to a game of Starcraft.\n",
				"%M challenges you to a game of Starcraft, insisting that you will lose in a horrible way.\n",
				"%M says %N sucks at Starcraft.");
		} else
			actionFail(creature, str);
	} else if(str == "whine") {
		if(target) {
			OUT4("You whine to %N.\n",
				"%M whines annoyingly to you.\n",
				"%M whines annoyingly to %N.");
		} else {
			OUT("You whine annoyingly.\n", "%M whines annoyingly.");
		}
	} else if(str == "groan") {
		if(target) {
			OUT4("You groan miserably at %N.\n",
				"%M groans at you miserably.\n",
				"%M groans miserably at %N.");
		} else {
			OUT("You groan miserably.\n", "%M groans miserably.");
		}
	} else if(str == "beam") {
		if(target) {
			OUT4("You beam at %N happily.\n",
				"%M beams happily at you.\n",
				"%M beams happily at %N.");
		} else {
			OUT("You beam happily.\n", "%M beams happily.");
		}
	} else if(str == "warm") {
		if(target) {
			OUT4("You give %N a warm smile.\n",
				"%M gives you a warm smile.\n",
				"%M gives %N a warm smile.");
		} else {
			OUT("You smile warmly.\n", "%M smiles warmly.");
		}
	} else if(str == "fangs") {
		if(!creature->isEffected("vampirism")) {
			sock->print("You don't have any fangs!\n");
			return(0);
		}

		if(target) {
			OUT4("You bare your fangs at %N.\n",
				"%M bares vampire fangs at you.\n",
				"%M bares vampire fangs at %N.");
		} else {
			OUT_HISHER("You show your fangs to everyone.\n", "%M shows %s bloody fangs to everyone.");
		}
	} else if(str == "hiss") {
		if(!creature->isEffected("vampirism")) {
			sock->print("Only vampires can hiss evilly at people.\n");
			return(0);
		}

		if(target) {
			OUT4("You hiss evilly at %N.\n",
				"%M hisses evilly at you.\n",
				"%M hisses evilly at %N.");
		} else {
			OUT("You hiss evilly at everyone in the room.\n", "%M hisses evilly at everyone in the room.");
		}
	} else if(str == "gnaw") {
		if(!creature->isEffected("lycanthropy")) {
			sock->print("Only werewolves can gnaw on people.\n");
			return(0);
		}

		if(target) {
			target->wake("You awaken suddenly!");
			OUT4("You ravenously gnaw on %N.\n",
				"%M gnaws on you ravenously.\n",
				"%M ravenously gnaws on %N.");
		} else {
			OUT("You gnaw on a bone.\n", "%M gnaws on a bone.");
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
	} else if(str == "sneeze") {
		if(target) {
			OUT4("You sneeze on %N.\n",
				"%M sneezes on you.\n",
				"%M sneezed on %N.");
		} else {
			OUT("You sneeze.\n", "%M sneezes.");
		}
	} else if(str == "show") {


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
	} else if(str == "fun") {
		if(target) {
			OUT4("You make fun of %N.\n",
				"%M makes fun of you.\n",
				"%M makes fun of %N.");
		} else
			actionFail(creature, "make fun of");
	} else if(str == "french") {
		if(target) {
			OUT4("You french kiss with %N.\n",
				"%M french kisses you.\n",
				"%M frenches with %N.");
		} else
			actionFail(creature, "french kiss");
	} else if(str == "pan") {
		if(target) {
			OUT4("You smack %N over the head with a frying pan.\n",
				"%M smacks you over the head with a frying pan.\n",
				"%M smacks %N over the head with a frying pan.");
		} else
			actionFail(creature, "smack", "m over the head with a frying pan?");
	} else if(str == "squeeze") {
		if(target) {
			OUT4("You squeeze %N tightly.\n",
				"%M squeezes you close.\n",
				"%M squeezes %N close.");
		} else
			actionFail(creature, str);
	} else if(str == "smooch") {
		if(target) {
			OUT4("You smooch with %N.\n",
				"%M smooches with you.\n",
				"%M smooches with %N.");
		} else
			actionFail(creature, str);
	} else if(str == "evil") {
		if(target) {
			OUT4("You look evilly at %N.\n",
				"%M gives you an evil look.\n",
				"%M gives %N an evil look.");
		} else
			actionFail(creature, "look evilly at");
	} else if(str == "wedgy") {
		if(target) {
			OUT4("You give %N a wedgy!\n",
				"%M gives you a wedgy!\n",
				"%M gives %N a wedgy...OWE!");
		} else
			actionFail(creature, "give", "m a wedgy");
	} else if(str == "bitchslap") {
		if(target) {
			OUT4("You bitchslap %N like a two cent hooker.\n",
				"%M bitchslaps you like a two cent hooker.\n",
				"%M bitchslaps %N like a two cent hooker.");
		} else
			actionFail(creature, str);
	} else if(str == "beckon") {
		if(target) {
			OUT4("You beckon %N to follow you.\n",
				"%M beckons you to follow.\n",
				"%M beckons %N to follow.");
		} else
			actionFail(creature, "beckon", "m to follow you");
	} else if(str == "bonk") {
		if(target) {
			OUT4("You bonk %N on the head. This means war!\n",
				"%M bonks you on the head. This means war!\n",
				"%M bonks %N on the head. This means war!");
		} else
			actionFail(creature, str);
	} else if(str == "thank") {
		if(target) {
			OUT4("You thank %N profusely.\n",
				"%M thanks you profusely.\n",
				"%M thanks %N profusely.");
		} else
			actionFail(creature, str);
	} else if(str == "jk") {
		OUT("You tell everyone you're just kidding.\n", "%M is only kidding.");
	}
	else if(str == "halo") {
		OUT_HISHER("You point at the halo about your head.\n", "%M points at the halo about %s head.");
	} else if(str == "die") {
		OUT_HISHER("You play dead dramatically.\n", "%M grabs %s chest and falls over.");
	} else if(str == "twitch") {
		OUT("You twitch insanely.\n", "%M twitches insanely.");
	} else if(str == "bubble") {
		OUT("You bubble happily.\n", "%M bubbles happily.");
	} else if(str == "moan") {
		OUT("You moan with pleasure.\n", "%M moans with pleasure.");
	} else if(str == "afk") {
		sock->print("Please use the command \"set afk\" to define yourself afk.\n");
	} else if(str == "ahem") {
		OUT_HISHER("You clear your throat loudly.\n", "%M clears %s throat loudly.");
	} else if(str == "attention") {
		OUT("You stand at attention.\n", "%M snaps to attention.");
	} else if(str == "babble") {
		OUT("You babble incoherently.\n", "%M babbles incoherently.");
	} else if(str == "beer") {
		OUT("You slam a beer.\n", "%M slams a beer.");
	} else if(str == "ballz") {
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
	} else if(str == "collapse") {
		OUT("You collapse from exhaustion.\n", "%M collapses from exhaustion.");
	} else if(str == "comb") {
		OUT_HISHER("You comb your hair.\n", "%M combs %s hair.");
	} else if(str == "compose") {
		OUT_HIMHER("You compose yourself.\n", "%M visibly composes %sself.");
	} else if(str == "crazy") {
		OUT_HISHER("You feel crazy.\n", "%M gets a crazy look in %s eyes.");
	} else if(str == "curl") {
		OUT("You curl up in a ball.\n", "%M curls up in a ball to sleep.");
	} else if(str == "curse") {
		OUT("You curse indignantly.\n", "%M curses like a sailor.");
	} else if(str == "daydream") {
		OUT("You daydream.\n", "%M daydreams.");
	} else if(str == "doh") {
		OUT("You slap yourself on the head.\n", "%M says, \"DOH!\"");
	} else if(str == "fidget") {
		OUT("You fidget uncontrollably.\n", "%M fidgets uncontrollably.");
	} else if(str == "foam") {
		OUT("You foam at the mouth.\n", "%M foams at the mouth.");
	} else if(str == "freak") {
		OUT("You freak out.\n", "%M freaks out in a bad way.");
	} else if(str == "freeze") {
		OUT_HISHER("You freeze.\n", "%M freezes in %s tracks.");
	} else if(str == "grimace") {
		OUT("You grimace.\n", "%M grimaces in pain.");
	} else if(str == "gag") {
		OUT("You gag uncontrollably.\n", "%M gags disgustedly.");
	} else if(str == "gibber") {
		OUT("You gibber insanely.\n", "%M gibbers insanely.");
	} else if(str == "grind") {
		OUT_HISHER("You grind your teeth.\n", "%M grinds %s teeth.");
	} else if(str == "lag") {
		OUT("You inform everyone you are lagging horrendously.\n", "%M is lagging horrendously.");
	} else if(str == "monkey") {
		OUT("You dance the monkey boy.\n", "%M does the monkey boyee yeeeaaaa....");
	} else if(str == "jiggy") {
		OUT("You get jiggy with it.\n", "%M gets jiggy with it.");
	} else if(str == "mope") {
		OUT("You mope sadly in a corner.\n", "%M mopes sadly in a corner.");
	} else if(str == "moo") {
		OUT("You moo.\n", "%M moos like a cow.");
	} else if(str == "mosh") {
		OUT_HISHER("You mosh around wildly.\n", "%M moshes in %s own mosh pit.");
	} else if(str == "murmur") {
		if(player && player->flagIsSet(P_SLEEPING)) {
			OUT_HISHER("You murmur.\n", "%M murmurs something in %s sleep.");
		} else {
			OUT("You murmur.\n", "%M murmurs something.");
		}
	} else if(str == "pace") {
		OUT("You pace.\n", "%M paces back and forth impatiently.");
	} else if(str == "panic") {
		OUT("You start to panic.\n", "%M begins to panic.");
	} else if(str == "pant") {
		OUT("You pant like a dog.\n", "%M pants like a dog.");
	} else if(str == "quack") {
		OUT("You quack like a duck.\n", "%M quacks like a duck.");
	} else if(str == "rant") {
		OUT("You rant and rave.\n", "%M rants and raves.");
	} else if(str == "roar") {
		OUT("You roar like a lion.\n", "%M roars like a lion.");
	} else if(str == "rofl") {
		OUT("You roll on the ground laughing.\n", "%M rolls on the ground in a fit of laughter.");
	} else if(str == "sad") {
		OUT_HISHER("You feel sad.\n", "%M gets a sad look in %s eyes.");
	} else if(str == "scream") {
		OUT("You scream in terror.\n", "%M screams in terror.");
	} else if(str == "shiver") {
		OUT("You shiver from a chill.\n", "%M shivers from a chill.");
	} else if(str == "shudder") {
		OUT("You shudder in horror.\n", "%M shudders in horror.");
	} else if(str == "snore") {
		if(player && !player->flagIsSet(P_SLEEPING) && !player->isStaff()) {
			player->print("You aren't sleeping!\n");
		} else {
			OUT("You snore loudly.\n", "%M snores loudly.");
		}
	} else if(str == "snort") {
		OUT("You snort like a hedgehog.\n", "%M snorts like a hedgehog.");
	} else if(str == "spaz") {
		OUT_HIMHER("You spaz out.\n", "%M spazzes out, running around and tripping all over %sself.");
	} else if(str == "spin") {
		OUT("You spin around happily.\n", "%M spins around happily.");
	} else if(str == "squeal") {
		OUT("You squeal like a stuck pig.\n", "%M squeals like a stuck pig.");
	} else if(str == "squirm") {
		OUT("You squirm and giggle.\n", "%M squirms and giggles.");
	} else if(str == "stretch") {
		OUT_HISHER("You stretch your body.\n", "%M stretches %s arms and legs.");
	} else if(str == "strip") {
		OUT("You strip off all your clothes.\n", "%M strips! Woohoo!");
	} else if(str == "stumble") {
		OUT("You stumble around aimlessly.\n", "%M stumbles around aimlessly.");
	} else if(str == "surrender") {
		OUT_HISHER("You surrender.\n", "%M throws up %s hands in surrender.");
	} else if(str == "sweat") {
		OUT("You sweat uncontrollably.\n", "%M sweats uncontrollably.");
	} else if(str == "tantrum") {
		OUT("You run around the room and break stuff.\n", "%M runs around the room and breaks stuff.");
	} else if(str == "triumph") {
		OUT("You cheer in triumph.\n", "%M cheers in triumph.");
	} else if(str == "wail") {
		OUT("You wail like a banshee.\n", "%M wails like a banshee.");
	} else if(str == "wiggle") {
		OUT("You wiggle insanely.\n", "%M wiggles insanely.");
	} else if(str == "writhe") {
		OUT("You writhe in pain.\n", "%M falls to the ground and writhes in pain.");
	} else if(str == "yodel") {
		OUT("You yodel loudly.\n", "%M yodels loudly.");
	} else if(str == "worry") {
		OUT_HISHER("You look worried.\n", "%M gets a worried look on %s face.");
	} else if(str == "fall") {
		OUT_HISHER("You fall on your face.\n", "%M falls on %s face.");
	} else if(str == "cluck") {
		OUT("You cluck like a chicken.\n", "%M clucks like a chicken.");
	} else if(str == "stagger") {
		OUT("You stagger around.\n", "%M staggers around.");
	} else if(str == "crow") {
		OUT("You crow like a rooster.\n", "%M crows like a rooster.");
	} else if(str == "bored") {
		OUT("You die of boredom.\n", "%M dies of boredom.");
	} else if(str == "cornholio") {
		OUT("You shake uncontrollably and put your shirt over your head.\n", "%M shakes and yells, \"I am CORNHOLIO!!!\"");
	} else if(str == "munch") {
		OUT("You munch on some food.\n", "%M munches on some food.");
	} else if(str == "nose") {
		OUT_HISHER("You blow your nose.\n", "%M blows %s nose. What a mess!");
	} else if(str == "count") {
		OUT("You count slowly.\n", "%M begins to count: 1...2...3...4...");
	} else if(str == "glaze") {
		OUT("Your eyes glaze over.\n", "%M's eyes begin to glaze over.");
	} else if(str == "wall") {
		OUT_HISHER("You stand your ground.\n", "%M stands %s ground like an impenetrable wall.");
	} else if(str == "fft") {
		OUT("You spit like a cat.\n", "%M spits like a cat: fft! fft!");
	} else if(str == "lol") {
		OUT("You laugh out loud.\n", "%M laughs out loud.");
	} else if(str == "wait") {
		OUT("You wait patiently.\n", "%M waits patiently.");
	} else if(str == "bear") {
		OUT("You roar like a grizzly bear.\n", "%M roars like a grizzly bear.");
	} else if(str == "cigar") {
		OUT("You smoke a cigar.\n", "%M smokes a cigar.");
	} else if(str == "phew") {
		OUT_HISHER("You wipe your brow in relief.\n", "%M wipes %s brow in relief.");
	} else if(str == "doodle") {
		OUT("You doodle pictures in the dirt.\n", "%M doodles pictures in the dirt.");
	} else if(str == "sniffle") {
		OUT("You sniffle sadly.\n", "%M sniffles sadly.");
	} else if(str == "sob") {
		OUT("You sob like a child.\n", "%M sobs like a child.");
	} else if(str == "shuffle") {
		OUT("You shuffle from side to side.\n", "%M shuffles nervously from one foot to the other.");
	} else if(str == "ok") {
		OUT("You say, \"ok.\"\n", "%M says, \"ok.\"");
	} else if(str == "wild") {
		OUT("You growl like a trapped wild animal.\n", "%M growls like a trapped wild animal.");
	} else if(str == "clean") {
		OUT("You clean up the room.\n", "%M cleans up the room.");
	} else if(str == "froth") {
		OUT("You froth at the mouth.\n", "%M froths at the mouth.");
	} else if(str == "bathe") {
		OUT("You take a bath.\n", "%M takes a bath.");
	} else if(str == "crack") {
		OUT_HISHER("You crack your knuckles.\n", "%M cracks %s knuckles.");
	} else if(str == "innocent") {
		OUT("You whistle innocently.\n", "%M whistles innocently.");
	} else if(str == "boggle") {
		OUT("You boggle at the concept.\n", "%M visibly boggles at the concept.");
	} else if(!player)
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
