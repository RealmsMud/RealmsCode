/*
 * objects.cpp
 *	 Object routines.
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
#include "objects.h"
#include "clans.h"
#include "alchemy.h"


bool Object::operator< (const Object& t) const {
	return(getCompareStr().compare(t.getCompareStr()) < 0);
}

bstring Object::getCompareStr() const {
	bstring toReturn = getName() + "-" + bstring(adjustment) + "-" + bstring(shopValue) + "-" + getId();
	return(toReturn);
}

bstring DroppedBy::getName() const {
	return(name);
}
bstring DroppedBy::getIndex() const {
	return(index);
}
bstring DroppedBy::getId() const {
	return(id);
}
bstring DroppedBy::getType() const {
	return(type);
}
void DroppedBy::clear() {
	name.clear();
	index.clear();
	id.clear();
	type.clear();
}

//*********************************************************************
//						operator<<
//*********************************************************************

std::ostream& operator<<(std::ostream& out, const DroppedBy& drop) {
	out << drop.name << " (" << drop.index;
	if(!drop.id.empty()) {
		if(!drop.index.empty())
			out << ":";
		out << drop.id;
	}
	out << ") [" << drop.type << "]";
	return(out);
}

bstring DroppedBy::str() {
	std::ostringstream oStr;
	oStr << name << " (" << index;
	if(!id.empty()) {
		if(!index.empty())
			oStr << ":";
		oStr << id;
	}
	oStr << ") [" << type << "]";
	return(oStr.str());
}

void Object::validateId() {
	if(id.empty() || id.equals("-1")) {
		setId(gServer->getNextObjectId());
	}
}

void Object::setDroppedBy(MudObject* dropper, bstring pDropType) {

	droppedBy.name = dropper->getName();
	droppedBy.id = dropper->getId();

	Monster* mDropper = dynamic_cast<Monster*>(dropper);
	Object* oDropper = dynamic_cast<Object*>(dropper);
	UniqueRoom* rDropper = dynamic_cast<UniqueRoom*>(dropper);
	AreaRoom* aDropper = dynamic_cast<AreaRoom*>(dropper);

	if(mDropper) {
		droppedBy.index = mDropper->info.rstr();
	} else if(oDropper) {
		droppedBy.index = oDropper->info.rstr();
	} else if(rDropper) {
		droppedBy.index = rDropper->info.rstr();
	} else if(aDropper) {
		droppedBy.index = aDropper->area->name + aDropper->fullName();
	}
	else {
		droppedBy.index = "";
	}

	droppedBy.type = pDropType;
}
//*********************************************************************
//							Object
//*********************************************************************

Object::Object() {
	int i;

	id = "-1";
	version = "0.00";
	description = effect = "";
	memset(key, 0, sizeof(key));
	memset(use_output, 0, sizeof(use_output));
	memset(use_attack, 0, sizeof(use_attack));
	weight = type = adjustment = shotsMax = shotsCur = armor =
		wearflag = magicpower = level = requiredSkill = clan =
		special = delay = quality = effectStrength = effectDuration = chargesCur = chargesMax = 0;
	memset(flags, 0, sizeof(flags));
	questnum = 0;
	numAttacks = bulk = maxbulk = lotteryCycle = 0;
	coinCost = shopValue = 0;
	size = NO_SIZE;

	for(i=0; i<6; i++)
		lotteryNumbers[i] = 0;

	material = NO_MATERIAL;
	keyVal = minStrength = 0;
	compass = 0;
	increase = 0;
	recipe = 0;
	made = 0;
	extra = 0;

	hooks.setParent(this);
}

Object::~Object() {
	if(compass)
		delete compass;
	if(increase)
		delete increase;
	alchemyEffects.clear();
	compass = 0;
	ObjectSet::iterator it;
	Object* obj;
	for(it = objects.begin() ; it != objects.end() ; ) {
		obj = (*it++);
		delete obj;
	}
	objects.clear();
	gServer->removeDelayedActions(this);
	moDestroy();
}

//*********************************************************************
//							init
//*********************************************************************
// Initiate the object - used when the object is first introduced into the
// world.

void Object::init(bool selRandom) {
	// this object may turn into another object
	if(selRandom)
		selectRandom();

	if(flagIsSet(O_RANDOM_ENCHANT))
		randomEnchant();

	loadContainerContents();
	setMade();
	setFlag(O_JUST_LOADED);
}

//*********************************************************************
//							getNewPotion
//*********************************************************************

Object* Object::getNewPotion() {
	Object* newPotion = new Object;

	newPotion->type = POTION;
	newPotion->setName( "generic potion");
	strcpy(newPotion->key[0], "generic");
	strcpy(newPotion->key[1], "potion");
	newPotion->weight = 1;
	newPotion->setFlag(O_SAVE_FULL);

	return(newPotion);
}

//*********************************************************************
//							doCopy
//*********************************************************************

void Object::doCopy(const Object& o) {
	int		i=0;

	moCopy(o);

	description = o.description;

	droppedBy = o.droppedBy;

	plural = o.plural;
	for(i=0; i<3; i++)
		strcpy(key[i], o.key[i]);
	strcpy(use_output, o.use_output);
	strcpy(use_attack, o.use_attack);
	version = o.version;
	weight = o.weight;
	type = o.type;
	adjustment = o.adjustment;
	chargesMax = o.chargesMax;
	chargesCur = o.chargesCur;
	shotsMax = o.shotsMax;
	shotsCur = o.shotsCur;
	damage = o.damage;
	armor = o.armor;
	wearflag = o.wearflag;
	magicpower = o.magicpower;
	info = o.info;
	level = o.level;
	quality = o.quality;
	requiredSkill = o.requiredSkill;
	clan = o.clan;
	special = o.special;
	for(i=0; i<OBJ_FLAG_ARRAY_SIZE; i++)
		flags[i] = o.flags[i];
	questnum = o.questnum;
	parent = o.parent;
	for(i=0; i<4; i++)
		lasttime[i] = o.lasttime[i];

	for(i=0; i<3; i++)
		in_bag[i] = o.in_bag[i];

	lastMod = o.lastMod;

	bulk = o.bulk;
	delay = o.delay;
	maxbulk = o.maxbulk;
	numAttacks = o.numAttacks;

	lotteryCycle = o.lotteryCycle;

	coinCost = o.coinCost;
	deed = o.deed;
	shopValue = o.shopValue;

	value.set(o.value);

	made = o.made;
	keyVal = o.keyVal;
	extra = o.extra;
	questOwner = o.questOwner;

	minStrength = o.minStrength;
	material = o.material;

	for(i=0; i<6; i++)
		lotteryNumbers[i] = o.lotteryNumbers[i];
	refund.set(o.refund);
	size = o.size;

	if(o.compass) {
		if(compass)
			delete compass;
		compass = new MapMarker;
		*compass = *o.compass;
	}
	if(o.increase) {
		if(increase)
			delete increase;
		increase = new ObjIncrease;
		*increase = *o.increase;
	}

	recipe = o.recipe;
	effect = o.effect;
	effectDuration = o.effectDuration;
	effectStrength = o.effectStrength;

	// copy everything contained inside this object
	Object *newObj;
	for(Object* obj : o.objects) {
		newObj = new Object;
		*newObj = *obj;
		addObj(newObj, false);
	}
	for(std::pair<int, AlchemyEffect> p : o.alchemyEffects) {
		alchemyEffects[p.first] = p.second;
	}
	subType = o.subType;

	std::list<CatRef>::const_iterator it;
	randomObjects.clear();
	for(it = o.randomObjects.begin() ; it != o.randomObjects.end() ; it++) {
		randomObjects.push_back(*it);
	}
}

DroppedBy& DroppedBy::operator=(const DroppedBy& o) {
	name = o.name;
	index = o.index;
	id = o.id;
	type = o.type;
	return(*this);
}

Object& Object::operator=(const Object& o) {
	doCopy(o);
	return(*this);
}

bool Object::operator==(const Object& o) const {
	int		i=0;

	if(	description != o.description ||
		version != o.version ||
		weight != o.weight ||
		type != o.type ||
		adjustment != o.adjustment ||
		shotsMax != o.shotsMax ||
		shotsCur != o.shotsCur ||
		damage != o.damage ||
		armor != o.armor ||
		wearflag != o.wearflag ||
		magicpower != o.magicpower ||
		info != o.info ||
		level != o.level ||
		quality != o.quality ||
		requiredSkill != o.requiredSkill ||
		clan != o.clan ||
		special != o.special ||
		questnum != o.questnum ||
		lastMod != o.lastMod ||
		bulk != o.bulk ||
		delay != o.delay ||
		maxbulk != o.maxbulk ||
		numAttacks != o.numAttacks ||
		lotteryCycle != o.lotteryCycle ||
		coinCost != o.coinCost ||
		deed != o.deed ||
		shopValue != o.shopValue ||
		made != o.made ||
		keyVal != o.keyVal ||
		extra != o.extra ||
		questOwner != o.questOwner ||
		minStrength != o.minStrength ||
		material != o.material ||
		size != o.size ||
		recipe != o.recipe ||
		effect != o.effect ||
		effectDuration != o.effectDuration ||
		effectStrength != o.effectStrength ||
		subType != o.subType ||
		randomObjects.size() != o.randomObjects.size() ||
		alchemyEffects.size() != o.alchemyEffects.size() ||
		!objects.empty() ||
		!o.objects.empty() ||
		strcmp(use_output, o.use_output) ||
		strcmp(use_attack, o.use_attack) ||
		value != o.value ||
		refund != o.refund
	)
		return(false);

	for(i=0; i<3; i++)
		if(strcmp(key[i], o.key[i]))
			return(false);
	for(i=0; i<OBJ_FLAG_ARRAY_SIZE; i++)
		if(flags[i] != o.flags[i])
			return(false);
	for(i=0; i<4; i++) {
		if(	lasttime[i].interval != o.lasttime[i].interval ||
			lasttime[i].ltime != o.lasttime[i].ltime ||
			lasttime[i].misc != o.lasttime[i].misc
		)
			return(false);
	}
	for(i=0; i<3; i++)
		if(in_bag[i] != o.in_bag[i])
			return(false);
	for(i=0; i<6; i++)
		if(lotteryNumbers[i] != o.lotteryNumbers[i])
			return(false);

	if(compass && o.compass) {
		if(*compass != *o.compass)
			return(false);
	} else if(compass || o.compass)
		return(false);

	/*
	for(std::pair<int, bstring> p : o.alchemyEffects) {
		alchemyEffects.insert(p);
	}
	*/

	std::list<CatRef>::const_iterator rIt, orIt;
	rIt = randomObjects.begin();
	orIt = o.randomObjects.begin();
	for(; rIt != randomObjects.end() ;) {
		if(*rIt != *orIt)
			return(false);
		rIt++;
		orIt++;
	}
	return(true);
}
bool Object::operator!=(const Object& o) const {
	return(!(*this==o));
}

