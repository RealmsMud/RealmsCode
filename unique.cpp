/*
 * unique.cpp
 *	 Unique item routines.
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___ 
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *  
 * 	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include "mud.h"
#include "dm.h"
#include "commands.h"
#include "unique.h"
#include "property.h"

//*********************************************************************
//							UniqueObject
//*********************************************************************

UniqueObject::UniqueObject() {
	itemLimit = 0;
}


//*********************************************************************
//						load
//*********************************************************************

void UniqueObject::load(xmlNodePtr curNode) {
	xmlNodePtr childNode = curNode->children;

	while(childNode) {
		if(NODE_NAME(childNode, "Item")) item.load(childNode);
		else if(NODE_NAME(childNode, "ItemLimit")) xml::copyToNum(itemLimit, childNode);

		childNode = childNode->next;
	}
}

//*********************************************************************
//						save
//*********************************************************************

void UniqueObject::save(xmlNodePtr curNode) const {
	item.save(curNode, "Item", true);
	xml::saveNonZeroNum(curNode, "ItemLimit", itemLimit);
}

//*********************************************************************
//							UniqueOwner
//*********************************************************************

UniqueOwner::UniqueOwner() {
	time = 0;
	owner = "";
}

long UniqueOwner::getTime() const {
	return(time);
}

bool UniqueOwner::is(const Player* player, const CatRef cr) const {
	return((!player || owner == player->getName()) && item == cr);
}

bool UniqueOwner::is(const Player* player, const Object* object) const {
	if(!object)
		return(owner == player->getName());
	return(is(player, object->info));
}

void UniqueOwner::set(const Player* player, const Object* object) {
	time = object->getMade() ? object->getMade() : ::time(0);
	owner = player->getName();
	item = object->info;
}

void UniqueOwner::set(const Player* player) {
	owner = player->getName();
}



//*********************************************************************
//						load
//*********************************************************************

void UniqueOwner::load(xmlNodePtr curNode) {
	xmlNodePtr childNode = curNode->children;

	while(childNode) {
		if(NODE_NAME(childNode, "Time")) xml::copyToNum(time, childNode);
		else if(NODE_NAME(childNode, "Item")) item.load(childNode);
		else if(NODE_NAME(childNode, "Owner")) xml::copyToBString(owner, childNode);

		childNode = childNode->next;
	}
}

//*********************************************************************
//						save
//*********************************************************************

void UniqueOwner::save(xmlNodePtr curNode) const {
	xml::saveNonZeroNum(curNode, "Time", time);
	item.save(curNode, "Item", true);
	xml::saveNonNullString(curNode, "Owner", owner);
}

//*********************************************************************
//						show
//*********************************************************************

void UniqueOwner::show(const Player* player) {
	bstring t = ctime(&time);
	t.trim();
	player->printColor("      Time: ^c%s^x   Owner: ^c%s^x   Object: ^c%s\n",
		t.c_str(), owner.c_str(), item.str().c_str());
}

//*********************************************************************
//						doRemove
//*********************************************************************

void UniqueOwner::doRemove(Player* player, Object* parent, Object* object, bool online, bool destroy) {
	if(!destroy)
		object->clearFlag(O_UNIQUE);
	else {
		// don't run remove, since that deletes the iterator;
		// just broadcast
		Unique::broadcastDestruction(owner, object);
		if(player)
			player->delObj(object);
		else if(parent)
			parent->delObj(object);

		UniqueRoom* uRoom=0;
		if(object->inUniqueRoom())
		    uRoom = object->getUniqueRoomParent();
		object->deleteFromRoom();
		if(uRoom)
			uRoom->saveToFile(0);
		delete object;
	}


	if(player) {
		player->save(online);
		if(!online)
			free_crt(player);
	}
}

//*********************************************************************
//						removeUnique
//*********************************************************************

void UniqueOwner::removeUnique(bool destroy) {
	Player* player = gServer->findPlayer(owner.c_str());
	bool online=true;

	if(!player && loadPlayer(owner.c_str(), &player))
		online = false;

	if(player) {
		for(Object* obj : player->objects) {
			if(Unique::isUnique(obj) && obj->info == item) {
				doRemove(player, 0, obj, online, destroy);
				return;
			}
			for(Object *subObj : obj->objects) {
				if(Unique::isUnique(subObj) && subObj->info == item) {
					doRemove(player, obj, subObj, online, destroy);
					return;
				}
			}
		}

		for(int i=0; i<MAXWEAR; i++) {
			if(	player->ready[i]) {
				if(	Unique::isUnique(player->ready[i]) &&
					player->ready[i]->info == item)
				{
					doRemove(player, 0, player->ready[i], online, destroy);
					return;
				}
				for(Object* obj : player->ready[i]->objects) {
					if(Unique::isUnique(obj) && obj->info == item) {
						doRemove(player, player->ready[i], obj, online, destroy);
						return;
					}
				}
			}
		}

		if(!online)
			free_crt(player);
	}

	// for properties, we only have to worry about primary owners
	Property* p=0;
	std::list<Property*>::iterator it;
	std::list<Range>::iterator rt;
	UniqueRoom*	uRoom=0;
	CatRef	cr;

	for(it = gConfig->properties.begin() ; it != gConfig->properties.end() ; it++) {
		p = (*it);
		if(	!(p->getType() == PROP_SHOP || p->getType() == PROP_STORAGE) &&
			!p->isOwner(owner)
		)
			continue;

		for(rt = p->ranges.begin() ; rt != p->ranges.end() ; rt++) {
			cr = (*rt).low;
			for(; cr.id <= (*rt).high; cr.id++) {

				if(!loadRoom(cr, &uRoom))
					continue;
				for(Object* obj : uRoom->objects) {
					if(Unique::isUnique(obj) && obj->info == item) {
						doRemove(0, 0, obj, online, destroy);
						return;
					}
					for(Object* subObj : obj->objects) {
						if(Unique::isUnique(subObj) && subObj->info == item) {
							doRemove(0, obj, subObj, online, destroy);
							return;
						}
					}
				}
			}
		}
	}
}


//*********************************************************************
//							Unique
//*********************************************************************

Unique::Unique() {
	globalLimit = playerLimit = inGame = decay = max = 0;
}
Unique::~Unique() {
	owners.clear();
}

int Unique::getGlobalLimit() const {
	return(globalLimit);
}
int Unique::getPlayerLimit() const {
	return(playerLimit);
}
int Unique::getInGame() const {
	return(inGame);
}
int Unique::getDecay() const {
	return(decay);
}
int Unique::getMax() const {
	return(max);
}


void Unique::setGlobalLimit(int num) {
	globalLimit = num;
}
void Unique::setPlayerLimit(int num) {
	playerLimit = num;
}
void Unique::setDecay(int num) {
	decay = num;
}
void Unique::setMax(int num) {
	max = num;
}


//*********************************************************************
//							inObjectsList
//*********************************************************************

bool Unique::inObjectsList(const CatRef cr) const {
	std::list<UniqueObject>::const_iterator it;
	for(it = objects.begin() ; it != objects.end() ; it++) {
		if((*it).item == cr)
			return(true);
	}
	return(false);
}

//*********************************************************************
//							setObjectLimit
//*********************************************************************

bool Unique::setObjectLimit(const CatRef cr, int id) {
	std::list<UniqueObject>::iterator it;
	for(it = objects.begin() ; it != objects.end() ; it++) {
		if((*it).item == cr) {
			(*it).itemLimit = id;
			return(true);
		}
	}
	return(false);
}

//*********************************************************************
//							numInOwners
//*********************************************************************

int Unique::numInOwners(const CatRef cr) const {
	std::list<UniqueOwner>::const_iterator it;
	int i=0;

	for(it = owners.begin() ; it != owners.end() ; it++) {
		if((*it).is(0, cr))
			i++;
	}
	return(i);
}

//*********************************************************************
//							numObjects
//*********************************************************************

int Unique::numObjects(const Player* player) const {
	std::list<UniqueOwner>::const_iterator it;
	int i=0;

	for(it = owners.begin() ; it != owners.end() ; it++) {
		if((*it).is(player, 0))
			i++;
	}
	return(i);
}

//*********************************************************************
//							getUnique
//*********************************************************************

Unique* Config::getUnique(const Object* object) const {
	if(!Unique::isUnique(object) || !validObjId(object->info))
		return(0);
	std::list<Unique*>::const_iterator it;

	for(it = uniques.begin() ; it != uniques.end() ; it++) {
		if((*it)->inObjectsList(object->info))
			return(*it);
	}
	return(0);
}

Unique* Config::getUnique(int id) const {
	std::list<Unique*>::const_iterator it;
	int i = 1;

	for(it = uniques.begin() ; it != uniques.end() ; it++) {
		if(i == id)
			return(*it);
		i++;
	}

	return(0);
}


//*********************************************************************
//							addUnique
//*********************************************************************

void Config::addUnique(Unique* unique) {
	uniques.push_back(unique);
}

//*********************************************************************
//							clearLimited
//*********************************************************************

void Config::clearLimited() {
	Unique* unique=0;

	while(!uniques.empty()) {
		unique = uniques.front();
		delete unique;
		uniques.pop_front();
	}
	uniques.clear();

	Lore* l=0;

	while(!lore.empty()) {
		l = lore.front();
		delete l;
		lore.pop_front();
	}
	lore.clear();
}

//*********************************************************************
//							canLoad
//*********************************************************************

bool Unique::canLoad(const Object* object) {
	Unique* unique = gConfig->getUnique(object);
	if(!unique)
		return(true);

	return(	unique->getInGame() < unique->getGlobalLimit() &&
			unique->checkItemLimit(object->info)
	);
}


//*********************************************************************
//							checkItemLimit
//*********************************************************************

bool Unique::checkItemLimit(const CatRef item) const {

	// checking itemLimit is a little more complicated
	std::list<UniqueObject>::const_iterator ut;
	for(ut = objects.begin() ; ut != objects.end() ; ut++) {
		if((*ut).item == item) {

			int i = (*ut).itemLimit;
			if(!i)
				return(true);

			// there IS a limit; see how many are currently in game
			std::list<UniqueOwner>::const_iterator it;
			for(it = owners.begin() ; it != owners.end() ; it++) {
				if((*it).is(0, item)) {
					i--;
					if(!i)
						return(false);
				}
			}


		}
	}
	return(true);
}

//*********************************************************************
//							canGet
//*********************************************************************

bool Unique::canGet(const Player* player, const CatRef item, bool transfer) const {
	if(player->isStaff())
		return(true);
	// if we're trying to transfer the item to this player,
	// we need inGame <= globalLimit, not inGame < globalLimit 
	if((inGame-transfer) >= globalLimit)
		return(false);
	if(playerLimit && numObjects(player) >= playerLimit)
		return(false);

	//  no need to check itemlimit if transfering
	if(!transfer && !checkItemLimit(item))
		return(false);

	return(true);
}

bool Unique::canGet(const Player* player, const Object* object, bool transfer) {
	Unique* unique = gConfig->getUnique(object);
	if(!player || (unique && !unique->canGet(player, object->info, transfer)))
		return(false);

	for(Object* obj : object->objects) {
		if(!canGet(player, obj, transfer))
			return(false);
	}
	return(true);
}

//*********************************************************************
//							is
//*********************************************************************

bool Unique::is(const Object* object) {
	return(isUnique(object) || hasUnique(object));
}

//*********************************************************************
//							isUnique
//*********************************************************************

bool Unique::isUnique(const Object* object) {
	return(object->getType() != MONEY && object->flagIsSet(O_UNIQUE));
}

//*********************************************************************
//							hasUnique
//*********************************************************************

bool Unique::hasUnique(const Object* object) {
	for(Object* obj : object->objects) {
		if(isUnique(obj))
			return(true);
	}
	return(false);
}

//*********************************************************************
//							remove
//*********************************************************************

bool Limited::remove(Player* player, const Object* object, bool save) {
	if(!player)
		return(false);

	bool	del = Lore::remove(player, object, true);

	// unique object
	Unique* unique = gConfig->getUnique(object);
	if(unique) {
		if(!player->isStaff())
			Unique::broadcastDestruction(player->getName(), object);

		del = deleteOwner(player, object, save, false) || del;
	}

	return(del);
}

//*********************************************************************
//							is
//*********************************************************************

bool Limited::is(const Object* object) {
	return(isLimited(object) || hasLimited(object));
}

//*********************************************************************
//							isLimited
//*********************************************************************

bool Limited::isLimited(const Object* object) {
	return(Unique::isUnique(object) || Lore::isLore(object));
}

//*********************************************************************
//							hasLimited
//*********************************************************************

bool Limited::hasLimited(const Object* object) {
	for(Object* obj : object->objects) {
		if(Limited::isLimited(obj))
			return(true);
	}
	return(false);
}

//*********************************************************************
//						deleteUniques
//*********************************************************************

void Config::deleteUniques(Player* player) {
	Object* object=0;

	ObjectSet::iterator it;
	for( it = player->objects.begin() ; it != player->objects.end() ; ) {
		object = (*it++);

		if(Limited::remove(player, object, false)) {
			player->delObj(object, false, false, true, false);
			delete object;
		}
	}
	player->checkDarkness();

	// verification, for objects in shop and storage and such
	std::list<Unique*>::iterator uIt;
	for(uIt = gConfig->uniques.begin() ; uIt != uniques.end() ; uIt++)
		(*uIt)->remove(player);

	gConfig->saveLimited();
}


//*********************************************************************
//							addOwner
//*********************************************************************
// for uniques:
// 		0 = not added
// 		1 = added
// 		2 = added and killMortalObjects is run
// adding lore items won't return anything

int Limited::addOwner(Player* player, const Object* object) {
	int		added=0;
	Unique* unique=0;

	for(Object* obj : object->objects) {
		unique = gConfig->getUnique(obj);
		if(unique) {
			if(!added)
				added = 1;
			if(unique->addOwner(player, obj))
				added = 2;
		}

		Lore::add(player, obj, true);
	}

	unique = gConfig->getUnique(object);
	if(unique) {
		if(!added)
			added = 1;
		if(unique->addOwner(player, object))
			added = 2;
	}

	Lore::add(player, object);

	if(added)
		gConfig->saveLimited();

	return(added);
}

//*********************************************************************
//							addOwner
//*********************************************************************
// true if killMortalObjects is run

bool Unique::addOwner(const Player* player, const Object* object) {
	if(!player || player->isStaff())
		return(false);
	UniqueOwner uo;

	uo.set(player, object);

	inGame++;
	owners.push_back(uo);
	if(inGame >= globalLimit || !checkItemLimit(object->info)) {
		gConfig->killMortalObjects();
		return(true);
	}
	return(false);
}


//*********************************************************************
//						deleteOwner
//*********************************************************************

bool Limited::deleteOwner(Player* player, const Object* object, bool save, bool lore) {
	if(!player)
		return(false);

	bool	del=false;
	Unique* unique=0;

	Object* obj;
	ObjectSet::iterator it;
	for(it = object->objects.begin() ; it != object->objects.end() ; ) {
		obj = (*it++);
		unique = gConfig->getUnique(obj);
		if(unique)
			del = unique->deleteOwner(player, obj) || del;
		if(lore)
			del = Lore::remove(player, obj, true) || del;
	}

	unique = gConfig->getUnique(object);
	if(unique)
		del = unique->deleteOwner(player, object) || del;
	if(lore)
		del = Lore::remove(player, object) || del;

	if(del && save)
		gConfig->saveLimited();

	return(del);
}

//*********************************************************************
//						deleteOwner
//*********************************************************************

bool Unique::deleteOwner(const Player* player, const Object* object) {
	std::list<UniqueOwner>::iterator it;

	for(it = owners.begin() ; it != owners.end() ; it++) {
		if((*it).is(player, object)) {
			inGame--;
			owners.erase(it);
			return(true);
		}
	}
	return(false);
}


//*********************************************************************
//							remove
//*********************************************************************

void Unique::remove(const Player* player) {
	std::list<UniqueOwner>::iterator it;

	for(it = owners.begin() ; it != owners.end() ;) {
		if((*it).is(player, 0)) {
			inGame--;
			owners.erase(it);
			it = owners.begin();
			continue;
		}
		it++;
	}
}

//*********************************************************************
//						load
//*********************************************************************

void Unique::load(xmlNodePtr curNode) {
	xmlNodePtr subNode, childNode = curNode->children;

	while(childNode) {
		if(NODE_NAME(childNode, "GlobalLimit")) xml::copyToNum(globalLimit, childNode);
		else if(NODE_NAME(childNode, "PlayerLimit")) xml::copyToNum(playerLimit, childNode);
		else if(NODE_NAME(childNode, "InGame")) xml::copyToNum(inGame, childNode);
		else if(NODE_NAME(childNode, "Decay")) xml::copyToNum(decay, childNode);
		else if(NODE_NAME(childNode, "Max")) xml::copyToNum(max, childNode);
		else if(NODE_NAME(childNode, "Owners")) {
			subNode = childNode->children;
			while(subNode) {
				if(NODE_NAME(subNode, "Owner")) {
					UniqueOwner uo;
					uo.load(subNode);
					owners.push_back(uo);
				}
				subNode = subNode->next;
			}
		}
		else if(NODE_NAME(childNode, "Objects")) {
			subNode = childNode->children;
			while(subNode) {
				if(NODE_NAME(subNode, "Object")) {
					UniqueObject ub;
					ub.load(subNode);
					objects.push_back(ub);
				}
				subNode = subNode->next;
			}
		}

		childNode = childNode->next;
	}
}

//*********************************************************************
//						save
//*********************************************************************

void Unique::save(xmlNodePtr curNode) const {
	xml::saveNonZeroNum(curNode, "GlobalLimit", globalLimit);
	xml::saveNonZeroNum(curNode, "PlayerLimit", playerLimit);
	xml::saveNonZeroNum(curNode, "InGame", inGame);

	xml::saveNonZeroNum(curNode, "Decay", decay);
	xml::saveNonZeroNum(curNode, "Max", max);

	std::list<UniqueOwner>::const_iterator it;
	if(!owners.empty()) {
		xmlNodePtr childNode = xml::newStringChild(curNode, "Owners");
		for(it = owners.begin() ; it != owners.end() ; it++) {
			xmlNodePtr subNode = xml::newStringChild(childNode, "Owner");
			(*it).save(subNode);
		}
	}

	std::list<UniqueObject>::const_iterator ct;
	if(!objects.empty()) {
		xmlNodePtr childNode = xml::newStringChild(curNode, "Objects");
		for(ct = objects.begin() ; ct != objects.end() ; ct++) {
			xmlNodePtr subNode = xml::newStringChild(childNode, "Object");
			(*ct).save(subNode);
		}
	}
}

//*********************************************************************
//						loadLimited
//*********************************************************************

bool Config::loadLimited() {
	xmlDocPtr	xmlDoc;
	xmlNodePtr	cur;
	char		filename[80];

	// build an XML tree from a the file
	sprintf(filename, "%s/limited.xml", Path::PlayerData);
	clearLimited();

	xmlDoc = xml::loadFile(filename, "Limited");
	if(xmlDoc == NULL)
		return(false);

	cur = xmlDocGetRootElement(xmlDoc);

	// First level we expect a Unique
	cur = cur->children;
	while(cur && xmlIsBlankNode(cur))
		cur = cur->next;

	if(cur == 0) {
		xmlFreeDoc(xmlDoc);
		xmlCleanupParser();
		return(false);
	}

	Unique* unique;
	Lore* l;
	while(cur != NULL) {
		if(NODE_NAME(cur, "Unique")) {
			unique = new Unique;
			unique->load(cur);
			uniques.push_back(unique);
		} else if(NODE_NAME(cur, "Lore")) {
			l = new Lore;
			l->load(cur);
			lore.push_back(l);
		}
		cur = cur->next;
	}
	xmlFreeDoc(xmlDoc);
	xmlCleanupParser();
	return(true);
}


//*********************************************************************
//						saveLimited
//*********************************************************************

void Config::saveLimited() const {
	std::list<Unique*>::const_iterator it;
	std::list<Lore*>::const_iterator lt;
	xmlDocPtr	xmlDoc;
	xmlNodePtr		rootNode, curNode;
	char			filename[80];

	xmlDoc = xmlNewDoc(BAD_CAST "1.0");
	rootNode = xmlNewDocNode(xmlDoc, NULL, BAD_CAST "Limited", NULL);
	xmlDocSetRootElement(xmlDoc, rootNode);

	for(it = uniques.begin() ; it != uniques.end() ; it++) {
		curNode = xml::newStringChild(rootNode, "Unique");
		(*it)->save(curNode);
	}
	for(lt = lore.begin() ; lt != lore.end() ; lt++) {
		curNode = xml::newStringChild(rootNode, "Lore");
		(*lt)->save(curNode);
	}

	sprintf(filename, "%s/limited.xml", Path::PlayerData);
	xml::saveFile(filename, xmlDoc);
	xmlFreeDoc(xmlDoc);
}

//*********************************************************************
//						firstObject
//*********************************************************************

const CatRef Unique::firstObject() {
	if(!objects.size()) {
		CatRef cr;
		return(cr);
	}
	return((*objects.begin()).item);
}

//*********************************************************************
//						listLimited
//*********************************************************************

void Config::listLimited(const Player* player) {
	std::list<Unique*>::iterator it;
	std::list<Lore*>::iterator lt;
	Object* object=0;
	CatRef cr;
	int id = 1;

	player->printColor("^yUniques:\n");

	for(it = uniques.begin() ; it != uniques.end() ; it++) {
		player->printColor("   Id: ^e%-2d^x   InGame: ^%c%-2d^x  GlobalLimit: ^c%-2d^x  First Object: ",
			id, (*it)->getInGame() >= (*it)->getGlobalLimit() ? 'c' : 'Y',
			(*it)->getInGame(), (*it)->getGlobalLimit());

		cr = (*it)->firstObject();
		if(!cr.id)
			player->printColor("^cnone\n");
		else {
			if(loadObject(cr, &object)) {
				player->printColor("^c%s\n", object->getCName());
				delete object;
			} else
				player->printColor("^c%s\n", cr.str().c_str());
		}
		id++;
	}

	player->printColor("\n\n^yLore:\n");
	for(lt = lore.begin() ; lt != lore.end() ; lt++) {
		player->printColor("   Limit: ^c%-2d^x   Object: ^c%s\n",
			(*lt)->getLimit(), (*lt)->getInfo().str().c_str());
	}

	player->print("\n");
}

//*********************************************************************
//						show
//*********************************************************************

void Unique::show(const Player* player) {
	Object* object=0;
	CatRef cr;

	player->printColor("^yUnique Item Group:\n");
	player->printColor("   InGame: ^c%d\n", inGame);
	player->printColor("   GlobalLimit: ^c%d^x  PlayerLimit: ^c%d\n", globalLimit, playerLimit);
	player->printColor("   Decay: ^c%d^x        Max: ^c%d\n\n", decay, max);

	player->print("   Owners:\n");
	std::list<UniqueOwner>::iterator it;

	for(it = owners.begin() ; it != owners.end() ; it++) {
		(*it).show(player);
	}
	player->print("\n");

	player->print("   Objects:\n");
	std::list<UniqueObject>::const_iterator ct;

	for(ct = objects.begin() ; ct != objects.end() ; ct++) {
		player->printColor("      ^c%s", (*ct).item.str().c_str());

		if(loadObject((*ct).item, &object)) {
			player->printColor(", ^c%s", object->getCName());
			delete object;
		}

		if((*ct).itemLimit)
			player->printColor(", ItemLimit: ^c%d", (*ct).itemLimit);

		player->print("\n");
	}
	player->print("\n");
}

//*********************************************************************
//						addObject
//*********************************************************************

void Unique::addObject(CatRef cr) {
	UniqueObject ub;
	ub.item = cr;
	objects.push_back(ub);
}

//*********************************************************************
//						deleteUnique
//*********************************************************************

void Config::deleteUnique(Unique* unique) {
	std::list<Unique*>::iterator it;

	for(it = uniques.begin() ; it != uniques.end() ; it++) {
		if((*it) == unique) {
			delete unique;
			uniques.erase(it);
			return;
		}
	}
}

//*********************************************************************
//						deUnique
//*********************************************************************

void Unique::deUnique(CatRef cr) {
	std::list<UniqueObject>::iterator ct;

	for(ct = objects.begin() ; ct != objects.end() ; ct++) {
		if((*ct).item == cr) {
			objects.erase(ct);
			break;
		}
	}

	std::list<UniqueOwner>::iterator it;

	for(it = owners.begin() ; it != owners.end() ;) {
		if((*it).is(0, cr)) {
			(*it).removeUnique(false);
			inGame--;
			owners.erase(it);
			it = owners.begin();
			continue;
		}
		it++;
	}

	if(!objects.size())
		gConfig->deleteUnique(this);
}

//*********************************************************************
//						transferOwner
//*********************************************************************
// for uniques only, not lore

void Unique::transferOwner(const Player* owner, const Player* target, const Object* object) {
	std::list<UniqueOwner>::iterator it;

	for(it = owners.begin() ; it != owners.end() ; it++) {
		if((*it).is(owner, object)) {
			(*it).set(target);
			gConfig->saveLimited();
			return;
		}
	}
}

//*********************************************************************
//						transferOwner
//*********************************************************************

void Limited::transferOwner(Player* owner, Player* target, const Object* object) {
	if(target && target->isStaff()) {

		deleteOwner(owner, object);

	} else if(owner && owner->isStaff()) {

		addOwner(target, object);

	} else {

		if(Lore::isLore(object) || Lore::hasLore(object)) {
			Lore::remove(owner, object, true);
			Lore::add(target, object, true);
		}

		Unique* unique = gConfig->getUnique(object);
		if(unique && owner && target)
			unique->transferOwner(owner, target, object);
	}
}

//*********************************************************************
//						dmUnique
//*********************************************************************

int dmUnique(Player* player, cmd* cmnd) {
	Object* object=0;
	Unique* unique=0;
	int		id=0, len=0;

	if(cmnd->num < 2) {
		cmnd->num = 2;
		strcpy(cmnd->str[1], "uniques");
		dmHelp(player, cmnd);
		return(0);
	}

	// they want to look at information
	if(!strcmp(cmnd->str[1], "list")) {
		id = atoi(getFullstrText(cmnd->fullstr, 2).c_str());
		if(!id) {

			gConfig->listLimited(player);

		} else {

			unique = gConfig->getUnique(id);
			if(unique)
				unique->show(player);
			else
				player->printColor("No unique group with id ^y%d^x was found!\n", id);

		}
		return(0);
	}

	// setting some info on a unique group
	if(!strcmp(cmnd->str[1], "set")) {
		id = atoi(getFullstrText(cmnd->fullstr, 2).c_str());
		if(id)
			unique = gConfig->getUnique(id);
		if(!unique) {
			player->print("What unique group do you want to modify?\n");
			return(0);
		}

		len = strlen(cmnd->str[2]);
		if(len < 1) {
			player->print("Modify what field?\n");
			return(0);
		}


		bstring command = getFullstrText(cmnd->fullstr, 4);
		if(!strncmp(command.c_str(), "limit", 5)) {

			id = atoi(getFullstrText(cmnd->fullstr, 5).c_str());
			if(id < 0) {
				player->print("That is not a valid number.\n");
				return(0);
			}

			CatRef	cr;
			getCatRef(getFullstrText(cmnd->fullstr, 3), &cr, 0);

			if(!unique->setObjectLimit(cr, id)) {
				player->printColor("^c%s^x is not in that unique group.\n", cr.str().c_str());
				return(0);
			}

			player->printColor("ItemLimit set to ^c%d^x.\n", id);

		} else {
			id = atoi(command.c_str());
			if(id < 0) {
				player->print("That is not a valid number.\n");
				return(0);
			}
			if(!strncmp(cmnd->str[2], "globallimit", len)) {
				if(!id) {
					player->print("Global limit must be atleast 1.\n");
					return(0);
				}
				unique->setGlobalLimit(id);
				player->printColor("GlobalLimit set to ^c%d^x.\n", unique->getGlobalLimit());
			} else if(!strncmp(cmnd->str[2], "playerlimit", len)) {
				unique->setPlayerLimit(id);
				player->printColor("PlayerLimit set to ^c%d^x.\n", unique->getPlayerLimit());
			} else if(!strncmp(cmnd->str[2], "decay", len)) {
				if(!id) {
					player->print("Decay must be atleast 1.\n");
					return(0);
				}
				unique->setDecay(id);
				player->printColor("Decay set to ^c%d^x.\n", unique->getDecay());
			} else if(!strncmp(cmnd->str[2], "max", len)) {
				unique->setMax(id);
				player->printColor("Max set to ^c%d^x.\n", unique->getMax());
			} else {
				player->print("Command not understood!\n");
				return(0);
			}
		}
		gConfig->saveLimited();
		return(0);
	}

	object = player->findObject(player, cmnd, 1);
	if(!object) {
		player->print("You don't have that object.\n");
		return(0);
	}

	unique = gConfig->getUnique(object);
	len = strlen(cmnd->str[2]);

	// they want info about this object
	if(len < 1) {
		if(unique)
			unique->show(player);
		else
			player->print("That object is not unique.\n");
		return(0);
	}

	if(!object->info.id) {
		player->print("You must save that object before you can modify its unique status.\n");
		return(0);
	}

	// we want to do something with this object
	if(!strncmp(cmnd->str[2], "add", len)) {

		if(unique) {
			player->print("That object is already part of a unique group.\n");
			return(0);
		}
		id = atoi(getFullstrText(cmnd->fullstr, 3).c_str());
		unique = 0;
		if(id)
			unique = gConfig->getUnique(id);
		if(!unique) {
			player->print("What unique group do you want to add this object to?\n");
			return(0);
		}

		unique->addObject(object->info);
		object->setFlag(O_UNIQUE);
		player->printColor("^yThis object is now unique.\n");

	} else if(!strncmp(cmnd->str[2], "create", len)) {

		if(unique) {
			player->print("That object is already part of a unique group.\n");
			return(0);
		}

		unique = new Unique;
		unique->setGlobalLimit(1);
		unique->setDecay(12*30);
		unique->addObject(object->info);
		object->setFlag(O_UNIQUE);

		gConfig->addUnique(unique);
		player->printColor("^yThis object is now unique.\n");

	} else if(!strncmp(cmnd->str[2], "delete", len)) {

		if(!unique) {
			player->print("This object does not belong to a unique group.\n");
			return(0);
		}
		if(strcmp(cmnd->str[3], "confirm")) {
			player->printColor("^rAre you sure you want to remove this object's unique status?\n");
			player->print("All existing objects in-game will be reflagged. This cannot be undone.\n");
			player->print("Number of items in game: %d\n", unique->numInOwners(object->info));
			return(0);
		}

		unique->deUnique(object->info);
		object->clearFlag(O_UNIQUE);
		player->printColor("^yThis object is no longer unique.\n");

	} else {
		player->print("Command not understood.\n");
		return(0);
	}

	dmResaveObject(player, object, true);
	gConfig->saveLimited();
	return(0);
}

//*********************************************************************
//						Lore
//*********************************************************************

Lore::Lore() {
	limit = 0;
}

CatRef Lore::getInfo() const {
	return(info);
}
int Lore::getLimit() const {
	return(limit);
}
void Lore::setInfo(const CatRef cr) {
	info = cr;
}
void Lore::setLimit(int i) {
	limit = i;
}

//*********************************************************************
//						load
//*********************************************************************

void Lore::load(xmlNodePtr curNode) {
	xmlNodePtr childNode = curNode->children;

	while(childNode) {
		if(NODE_NAME(childNode, "Limit")) xml::copyToNum(limit, childNode);
		else if(NODE_NAME(childNode, "Info")) info.load(childNode);

		childNode = childNode->next;
	}
}

//*********************************************************************
//						save
//*********************************************************************

void Lore::save(xmlNodePtr curNode) const {
	xml::saveNonZeroNum(curNode, "Limit", limit);
	info.save(curNode, "Info", true);
}

//*********************************************************************
//						isLore
//*********************************************************************

bool Lore::isLore(const Object* object) {
	return(object->info.id && object->getType() != MONEY && object->flagIsSet(O_LORE));
}

//*********************************************************************
//							hasUnique
//*********************************************************************

bool Lore::hasLore(const Object* object) {
	for(Object* obj : object->objects ) {
		if(isLore(obj))
			return(true);
	}
	return(false);
}
//*********************************************************************
//						canHave
//*********************************************************************

bool Lore::canHave(const Player* player, const CatRef cr) {
	if(!validObjId(cr))
		return(true);
	int i=0, limit=1;

	// if we have a lore object in memory, we update the limit, otherwise
	// the default is 1
	Lore* lore = gConfig->getLore(cr);
	if(lore)
		limit = lore->getLimit();

	std::list<CatRef>::const_iterator it;

	for(it = player->lore.begin() ; it != player->lore.end() ; it++) {
		if((*it) == cr) {
			i++;
			if(i >= limit)
				return(false);
		}
	}
	return(true);
}

bool Lore::canHave(const Player* player, const Object* object, bool checkBagOnly) {
	if(player->isStaff())
		return(true);

	if(!checkBagOnly) {

		if(isLore(object) && !canHave(player, object->info))
			return(false);

	} else {
		for(Object* obj : object->objects) {
			if(isLore(obj) && !canHave(player, obj->info))
				return(false);
		}

	}
	return(true);
}

bool Lore::canHave(const Player* player, const Object* object) {
	return(canHave(player, object, true) && canHave(player, object, false));
}

//*********************************************************************
//						add
//*********************************************************************

void Lore::add(Player* player, const Object* object, bool subObjects) {
	if(!player)
		return;
	if(subObjects) {
		for(Object* obj : object->objects) {
			add(player, obj, true);
		}
	}
	if(isLore(object)) {
		player->lore.push_back(object->info);
		player->save(true);
	}
}

//*********************************************************************
//						remove
//*********************************************************************

bool Lore::remove(Player* player, const Object* object, bool subObjects) {
	if(!player)
		return(false);
	bool del=false;

	if(subObjects) {
		for(Object* obj : object->objects) {
			del = remove(player, obj, true) || del;
		}
	}

	if(isLore(object)) {
		std::list<CatRef>::iterator it;

		for(it = player->lore.begin() ; it != player->lore.end() ; it++) {
			if((*it) == object->info) {
				player->lore.erase(it);
				player->save(true);
				return(true);
			}
		}
	}
	return(del);
}


//*********************************************************************
//						reset
//*********************************************************************

void Lore::reset(Player* player, Creature* creature) {

	if(!creature) {
		player->lore.clear();
		creature = player;
	}
	for(Object* obj : creature->objects) {
		Lore::add(player, obj, true);
	}

	for(int i=0; i<MAXWEAR; i++) {
		if(creature->ready[i])
			Lore::add(player, creature->ready[i], true);
	}

	if(player != creature)
		return;

	// for properties, we only have to worry about primary owners
	Property* p=0;
	std::list<Property*>::iterator it;
	std::list<Range>::iterator rt;
	UniqueRoom*	uRoom=0;
	CatRef	cr;

	for(it = gConfig->properties.begin() ; it != gConfig->properties.end() ; it++) {
		p = (*it);
		if(	!(p->getType() == PROP_SHOP || p->getType() == PROP_STORAGE) ||
			!p->isOwner(player->getName())
		)
			continue;

		for(rt = p->ranges.begin() ; rt != p->ranges.end() ; rt++) {
			cr = (*rt).low;

			for(; cr.id <= (*rt).high; cr.id++) {

				if(!loadRoom(cr, &uRoom))
					continue;

				// don't include objects in main shop room
				if(p->getType() == PROP_SHOP && uRoom->flagIsSet(R_SHOP))
					continue;
				for(Object* obj : uRoom->objects) {
					// don't include objects on floor of storage room (chests only)
					if(p->getType() != PROP_STORAGE || obj->info.isArea("stor"))
						Lore::add(player, obj, true);
				}
			}

		}

	}
	for(Monster* pet : player->pets) {
		if(pet->isPet())
			reset(player, pet);
	}
}


//*********************************************************************
//						getLore
//*********************************************************************

Lore* Config::getLore(const CatRef cr) const {
	std::list<Lore*>::const_iterator it;

	for(it = lore.begin() ; it != lore.end() ; it++) {
		if((*it)->getInfo() == cr)
			return(*it);
	}

	return(0);
}

//*********************************************************************
//						getLore
//*********************************************************************

void Config::delLore(const CatRef cr) {
	std::list<Lore*>::iterator it;
	Lore*	l=0;

	for(it = lore.begin() ; it != lore.end() ;) {
		if((*it)->getInfo() == cr) {
			l = (*it);
			delete l;
			lore.erase(it);
			continue;
		}
		it++;
	}
}

//*********************************************************************
//						getLore
//*********************************************************************

void Config::addLore(const CatRef cr, int i) {
	Lore* l = new Lore;
	l->setInfo(cr);
	l->setLimit(i);
	lore.push_back(l);
}


//*********************************************************************
//						uniqueDecay
//*********************************************************************

void Config::uniqueDecay(Player* player) {
	std::list<Unique*>::iterator it;
	long t = time(0);

	for(it = uniques.begin() ; it != uniques.end() ; it++) {
		(*it)->runDecay(t, player);
	}

	saveLimited();
}

//*********************************************************************
//						runDecay
//*********************************************************************

void Unique::runDecay(long t, Player* player) {
	std::list<UniqueOwner>::iterator it;
	bool ran = false;

	// player is normally empty, but if set, we only need to delete
	// from that person

	// don't run the loop 2x here since chances are determined
	for(it = owners.begin() ; it != owners.end() ; it++) {
		if(!player || (*it).is(player, 0))
			ran = (*it).runDecay(t, decay, max) || ran;
	}

	// if something decayed, we should update
	if(ran) {
		for(it = owners.begin() ; it != owners.end() ; ) {
			// time = 0 means we should destroy it now
			if(!(*it).getTime()) {
				inGame--;
				owners.erase(it);
				it = owners.begin();
				continue;
			}
			it++;
		}
	}
}

//*********************************************************************
//						runDecay
//*********************************************************************

bool UniqueOwner::runDecay(long t, int decay, int max) {
	long chance=0;

	// decay is in game days, turn to reals seconds
	if(!max) {
		// a Boolean, so before = 0%, after = 100%
		chance = ((t - time) >= (decay * 48*60)) * 100;
	} else {
		chance = ((t - time) - (decay * 48*60)) * 100 / max;
	}

	if(mrand(1,100) > chance) 
		return(false);

	Player* player = gServer->findPlayer(owner.c_str());

	if(player && player->inCombat()) {
		// we don't want to be removed just yet
		player->setFlag(P_UNIQUE_TO_DECAY);
		time = 1;
	} else {
		// this signals we want to be deleted
		time = 0;
		removeUnique(true);
	}
	return(true);
}

void Unique::broadcastDestruction(const bstring owner, const Object* object) {
	Player* player = gServer->findPlayer(owner.c_str());
	if(player)
		player->printColor("^yThe %s^y vanishes!\n", object->getCName());
	broadcast("^y### Tragically, %s's unique item %s^y just broke!", owner.c_str(), object->getCName());
}
