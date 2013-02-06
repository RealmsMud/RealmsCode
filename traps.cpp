/*
 * traps.cpp
 *	 Trap routines.
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
#include "traps.h"

//*********************************************************************
//						teleport_trap
//*********************************************************************
// This functions allows a player to be teleported via a trap.

void teleport_trap(Player* player) {
	BaseRoom *newRoom=0;

	if(player->isStaff())
		return;

	newRoom = player->teleportWhere();

	player->deleteFromRoom();
	player->addToRoom(newRoom);
	player->doPetFollow();

	player->print("Your body is violently pulled in all directions.\n");
//	player->hp.getCur() -= player->hp.getMax()/2;
	player->doDamage(player, player->hp.getMax()/2, NO_CHECK);
	player->hp.setCur(MAX(5, player->hp.getCur()));
	// This is all done so a teleport trap
	// will not be abused for exploring.
	player->smashInvis();

	player->lasttime[LT_SPELL].ltime = time(0);
	player->lasttime[LT_SPELL].interval = 120L;

	player->stun(mrand(20,95));
}

//*********************************************************************
//						rock_slide
//*********************************************************************

void rock_slide(Player* player) {
	Player	*target=0;
	int		dmg=0;

	PlayerSet::iterator pIt = player->getRoomParent()->players.begin();
	PlayerSet::iterator pEnd = player->getRoomParent()->players.end();
	while(pIt != pEnd) {
		target = (*pIt++);

		if(!target)
			continue;

		dmg = mrand(10, 20);

		if(target->getClass() == LICH)
			dmg *= 2;

		if(target->chkSave(LCK, target, 0))
			dmg /=2;

		target->printColor("Falling rocks crush you for %s%d^x damage.\n", target->customColorize("*CC:DAMAGE*").c_str(), dmg);
		target->doDamage(target, dmg, NO_CHECK);
		if(target->hp.getCur() < 1) {
			target->print("You are crushed to death by falling rocks.\n");
			target->die(ROCKS);
		}

	}

}

//*********************************************************************
//						doCheckTraps
//*********************************************************************

// This function checks a room to see if there are any traps and whether
// the player pointed to by the first parameter fell into any of them.


int Player::doCheckTraps(UniqueRoom* room) {
	Player	*ply=0;
	BaseRoom *newRoom=0;
	UniqueRoom	*uRoom=0;
	Creature* target=0;
	int		toHit=0, a=0, num=0, roll=0, saved=0;
	Location l;
	Damage trapDamage;


	if(isCt()) {
		printColor("^mTrap passed.\n");
		return(0);
	}
	if(isEffected("mist"))
		return(0);


	if(room->getTrapExit().id)
		l.room = room->getTrapExit();
	else
		l = bound;


	switch(room->getTrap()) {
	case TRAP_PIT:
	case TRAP_DART:
	case TRAP_BLOCK:
	case TRAP_NAKED:
	case TRAP_ALARM:
	case TRAP_ARROW:
	case TRAP_SPIKED_PIT:
	case TRAP_WORD:
	case TRAP_FIRE:
	case TRAP_FROST:
	case TRAP_ELEC:
	case TRAP_ACID:
	case TRAP_ROCKS:
	case TRAP_ICE:
	case TRAP_SPEAR:
	case TRAP_CROSSBOW:
	case TRAP_GASP:
	case TRAP_GASB:
	case TRAP_GASS:
	case TRAP_MUD:
	case TRAP_WEB:
	case TRAP_BONEAV:
	case TRAP_PIERCER:
	case TRAP_CHUTE:
		if(flagIsSet(P_PREPARED) && mrand(1,20) < dexterity.getCur()/10) {
			clearFlag(P_PREPARED);
			return(0);
		}
		clearFlag(P_PREPARED);
		if(mrand(1,100) < dexterity.getCur()/10)
			return(0);

		break;
	case TRAP_FALL:
		if(flagIsSet(P_PREPARED) && mrand(1,30) < dexterity.getCur()/20) {
			clearFlag(P_PREPARED);
			return(0);
		}
		clearFlag(P_PREPARED);
		if(mrand(1,100) < dexterity.getCur()/20)
			return(0);
		break;

	case TRAP_DISP:
		if(cClass == MAGE || cClass == LICH || cClass == CLERIC || cClass == DRUID) {
			if(flagIsSet(P_PREPARED) && mrand(1,30) < ((intelligence.getCur()/10 + piety.getCur()/10)/2)) {
				clearFlag(P_PREPARED);
				return(0);
			}
			clearFlag(P_PREPARED);
			if(mrand(1,100) < intelligence.getCur()/10)
				return(0);
		} else if(flagIsSet(P_PREPARED) && (mrand(1,100)<=5)) {
			clearFlag(P_PREPARED);
			return(0);
		}
		break;
	case TRAP_MPDAM:
	case TRAP_RMSPL:
		if(flagIsSet(P_PREPARED) && mrand(1,30) < ((intelligence.getCur()/10 + piety.getCur()/10)/2)) {
			clearFlag(P_PREPARED);
			return(0);
		}
		clearFlag(P_PREPARED);
		if(mrand(1,100) < intelligence.getCur()/10)
			return(0);
		break;
	case TRAP_TPORT:
	case TRAP_ETHEREAL_TRAVEL:
		if(flagIsSet(P_PREPARED) && mrand(1,50) < (level/2+(intelligence.getCur()/10 + piety.getCur()/10)/2)) {
			clearFlag(P_PREPARED);
			return(0);
		}
		clearFlag(P_PREPARED);
		if(mrand(1,100) < intelligence.getCur()/10)
			return(0);
		break;

	default:
		return(0);
	}


	switch(room->getTrap()) {
	case TRAP_PIT:
		if(!isEffected("levitate")) {
			newRoom = l.loadRoom(this);
			if(!newRoom)
				return(0);

			print("You fell into a pit trap!\n");
			broadcast(getSock(), getRoomParent(), "%M fell into a pit trap!", this);

			deleteFromRoom(room);
			addToRoom(newRoom);
			doPetFollow();

			trapDamage.set(mrand(1,15));

			if(chkSave(DEA, this, 0)) {
				print("You managed to control your fall.\n");
				trapDamage.set(trapDamage.get() / 2);
			}
			print("You lost %d hit points.\n", trapDamage.get());
			hp.decrease(trapDamage.get());

			if(hp.getCur() < 1)
				die(PIT);
			else
				checkTraps(newRoom->getAsUniqueRoom());
		}
		break;

	case TRAP_FALL:
		if(!isEffected("levitate") && !isEffected("fly")) {
			newRoom = l.loadRoom(this);
			if(!newRoom)
				return(0);

			print("You tumble downward uncontrollably!\n");
			broadcast(getSock(), getRoomParent(), "%M tumbles downward uncontrollably!", this);
			deleteFromRoom(room);
			addToRoom(newRoom);
			doPetFollow();
			trapDamage.set(mrand(1,20));

			if(chkSave(DEA, this, -25)) {
				print("You manage slow your fall.\n");
				trapDamage.set(trapDamage.get() / 2);
			}
			print("You lost %d hit points.\n", trapDamage.get());
			hp.decrease(trapDamage.get());
			if(hp.getCur() < 1)
				die(SPLAT);
			else
				checkTraps(newRoom->getAsUniqueRoom());
		}
		break;

	case TRAP_DISP:

		if(chkSave(LCK, this, -10))
			return(0);
		newRoom = l.loadRoom(this);
		if(!newRoom)
			return(0);


		print("You are overcome by vertigo!\n");
		broadcast(getSock(), getRoomParent(), "%M vanishes!", this);
		deleteFromRoom(room);
		addToRoom(newRoom);
		doPetFollow();
		checkTraps(newRoom->getAsUniqueRoom());
		break;

	case TRAP_CHUTE:
		newRoom = l.loadRoom(this);
		if(!newRoom)
			return(0);

		print("You fall down a chute!\n");
		broadcast(getSock(), getRoomParent(), "%M fell down a chute!", this);
		deleteFromRoom(room);
		addToRoom(newRoom);
		doPetFollow();
		checkTraps(newRoom->getAsUniqueRoom());

		break;

	case TRAP_ROCKS:
		if(!isEffected("fly")) {
			print("You have triggered a rock slide!\n");
			broadcast(getSock(), getRoomParent(), "%M triggers a rock slide!", this);

			rock_slide(this);
		}
		break;

	case TRAP_BONEAV:
		newRoom = l.loadRoom(this);
		if(!newRoom)
			return(0);

		print("You have triggered an avalanche of bones!\n");
		print("You are knocked down by its immense weight!\n");
		broadcast(getSock(), getRoomParent(), "%M was crushed by an avalanche of bones!", this);
		deleteFromRoom(room);
		addToRoom(newRoom);
		doPetFollow();
		trapDamage.set(mrand(hp.getMax() / 4, hp.getMax() / 2));

		if(chkSave(DEA, this, -15)) {
			print("You manage to only be partially buried.\n");
			trapDamage.set(trapDamage.get() / 2);
		}
		print("You lost %d hit points.\n", trapDamage.get());
		hp.decrease(trapDamage.get());
		if(hp.getCur() < 1)
			die(BONES);
		else
			checkTraps(newRoom->getAsUniqueRoom());

		break;

	case TRAP_SPIKED_PIT:
		if(!isEffected("levitate")) {
			newRoom = l.loadRoom(this);
			if(!newRoom)
				return(0);

			print("You fell into a pit!\n");
			print("You are impaled by spikes!\n");
			broadcast(getSock(), getRoomParent(), "%M fell into a pit!", this);
			broadcast(getSock(), getRoomParent(), "It has spikes at the bottom!");
			deleteFromRoom(room);
			addToRoom(newRoom);
			doPetFollow();
			trapDamage.set(mrand(15,30));

			if(chkSave(DEA,this,0)) {
				print("The spikes barely graze you.\n");
				trapDamage.set(trapDamage.get() / 2);
			}
			if(isBrittle())
				trapDamage.set(trapDamage.get() * 3 / 2);
			print("You lost %d hit points.\n", trapDamage.get());
			hp.decrease(trapDamage.get());
			if(hp.getCur() < 1)
				die(SPIKED_PIT);
			else
				checkTraps(newRoom->getAsUniqueRoom());
		}
		break;

	case TRAP_DART:
		print("You triggered a hidden dart!\n");
		broadcast(getSock(), getRoomParent(), "%M gets hit by a hidden poisoned dart!", this);
		trapDamage.set(mrand(1,10));
		if(chkSave(DEA,this,-1)) {
			print("The dart barely scratches you.\n");
			trapDamage.set(trapDamage.get() / 2);
		}

		print("You lost %d hit points.\n", trapDamage.get());
		if(!immuneToPoison() && !chkSave(POI, this, -1)) {
			num = room->getTrapStrength() ? room->getTrapStrength() : 2;

			poison(0, num, standardPoisonDuration(num, constitution.getCur()));
			poisonedBy = "a poisoned dart";
		}
		hp.decrease(trapDamage.get());
		if(hp.getCur() < 1)
			die(DART);
		break;

	case TRAP_ARROW:
		print("You triggered an arrow trap!\n");
		print("A flight of arrows peppers you!\n");
		broadcast(getSock(), getRoomParent(), "%M gets hit by a flight of arrows!", this);
		trapDamage.set(mrand(15,20));

		if(chkSave(DEA,this,0)) {
			print("You manage to dive and take half damage.\n");
			trapDamage.set(trapDamage.get() / 2);
		}
		print("You lost %d hit points.\n", trapDamage.get());

		hp.decrease(trapDamage.get());

		if(hp.getCur() < 1)
			die(ARROW);
		break;

	case TRAP_SPEAR:
		print("You triggered a spear trap!\n");
		print("A giant spear impales you!\n");
		broadcast(getSock(), getRoomParent(), "%M gets hit by a giant spear!", this);
		trapDamage.set(mrand(10, 15));

		if(chkSave(DEA, this, 0)) {
			print("The glances off of you.\n");
			trapDamage.set(trapDamage.get() / 2);
		}
		print("You lost %d hit points.\n", trapDamage.get());

		hp.decrease(trapDamage.get());

		if(hp.getCur() < 1)
			die(SPEAR);
		break;

	case TRAP_CROSSBOW:
		print("You triggered a crossbow trap!\n");
		print("A giant poisoned crossbow bolt hits you in the chest!\n");
		broadcast(getSock(), getRoomParent(), "%M gets hit by a giant crossbow bolt!", this);
		trapDamage.set(mrand(20, 25));

		if(chkSave(DEA, this, 0)) {
			print("The bolt barely scratches you.\n");
			trapDamage.set(trapDamage.get() / 2);
		}


		if(!immuneToPoison() && !chkSave(POI, this, 0)) {
			num = room->getTrapStrength() ? room->getTrapStrength() : 7;

			poison(0, num, standardPoisonDuration(num, constitution.getCur()));
			poisonedBy = "a poisoned crossbow bolt";
		}

		print("You lost %d hit points.\n", trapDamage.get());

		hp.decrease(trapDamage.get());

		if(hp.getCur() < 1)
			die(CROSSBOW_TRAP);
		break;

	case TRAP_GASP:
		print("You triggered a gas trap!\n");
		print("Gas rapidly fills the room!\n");
		broadcast(getSock(), getRoomParent(), "Gas rapidly fills the room!");

		if(!immuneToPoison()) {
			if(!chkSave(POI, this, -15)) {
				print("The billowing green cloud surrounds you!\n");

				num = room->getTrapStrength() ? room->getTrapStrength() : 15;

				poison(0, num, standardPoisonDuration(num, constitution.getCur()));
				poisonedBy = "poison gas";
			} else {
				print("You managed to hold your breath.\n");
			}
		} else {
			print("The billowing cloud of green gas has no effect on you.\n");
		}

		break;

	case TRAP_GASB:
		print("You triggered a gas trap!\n");
		print("Gas rapidly fills the room!\n");
		broadcast(getSock(), getRoomParent(), "Gas rapidly fills the room!");

		if(!chkSave(DEA, this, 0)) {
			print("The billowing red cloud blinds your eyes!\n");
			addEffect("blindness", 120 - (constitution.getCur()/10), 1);
		}
		break;

	case TRAP_GASS:
		print("You triggered a gas trap!\n");
		print("Gas rapidly fills the room!\n");
		broadcast(getSock(), getRoomParent(), "Gas rapidly fills the room!");

		for(Player* fPly : getRoomParent()->players) {
			if(!fPly->chkSave(DEA,fPly,0)) {
				if(!fPly->flagIsSet(P_RESIST_STUN)) {
					fPly->print("Billowing white clouds surrounds you!\n");
					fPly->print("You are stunned!\n");
					fPly->stun(mrand(10, 18));
				} else {
					fPly->print("The billowing cloud of white gas has no effect on you.\n");
				}
			} else
				fPly->print("The billowing cloud of white gas has no effect on you.\n");
		}
		break;

	case TRAP_MUD:
	case TRAP_WEB:

		if(room->getTrap() == TRAP_MUD) {
			print("You sink into chest high mud!\n");
			print("You are stuck!\n");
			broadcast(getSock(), getRoomParent(), "%M sank chest deep into some mud!", this);
		} else if(room->getTrap() == TRAP_WEB) {
			print("You brush against some large spider webs!\n");
			print("You are stuck!\n");
			broadcast(getSock(), getRoomParent(), "%M stuck to some large spider webs!", this);
		}

		if(chkSave(DEA, this , -1))
			stun(mrand(5,10));
		else
			stun(mrand(20,30));
		break;

	case TRAP_ETHEREAL_TRAVEL:

		if(checkDimensionalAnchor()) {
			printColor("^yYour dimensional-anchor protects you from the ethereal-travel trap!^w\n");
		} else if(!chkSave(SPL, this, -25)) {
			printColor("^BYou become tranlucent and fade away!\n");
			printColor("You've been transported to the ethereal plane!\n");
			broadcast(getSock(), getRoomParent(), "^B%N becomes translucent and fades away!", this);
			etherealTravel(this);
		} else {
			print("You feel a warm feeling all over.\n");
		}
		break;


	case TRAP_PIERCER:
		num = MAX(1,MIN(8,mrand((room->getTrapStrength()+1)/2, room->getTrapStrength()+1)));
		for(a=0;a<num;a++) {
			target = getRandomPlayer(room);
			if(!target)
				target = getRandomMonster(room);
			if(!target)
				continue;

			ply = target->getAsPlayer();

			if(ply && ply->flagIsSet(P_DM_INVIS))
				continue;

			trapDamage.reset();
			trapDamage.set(mrand(target->hp.getMax()/20, target->hp.getMax()/10));
			target->modifyDamage(0, PHYSICAL, trapDamage);

			toHit = 14 - (trapDamage.get()/10);
			if(target->isInvisible() && target->isPlayer())
				toHit= 20 - (trapDamage.get()/10);
			roll = mrand(1,20);

			if(roll >= toHit) {
				broadcast(target->getSock(), room, "A living stalagtite falls upon %N from above!", target);
				target->print("A living stalagtite falls on you from above!\n");
				saved = chkSave(DEA, this, 0);
				if(saved)
					target->print("It struck a glancing blow.\n");
				target->printColor("You are hit for %s%d^x damage.\n", target->customColorize("*CC:DAMAGE*").c_str(), trapDamage.get());

				target->hp.decrease(trapDamage.get());
				if(target->hp.getCur() < 1) {
					if(ply) {
						ply->die(PIERCER);
						continue;
					} else {
						Monster* mon = target->getAsMonster();
						broadcast(NULL,  room, "%M was killed!", mon);
						if(mon->flagIsSet(M_PERMENANT_MONSTER))
							mon->diePermCrt();
						mon->deleteFromRoom();

						free_crt(target);
						continue;
					}
				}
				continue;
			} else {
				target->print("A falling stalagtite almost hit you!\n");
				broadcast(target->getSock(), room, "A falling stalagtite almost hits %N!", target);
				continue;
			}

		}
		break;

	case TRAP_FIRE:
		print("You set off a fire trap!\n");
		print("Flames engulf you!\n");
		broadcast(getSock(), getRoomParent(), "%M is engulfed by flames!", this);
		trapDamage.set(mrand(20,40));

		if(chkSave(BRE, this, 0)) {
			trapDamage.set(trapDamage.get() / 2);
			print("You are only half hit by the explosion.\n");
		}

		if(isEffected("heat-protection") || isEffected("alwayswarm"))
			trapDamage.set(trapDamage.get() / 2);
		if(isEffected("resist-fire"))
			trapDamage.set(trapDamage.get() / 2);

		printColor("You are burned for %s%d^x damage.\n", customColorize("*CC:DAMAGE*").c_str(), trapDamage.get());

		hp.decrease(trapDamage.get());

		if(hp.getCur() < 1)
			die(FIRE_TRAP);
		break;

	case TRAP_FROST:
		print("You are hit by a blast of ice!\n");
		print("Frost envelopes you!\n");
		broadcast(getSock(), getRoomParent(), "%M is engulfed by a cloud of frost!", this);
		trapDamage.set(mrand(20,40));

		if(chkSave(BRE, this, 0)) {
			trapDamage.set(trapDamage.get() / 2);
			print("You are only partially frostbitten.\n");
		}

		if(isEffected("warmth") || isEffected("alwayscold"))
			trapDamage.set(trapDamage.get() / 2);
		if(isEffected("resist-cold"))
			trapDamage.set(trapDamage.get() / 2);

		printColor("You are frozen for %s%d^x damage.\n", customColorize("*CC:DAMAGE*").c_str(), trapDamage.get());

		hp.decrease(trapDamage.get());

		if(hp.getCur() < 1)
			die(FROST);
		break;

	case TRAP_ELEC:
		print("You are hit by a crackling blue bolt!\n");
		print("Electricity pulses through your body!\n");
		broadcast(getSock(), getRoomParent(), "%M is surrounded by crackling blue arcs!", this);
		trapDamage.set(mrand(20,40));

		if(chkSave(BRE, this, 0)) {
			trapDamage.set(trapDamage.get() / 2);
			print("The electricity only half engulfed you.\n");
		}

		if(isEffected("resist-electric"))
			trapDamage.set(trapDamage.get() / 2);

		printColor("You are electrocuted for %s%d^x damage.\n", customColorize("*CC:DAMAGE*").c_str(), trapDamage.get());

		hp.decrease(trapDamage.get());

		if(hp.getCur() < 1)
			die(ELECTRICITY);
		break;

	case TRAP_ACID:
		print("You are blasted by a jet of acid!\n");
		print("You are immersed in dissolving liquid.\n");
		broadcast(getSock(), getRoomParent(), "%M is immersed in acid!", this);
		trapDamage.set(mrand(20,30));

		if(chkSave(BRE, this, 0)) {
			trapDamage.set(trapDamage.get() / 2);
			print("The acid doesn't totally cover you.\n");
		} else {
			print("Your skin dissolves to the bone.\n");
			loseAcid();
		}


		printColor("You are burned for %s%d^x damage.\n", customColorize("*CC:DAMAGE*").c_str(), trapDamage.get());

		hp.decrease(trapDamage.get());


		if(hp.getCur() < 1)
			die(ACID);
		break;

	case TRAP_BLOCK:
		print("You triggered a falling block!\n");
		broadcast(getSock(), getRoomParent(), "A large block falls on %N.", this);
		trapDamage.set(hp.getMax() / 2);

		if(chkSave(DEA, this, -5)) {
			trapDamage.set(trapDamage.get() / 2);
			print("The block hits you in a glancing blow\nas you quickly dive out of the way..\n");
		}
		print("You lost %d hit points.\n", trapDamage.get());
		hp.decrease(trapDamage.get());
		if(hp.getCur() < 1)
			die(BLOCK);
		break;

	case TRAP_ICE:
		print("A giant icicle falls from above!\n");
		broadcast(getSock(), getRoomParent(), "%N's vibrations caused a giant icicle to drop!", this);
		trapDamage.set(hp.getMax() / 2);

		if(chkSave(DEA, this, -5)) {
			trapDamage.set(trapDamage.get() / 2);
			print("The icicle slashes you but doesn't impale you.\n");
		} else {
			print("You are impaled!\n");
			broadcast(getSock(), getRoomParent(), "%N is impaled by a huge icicle!", this);
		}
		print("You lost %d hit points.\n", trapDamage.get());
		hp.decrease(trapDamage.get());
		if(hp.getCur() < 1)
			die(ICICLE_TRAP);
		break;

	case TRAP_MPDAM:

		if(mp.getCur() <= 0)
			break;

		print("You feel an exploding force in your mind!\n");
		broadcast(getSock(), getRoomParent(), "An energy bolt strikes %N.", this);

		if(chkSave(MEN, this,0)) {
			trapDamage.set(MIN(mp.getCur(),mp.getMax() / 2));
			print("You lost %d magic points.\n", trapDamage.get());
		} else {
			trapDamage.set(mp.getCur());
			print("You feel the magical energy sucked from your mind.\n");
		}

		mp.decrease(trapDamage.get());
		break;

	case TRAP_RMSPL:
		print("A foul smelling charcoal cloud surrounds you.\n");

		broadcast(getSock(), getRoomParent(), "A charcoal cloud surrounds %N.", this);
		if(!chkSave(SPL, this,0)) {
			doDispelMagic();
			print("Your magic begins to dissolve.\n");
		}
		break;

	case TRAP_NAKED:
		printColor("^gYou are covered in oozing green slime.\n");

		if(chkSave(DEA, this, 0)) {
			broadcast(getSock(), getRoomParent(), "^gA foul smelling and oozing green slime envelops %N.", this);
			stun(mrand(1,10));
		} else {
			print("Your possessions begin to dissolve!\n");
			lose_all(this, false, "slime-trap");
			stun(mrand(5,20));
		}
		break;

	case TRAP_TPORT:

		if(checkDimensionalAnchor()) {
			printColor("^yYour dimensional-anchor protects you from the teleportation trap!^w\n");
		} else if(!chkSave(SPL, this, 0)) {
			printColor("^yThe world changes rapidly around you!\n");
			printColor("You've been teleported!\n");
			broadcast(getSock(), getRoomParent(), "%N disappears!", this);
			teleport_trap(this);
		} else {
			print("You feel a strange tingling all over.\n");
		}
		break;



	case TRAP_WORD:

		if(checkDimensionalAnchor()) {
			printColor("^yYour dimensional-anchor protects you from the word-of-recall trap!^w\n");
		} else {
			printColor("^cYou phase in and out of existence.\n");
			broadcast(getSock(), getRoomParent(), "%N disappears.", this);

			newRoom = getRecallRoom().loadRoom(this);
			if(!newRoom)
				break;
			deleteFromRoom();
			addToRoom(newRoom);
			doPetFollow();
			break;
		}

	case TRAP_ALARM:
		print("You set off an alarm!\n");
		print("You hope there aren't any guards around.\n\n");
		broadcast(getSock(), getRoomParent(), "%M sets off an alarm!\n", this);

		CatRef	acr;

		if(room->getTrapExit().id)
			acr = room->getTrapExit();
		else {
			acr = room->info;
			acr.id++;
		}

		if(!loadRoom(acr, &uRoom))
			return(0);

		uRoom->addPermCrt();

		Monster *tmp_crt=0;
		MonsterSet::iterator mIt = uRoom->monsters.begin();
		while(mIt != uRoom->monsters.end()) {
			tmp_crt = (*mIt++);
			if(tmp_crt->flagIsSet(M_PERMENANT_MONSTER)) {
				if(!uRoom->players.empty())
					broadcast(tmp_crt->getSock(), tmp_crt->getRoomParent(),
						"%M hears an alarm and leaves to investigate.",tmp_crt);
				else
					gServer->addActive(tmp_crt);
				tmp_crt->clearFlag(M_PERMENANT_MONSTER);
				tmp_crt->setFlag(M_AGGRESSIVE);
				tmp_crt->diePermCrt();
				tmp_crt->deleteFromRoom();
				tmp_crt->addToRoom(room);
				broadcast(tmp_crt->getSock(), tmp_crt->getRoomParent(),
					"%M comes to investigate the alarm.", tmp_crt);
			}
		}
		break;
	}
	return(2);
}

//*********************************************************************
//						checkTraps
//*********************************************************************
// A wrapper for do_check_traps so we can have a trap spring
// on everyone in the room. self is whether or not they moved themselves
// into a room - followers of a leader ALL springing weight traps would be bad.

int Player::checkTraps(UniqueRoom* room, bool self, bool isEnter) {
	if(!room)
		return(0);

	Player	*target=0;

	if(!room->getTrap()) {
		clearFlag(P_PREPARED);
		return(0);
	}

	if(isEnter && room->flagIsSet(R_NO_SPRING_TRAP_ON_ENTER))
		return(0);

	// if it's not weight-based, continue normally
	if(!room->getTrapWeight()) {
		return(doCheckTraps(room));
	} else if(
		self &&
		countForWeightTrap() &&
		room->getWeight() >= room->getTrapWeight()
	) {
		// some traps already do area-of-effect; if we do them in
		// this loop, it'd be bad. run them normally.
		switch(room->getTrap()) {
			case TRAP_ROCKS:
			case TRAP_GASS:
			case TRAP_PIERCER:
			case TRAP_ALARM:
			case TRAP_BONEAV:
				return(doCheckTraps(room));
			default:
				break;
		}

		// we loop through everyone in the room!

	    PlayerSet::iterator pIt = room->players.begin();
	    PlayerSet::iterator pEnd = room->players.end();
	    while(pIt != pEnd) {
	        target = (*pIt++);
			if(target)
				target->doCheckTraps(room);
		}
	}
	return(0);
}

//*********************************************************************
//						countForWeightTrap
//*********************************************************************

bool Creature::countForWeightTrap() const {
	if(isMonster())
		return(!isEffected("levitate") && !isEffected("fly"));
	return(!isStaff() && !isEffected("mist") && !isEffected("levitate") && !isEffected("fly"));
}

//*********************************************************************
//						resistLose
//*********************************************************************

bool resistLose(Object* object) {
	if(object->flagIsSet(O_RESIST_DISOLVE) || object->getQuestnum())
		return(true);

	int r = 15 + abs(object->getAdjustment()) * 5;

	// containers with lots of items are more resistant,
	// since it would suck to lose a lot of items
	if(object->getType() == CONTAINER)
		r += object->getShotsCur() * 5;

	return(mrand(1,100) < r);
}

//*********************************************************************
//						lose_all
//*********************************************************************

void lose_all(Player* player, bool destroyAll, const char* lostTo) {
	Object* object=0;
	int		i=0;

	// if we send !destroyAll, there are some scenarios where
	// we don't destroy everything

	// remove all equipped items
	for(i=0; i<MAXWEAR; i++) {
		if(player->ready[i]) {
			if(!destroyAll && resistLose(player->ready[i])) {
				player->printColor("%O resisted destruction.\n", player->ready[i]);
				continue;
			}

			logn("log.dissolve", "%s(L%d) lost %s to %s in room %s.\n",
				player->name, player->getLevel(), player->ready[i]->name, lostTo, player->getRoomParent()->fullName().c_str());
			player->ready[i]->popBag(player, true, false, false, false, true);
			player->unequip(i+1, UNEQUIP_DELETE);
		}
	}

	player->computeAC();
	player->computeAttackPower();

	// delete all possessions
	ObjectSet::iterator it;
	for( it = player->objects.begin() ; it != player->objects.end() ; ) {
		object = (*it++);

		if(!destroyAll && resistLose(object)) {
			player->printColor("%O resisted destruction.\n", object);
			continue;
		}

		logn("log.dissolve", "%s(L%d) lost %s to %s in room %s.\n",
			player->name, player->getLevel(), object->getCName(), lostTo, player->getRoomParent()->fullName().c_str());
		object->popBag(player, true, false, false, false, true);
		player->delObj(object, true, false, true, false);
		delete object;
	}
	player->checkDarkness();
}

//*********************************************************************
//						dissolveItem
//*********************************************************************
// dissolve_item will randomly select one equipted (including held or
// wield) items on the given player and then delete it. The player
// receives a message that the item was destroyed as well as who is
// responsible for the deed.

void Player::dissolveItem(Creature* creature) {
	char	checklist[MAXWEAR];
	int		numwear=0, i=0, n=0;

	for(i=0; i<MAXWEAR; i++) {
		checklist[i] = 0;
		// if(i==WIELD-1 || i==HELD-1) continue;
		if(ready[i])
			checklist[numwear++] = i+1;
	}

	if(!numwear)
		n = 0;
	else {
		i = mrand(0, numwear-1);
		n = (int) checklist[i];
	}
	if(n) {
		if(ready[n-1]) {
			if(ready[n - 1]->flagIsSet(O_RESIST_DISOLVE)) {
				printColor("%M tried to dissolve your %s.\n", creature, ready[n-1]->name);
				n = 0;
				return;
			}
		}
	}


	if(n) {
		broadcast(getSock(), getRoomParent(),"%M destroys %N's %s.", creature, this, ready[n-1]->name);
		printColor("%M destroys your %s.\n",creature, ready[n-1]->name);
		logn("log.dissolve", "%s(L%d) lost %s to acid in room %s.\n",
			name, level, ready[n-1]->name, getRoomParent()->fullName().c_str());
		// Unequip it and don't add it to the inventory, delete it
		unequip(n, UNEQUIP_DELETE);
		computeAC();
	}
}

//*********************************************************************
//						loseAcid
//*********************************************************************
// This function goes through a players possessions worn AND carried
// and determines whether it was fragged due to dissolving acid.

void Player::loseAcid() {
	Object* object=0;
	int		i=0;

	// Check all equipped items
	for(i=0; i<MAXWEAR; i++) {
		if(ready[i]) {
			if( !ready[i]->flagIsSet(O_RESIST_DISOLVE) &&
				(mrand(1,100) <= 5 - abs(ready[i]->getAdjustment()))
			) {
				if(ready[i]->flagIsSet(O_NO_PREFIX)) {
					printColor("^r%s was dissolved by acid!\n", ready[i]->name);
				} else {
					printColor("^rYour %s was dissolved by acid!\n", ready[i]->name);
				}

				logn("log.dissolve", "%s(L%d) lost %s to acid in room %s.\n",
					name, level, ready[i]->name, getRoomParent()->fullName().c_str());
				unequip(i+1, UNEQUIP_DELETE);
			}
		}
	}


	computeAC();
	computeAttackPower();

	// Remove and possibly dissolve possessions
	ObjectSet::iterator it;
	for( it = objects.begin() ; it != objects.end() ; ) {
		object = (*it++);
		if( !object->flagIsSet(O_RESIST_DISOLVE) &&
			(mrand(1,100) <= 5 - abs(object->getAdjustment())))
		{
			if(object->flagIsSet(O_NO_PREFIX)) {
				printColor("^r%s is dissolved by acid!\n", object->name);
			} else {
				printColor("^rYour %s is dissolved by acid!\n", object->name);
			}
			logn("log.dissolve", "%s(L%d) lost %s to acid in room %s.\n",
				name, level, object->getCName(), getRoomParent()->fullName().c_str());
			delObj(object, true, false, true, false);
			delete object;
		}
	}
	checkDarkness();
}