//*********************************************************************
//							getActualWeight
//*********************************************************************
// This function computes the total amount of weight of an object and
// all its contents.

int Object::getActualWeight() const {
	int		n=0;

	n = weight;

	for(Object* obj : objects) {
		if(!obj->flagIsSet(O_WEIGHTLESS_CONTAINER))
			n += obj->getActualWeight();
	}

	return(n);
}

//*********************************************************************
//							getActualBulk
//*********************************************************************

int Object::getActualBulk() const {
	int		n=0;

	if(flagIsSet(O_BULKLESS_OBJECT))
		return(0);

	if(bulk <= 0) {
		switch(type) {
		case	WEAPON:
			{
				bstring category = getWeaponCategory();
				if(category == "crushing")
					n = 5;
				else if(category == "piercing")
					n = 4;
				else if(category == "slashing")
					n = 4;
				else if(category == "ranged")
					n = 6;
				else if(category == "chopping")
					n = 8;
				else
					n = 4;
			}
			break;
		case	MONEY:
		case	POISON:
		case	POTION:
			n = 4;
			break;
		case ARMOR:
			switch(wearflag) {
			case BODY:
				n = 20;
				break;
			case ARMS:
				n = 12;
				break;
			case LEGS:
				n = 14;
				break;
			case NECK:
				n = 6;
				break;
			case BELT:
			case HANDS:
			case HEAD:
			case FEET:
			case HELD:
			case FACE:
				n = 4;
				break;
			case FINGER:

			case FINGER2:
			case FINGER3:
			case FINGER4:
			case FINGER5:
			case FINGER6:
			case FINGER7:
			case FINGER8:
				n = 1;
				break;
			case SHIELD:
				n = 10;
				break;
			default:
				n =1;
				break;
			}
			break;
		case SCROLL:
		case WAND:
		case SONGSCROLL:
		case MISC:
			n = 3;
			break;
		case KEY:
			n = 1;
			break;
		case CONTAINER:
			n = 5;
			break;
		case LIGHTSOURCE:
		case BANDAGE:
			n = 2;
			break;
		}
	} else
		n = bulk;

	return(n);
}

