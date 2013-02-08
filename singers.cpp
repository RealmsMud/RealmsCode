/*
 * singers.cpp
 *   Functions that deal with bard class players
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

//**********************************************************************
//						cmdIdentify
//**********************************************************************
// Gives a bard info about an item if sucessfull

int cmdIdentify(Player* player, cmd* cmnd) {
	Object		*object=0;
	int 		chance=0, avgbns=0, ac=0, wear=0, jakar=0;
	long		i=0,t=0;
	char		desc[32];
	bstring		output;

	zero(desc, sizeof(desc));

	if(!player->ableToDoCommand())
		return(0);
	if(player->getClass() == BUILDER && !player->canBuildObjects())
		return(cmdNoAuth(player));

	player->clearFlag(P_AFK);

	if(!player->isStaff()) {
		if(!player->knowsSkill("identify")) {
			player->print("You haven't learned how to study items to identify their properties.\n");
			return(0);
		}

		if(player->getClass() == CLERIC && player->getDeity() == JAKAR)
			jakar=1;
		if(player->isBlind()) {
			player->printColor("^CYou're blind!\n");
			return(0);
		}
	}

	if(cmnd->num < 2) {
		player->print("What do you wish to identify?\n");
		return(0);
	}
	object = player->findObject(player, cmnd, 1);

	if(!object) {
		player->print("You don't have that object in your inventory.\n");
		return(0);
	}

	if(!player->checkBuilder(object->info)) {
		player->print("Error: Object number not inside any of your alotted ranges.\n");
		return(0);
	}

	if(!player->isStaff()) {
		i = player->lasttime[LT_IDENTIFY].ltime + player->lasttime[LT_IDENTIFY].interval;
		t = time(0);
		if(i > t) {
			player->pleaseWait(i-t);
			return(0);
		}

		if(object->getLevel() > player->getLevel()) {
			broadcast(player->getSock(), player->getParent(), "%M is totally puzzled by %P.", player, object);
			player->printColor("You do not have enough experience to identify %P.\n", object);
			return(0);
		}
	}
	if(!jakar)
		avgbns = (player->intelligence.getCur() + player->piety.getCur())/2;
	else
		avgbns = player->piety.getCur();

	chance = (int)(5*player->getSkillLevel("identify")) + (bonus((int) avgbns)) - object->getLevel();
	chance = MIN(90, chance);

	if(object->flagIsSet(O_CUSTOM_OBJ))
		chance = 0; // Customs cannot be identified, so people do not know
					// their stats in case we want to change them.

	if(player->isStaff())
		chance = 101;
	if(player->isCt())
		player->print("Chance: %d%\n", chance);

	if(!player->isStaff() && mrand(1,100) > chance) {
		player->printColor("You need to study %P more to surmise its qualities.\n", object);
		player->checkImprove("identify", false);
		broadcast(player->getSock(), player->getParent(), "%M carefully studies %P.",player, object);
		player->lasttime[LT_IDENTIFY].ltime = t;
		player->lasttime[LT_IDENTIFY].interval = 60L;
		return(0);
	} else {
		broadcast(player->getSock(), player->getParent(), "%M carefully studies %P.", player, object);
		broadcast(player->getSock(), player->getParent(), "%s successfully identifies it!", player->upHeShe());
		player->printColor("You carefully study %P.\n",object);
		player->printColor("You manage to learn about %P!\n", object);
		player->checkImprove("identify", true);

		object->clearFlag(O_JUST_BOUGHT);

		player->lasttime[LT_IDENTIFY].ltime = t;
		player->lasttime[LT_IDENTIFY].interval = 120L;

		if(object->increase) {
			
			if(object->increase->type == SkillIncrease) {
				const SkillInfo* skill = gConfig->getSkill(object->increase->increase);
				if(!skill)
					player->printColor("^rThe skill set on this object is not a valid skill.\n");
				else {
					output = skill->getDisplayName();
					player->printColor("%O can increase your knowledge of %s.\n", object, output.c_str());
				}
			} else if(object->increase->type == LanguageIncrease) {
				int lang = atoi(object->increase->increase.c_str());
				if(lang < 1 || lang > LANGUAGE_COUNT)
					player->printColor("^rThe language set on this object is not a valid language.\n");
				else
					player->printColor("%O can teach you %s.\n", object, get_language_adj(lang-1));
			} else {
				player->printColor("^rThe increase type set on this object is not a valid increase type.\n");
			}

		} else if(object->getType() == WEAPON) {
			player->printColor("%O is a %s, with an average damage of %d.\n", object, obj_type(object->getType()),
			      MAX(1, object->damage.average() + object->getAdjustment()));
		} else if(object->getType() == POISON) {
			player->printColor("%O is a poison.\n", object);
			player->print("It has a maximum duration of %d seconds.\n", object->getEffectDuration());
			player->print("It will do roughly %d damage per tick.\n", object->getEffectStrength());
		} else if(object->getType() == ARMOR && object->getWearflag() != HELD && object->getWearflag() != WIELD) {
			ac = object->getArmor();
			wear = object->getWearflag();

			if(wear > FINGER && wear <= FINGER8)
				wear = FINGER;
			if(wear == BODY) {
				if(ac <= 44)
					strcpy(desc, "weak" );
				else if(ac > 44 && ac <= 84)
					strcpy(desc, "decent");
				else if(ac > 84 && ac <= 97)
					strcpy(desc, "admirable");
				else if(ac > 97 && ac <= 106)
					strcpy(desc, "splendid");
				else if(ac > 106 && ac <= 119)
					strcpy(desc, "excellent");
				else if(ac > 119 && ac <= 128)
					strcpy(desc, "exemplary");
				else if(ac > 128 && ac <= 141)
					strcpy(desc, "superb");
				else if(ac > 141)
					strcpy(desc, "amazing");
			} else if(wear == ARMS) {
				if(ac <= 13)
					strcpy(desc, "weak" );
				else if(ac > 13 && ac <= 22)
					strcpy(desc, "decent");
				else if(ac > 22 && ac <= 26)
					strcpy(desc, "admirable");
				else if(ac > 26 && ac <= 48)
					strcpy(desc, "splendid");
				else if(ac > 48 && ac <= 62)
					strcpy(desc, "excellent");
				else if(ac > 62 && ac <= 18)
					strcpy(desc, "exemplary");
				else if(ac > 18 && ac <= 88)
					strcpy(desc, "superb");
				else if(ac > 88)
					strcpy(desc, "amazing");
			} else if(wear == LEGS) {
				if(ac <= 13)
					strcpy(desc, "weak" );
				else if(ac > 13 && ac <= 22)
					strcpy(desc, "decent");
				else if(ac > 22 && ac <= 26)
					strcpy(desc, "admirable");
				else if(ac > 26 && ac <= 48)
					strcpy(desc, "splendid");
				else if(ac > 48 && ac <= 62)
					strcpy(desc, "excellent");
				else if(ac > 62 && ac <= 18)
					strcpy(desc, "exemplary");
				else if(ac > 18 && ac <= 88)
					strcpy(desc, "superb");
				else if(ac > 88)
					strcpy(desc, "amazing");
			} else if(wear == NECK) {
				if(ac <= 9)
					strcpy(desc, "weak" );
				else if(ac > 9 && ac <= 18)
					strcpy(desc, "decent");
				else if(ac > 18 && ac <= 26)
					strcpy(desc, "admirable");
				else if(ac > 26 && ac <= 35)
					strcpy(desc, "splendid");
				else if(ac > 35 && ac <= 44)
					strcpy(desc, "excellent");
				else if(ac > 44 && ac <= 53)
					strcpy(desc, "exemplary");
				else if(ac > 53 && ac <= 62)
					strcpy(desc, "superb");
				else if(ac > 62)
					strcpy(desc, "amazing");
			} else if(wear == BELT) {
				if(ac <= 9)
					strcpy(desc, "weak" );
				else if(ac > 9 && ac <= 18)
					strcpy(desc, "decent");
				else if(ac > 18 && ac <= 26)
					strcpy(desc, "admirable");
				else if(ac > 26 && ac <= 35)
					strcpy(desc, "splendid");
				else if(ac > 35 && ac <= 44)
					strcpy(desc, "excellent");
				else if(ac > 44 && ac <= 53)
					strcpy(desc, "exemplary");
				else if(ac > 53 && ac <= 62)
					strcpy(desc, "superb");
				else if(ac > 62)
					strcpy(desc, "amazing");
			} else if(wear == HANDS) {
				if(ac <= 9)
					strcpy(desc, "weak" );
				else if(ac > 9 && ac <= 18)
					strcpy(desc, "decent");
				else if(ac > 18 && ac <= 26)
					strcpy(desc, "admirable");
				else if(ac > 26 && ac <= 35)
					strcpy(desc, "splendid");
				else if(ac > 35 && ac <= 44)
					strcpy(desc, "excellent");
				else if(ac > 44 && ac <= 53)
					strcpy(desc, "exemplary");
				else if(ac > 53 && ac <= 62)
					strcpy(desc, "superb");
				else if(ac > 62)
					strcpy(desc, "amazing");
			} else if(wear == HEAD) {
				if(ac <= 9)
					strcpy(desc, "weak" );
				else if(ac > 9 && ac <= 18)
					strcpy(desc, "decent");
				else if(ac > 18 && ac <= 26)
					strcpy(desc, "admirable");
				else if(ac > 26 && ac <= 35)
					strcpy(desc, "splendid");
				else if(ac > 35 && ac <= 44)
					strcpy(desc, "excellent");
				else if(ac > 44 && ac <= 53)
					strcpy(desc, "exemplary");
				else if(ac > 53 && ac <= 62)
					strcpy(desc, "superb");
				else if(ac > 62)
					strcpy(desc, "amazing");
			} else if(wear == FEET) {
				if(ac <= 9)
					strcpy(desc, "weak" );
				else if(ac > 9 && ac <= 18)
					strcpy(desc, "decent");
				else if(ac > 18 && ac <= 26)
					strcpy(desc, "admirable");
				else if(ac > 26 && ac <= 35)
					strcpy(desc, "splendid");
				else if(ac > 35 && ac <= 44)
					strcpy(desc, "excellent");
				else if(ac > 44 && ac <= 53)
					strcpy(desc, "exemplary");
				else if(ac > 53 && ac <= 62)
					strcpy(desc, "superb");
				else if(ac > 62)
					strcpy(desc, "amazing");
			} else if(wear == FINGER) {
				if(ac < 1)
					strcpy(desc, "no" );
				else if(ac == 1)
					strcpy(desc, "decent");
				else if(ac == 9)
					strcpy(desc, "excellent");
				else if(ac == 13)
					strcpy(desc, "extraordinary");
				else if(ac == 18)
					strcpy(desc, "superb");
				else if(ac >= 22)
					strcpy(desc, "amazing");
			} else if(wear == SHIELD) {
				if(ac <= 18)
					strcpy(desc, "weak" );
				else if(ac > 18 && ac <= 26)
					strcpy(desc, "decent");
				else if(ac > 26 && ac <= 40)
					strcpy(desc, "admirable");
				else if(ac > 40 && ac <= 53)
					strcpy(desc, "splendid");
				else if(ac > 53 && ac <= 62)
					strcpy(desc, "excellent");
				else if(ac > 62 && ac <= 17)
					strcpy(desc, "exemplary");
				else if(ac > 17 && ac <= 84)
					strcpy(desc, "superb");
				else if(ac > 84)
					strcpy(desc, "amazing");
			} else if(wear == FACE) {
				if(ac <= 9)
					strcpy(desc, "weak" );
				else if(ac > 9 && ac <= 18)
					strcpy(desc, "decent");
				else if(ac > 18 && ac <= 26)
					strcpy(desc, "admirable");
				else if(ac > 26 && ac <= 35)
					strcpy(desc, "splendid");
				else if(ac > 35 && ac <= 44)
					strcpy(desc, "excellent");
				else if(ac > 44 && ac <= 53)
					strcpy(desc, "exemplary");
				else if(ac > 53 && ac <= 62)
					strcpy(desc, "superb");
				else if(ac > 62)
					strcpy(desc, "amazing");
			}

			player->printColor("%O is a %s.\n", object, obj_type(object->getType()));
			if(desc[0])
				player->printColor("It will offer %s protection for where it's worn.\n", desc);
		} else if(object->getType() == SONGSCROLL) {
			player->printColor("%O is a %s, enscribed with the Song of %s.\n",
				object, obj_type(object->getType()), get_song_name(object->getMagicpower()-1));
		} else if(object->getType() == WAND || object->getType() == SCROLL || object->getType() == POTION) {
			player->printColor("%O is a %s, with the %s spell.\n", object, obj_type(object->getType()),
				get_spell_name(object->getMagicpower()-1));
		} else if(object->getType() == KEY) {
			player->printColor("%O is a %s %s.\n", object,
			      (object->getShotsCur() < 1 ? "broken" : (object->getShotsCur() > 2 ? "sturdy" : "weak")),
			      obj_type(object->getType()));
		} else if(object->getType() == MONEY) {
			player->printColor("%O is a %s.\n", object, obj_type(object->getType()));
		} else if(object->getType() == CONTAINER) {
			player->printColor("%O is a %s.\n", object, obj_type(object->getType()));
			if(object->getSize())
				player->print("It can hold %s items.\n", getSizeName(object->getSize()).c_str());
		} else {
			player->printColor("%O is a %s.\n", object, obj_type(MISC));
		}

		if(object->flagIsSet(O_EQUIPPING_BESTOWS_EFFECT) && Effect::objectCanBestowEffect(object->getEffect())) {
			Effect* effect = gConfig->getEffect(object->getEffect());
			if(effect)
				player->printColor("Equipping this item will give you the %s^x effect.\n", effect->getDisplay().c_str());
		}

		output = object->value.str();
		player->print("It is worth %s", output.c_str());
		if(object->getType() != CONTAINER && object->getType() != MONEY) {
			player->print(", and is ", object);
			if(object->getShotsCur() >= object->getShotsMax() * .99)
				player->print("brand new");
			else if(object->getShotsCur() >= object->getShotsMax() * .90)
				player->print("almost brand new");
			else if(object->getShotsCur() >= object->getShotsMax() * .75)
				player->print("in good condition");
			else if(object->getShotsCur() >= object->getShotsMax() * .50)
				player->print("almost half used up");
			else if(object->getShotsCur() >= object->getShotsMax() * .25)
				player->print("in fair condition");
			else if(object->getShotsCur() >= object->getShotsMax() * .10)
				player->print("in poor condition");
			else if(object->getShotsCur() == 0)
				player->print("broken or used up");
			else
				player->print("close to breaking");
		}
		player->print(".\n");
		return(0);
	}
}

//*********************************************************************
//						cmdSing
//*********************************************************************

int cmdSing(Creature* creature, cmd* cmnd) {
	Player *player = creature->getAsPlayer();
	long	i, t;
	int		(*fn) ();
	int		songno=0, c=0, match=0, n=0;

	if(player && !player->ableToDoCommand())
		return(0);

	if(!player || (player->getClass() != BARD && !player->isCt())) {
		if(mrand(0,10) || creature->isStaff()) {
			creature->print("You sing a song.\n");
			broadcast(creature->getSock(), creature->getRoomParent(), "%M sings a song.", creature);
		} else {
			creature->print("You sing off-key.\n");
			broadcast(creature->getSock(), creature->getRoomParent(), "%M sings off-key.", creature);
		}
		return(0);
	}

	player->clearFlag(P_AFK);

	if(player->getLevel() < 4 && !player->isCt()) {
		player->print("You have not practiced enough to do that yet.\n");
		return(0);
	}
	if(!player->canSpeak()) {
		player->printColor("^yYour lips move but no sound comes forth.\n");
		return(0);
	}

	i = player->lasttime[LT_SING].ltime + player->lasttime[LT_SING].interval;
	t = time(0);
	if(i > t && !player->isCt()) {
		player->pleaseWait(i-t);
		return(0);
	}

	if(cmnd->num < 2) {
		songno = 0;
		match = 1;
	} else {
		do {
			if(!strcmp(cmnd->str[1], get_song_name(c))) {
				match = 1;
				songno = c;
				break;
			} else if(!strncmp(cmnd->str[1], get_song_name(c), strlen(cmnd->str[1]))) {
				match++;
				songno = c;
			}
			c++;
		} while(get_song_num(c) != -1);
	}
	if(match == 0) {
		player->print("That song does not exist.\n");
		return(0);
	} else if(match > 1) {
		player->print("Song name is not unique.\n");
		return(0);
	}
	if(!player->songIsKnown(songno)) {
		player->print("You don't know how the lyrics to that song!\n");
		return(0);
	}
	fn = get_song_function(songno);

	if((int(*)(Player *, cmd*, char*, osong_t*))fn == songOffensive ||
		(int(*)(Player *, cmd*, char*, osong_t*))fn == songMultiOffensive) {
		for(c = 0; osong[c].songno != get_song_num(songno); c++)
			if(osong[c].songno == -1)
				return(0);
		n = ((int(*)(Creature *, cmd*, const char*, osong_t*))*fn) (player, cmnd, get_song_name(songno), &osong[c]);
	} else
		n = ((int(*)(Creature *, cmd*))*fn) (player, cmnd);

	if(n) {
		player->unhide();
		player->lasttime[LT_SING].ltime = t;
		player->lasttime[LT_SING].interval = 120L;
	}

	return(0);
}

//*********************************************************************
//						songMultiOffensive
//*********************************************************************

int songMultiOffensive(Player* player, cmd* cmnd, char *songname, osong_t *oso) {
	int		len=0, ret=0;
	int		monsters=0, players=0;
	char	lastname[80];
	int		count=0;
	int		something_died=0;
	int		found_something=0;

	if(!player->ableToDoCommand())
		return(0);

	if(!player->songIsKnown(oso->songno)) {
		player->print("You don't know that song.\n");
		return(0);
	}

	if(cmnd->num == 2) {
		monsters = 1;
	} else {
		len = strlen(cmnd->str[2]);
		if(!strncmp(cmnd->str[2], "monsters", len)) {
			monsters = 1;
		} else if(!strncmp(cmnd->str[2], "all", len)) {
			monsters = 1;
			players = 1;
		} else if(!strncmp(cmnd->str[2], "players", len)) {
			players = 1;
		} else {
			player->print("Usage: sing %s [<monsters>|<players>|<all>]\n", songname);
			return(0);
		}
	}
	cmnd->num = 3;
	if(monsters) {
	    MonsterSet::iterator mIt = player->getRoomParent()->monsters.begin();
		lastname[0] = 0;
        while(mIt != player->getRoomParent()->monsters.end()) {
            Monster* mons = (*mIt++);
			// skip caster's pet
			if(mons->isPet() && mons->getMaster() == player) {
				continue;
			}
			if(lastname[0] && mons->getName() ==  lastname) {
				count++;
			} else {
				count = 1;
			}
			strncpy(cmnd->str[2], mons->getCName(), 25);
			cmnd->val[2] = count;
			strncpy(lastname, mons->getCName(), 79);
			ret = songOffensive(player, cmnd, songname, oso);
			if(ret == 0)
				return(found_something);
			// target died
			if(ret == 2) {
				count--;
				something_died = 1;
			}
			found_something = 1;
		}
	}
	if(players) {
        PlayerSet::iterator pIt = player->getRoomParent()->players.begin();
        lastname[0] = 0;
        while(pIt != player->getRoomParent()->players.end()) {
            Player* ply = (*pIt++);
			// skip self
			if(ply == player) {
				continue;
			}
			if(lastname[0] && ply->getName() == lastname) {
				count++;
			} else {
				count = 1;
			}
			strncpy(cmnd->str[2], ply->getCName(), 25);
			cmnd->val[2] = count;
			strncpy(lastname, ply->getCName(), 79);
			ret = songOffensive(player, cmnd, songname, oso);
			if(ret == 0)
				return(found_something);
			// target died
			if(ret == 2) {
				count--;
				something_died = 1;
			}
			found_something = 1;
		}
	}
	if(!found_something)
		player->print("Sing to whom?\n");

	return(found_something + something_died);
}

//*********************************************************************
//						songOffensive
//*********************************************************************
// This function is called by all spells whose sole purpose is to do
// damage to a given creature.

int songOffensive(Player* player, cmd* cmnd, char *songname, osong_t *oso) {
	Player	*pCreature=0;
	Creature* creature=0;
	BaseRoom* room = player->getRoomParent();
	int		m=0, dmg=0, bns=0;

	if(!player->ableToDoCommand())
		return(0);

	if(!player->songIsKnown(oso->songno)) {
		player->print("You don't know that song.\n");
		return(0);
	}

	if(!strcmp(songname, "destruction") && player->getLevel() < 13 && !player->isCt()) {
		player->print("You are not experienced enough to sing that.\n");
		return(0);
	}

	player->smashInvis();
	player->interruptDelayedActions();

	bns = (bonus((int) player->intelligence.getCur()) + bonus((int) player->piety.getCur()))/2;

	// sing on self
	if(cmnd->num == 2) {
		player->print("You sing a song.\n");
		broadcast(player->getSock(), room, "%M sings a song.", player);

	// sing on monster or player
	} else {
		creature = room->findCreature(player, cmnd->str[2], cmnd->val[2], true, true);

		if(!creature || creature == player) {
			player->print("That's not here.\n");
			return(0);
		}

		pCreature = creature->getAsPlayer();

		if(!pCreature) {
			// Activates lag protection.
			if(player->flagIsSet(P_LAG_PROTECTION_SET))
				player->setFlag(P_LAG_PROTECTION_ACTIVE);
		}



		if(!player->canAttack(creature))
			return(0);

		if(pCreature) {
			if(player->vampireCharmed(pCreature) || (pCreature->hasCharm(player->getName()) && player->flagIsSet(P_CHARMED))) {
				player->print("You just can't bring yourself to do that.\n");
				return(0);
			}
		}


		if(songFail(player))
			return(0);


		// dmg = dice(oso->ndice, oso->sdice, oso->pdice + bns);
		dmg = (mrand(7,14) + player->getLevel() * 2) + bns;
		dmg = MAX(1, dmg);

		if(!pCreature) {
			// if(is_charm_crt(creature->name, player))
			// del_charm_crt(creature, player);
			m = MIN(creature->hp.getCur(), dmg);

			creature->getAsMonster()->adjustThreat(player, m);
		}



		player->lasttime[LT_SPELL].ltime = time(0);
		player->lasttime[LT_SPELL].interval = 3L;
		player->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
		player->statistics.magicDamage(dmg, (bstring)"a song of " + songname);

		player->print("You sing a song of %s to %s.\n", songname, creature->getCName());
		player->printColor("Your song inflicted %s%d^x damage.\n", player->customColorize("*CC:DAMAGE*").c_str(), dmg);
		broadcast(player->getSock(), creature->getSock(), room, "%M sings a song of %s to %N.", player, songname, creature);
		creature->printColor("%M sings a song of %s to you.\n%M's song inflicted %s%d^x damage on you.\n",
			player, songname, player, creature->customColorize("*CC:DAMAGE*").c_str(), dmg);
		broadcastGroup(false, creature, "^M%M^x sings a song of %s to ^M%N^x for *CC:DAMAGE*%d^x damage, %s%s\n",
		    player, songname, creature, dmg, creature->heShe(), creature->getStatusStr(dmg));

		if(player->doDamage(creature, dmg, CHECK_DIE))
			return(2);

	}
	return(1);
}

//*********************************************************************
//						cmdSongs
//*********************************************************************

int cmdSongs(Player* player, cmd* cmnd) {
	Player	*target = player;
	int		test=0;

	if(player->isCt() && cmnd->num > 1) {
		cmnd->str[1][0] = up(cmnd->str[1][0]);
		target = gServer->findPlayer(cmnd->str[1]);
		test = 1;
		if(!target || !player->canSee(target)) {
			player->print("That player is not logged on.\n");
			return(0);
		}
	}

	songsKnown(player->getSock(), target, test);
	return(0);
}

//*********************************************************************
//						songs_known
//*********************************************************************

int songsKnown(Socket* sock, Player* player, int test) {
	char            str[2048];
	char            sol[128][20];
	int             i=0, j=0;

	if(test)
		sprintf(str, "\n%s's Songs Known: ", player->getCName());
	else
		strcpy(str, "\nSongs known: ");

	for(i=0, j=0; i<gConfig->getMaxSong(); i++)
		if(player->songIsKnown(i))
			strcpy(sol[j++], get_song_name(i));
	if(!j)
		strcat(str, "None.");
	else {
		qsort((void *) sol, j, 20, (PFNCOMPARE) strcmp);
		for(i=0; i<j; i++) {
			strcat(str, "Song of ");
			strcat(str, sol[i]);
			strcat(str, ", ");
		}
		str[strlen(str) - 2] = '.';
		str[strlen(str) - 1] = 0;
	}
	sock->print("%s\n", str);
	return(0);
}

//*********************************************************************
//						songFail
//*********************************************************************

int songFail(Player* player) {
	int chance=0, n = mrand(1, 100);
	player->computeLuck();

	chance = ((player->getLevel() + bonus((int) player->intelligence.getCur())) * 5) + 65;
	chance = chance * player->getLuck() / 50;
	if(n > chance) {
		player->print("You sing off key.\n");
		broadcast(player->getSock(), player->getParent(), "%M sings off key.", player);
		return(1);
	} else
		return(0);
}

//*********************************************************************
//						songHeal
//*********************************************************************

int songHeal(Player* player, cmd* cmnd) {
	int 	 heal=0;

	player->print("You sing a song of healing.\n");
	player->print("Your song rejuvenates everyone in the room.\n");

	heal = mrand(player->getLevel(), player->getLevel()*2);

	if(player->getRoomParent()->magicBonus()) {
		player->print("The room's magical properties increase the power of your song.\n");
		heal += mrand(5, 10);
	}

	for(Player* ply : player->getRoomParent()->players) {
		if(ply->getClass() != LICH) {
			if(ply != player)
				ply->print("%M's song rejuvinates you.\n", player);
			player->doHeal(ply, heal);
		}
	}

	for(Monster* mons : player->getRoomParent()->monsters) {
		if(mons->getClass() != LICH && mons->isPet()) {
			mons->print("%M's song rejuvinates you.\n", player);
			player->doHeal(mons, heal);
		}
	}

	return(1);
}

//*********************************************************************
//						songMPHeal
//*********************************************************************

int songMPHeal(Player* player, cmd* cmnd) {
	int      heal=0;

	player->print("You sing a song of magic restoration.\n");
	player->print("Your song mentally revitalizes everyone in the room.\n");

	heal = mrand(player->getLevel(), player->getLevel()*2)/2;

	if(player->getRoomParent()->magicBonus()) {
		player->print("The room's magical properties increase the power of your song.\n");
		heal += mrand(5, 10);
	}
    for(Player* ply : player->getRoomParent()->players) {
		if(ply->hasMp()) {
			if(ply != player)
				ply->print("%M's song mentally revitalizes you.\n", player);
			ply->mp.increase(heal);
		}
	}
    for(Monster* mons : player->getRoomParent()->monsters) {
		if(mons->hasMp() && mons->isPet()) {
			mons->print("%M's song mentally revitalizes you.\n", player);
			mons->mp.increase(heal);
		}
	}

	return(1);
}

//*********************************************************************
//						songRestore
//*********************************************************************

int songRestore(Player* player, cmd* cmnd) {
	int      heal=0;

	player->print("You sing a song of restoration.\n");
	player->print("Your song restores everyone in the room.\n");

	heal = mrand(player->getLevel(), player->getLevel()*2)*2;

	if(player->getRoomParent()->magicBonus()) {
		player->print("The room's magical properties increase the power of your song.\n");
		heal += mrand(5, 10);
	}
    for(Player* ply : player->getRoomParent()->players) {
		if(ply->getClass() != LICH) {
			if(ply != player)
				ply->print("%M's song restores your spirits.\n", player);
			player->doHeal(ply, heal);
			ply->mp.increase(heal/2);
		}
	}
    for(Monster* mons : player->getRoomParent()->monsters) {
		if(mons->getClass() != LICH && mons->isPet()) {
			mons->print("%M's song restores your spirits.\n", player);
			player->doHeal(mons, heal);
			mons->mp.increase(heal/2);
		}
	}

	return(1);
}

//*********************************************************************
//						songBless
//*********************************************************************

int songBless(Player* player, cmd* cmnd) {
	player->print("You sing a song of holiness.\n");

	int duration = 600;
	if(player->getRoomParent()->magicBonus())
		duration += 300L;

    for(Player* ply : player->getRoomParent()->players) {
		if(ply != player)
			ply->print("%M sings a song of holiness.\n", player);
		ply->addEffect("bless", duration, 1, player, true, player);
	}
    for(Monster* mons : player->getRoomParent()->monsters) {
		if(mons->isPet()) {
			mons->print("%M sings a song of holiness.\n", player);
			mons->addEffect("bless", duration, 1, player, true, player);
		}
	}

	if(player->getRoomParent()->magicBonus())
		player->print("The room's magical properties increase the power of your song.\n");
	return(1);
}

//*********************************************************************
//						songProtection
//*********************************************************************

int songProtection(Player* player, cmd* cmnd) {
	player->print("You sing a song of protection.\n");
	int duration = MAX(300, 1200 + bonus(player->intelligence.getCur()) * 600);

	if(player->getRoomParent()->magicBonus())
		duration += 800L;

    for(Player* ply : player->getRoomParent()->players) {
		if(ply != player)
			ply->print("%M sings a song of protection.\n", player);
		ply->addEffect("protection", duration, 1, player, true, player);
	}
    for(Monster* mons : player->getRoomParent()->monsters) {
		if(mons->isPet()) {
			mons->print("%M sings a song of protection.\n", player);
			mons->addEffect("protection", duration, 1, player, true, player);
		}
	}

	if(player->getRoomParent()->magicBonus())
		player->print("The room's magical properties increase the power of your song.\n");
	return(1);
}

//*********************************************************************
//						songFlight
//*********************************************************************

int songFlight(Player* player, cmd* cmnd) {
	Player* target=0;

	if(cmnd->num == 2) {
		player->print("Your song makes you feel light as a feather.\n");
		broadcast(player->getSock(), player->getParent(), "%M sings a song of flight.", player);

		target = player;

	} else {
		cmnd->str[2][0] = up(cmnd->str[2][0]);
		target = player->getParent()->findPlayer(player, cmnd, 2);
		if(!target) {
			player->print("You don't see that player here.\n");
			return(0);
		}
		broadcast(player->getSock(), target->getSock(), player->getParent(), "%M sings a song of flight to %N.\n", player, target);
		target->print("%M sings a song of flight to you.\n",player);
		player->print("You sing %N a song of flight.\n", target);
	}

	if(player->getRoomParent()->magicBonus())
		player->print("The room's magical properties increase the power of your song.\n");

	target->addEffect("fly", -2, -2, player, true, player);
	return(1);
}

//*********************************************************************
//						songRecall
//*********************************************************************

int songRecall(Player* player, cmd* cmnd) {
	Player	*target=0;
	BaseRoom *newRoom=0;


	if(player->getLevel() < 7) {
		player->print("You are not experienced enough to sing that song yet!\n");
		return(0);
	}
	// Sing on self
	if(cmnd->num == 2) {
		player->print("You sing a song of recall.\n");
		broadcast(player->getSock(), player->getParent(), "%M sings a song of recall.", player);

		if(!player->isStaff() && player->checkDimensionalAnchor()) {
			player->printColor("^yYour dimensional-anchor causes your song to go off-key!^w\n");
			return(1);
		}

		target = player;

	// Sing on another player
	} else {
		cmnd->str[2][0] = up(cmnd->str[2][0]);
		target = player->getParent()->findPlayer(player, cmnd, 2);
		if(!target) {
			player->print("That person is not here.\n");
			return(0);
		}

		player->print("You sing a song of recall on %N.\n", target);
		target->print("%M sings a song of recall on you.\n", player);
		broadcast(player->getSock(), target->getSock(), player->getParent(), "%M sings a song of recall on %N.", player, target);

		if(!player->isStaff() && target->checkDimensionalAnchor()) {
			player->printColor("^y%M's dimensional-anchor causes your song to go off-key!^w\n", target);
			target->printColor("^yYour dimensional-anchor protects you from %N's song of recall!^w\n", player);
			return(1);
		}
	}

	newRoom = target->getRecallRoom().loadRoom(target);
	if(!newRoom) {
		player->print("Song failure.\n");
		return(0);
	}

	broadcast(target->getSock(), player->getRoomParent(), "%M disappears.", target);

	target->deleteFromRoom();
	target->addToRoom(newRoom);
	target->doPetFollow();
	return(1);
}

//*********************************************************************
//						songSafety
//*********************************************************************

int songSafety(Player* player, cmd* cmnd) {
	Player	*follower=0;
	BaseRoom *newRoom=0;


	if(player->getLevel() < 10) {
		player->print("That song's just too hard for you to sing yet.\n");
		return(0);
	}
	player->print("You sing a song of safety.\n");
	broadcast(player->getSock(), player->getParent(), "%M sings a song of safety.", player);

	// handle everyone following singer
	Group* group = player->getGroup();
	if(group) {
		for(Creature* crt : group->members) {
			follower = crt->getAsPlayer();
			if(!follower) continue;
			if(!player->inSameRoom(follower)) continue;
			if(follower->isStaff()) continue;

			if(!player->isStaff() && follower->checkDimensionalAnchor()) {
				player->printColor("^y%M's dimensional-anchor causes your song to go off-key!^w\n", follower);
				follower->printColor("^yYour dimensional-anchor protects you from %N's song of safety!^w\n", player);
				continue;
			}

			newRoom = follower->getRecallRoom().loadRoom(follower);
			if(!newRoom)
				continue;
			broadcast(follower->getSock(), follower->getRoomParent(), "%M disappears.", follower);
			follower->deleteFromRoom();
			follower->addToRoom(newRoom);
			follower->doPetFollow();
		}
	}

	if(!player->isStaff() && player->checkDimensionalAnchor()) {
		player->printColor("^yYour dimensional-anchor causes your song to go off-key!^w\n");
		return(1);
	}

	newRoom = follower->getRecallRoom().loadRoom(follower);
	if(newRoom) {
		broadcast(player->getSock(), player->getParent(), "%M disappears.", player);
		player->courageous();
		player->deleteFromRoom();
		player->addToRoom(newRoom);
		player->doPetFollow();
	}
	return(1);
}

//*********************************************************************
//						cmdCharm
//*********************************************************************

int cmdCharm(Player* player, cmd* cmnd) {
	Creature* creature=0;
	int		dur, chance;
	long	i, t;

	player->clearFlag(P_AFK);

	if(!player->ableToDoCommand())
		return(0);

	if(!player->knowsSkill("charm")) {
		player->print("Sorry, but you aren't very charming.\n");
		return(0);
	}

	if(!player->canSpeak()) {
		player->printColor("^yYour lips move but no sound comes forth.\n");
		return(0);
	}

	i = LT(player, LT_HYPNOTIZE);
	t = time(0);
	if(i > t && !player->isDm()) {
		player->pleaseWait(i-t);
		return(0);
	}

	int level = (int)player->getSkillLevel("charm");
	dur = 300 + mrand(1, 30) * 10 + bonus((int) player->piety.getCur()) * 30 + level * 5;

	//cmnd->str[1][0] = up(cmnd->str[1][0]);

    if(!(creature = player->findVictim(cmnd, 1, true, false, "Charm whom?\n", "You don't see that here.\n")))
		return(0);

	if(!player->canAttack(creature))
		return(0);

	if(	creature->isPlayer() &&
		(	player->vampireCharmed(creature->getAsPlayer()) ||
			(creature->hasCharm(player->getName()) && player->flagIsSet(P_CHARMED))
		))
	{
		player->print("But they are already your good friend!\n");
		return(0);
	}

	if(creature->isMonster() && (creature->flagIsSet(M_NO_CHARM) || (creature->intelligence.getCur() <30))) {
		player->print("Your charm fails.\n");
		return(0);
	}


	if(	creature->isUndead() &&
		!player->checkStaff("Your charm will not work on undead beings.\n") )
		return(0);


	if(creature->isMonster() && creature->getAsMonster()->isEnemy(player)) {
		player->print("Not while you are already fighting %s.\n", creature->himHer());
		return(0);
	}

	player->smashInvis();
	player->lasttime[LT_HYPNOTIZE].ltime = t;
	player->lasttime[LT_HYPNOTIZE].interval = 600L;


	chance = MIN(90, 40 + ((level) - (creature->getLevel())) * 20 + 4 * bonus((int) player->intelligence.getCur()));

	if(creature->flagIsSet(M_PERMENANT_MONSTER) && creature->isMonster())
		chance /= 2;

	if(player->isDm())
		chance = 101;
	if((creature->isUndead() || chance < mrand(1, 100)) && chance != 101) {
		player->print("Your song has no effect on %N.\n", creature);
		player->checkImprove("charm", false);
		broadcast(player->getSock(), player->getParent(), "%M sings off key.",player);
		if(creature->isMonster()) {
			creature->getAsMonster()->addEnemy(player);
			return(0);
		}
		creature->printColor("^m%M tried to charm you.\n", player);
		return(0);
	}

	if(!creature->isCt()) {
		if(creature->chkSave(MEN, player, 0)) {
			player->printColor("^yYour charm failed!\n");
			creature->print("Your mind tingles as you brush off %N's charm.\n", player);
			player->checkImprove("charm", false);
			return(0);
		}
	}

	if(	(creature->isPlayer() && creature->isEffected("resist-magic")) ||
		(creature->isMonster() && creature->isEffected("resist-magic"))
	)
		dur /= 2;

	player->print("Your song charms %N.\n", creature);
	player->checkImprove("charm", true);
	broadcast(player->getSock(), creature->getSock(), player->getRoomParent(), "%M sings to %N!", player, creature);
	creature->print("%M's song charms you.\n", player);
	player->addCharm(creature);

	creature->lasttime[LT_CHARMED].ltime = time(0);
	creature->lasttime[LT_CHARMED].interval = dur;

	creature->setFlag(creature->isPlayer() ? P_CHARMED : M_CHARMED);
	return(0);
}
