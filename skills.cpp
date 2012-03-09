/*
 * skills.cpp
 *	 Skill manipulation functions.
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
#include "skills.h"
#include "commands.h"
#include "clans.h"
#include <sstream>

#define NOT_A_SKILL -10

int SkillInfo::getGainType() const { return(gainType); }
int Skill::getGainBonus() const { return(gainBonus); }
bool SkillInfo::isKnownOnly() const { return(knownOnly); }
bool Config::isKnownOnly(const bstring& skillName) const {
	std::map<bstring, SkillInfo*>::const_iterator it = skills.find(skillName);
	if(it != skills.end())
		return(((*it).second)->isKnownOnly());
	return(false);
}
//*****************************************************************
// Skill - Keeps track of skill information for a Creature
//*****************************************************************

Skill::Skill() {
	reset();
}
Skill::Skill(const bstring& pName, int pGained) {
	reset();
	setName(pName);
	gained = pGained;
}

void Skill::reset() {
	name = "";
	gained = 0;
	gainBonus = 0;
	skillInfo = 0;
}

// End constructors
//--------------------------------------------------------------------




//--------------------------------------------------------------------
// Get/Set Functions
void SkillInfo::setName(bstring pName) {
	name = pName;
}

void SkillCommand::setName(bstring pName) {
	SkillInfo::name = pName;
	Command::name = pName;
}
bstring Skill::getName() const { return(name); }
int Skill::getGained() const { return(gained); }
int Skill::getGainType() const {
	SkillInfo *s = gConfig->getSkill(name);
	return(s ? s->getGainType() : NOT_A_SKILL);
}
bstring Skill::getDisplayName() const { return(gConfig->getSkillDisplayName(name)); }
bstring Skill::getGroup() const { return(gConfig->getSkillGroup(name)); }

void Skill::setGained(int pGained) {
	gained = pGained;
}

void Skill::setName(bstring pName) {
	name = pName;
	updateParent();
}

void Skill::updateParent() {
	skillInfo = gConfig->getSkill(name);
}
// End Get/Set Functions
//--------------------------------------------------------------------


//--------------------------------------------------------------------
// Misc Functions
void Skill::upBonus(int amt) {
	gainBonus += amt;
}
void Skill::clearBonus() {
	gainBonus = 0;
}
void Skill::improve(int amt) {
	gained += amt;
}
// End Misc Functions
//--------------------------------------------------------------------




//*****************************************************************
//						Skill
//*****************************************************************
// Class to store base information about skills

//********************************************************************
//						setGroup
//********************************************************************
// Sets group, aborts if not valid

bool SkillInfo::setGroup(bstring &pGroup) {
	ASSERTLOG(pGroup != "");
	bstring groupName = gConfig->getSkillGroupDisplayName(pGroup);
	if(groupName == "") {
		return(false);
	}
	group = pGroup;
	return true;
}

bstring SkillInfo::getName() const { return(name); }
bstring SkillInfo::getGroup() const { return(group); }
bstring SkillInfo::getDisplayName() const { return(displayName); }
bstring SkillInfo::getDescription() const { return(description); }

//********************************************************************
//						checkImprove
//********************************************************************
//  Parameters: skillName - What skill are we checking for improvement
//				success - Was the skill sucessfull
//				attribute - What attribute will be helpful in raising the skill? (default: INT)
//				bns - Any bonus to the improve calculation (default: 0)

void Creature::checkImprove(const bstring& skillName, bool success, int attribute, int bns) {
	if(isMonster())
		return;
	if(inJail())
		return;

	Skill* crSkill = getSkill(skillName);
	if(!crSkill)
		return;

	int gainType = crSkill->getGainType();
	// not a skill!
	if(gainType == NOT_A_SKILL) {
		broadcast(::isDm, "^y*** Skill \"%s\" was requested by the mud, but was not\n    found in the skill list. Check *skills to verify.", skillName.c_str());
		return;
	}
	long	j=0,t;

	t = time(0);
	j = LT(this, LT_SKILL_INCREASE);
	//
	if(t < j)
		return;

	int chance = 0;
	int gained = crSkill->getGained();

	chance = 100 - (gained/3);
	// Chance is too high right now, reduce it by half
	chance /= 2;

	if(gained < 100) {
		// You learn more from your failures at lower skill levels
		if(!success)
			chance *= 2;
	} else {
		// You learn less from failures at higher skill levels
		if(!success)
			chance /= 2;
	}

	if(gainType == SKILL_EASY) {
		// make it harder
		chance /= 2;
	} else if(gainType != SKILL_NORMAL) {
		chance += crSkill->getGainBonus();
	}

	// Unless max for level, 3% chance minimum
	chance = MAX(chance, 3);

	// 10 skill points per level possible, can't go over that
	if(gained >= (level*10))
		chance = 0;

    int roll = mrand(1, 100);
	if(roll <= chance) {
		bool af = gConfig->isAprilFools();
		if(success) {
			printColor("Your practice %spays off, you have become %s at ^W%s^x!\n",
				af ? "fails to " : "", af ? "worse" : "better",
				gConfig->getSkillDisplayName(skillName).c_str());
		} else {
			printColor("You %slearn from your mistakes, you have become %s at ^W%s^x!\n",
				af ? "fail to " : "", af ? "worse" : "better",
				gConfig->getSkillDisplayName(skillName).c_str());
		}
		if(isPlayer()) {
			lasttime[LT_SKILL_INCREASE].ltime = t;
			// Length of wait is based on skill level, not player level
			long wait = 10L + (gained/10);
			wait = MIN(wait, 150L);
			lasttime[LT_SKILL_INCREASE].interval = wait;
		}
		// Chance for a double improve for a hard skill
		if(gainType == SKILL_HARD && mrand(1,100) < 33)
			crSkill->improve(2);
		else
			crSkill->improve();

		crSkill->clearBonus();		
		
	} else {
		// See if we have a hard skill on our hands, if so add a bonus to increase
		if(gainType == SKILL_MEDIUM) {
			crSkill->upBonus();
		} else if(gainType == SKILL_HARD) {
			crSkill->upBonus(2);
		}
	}
}


//********************************************************************
//						knowsSkill
//********************************************************************

bool Creature::knowsSkill(const bstring& skillName) const {
	if(isMonster())	return(true);
	if(isCt())	  		return(true);
	if(skillName == "")	return(false);

	std::map<bstring, Skill*>::const_iterator csIt;
	if( (csIt = skills.find(skillName)) == skills.end())
		return(false);
	else
		return(true);
}

//********************************************************************
//						getSkill
//********************************************************************
// Returns the requested skill if it can be found on the creature

Skill* Creature::getSkill(const bstring& skillName) const {
	if(skillName == "")	return(null);

	SkillMap::const_iterator csIt = skills.find(skillName);
	if( csIt == skills.end())
		return(NULL);
	else
		return((*csIt).second);
}

//********************************************************************
//						addSkill
//********************************************************************
// Add a new skill of 'skillName' at 'gained' level

void Creature::addSkill(const bstring& skillName, int gained) {
	if(skillName == "")
		return;

	Skill* skill = new Skill(skillName, gained);
	skills[skillName] = skill;
}

//********************************************************************
//						remSkill
//********************************************************************

void Creature::remSkill(const bstring& skillName) {
	if(skillName == "")
		return;
	SkillMap::iterator it = skills.find(skillName);
	if(it == skills.end())
	    return;
	Skill* skill = (*it).second;


	if(!skill)
		return;
	delete skill;
	skills.erase(skillName);
}

#define SKILL_CHART_SIZE		21
char skillLevelStr[][SKILL_CHART_SIZE] =
{
	"^rHorrible^x",			// 0-24
	"^rPoor^x",				// 25-49
	"^rFair^x",				// 50-74
	"^mMediocre^x",			// 75-99
	"^mDecent^x",			// 100-124
	"^mBelow Average^x",	// 125-149
	"^wCompetent^x",		// 150-174
	"^wAverage^x",			// 175-199
	"^wAbove Average^x",	// 200-224
	"^cProficient^x",		// 225-249
	"^cSkilled^x",			// 250-274
	"^cTalented^x",			// 275-299
	"^bVery Good^x",		// 300-324
	"^bAdept^x",			// 325-349
	"^bSuperb^x",			// 350-374
	"^gExceptional^x",		// 375-399
	"^gExpert^x",			// 400-424
	"^gMaster^x",			// 425-449
	"^ySuperior Master^x",	// 450-474
	"^yGrand Master^x",		// 474-499
	"^yGodlike^x"			// 500
};

char craftSkillLevelStr[][25] =
{
	"Novice",
	"Apprentice",
	"Journeyman",
	"Expert",
	"Artisian",
	"Master",
	"Grand Master"
};


//********************************************************************
//						showSkills
//********************************************************************
// Show the skills and skill levels of 'player'  to 'sock'

int showSkills(Player* toShow, Creature* player, bool magicOnly=false) {
	std::map<bstring, bstring>::iterator sgIt;
	std::map<bstring, Skill*>::iterator sIt;
	Skill* crtSkill=0;
	int known=0;
	double skill=0;
	const Clan *clan=0;

	bool showProgress = player->flagIsSet(P_SHOW_SKILL_PROGRESS);
	bool showDigits = !showProgress;

	int barLength = 20;

	if(player->getClan())
		clan = gConfig->getClan(player->getClan());


	// Tell the player what skills they are looking at
	if(toShow->getAsPlayer() == player)
		toShow->printColor("^YYour Skills:");
	else
		toShow->printColor("^Y%s's Skills:", player->name);

	if(magicOnly)
		toShow->printColor(" ^Ytype \"skills\" to show non-magical skills.");
	else if(player->getClass() != BERSERKER)
		toShow->printColor(" ^Ytype \"skills magic\" to show magical skills.");

	toShow->print("\n");



	for(sgIt = gConfig->skillGroups.begin() ; sgIt != gConfig->skillGroups.end() ; sgIt++) {
		if(	(*sgIt).first == "arcane" ||
			(*sgIt).first == "divine" ||
			(*sgIt).first == "magic"
		) {
			if(!magicOnly)
				continue;
		} else {
			if(magicOnly)
				continue;
		}

		std::ostringstream oStr;
		known = 0;
		oStr << "^W" << (*sgIt).second << "^x\n";
		for(sIt = player->skills.begin() ; sIt != player->skills.end() ; sIt++) {
			crtSkill = (*sIt).second;
			if(crtSkill->getGroup() == (*sgIt).first) {
				//1+player->saves[st].chance)/10
				known++;

				//bool isCraft = crtSkill->getGroup() == "craft";


				int curSkill = 0;
				float maxSkill = 0;
				//if(isCraft) {
				//	maxSkill = MIN(player->getLevel()*10, 100);
				//	curSkill = MIN(crtSkill->getGained(), (int)maxSkill);
				//} else {
					maxSkill = MIN(player->getLevel()*10.0, MAXALVL*10.0);
					curSkill = MIN(crtSkill->getGained(), maxSkill);
				//}

				skill = curSkill;
				if(clan)
					skill += clan->getSkillBonus(crtSkill->getName());
				skill /= 25;

				int displayNum = (int)skill;

				displayNum = MAX(0, MIN(SKILL_CHART_SIZE-1, displayNum));

				//bstring progressBar(int barLength, float percentFull, bstring text, char progressChar, bool enclosed)
				oStr << " ";
				if(showProgress) {
					oStr << " ";

					bstring progress = "" + bstring(curSkill) + "/" + bstring(maxSkill);
					if(gConfig->isKnownOnly(crtSkill->getName()))
						oStr << progressBar(barLength, 1);
					else
						oStr << progressBar(barLength, (1.0*curSkill)/maxSkill, progress.c_str());
				}

				oStr << " " << crtSkill->getDisplayName() << " - ";
				if(gConfig->isKnownOnly(crtSkill->getName())) {
					oStr << "^WKnown^x\n";
					continue;
				}
				//if(!isCraft) {
				//	// Not a crafting skill
					oStr << skillLevelStr[displayNum];
				//} else {
				//	// Crafting Skill
				//	int disp = (curSkill/maxSkill)*6;
				//	oStr << craftSkillLevelStr[disp];
				//}
				if(showDigits )
					oStr << " (" << curSkill << ")";

				skill = 0;
				if(clan)
					skill = clan->getSkillBonus(crtSkill->getName());
				if((int)skill)
					oStr << " (Clan: " << skill << ")";
				if((*sIt).first == "defense" && player->isEffected("protection"))
					oStr << " (Protection: 10)";

				if(toShow->isCt()) {  // && !isCraft)
//					oStr << " (" << crtSkill->getGained() << ")";
					oStr << " [" << player->getSkillLevel(crtSkill->getName()) << "]";
				}
				oStr << "\n";
			}
		}
		if(known)
			toShow->printColor("%s", oStr.str().c_str());
	}
	return(0);
}

//********************************************************************
//						getSkillLevel
//********************************************************************
// Return the player level equilvalent of the given skill

double Creature::getSkillLevel(const bstring& skillName) const {
	if(isMonster())
		return(level);

	Skill* skill = getSkill(skillName);
	if(!skill) {
		if(isCt())
			return(MAXALVL);
		else
			return(0);
	}
	int gained = getSkillGained(skillName);

	if(clan) {
		const Clan* c = gConfig->getClan(clan);
		if(clan)
			gained += c->getSkillBonus(skillName);
	}

	double level = 0.0;
	if(gained <= 100) {
		// If less than 100, adding 5 to help out lower levels
		level = ((double)gained + 5.0) / 10.0;
	} else {
		// If over 100, divide by 9.5 to provide more returns for higher levels
		level = (double)gained / 9.5;
	}
	return(level);
}

//********************************************************************
//						getSkillGained
//********************************************************************

double Creature::getSkillGained(const bstring& skillName) const {
	Skill* skill = getSkill(skillName);
	if(skill==NULL) {
		if(isCt())
			return(MAXALVL*10);
		else
			return(0);
	}
	double gained = (skill->getGained()*1.0);
	// Prevents a level 30 from deleveling to 25 and still fighting like a level 30
	if(gained/10 > level)
		gained = level*10.0;

	return(gained);
}

double Creature::getTradeSkillGained(const bstring& skillName) const {
	Skill* skill = getSkill(skillName);
	if(skill==NULL) {
		if(isCt())
			return(MAXALVL*10);
		else
			return(0);
	}
	double gained = (skill->getGained()*1.0);

	if(level < 7) {
		gained = MIN(gained, 75.0);
	} else if (level < 13) {
		gained = MIN(gained, 150.0);
	} else {
		gained = MIN(gained, 300.0);
	}

//	// Prevents a level 30 from deleveling to 25 and still fighting like a level 30
//	if(gained/10 > level)
//		gained = level*10.0;

	return(gained);
}

//********************************************************************
//						dmSkills
//********************************************************************
// List the skill table, or optionally a player's known skills

int dmSkills(Player* player, cmd* cmnd) {


	if(cmnd->num < 2) {
		std::map<bstring, bstring>::iterator sgIt;
		std::map<bstring, SkillInfo*>::iterator sIt;
		player->printColor("^xSkill Groups\n%-20s - %40s\n---------------------------------------------------------------\n", "Name", "DisplayName");
		for(sgIt = gConfig->skillGroups.begin() ; sgIt != gConfig->skillGroups.end() ; sgIt++) {
			player->print("%-20s - %40s\n", (*sgIt).first.c_str(), (*sgIt).second.c_str());
		}

		player->printColor("\n^xSkills\n%-20s - %20s - %-15s\n-------------------------------------------------------------\n", "Name", "DisplayName", "Group");
		SkillInfo* skill;
		bstring curGroup;
		for(sgIt = gConfig->skillGroups.begin() ; sgIt != gConfig->skillGroups.end() ; sgIt++) {
			curGroup = (*sgIt).first;
			for(sIt = gConfig->skills.begin() ; sIt != gConfig->skills.end() ; sIt++) {
				skill = (*sIt).second;
				if(skill->getGroup() == curGroup) {
					char color[3] = "";
					if(skill->getGainType() == SKILL_MEDIUM)
						strcpy(color, "^y");
					else if(skill->getGainType() == SKILL_HARD)
						strcpy(color, "^r");
					else if(skill->getGainType() == SKILL_EASY)
						strcpy(color, "^g");
					player->printColor("%s%-20s - %20s - %-15s\n",color,
							skill->getName().c_str(), skill->getDisplayName().c_str(), skill->getGroup().c_str());
				}
			}
		}
	} else {
		Creature* target=0;
		cmnd->str[1][0] = up(cmnd->str[1][0]);
		target = gServer->findPlayer(cmnd->str[1]);
		cmnd->str[1][0] = low(cmnd->str[1][0]);
		if(!target || (!player->isCt() && target->flagIsSet(P_DM_INVIS)))
			target = player->getParent()->findCreature(player, cmnd);

		if(!target) {
			player->print("Target not found.\n");
			return(0);
		}
		player->print("Skills for: %s\n", target->name);
		showSkills(player, target, true);
	}
	return(0);
}

//********************************************************************
//						dmSetSkills
//********************************************************************

int dmSetSkills(Player *admin, cmd* cmnd) {
	Player* target=0;
	
	if(cmnd->num < 2) {
		admin->print("Set skills for who?\n");
		return(0);
	}

	cmnd->str[1][0] = up(cmnd->str[1][0]);
	target = gServer->findPlayer(cmnd->str[1]);

	if(!target) {
		admin->print("No player logged on with that name.\n");
		return(0);
	}
	
	PlayerClass *pClass = gConfig->classes[target->getClassString()];
	if(pClass) {
		target->checkSkillsGain(pClass->getSkillBegin(), pClass->getSkillEnd(), true);
		LevelGain *lGain;
		for(int level = 2 ; level <= target->getLevel() ; level++ ) {
			lGain = pClass->getLevelGain(level);
			if(!lGain) {
				admin->print("Error: Can't find any information for level %d!\n", level);
			} else {
				if(lGain->hasSkills()) {
					target->checkSkillsGain(lGain->getSkillBegin(), lGain->getSkillEnd(), true);
				}
			}
		}
		admin->print("%s's skills have been set for %s level.\n", target->name, target->hisHer());
	} else {
		admin->print("Unable to find class %s.\n", target->getClassString().c_str());
	}

	return(0);
}

//********************************************************************
//						cmdSkills
//********************************************************************
// Display all skills a player knows and their level

int cmdSkills(Player* player, cmd* cmnd) {
	Creature* target = player;
	bool magicOnly=false;
	bool online=true;
	int pos = 1;

	magicOnly = cmnd->str[1][0] && !strncmp(cmnd->str[1], "magic", strlen(cmnd->str[1]));

	if(player->isCt()) {
		if(magicOnly)
			pos = 2;
		if(cmnd->str[pos][0]) {
			cmnd->str[pos][0] = up(cmnd->str[pos][0]);
			target = gServer->findPlayer(cmnd->str[pos]);

			if(!target) {
				Player* pTarget = 0;
				loadPlayer(cmnd->str[pos], &pTarget);
				target = pTarget;
				online = false;
			}

			if(!target) {
				player->print("No player logged on with that name.\n");
				return(0);
			}
			if(target->isDm() && !player->isDm()) {
				player->print("You cannot view that player's skills.\n");
				if(!online)
					free_crt(target);
				return(0);
			}
		}
	}

	showSkills(player, target, magicOnly);
	if(!online)
		free_crt(target);
	return(0);
}
// End Skill functions related to Creatures
//--------------------------------------------------------------------





struct {
    const char *songstr;
    int songno;
    int (*songfn)();
} songlist[] = {
	{ "healing",			SONG_HEAL,				(int(*)())songHeal				},
	{ "magic",				SONG_MP,				(int(*)())songMPHeal			},
	{ "restoration",		SONG_RESTORE,			(int(*)())songRestore			},
	{ "destruction",		SONG_DESTRUCTION,		(int(*)())songOffensive			},
	{ "mass-destruction",	SONG_MASS_DESTRUCTION,	(int(*)())songMultiOffensive	},
	{ "holiness",			SONG_BLESS,				(int(*)())songBless				},
	{ "protection",			SONG_PROTECTION,		(int(*)())songProtection		},
	{ "flight",				SONG_FLIGHT,			(int(*)())songFlight			},
	{ "recall",				SONG_RECALL,			(int(*)())songRecall			},
	{ "safety",				SONG_SAFETY,			(int(*)())songSafety			},
	{ "@",					-1,						0								}
};
int songlist_size = sizeof(songlist)/sizeof(*songlist);


//**********************************************************************
// get_song_name
//**********************************************************************
const char *get_song_name(int nIndex) {
    // do bounds checking
    ASSERTLOG(nIndex >= 0);
    ASSERTLOG(nIndex < songlist_size);

    nIndex = MAX(0, MIN(nIndex, songlist_size));

    return(songlist[nIndex].songstr);
}

//**********************************************************************
// get_song_num()
//**********************************************************************
int get_song_num(int nIndex) {
    // do bounds checking
    ASSERTLOG(nIndex >= 0);
    ASSERTLOG(nIndex < songlist_size);

    nIndex = MAX(0, MIN(nIndex, songlist_size));

    return(songlist[nIndex].songno);
}


//**********************************************************************
// get_song_function()
//**********************************************************************
SONGFN get_song_function(int nIndex) {
    // do bounds checking
    ASSERTLOG(nIndex >= 0);
    ASSERTLOG(nIndex < songlist_size);

    nIndex = MAX(0, MIN(nIndex, songlist_size));

    return(songlist[nIndex].songfn);
}

//**********************************************************************
//						getMaxSong
//**********************************************************************

int Config::getMaxSong() {
    return(songlist_size-1);
}


// Clears skills
void Config::clearSkills() {
	// Clear skill groups
	skillGroups.clear();

	// Clear & delete skills
	SkillInfoMap::iterator sIt;
	SkillInfo* skill;
	for(sIt = skills.begin() ; sIt != skills.end() ; sIt++) {
		skill = (*sIt).second;
		delete skill;
		//skills.erase(sIt);
	}
	skills.clear();
	// Every skill command is in the skills list, so they've already been erased, just clear them
	skillCommands.clear();
}
// Updates skill pointers on players
void Config::updateSkillPointers() {
	for(PlayerMap::value_type pp : gServer->players) {
		for(SkillMap::value_type sp : pp.second->skills) {
			sp.second->updateParent();
		}
	}

}


//********************************************************************
//						skillExists
//********************************************************************
// True if the skill exists

bool Config::skillExists(const bstring& skillName) const {
	std::map<bstring, SkillInfo*>::const_iterator it = skills.find(skillName);
	return(it != skills.end());
}

//********************************************************************
//						getSkill
//********************************************************************
// Returns the given skill skill

SkillInfo* Config::getSkill(const bstring& skillName) const {
	std::map<bstring, SkillInfo*>::const_iterator it = skills.find(skillName);
	if(it != skills.end())
		return((*it).second);
	return(0);
}

//********************************************************************
//						getSkillDisplayName
//********************************************************************
// Get the display name of the skill

bstring Config::getSkillDisplayName(const bstring& skillName) const {
	std::map<bstring, SkillInfo*>::const_iterator it = skills.find(skillName);
	if(it != skills.end())
		return(((*it).second)->getDisplayName());
	return("");
}

//********************************************************************
//						getSkillGroupDisplayName
//********************************************************************
// Get the group display name of the skill

bstring Config::getSkillGroupDisplayName(const bstring& groupName) const {
	std::map<bstring, bstring>::const_iterator it = skillGroups.find(groupName);
	if(it != skillGroups.end())
		return((*it).second);
	return("");
}

//********************************************************************
//						getSkillGroup
//********************************************************************
// Get the skill group of the skill

bstring Config::getSkillGroup(const bstring& skillName) const {
	std::map<bstring, SkillInfo*>::const_iterator it = skills.find(skillName);
	if(it != skills.end())
		return(((*it).second)->getGroup());
	return("");
}

// End Skill Related Functions
//--------------------------------------------------------------------


SkillInfo::SkillInfo() {
	gainType = SKILL_NORMAL;
	knownOnly = false;
}