//*********************************************************************
//							raceRestrict
//*********************************************************************
// become a random object

void Object::selectRandom() {
	std::list<CatRef>::const_iterator it;
	CatRef cr;
	Object* object=0;
	int which = randomObjects.size();

	// Load all means we will become every object in this list (trade only)
	// rather than a random one. Thus, don't use this code.
	if(!which || flagIsSet(O_LOAD_ALL))
		return;
	which = mrand(1, which);

	for(it = randomObjects.begin(); it != randomObjects.end(); it++) {
		which--;
		if(!which) {
			cr = *it;
			break;
		}
	}

	if(!loadObject(cr, &object))
		return;

	*this = *object;
	delete object;
}

//*********************************************************************
//							raceRestrict
//*********************************************************************

bool Object::raceRestrict(const Creature* creature, bool p) const {
	if(raceRestrict(creature)) {
		if(p) creature->checkStaff("Your race prevents you from using %P.\n", this);
		if(!creature->isStaff()) return(true);
	}
	return(false);
}

bool Object::raceRestrict(const Creature* creature) const {
	bool pass = false;

	// if no flags are set
	if(	!flagIsSet(O_SEL_DWARF) &&
		!flagIsSet(O_SEL_ELF) &&
		!flagIsSet(O_SEL_HALFELF) &&
		!flagIsSet(O_SEL_HALFLING) &&
		!flagIsSet(O_SEL_HUMAN) &&
		!flagIsSet(O_SEL_ORC) &&
		!flagIsSet(O_SEL_HALFGIANT) &&
		!flagIsSet(O_SEL_GNOME) &&
		!flagIsSet(O_SEL_TROLL) &&
		!flagIsSet(O_SEL_HALFORC) &&
		!flagIsSet(O_SEL_OGRE) &&
		!flagIsSet(O_SEL_DARKELF) &&
		!flagIsSet(O_SEL_GOBLIN) &&
		!flagIsSet(O_SEL_MINOTAUR) &&
		!flagIsSet(O_SEL_SERAPH) &&
		!flagIsSet(O_SEL_KOBOLD) &&
		!flagIsSet(O_SEL_CAMBION) &&
		!flagIsSet(O_SEL_BARBARIAN) &&
		!flagIsSet(O_SEL_KATARAN) &&
		!flagIsSet(O_SEL_TIEFLING) &&
		!flagIsSet(O_RSEL_INVERT)
	)
		return(false);

	// if the race flag is set and they match, they pass
	pass = (
		(flagIsSet(O_SEL_DWARF) && creature->isRace(DWARF)) ||
		(flagIsSet(O_SEL_ELF) && creature->isRace(ELF)) ||
		(flagIsSet(O_SEL_HALFELF) && creature->isRace(HALFELF)) ||
		(flagIsSet(O_SEL_HALFLING) && creature->isRace(HALFLING)) ||
		(flagIsSet(O_SEL_HUMAN) && creature->isRace(HUMAN)) ||
		(flagIsSet(O_SEL_ORC) && creature->isRace(ORC)) ||
		(flagIsSet(O_SEL_HALFGIANT) && creature->isRace(HALFGIANT)) ||
		(flagIsSet(O_SEL_GNOME) && creature->isRace(GNOME)) ||
		(flagIsSet(O_SEL_TROLL) && creature->isRace(TROLL)) ||
		(flagIsSet(O_SEL_HALFORC) && creature->isRace(HALFORC)) ||
		(flagIsSet(O_SEL_OGRE) && creature->isRace(OGRE)) ||
		(flagIsSet(O_SEL_DARKELF) && creature->isRace(DARKELF)) ||
		(flagIsSet(O_SEL_GOBLIN) && creature->isRace(GOBLIN)) ||
		(flagIsSet(O_SEL_MINOTAUR) && creature->isRace(MINOTAUR)) ||
		(flagIsSet(O_SEL_SERAPH) && creature->isRace(SERAPH)) ||
		(flagIsSet(O_SEL_KOBOLD) && creature->isRace(KOBOLD)) ||
		(flagIsSet(O_SEL_CAMBION) && creature->isRace(CAMBION)) ||
		(flagIsSet(O_SEL_BARBARIAN) && creature->isRace(BARBARIAN)) ||
		(flagIsSet(O_SEL_KATARAN) && creature->isRace(KATARAN)) ||
		(flagIsSet(O_SEL_TIEFLING) && creature->isRace(TIEFLING))
	);

	if(flagIsSet(O_RSEL_INVERT)) pass = !pass;

	return(!pass);
}

//*********************************************************************
//							classRestrict
//*********************************************************************
// We need more information here - we can't just pass/fail them like
// we usually do.

bool Object::classRestrict(const Creature* creature, bool p) const {
	if(classRestrict(creature)) {
		if(p) creature->checkStaff("Your class prevents you from using %P.\n", this);
		if(!creature->isStaff()) return(true);
	}
	return(false);
}

bool Object::classRestrict(const Creature* creature) const {
	bool pass = false;
	const Player* player = creature->getAsConstPlayer();

	int cClass = creature->getClass();
	if(player && player->getClass() == MAGE && (player->getSecondClass() == ASSASSIN || player->getSecondClass() == THIEF))
		cClass = player->getSecondClass();

	if(flagIsSet(O_NO_MAGE) && (cClass == MAGE || cClass == LICH))
		return(true);

	// if no flags are set
	if(	!flagIsSet(O_SEL_ASSASSIN) &&
		!flagIsSet(O_SEL_BERSERKER) &&
		!flagIsSet(O_SEL_CLERIC) &&
		!flagIsSet(O_SEL_FIGHTER) &&
		!flagIsSet(O_SEL_MAGE) &&
		!flagIsSet(O_SEL_PALADIN) &&
		!flagIsSet(O_SEL_RANGER) &&
		!flagIsSet(O_SEL_THIEF) &&
		!flagIsSet(O_SEL_VAMPIRE) &&
		!flagIsSet(O_SEL_MONK) &&
		!flagIsSet(O_SEL_DEATHKNIGHT) &&
		!flagIsSet(O_SEL_DRUID) &&
		!flagIsSet(O_SEL_LICH) &&
		!flagIsSet(O_SEL_WEREWOLF) &&
		!flagIsSet(O_SEL_BARD) &&
		!flagIsSet(O_SEL_ROGUE) &&
		!flagIsSet(O_CSEL_INVERT)
	) {
		// we need to do some special rules before we say they can use the item

		// only blunts for monks
		if(wearflag == WIELD && cClass == MONK && getWeaponCategory() != "crushing")
			return(true);

		// only sharps for wolves
		if(wearflag == WIELD && creature->isEffected("lycanthropy") && getWeaponCategory() != "slashing")
			return(true);

		if(type == ARMOR && (cClass == MONK || creature->isEffected("lycanthropy")))
			return(true);

		// no rings or shields for monk/wolf/lich
		if(	(	wearflag == FINGER ||
				wearflag == SHIELD
			) &&
			(	cClass == MONK ||
				creature->isEffected("lycanthropy") ||
				cClass == LICH
			)
		)
			return(true);

		return(false);
	}

	// if the class flag is set and they match, they pass
	pass = (
		(flagIsSet(O_SEL_ASSASSIN) && cClass == ASSASSIN) ||
		(flagIsSet(O_SEL_BERSERKER) && cClass == BERSERKER) ||
		(flagIsSet(O_SEL_CLERIC) && cClass == CLERIC) ||
		(flagIsSet(O_SEL_FIGHTER) && cClass == FIGHTER) ||
		(flagIsSet(O_SEL_MAGE) && cClass == MAGE) ||
		(flagIsSet(O_SEL_PALADIN) && cClass == PALADIN) ||
		(flagIsSet(O_SEL_RANGER) && cClass == RANGER) ||
		(flagIsSet(O_SEL_THIEF) && cClass == THIEF) ||
		(flagIsSet(O_SEL_VAMPIRE) && creature->isEffected("vampirism")) ||
		(flagIsSet(O_SEL_MONK) && cClass == MONK) ||
		(flagIsSet(O_SEL_DEATHKNIGHT) && cClass == DEATHKNIGHT) ||
		(flagIsSet(O_SEL_DRUID) && cClass == DRUID) ||
		(flagIsSet(O_SEL_LICH) && cClass == LICH) ||
		(flagIsSet(O_SEL_WEREWOLF) && creature->isEffected("lycanthropy")) ||
		(flagIsSet(O_SEL_BARD) && cClass == BARD) ||
		(flagIsSet(O_SEL_ROGUE) && cClass == ROGUE)
	);

	if(flagIsSet(O_CSEL_INVERT)) pass = !pass;

	return(!pass);
}


//*********************************************************************
//							levelRestrict
//*********************************************************************

bool Object::levelRestrict(const Creature* creature, bool p) const {
	if(level > creature->getLevel()) {
		if(p) creature->checkStaff("You are not experienced enough to use that.\n");
		if(!creature->isStaff()) return(true);
	}
	return(false);
}

bool Object::skillRestrict(const Creature* creature, bool p) const {
	int skillLevel = 0;

	bstring skill = "";

	if(type == ARMOR) {
		skill = getArmorType();
		if(skill == "shield" || skill == "ring")
			skill = "";
	} else if(type == WEAPON) {
		skill = getWeaponType();
	} else if(flagIsSet(O_FISHING)) {
		skill = "fishing";
	}
	if(skill != "") {
		skillLevel = (int)creature->getSkillGained(skill);
		if(requiredSkill > skillLevel) {
			if(p) creature->checkStaff("You do not have enough training in ^W%s%s^x to use that!\n", skill.c_str(), type == ARMOR ? " armor" : "");
			if(!creature->isStaff()) return(true);
		}
	}

	return(false);
}

//*********************************************************************
//							levelRestrict
//*********************************************************************

bool Object::alignRestrict(const Creature* creature, bool p) const {
	if(	(flagIsSet(O_GOOD_ALIGN_ONLY) && creature->getAdjustedAlignment() < NEUTRAL) ||
		(flagIsSet(O_EVIL_ALIGN_ONLY) && creature->getAdjustedAlignment() > NEUTRAL) )
	{
		if(p) {
			creature->checkStaff("%O shocks you and you drop it.\n", this);
			if(!creature->isStaff())
				broadcast(creature->getSock(), creature->getConstRoomParent(), "%M is shocked by %P.", creature, this);
		}
		if(!creature->isStaff()) return(true);
	}
	return(false);
}



//*********************************************************************
//							sexRestrict
//*********************************************************************

bool Object::sexRestrict(const Creature* creature, bool p) const {
	Sex sex = creature->getSex();
	if(flagIsSet(O_MALE_ONLY) && sex != SEX_MALE) {
		if(p) creature->checkStaff("Only males can use that.\n");
		if(!creature->isStaff()) return(true);
	}
	if(flagIsSet(O_FEMALE_ONLY) && sex != SEX_FEMALE) {
		if(p) creature->checkStaff("Only females can use that.\n");
		if(!creature->isStaff()) return(true);
	}
	if(flagIsSet(O_SEXLESS_ONLY) && sex != SEX_NONE) {
		if(p) creature->checkStaff("Only creatures without a gender can use that.\n");
		if(!creature->isStaff()) return(true);
	}
	return(false);
}


//*********************************************************************
//							strRestrict
//*********************************************************************

bool Object::strRestrict(Creature* creature, bool p) const {
	if(minStrength > creature->strength.getCur()) {
		if(!p) creature->checkStaff("You are currently not strong enough to use that.\n");
		if(!creature->isStaff()) return(true);
	}
	return(false);
}


//*********************************************************************
//							clanRestrict
//*********************************************************************

bool Object::clanRestrict(const Creature* creature, bool p) const {
	if(clanRestrict(creature)) {
		if(p) creature->checkStaff("Your allegience prevents you from using %P.\n", this);
		if(!creature->isStaff()) return(true);
	}
	return(false);
}

bool Object::clanRestrict(const Creature* creature) const {
	int c = creature->getClan();
	if(creature->getDeity()) {
		const Clan* clan = gConfig->getClanByDeity(creature->getDeity());
		if(clan && clan->getId() != 0)
			c = clan->getId();
	}

	// if the object requires pledging, let any clan pass
	if(flagIsSet(O_PLEDGED_ONLY))
		return(!c);

	// if no flags are set
	if(	!flagIsSet(O_CLAN_1) &&
		!flagIsSet(O_CLAN_2) &&
		!flagIsSet(O_CLAN_3) &&
		!flagIsSet(O_CLAN_4) &&
		!flagIsSet(O_CLAN_5) &&
		!flagIsSet(O_CLAN_6) &&
		!flagIsSet(O_CLAN_7) &&
		!flagIsSet(O_CLAN_8) &&
		!flagIsSet(O_CLAN_9) &&
		!flagIsSet(O_CLAN_10) &&
		!flagIsSet(O_CLAN_11) &&
		!flagIsSet(O_CLAN_12) )
		return(false);

	// if the clan flag is set and they match, they pass
	if(	(flagIsSet(O_CLAN_1) && c == 1) ||
		(flagIsSet(O_CLAN_2) && c == 2) ||
		(flagIsSet(O_CLAN_3) && c == 3) ||
		(flagIsSet(O_CLAN_4) && c == 4) ||
		(flagIsSet(O_CLAN_5) && c == 5) ||
		(flagIsSet(O_CLAN_6) && c == 6) ||
		(flagIsSet(O_CLAN_7) && c == 7) ||
		(flagIsSet(O_CLAN_8) && c == 8) ||
		(flagIsSet(O_CLAN_9) && c == 9) ||
		(flagIsSet(O_CLAN_10) && c == 10) ||
		(flagIsSet(O_CLAN_11) && c == 11) ||
		(flagIsSet(O_CLAN_12) && c == 12) )
		return(false);

	return(true);
}


//*********************************************************************
//							lawchaoRestrict
//*********************************************************************

bool Object::lawchaoRestrict(const Creature* creature, bool p) const {
	if(	(flagIsSet(O_CHAOTIC_ONLY) && !creature->flagIsSet(P_CHAOTIC)) ||
		(flagIsSet(O_LAWFUL_ONLY) && creature->flagIsSet(P_CHAOTIC)) )
	{
		if(p) creature->checkStaff("You are unable to use %P.\n", this);
		if(!creature->isStaff()) return(true);
	}
	return(false);
}


//*********************************************************************
//						doRestrict
//*********************************************************************

bool Object::doRestrict(Creature* creature, bool p) {
	if(clanRestrict(creature, p))
		return(true);
	if(levelRestrict(creature, p))
		return(true);
	if(skillRestrict(creature, p))
		return(true);
	if(strRestrict(creature, p))
		return(true);
	if(classRestrict(creature, p))
		return(true);
	if(raceRestrict(creature, p))
		return(true);
	if(sexRestrict(creature, p))
		return(true);
	if(clanRestrict(creature, p))
		return(true);
	if(lawchaoRestrict(creature, p))
		return(true);
	if(alignRestrict(creature, p)) {
		if(p && !creature->isStaff()) {
			creature->delObj(this, false, true);
			addToRoom(creature->getRoomParent());
		}
		return(true);
	}
	return(false);
}

//*********************************************************************
//							getCompass
//*********************************************************************

bstring Object::getCompass(const Creature* creature, bool useName) {
	std::ostringstream oStr;

	if(useName)
		oStr << getObjStr(creature, CAP, 1);
	else
		oStr <<  "It";
	oStr <<  " ";

	if(!compass) {
		oStr << "appears to be broken.\n";
		return(oStr.str());
	}

	const MapMarker *mapmarker=0;
	if(creature->inAreaRoom())
		mapmarker = &creature->getConstAreaRoomParent()->mapmarker;

	if(	!creature->inAreaRoom() ||
		!mapmarker->getArea() ||
		!compass->getArea() ||
		mapmarker->getArea() != compass->getArea() ||
		*mapmarker == *compass
	) {
		oStr << "is currently spinning in circles.\n";
		return(oStr.str());
	}

	oStr << "points " << mapmarker->direction(compass) << ". The target is "
		 << mapmarker->distance(compass) << ".\n";
	return(oStr.str());
}


//*********************************************************************
//						getObjStr
//*********************************************************************

bstring Object::getObjStr(const Creature* viewer, int flags, int num) const {
	std::ostringstream objStr;
	bstring toReturn = "";
	char ch;

	if(!this)
		return("(NULL OBJ)");

	if(flagIsSet(O_DARKNESS))
		objStr << "^D";

	if(viewer != NULL)
		flags |= viewer->displayFlags();
	bool irrPlural = false;


	// use new plural code - on object
	if(num > 1 && plural != "") {
		objStr << int_to_text(num) << " "  << plural;
		irrPlural = true;
	}

	if(!irrPlural) {
		// Either not an irregular plural, or we couldn't find a match in the irregular plural file
		if(num == 0) {
			if(!flagIsSet(O_NO_PREFIX)) {
				objStr << "the " << getName();
			} else
				objStr << getName();
		}
		else if(num == 1) {
			if(flagIsSet(O_NO_PREFIX) || (info.id == 0 && !strcmp(key[0], "gold") && type == MONEY))
				objStr <<  "";
			else if(flagIsSet(O_SOME_PREFIX))
				objStr << "some ";
			else {
				// handle articles even when the item starts with a color
				int pos=0;
				while(getName()[pos] == '^') pos += 2;
				ch = low(getName()[pos]);

				if(ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o' || ch == 'u')
					objStr << "an ";
				else
					objStr << "a ";
			}
			objStr << getName();
		}
		else {
			char tempStr[2056];
			strcpy( tempStr, int_to_text(num) );
			strcat(tempStr, " ");

			if(flagIsSet(O_SOME_PREFIX))
				strcat(tempStr, "sets of ");

			strcat(tempStr, getCName());
			if(!flagIsSet(O_SOME_PREFIX)) {
				tempStr[strlen(tempStr)+1] = 0;
				tempStr[strlen(tempStr)+2] = 0;
				if(tempStr[strlen(tempStr)-1] == 's' ||
						tempStr[strlen(tempStr)-1] == 'x') {
					tempStr[strlen(tempStr)] = 'e';
					tempStr[strlen(tempStr)] = 's';
				} else
					tempStr[strlen(tempStr)] = 's';
			}
			objStr << tempStr;
		}
	}


	if(flagIsSet(O_NULL_MAGIC) && ((flags & ISDM) || (flags & ISCT))) {
		objStr << " (+" << adjustment << ")(n)";
	} else if((flags & MAG) && adjustment && !flagIsSet(O_NULL_MAGIC)) {
		objStr << " (";
		if(adjustment >= 0)
			objStr << "+";
		objStr << adjustment << ")";
	} else if((flags & MAG) && (magicpower || flagIsSet(O_MAGIC)) && !flagIsSet(O_NULL_MAGIC))
		objStr << " (M)";


	if(flags & ISDM) {
		if(flagIsSet(O_HIDDEN))
			objStr <<  " (h)";
		if(isEffected("invisibility"))
			objStr << " (*)";
		if(flagIsSet(O_SCENERY))
			objStr << " (s)";
		if(flagIsSet(O_NOT_PEEKABLE))
			objStr << "(NoPeek)";
		if(flagIsSet(O_NO_STEAL))
			objStr << "(NoSteal)";
		if(flagIsSet(O_BODYPART))
			objStr << "(BodyPart)";

	}

	if(isBroken())
		objStr << " (broken)";
	objStr << "^x";

	toReturn = objStr.str();

	if(flags & CAP) {
		int pos = 0;
		// don't capitalize colors
		while(toReturn[pos] == '^') pos += 2;
		toReturn[pos] = up(toReturn[pos]);
	}
	
	return(toReturn);
}


//*********************************************************************
//						popBag
//*********************************************************************
// removes an object from the bag, usually when the bag is stolen or destroyed

void Object::popBag(Creature* creature, bool quest, bool drop, bool steal, bool bodypart, bool dissolve) {
	Object* object=0;

	ObjectSet::iterator it;
	for(it = objects.begin() ; it != objects.end() ; ) {
		object = (*it++);

		if(	(quest && object->questnum) ||
			(dissolve && object->flagIsSet(O_RESIST_DISOLVE)) ||
			(drop && object->flagIsSet(O_NO_DROP)) ||
			(steal && object->flagIsSet(O_NO_STEAL)) ||
			(bodypart && object->flagIsSet(O_BODYPART))
		) {
			this->delObj(object);
			creature->addObj(object);
		}
	}
}

//*********************************************************************
//						isKey
//*********************************************************************

bool Object::isKey(const UniqueRoom* room, const Exit* exit) const {
	// storage room exist must be a storage room key
	if(exit->flagIsSet(X_TO_STORAGE_ROOM))
		return(getName() == "storage room key");

	// key numbers must match
	if(getKey() != exit->getKey())
		return(false);

	if(!room)
		return(true);

	// For the key to work, it must be from the same area
	// ie: no using oceancrest keys in highport!
	bstring area = exit->getKeyArea();
	if(area == "")
		area = room->info.area;

	if(!info.isArea("") && !info.isArea(area))
		return(false);
	return(true);
}


//*********************************************************************
//						spawnObjects
//*********************************************************************
// room = the catref for the destination room

void spawnObjects(const bstring& room, const bstring& objects) {
	UniqueRoom *dest = 0;
	Object* object=0;
	CatRef	cr;

	getCatRef(room, &cr, 0);

	if(!loadRoom(cr, &dest))
		return;

	bstring obj = "";
	int i=0;
	dest->expelPlayers(true, false, false);

	// make sure any existing objects of this type are removed
	if(!dest->objects.empty()) {
		do {
			obj = getFullstrTextTrun(objects, i++);
			if(obj != "")
			{
				getCatRef(obj, &cr, 0);
				ObjectSet::iterator it;
				for( it = dest->objects.begin() ; it != dest->objects.end() ; ) {
					object = (*it++);

					if(object->info == cr) {
						object->deleteFromRoom();
						delete object;
					}
				}
			}
		} while(obj != "");
	}

	// add the objects
	i=0;

	do {
		obj = getFullstrTextTrun(objects, i++);
		if(obj != "")
		{
			getCatRef(obj, &cr, 0);

			if(loadObject(cr, &object)) {
				// no need to spawn darkmetal items in a sunlit room
				if(object->flagIsSet(O_DARKMETAL) && dest->isSunlight())
					delete object;
				else {
					object->addToRoom(dest);
					object->setDroppedBy(dest, "SpawnObjects");
				}
			}
		}
	} while(obj != "");
}


double Object::getDps() {
    short num = numAttacks;
    if(!numAttacks)
        num = 1;

    return(( (damage.average() + adjustment) * (1+num) / 2) / (getWeaponDelay()/10.0));
}
