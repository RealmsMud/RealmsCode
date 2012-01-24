/*
 * craft.cpp
 *	 Main file for craft-related functions
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
#include "craft.h"
#include "factions.h"
#include "commands.h"
#include "unique.h"
#include "math.h"

#include <sstream>
#include <iomanip>
#include <locale>

#define RECIPE_WIDTH	37

int numIngredients(Size size);


//*********************************************************************
//						findHot
//*********************************************************************

Object* findHot(const Player* player) {
	// we can cook from any hot object
	otag* op = player->getRoom()->first_obj;
	while(op) {
		if(Recipe::goodObject(player, op->obj) && op->obj->flagIsSet(O_HOT))
			return(op->obj);
		op = op->next_tag;
	}
	return(0);
}

//*********************************************************************
//						Recipe
//*********************************************************************

Recipe::Recipe() {
	id = minSkill = experience = 0;
	requireRecipe = sizable = false;
	skill = resultName = creator = "";
}

//*********************************************************************
//						save
//*********************************************************************

void Recipe::save(xmlNodePtr rootNode) const {
	xmlNodePtr curNode = xml::newStringChild(rootNode, "Recipe");
	xml::newNumProp(curNode, "Id", id);

	result.save(curNode, "Result", false);
	xml::saveNonNullString(curNode, "Skill", skill);
	xml::saveNonNullString(curNode, "Creator", creator);
	xml::saveNonZeroNum(curNode, "Experience", experience);
	xml::saveNonZeroNum(curNode, "RequireRecipe", requireRecipe);
	xml::saveNonZeroNum(curNode, "Sizable", sizable);

	saveList(curNode, "Ingredients", &ingredients);
	saveList(curNode, "Reusables", &reusables);
	saveList(curNode, "Equipment", &equipment);
}

//*********************************************************************
//						load
//*********************************************************************

void Recipe::load(xmlNodePtr curNode) {
	id = xml::getIntProp(curNode, "Id");
	curNode = curNode->children;
	while(curNode) {
		if(NODE_NAME(curNode, "Result")) result.load(curNode);
		else if(NODE_NAME(curNode, "MinSkill")) xml::copyToNum(minSkill, curNode);
		else if(NODE_NAME(curNode, "Experience")) xml::copyToNum(experience, curNode);
		else if(NODE_NAME(curNode, "Skill")) { xml::copyToBString(skill, curNode); }
		else if(NODE_NAME(curNode, "Creator")) { xml::copyToBString(creator, curNode); }
		else if(NODE_NAME(curNode, "Sizable")) xml::copyToBool(sizable, curNode);
		else if(NODE_NAME(curNode, "RequireRecipe")) xml::copyToBool(requireRecipe, curNode);
		else if(NODE_NAME(curNode, "Ingredients")) loadList(curNode->children, &ingredients);
		else if(NODE_NAME(curNode, "Reusables")) loadList(curNode->children, &reusables);
		else if(NODE_NAME(curNode, "Equipment")) loadList(curNode->children, &equipment);
		curNode = curNode->next;
	}
}

//*********************************************************************
//						saveList
//*********************************************************************

void Recipe::saveList(xmlNodePtr curNode, bstring name, const std::list<CatRef>* list) const {
	if(!list->size())
		return;
	xmlNodePtr childNode = xml::newStringChild(curNode, name.c_str());
	CatRef	cr;

	std::list<CatRef>::const_iterator it;
	for(it = list->begin(); it != list->end() ; it++) {
		cr = (*it);
		cr.save(childNode, "Item", false);
	}
}

//*********************************************************************
//						loadList
//*********************************************************************

void Recipe::loadList(xmlNodePtr curNode, std::list<CatRef>* list) {
	while(curNode) {
		if(NODE_NAME(curNode, "Item")) {
			CatRef cr;
			cr.load(curNode);
			list->push_back(cr);
		}
		curNode = curNode->next;
	}
}



int Recipe::getId() const { return(id); }
void Recipe::setId(int i) { id = i; }
int Recipe::getExperience() const { return(experience); }
void Recipe::setExperience(int exp) { experience = exp; }
bool Recipe::isSizable() const { return(sizable); }
void Recipe::setSizable(bool size) { sizable = size; }
CatRef Recipe::getResult() const { return(result); }
void Recipe::setResult(CatRef cr) {
	result = cr;
	resultName = "";
}
bstring Recipe::getResultName(bool appendCr) {
	Object* object=0;
	if(resultName == "" || resultName == "<unknown item>") {
		if(object || loadObject(result, &object)) {
			resultName = object->getObjStr(NULL, INV | MAG, 1);
			delete object;
		} else
			resultName = "<unknown item>";
	}
	std::ostringstream oStr;
	oStr << resultName;
	if(appendCr)
		oStr << " (" << result.rstr() << ")";
	return(oStr.str());
}
bstring Recipe::getSkill() const { return(skill); }
void Recipe::setSkill(bstring s) { skill = s; }
bstring Recipe::getCreator() const { return(creator); }
void Recipe::setCreator(bstring c) { creator = c; }
int Recipe::getMinSkill() const { return(minSkill); }
bool Recipe::requiresRecipe() const { return(requireRecipe); }
void Recipe::setRequiresRecipe(bool r) { requireRecipe = r; }

//**********************************************************************
//						isSkilled
//**********************************************************************

bool Recipe::isSkilled(const Player* player, Size recipeSize) const {
	if(skill == "")
		return(true);
	if(!player->knowsSkill(skill))
		return(false);
	int skillLvl = (int)player->getSkillGained(skill);
	// crafting recipes that are not your size is difficult
	if(isSizable()) {
		// invalid sizes cannot be crafted!
		if(recipeSize == NO_SIZE)
			return(false);
		if(recipeSize != player->getSize())
			skillLvl -= abs(player->getSize() - recipeSize) * 10;
	}
	return(skillLvl >= minSkill);
}

//**********************************************************************
//						isValid
//**********************************************************************

bool Recipe::isValid() const {
	return(result.id && !ingredients.empty());
}

//**********************************************************************
//						goodOject
//**********************************************************************
// id=0 means don't bother checking

bool Recipe::goodObject(const Player* player, const Object* object, const CatRef* cr) {
	return(	(!cr || *&object->info == *cr) &&
			player->canSee(object) &&
			(	(	object->getShotsCur() > 0 &&
					object->getShotsCur() == object->getShotsMax()
				) || (
					object->getType() == CONTAINER &&
					object->getShotsCur() == 0
			) )
	);
}

//**********************************************************************
//						check
//**********************************************************************

bool Recipe::check(const Player* player, const std::list<CatRef>* list, bstring type, int numIngredients) const {
	Object* object=0;
	otag*	op=0, *first = (type == "equipment" ? player->getRoom()->first_obj : player->first_obj);
	std::list<CatRef>::const_iterator it;
	int has=0;
	if(!isSizable())
		numIngredients = 1;

	for(it = list->begin(); it != list->end() ; it++) {
		op = first;
		while(op) {
			// do they have this ingredient?
			object = op->obj;
			has = 0;
			if(!object->flagIsSet(O_BEING_PREPARED) && Recipe::goodObject(player, object, &(*it))) {
				player->printColor("You prepare %1P.\n", object);
				if(type != "equipment")
					object->setFlag(O_BEING_PREPARED);

				has++;
				while(has < numIngredients && op->next_tag) {
					if(Recipe::goodObject(player, op->next_tag->obj, &(*it))) {
						if(!op->next_tag->obj->flagIsSet(O_BEING_PREPARED)) {
							player->printColor("You prepare %1P.\n", op->next_tag->obj);
							if(type != "equipment")
								op->next_tag->obj->setFlag(O_BEING_PREPARED);
						}

						has++;
						if(has == numIngredients)
							break;
					}
					op = op->next_tag;
				}
				if(has == numIngredients)
					break;
			}
			op = op->next_tag;
		}
		if(!op) {
			player->unprepareAllObjects();
			player->print("You lack the necessary %s listed in this recipe.\n", type.c_str());
			return(false);
		}
	}
	return(true);
}

//**********************************************************************
//						check
//**********************************************************************

bool Recipe::check(std::list<CatRef>* list, const std::list<CatRef>* require, int numIngredients) const {
	std::list<CatRef>::iterator lIt;
	std::list<CatRef>::const_iterator rIt;
	bool	found=false;
	int		has=0;

	// go through the ingredients
	for(rIt = require->begin(); rIt != require->end() ; rIt++) {
		has = 0;
		while(has < numIngredients) {
			found = false;
			for(lIt = list->begin(); lIt != list->end() ; lIt++) {
				// if the ingredient is found in the list, remove it and go on to the next
				if((*lIt) == (*rIt)) {
					list->erase(lIt);
					has++;
					found = true;
					break;
				}
			}
			if(!found)
				return(false);
		}
	}
	return(true);
}

//**********************************************************************
//						listIngredients
//**********************************************************************

bstring Recipe::listIngredients(const std::list<CatRef>* list) const {
	Object* object=0;
	std::list<CatRef>::const_iterator it;
	std::ostringstream oStr;
	int		num=1;
	CatRef lastObject;
	lastObject.id = -1;

	oStr.setf(std::ios::left, std::ios::adjustfield);
	for(it = list->begin(); it != list->end() ; it++) {

		if(lastObject.id == -1) {
			// first object: we offset by one, load and continue
			lastObject = (*it);
			continue;
		} else if(lastObject == (*it)) {
			// we have another of the same object, up the tally
			num++;
			continue;
		}

		oStr << "   |   " << std::setw(RECIPE_WIDTH);
		if(loadObject(lastObject, &object)) {
			oStr << object->getObjStr(NULL, INV | MAG, num);
			delete object;
		} else {
			oStr << "<unknown item>";
		}
		oStr << "|\n";

		lastObject = (*it);
		num = 1;
	}

	// we offset by one, so do the last one now
	oStr << "   |   " << std::setw(RECIPE_WIDTH);
	if(loadObject(lastObject, &object)) {
		oStr << object->getObjStr(NULL, INV | MAG, num);
		delete object;
	} else {
		oStr << "<unknown item>";
	}
	oStr << "|\n";

	return(oStr.str());
}


//**********************************************************************
//						display
//**********************************************************************
std::ostream& operator<<(std::ostream& out, Recipe& recipe) {
    out << recipe.display();
    return(out);
}

std::ostream& operator<<(std::ostream& out, Recipe* recipe) {
    if(recipe)
        out << *recipe;

    return(out);
}

bstring Recipe::display() {
	std::ostringstream oStr;

	if(!isValid()) {
		oStr << "Sorry, that recipe is faulty.\n";
		return oStr.str();
	}

	oStr.setf(std::ios::left, std::ios::adjustfield);
	oStr << "  _______________________________________\n"
		 << " /\\                                      \\\n";

	if(skill != "") {
		oStr << " \\_| ^WSkill Required:^x                      |\n"
			 << "   |   " << std::setw(35) << skill << "|\n"
			 << "   |                                      |\n"
			 << "   | ^WIngredients:^x                         |\n";
	} else {
		oStr << " \\_| ^WIngredients:^x                         |\n";
	}

	oStr << listIngredients(&ingredients);

	if(!reusables.empty()) {
		oStr << "   |                                      |\n"
			 << "   | ^WReusable Items:^x                      |\n"
			 << listIngredients(&reusables);
	}

	if(!equipment.empty() || skill == "cooking") {
		oStr << "   |                                      |\n"
			 << "   | ^WEquipment:^x                           |\n";
		if(!equipment.empty())
			oStr << listIngredients(&equipment);
		else if(skill == "cooking")
			oStr << "   |   any sufficiently hot object        |\n";
	}

	oStr << "   |                                      |\n"
		 << "   | ^WResult:^x                              |\n"
		 << "   |   " << std::setw(RECIPE_WIDTH) << getResultName() << "|\n";

	if(isSizable()) {
		oStr << "   |                                      |\n"
			 << "   | This recipe may be used to create    |\n"
			 << "   | objects of different sizes.          |\n";
	}

	oStr << "   |   ___________________________________|_\n"
		 << "    \\_/____________________________________/\n";

	return(oStr.str());
}


//**********************************************************************
//						canUseEquipment
//**********************************************************************

bool Recipe::canUseEquipment(const Player* player, bstring skill) const {
	if(!equipment.empty() || skill == "cooking") {
		ctag*	cp = player->getRoom()->first_mon;

		while(cp) {
			if(cp->crt->canSee(player)) {
				if(	cp->crt->getConstMonster()->isEnemy(player) ||
					!Faction::willDoBusinessWith(player, cp->crt->getMonster()->getPrimeFaction())
				) {
					player->print("%M won't let you use any equipment in this room.", cp->crt);
					return(false);
				}
			}
			cp = cp->next_tag;
		}
	}
	return(true);
}

//**********************************************************************
//						canBeEdittedBy
//**********************************************************************

bool Recipe::canBeEdittedBy(const Player* player) const {
	if(player->isDm())
		return(true);
	return(!strcmp(player->name, creator.c_str()));
}

//**********************************************************************
//						addRecipe
//**********************************************************************
// to be called when creating a brand new recipe, not when loading

void Config::addRecipe(Recipe* recipe) {
	std::map<int, Recipe*>::iterator rIt;
	int id = 0;

	for(rIt = recipes.begin(); rIt != recipes.end() ; rIt++) {
		id = MAX(id, (*rIt).first);
	}

	id++;
	recipe->setId(id);
	recipes[id] = recipe;
}

//**********************************************************************
//						remRecipe
//**********************************************************************

void Config::remRecipe(Recipe* recipe) {
	recipes.erase(recipe->getId());
}

//**********************************************************************
//						searchRecipes
//**********************************************************************
// items in inventory are prepared, this sees if they match any known recipe
// this function essentially lets people experiment and find new recipes

Recipe* Config::searchRecipes(const Player* player, bstring skill, Size recipeSize, int numIngredients, const Object* object) {
	std::list<CatRef> list, tList;
	std::list<CatRef>::const_iterator iIt;
	std::map<int, Recipe*>::iterator rIt;
	Object* hot=0;
	Recipe*	recipe=0;
	otag*	op = player->first_obj;
	int		flags = player->displayFlags();
	unsigned int num=0;
	bstring	str = "";
	// passing in object means we don't want to print failures
	bool print = !object;

	if(object) {
		if(Recipe::goodObject(player, object)) {
			player->printColor("You prepare %1P.\n", object);
			list.push_back(object->info);
			tList.push_back(object->info);
		}
	} else {
		// make our life easier and just make a list of objects
		while(op) {
			if(Recipe::goodObject(player, op->obj) && op->obj->flagIsSet(O_BEING_PREPARED)) {
				list.push_back(op->obj->info);
				tList.push_back(op->obj->info);
			}
			op = op->next_tag;
		}
	}

	if(list.empty()) {
		if(print)
			player->print("You do not have any items prepared.\n");
		return(0);
	}

	num = list.size();

	// go through the recipes list and see which recipes match ALL the items
	for(rIt = recipes.begin(); rIt != recipes.end() ; rIt++) {
		recipe = (*rIt).second;

		// if they need a recipe object, they can't use it from this function
		if(!recipe || recipe->requiresRecipe())
			continue;
		// not the right type of recipe, don't even look at it
		if(recipe->getSkill() != skill)
			continue;
		if(!recipe->isValid() || !recipe->isSkilled(player, recipeSize))
			continue;

		// not the right amount of ingredients, don't even look at it
		if(num != (recipe->ingredients.size() + recipe->reusables.size()))
			continue;

		// if we cleared the list, we have one more hurdle to clear
		if(	recipe->check(&list, &recipe->ingredients, recipe->isSizable() ? numIngredients : 1) &&
			recipe->check(&list, &recipe->reusables, 1) &&
			list.empty()
		) {
			// check equipment
			bool hasEquipment=true;
			str = "";

			if(skill == "cooking" && recipe->equipment.empty()) {
				// we can cook from any hot object
				hot = findHot(player);
				if(hot) {
					str += "You prepare ";
					str += hot->getObjStr(NULL, flags, 1);
					str += ".\n";
				} else
					hasEquipment = false;
			} else {
				for(iIt = recipe->equipment.begin(); iIt != recipe->equipment.end() ; iIt++) {
					op = player->getRoom()->first_obj;
					while(op) {
						if(Recipe::goodObject(player, op->obj, &(*iIt))) {
							str += "You prepare ";
							str += op->obj->getObjStr(NULL, flags, 1);
							str += ".\n";
							break;
						}
						op = op->next_tag;
					}
					if(!op) {
						hasEquipment = false;
						break;
					}
				}
			}

			// if they have all the items AND all the equipment, they can
			// probably use this recipe
			if(hasEquipment) {
				if(!recipe->canUseEquipment(player, skill))
					return(0);
				if(str != "")
					player->printColor("%s", str.c_str());
				return(recipe);
			}
		}

		// refill the original list
		list.clear();
		list.assign(tList.begin(), tList.end());
	}

	if(print)
		player->print("Your combination of prepared items didn't produce anything.\n");
	return(0);
}

//**********************************************************************
//						cmdRecipes
//**********************************************************************

int cmdRecipes(Player* player, cmd* cmnd) {
	Recipe* recipe=0;
	int		i=0, shown=0, truncate=0;
	std::list<int>::iterator it;
	std::ostringstream oStr;
	bstring filter = "";

	oStr.setf(std::ios::left, std::ios::adjustfield);

	// the parser doesn't understand "recipe 5" as two strings. this will force it to.
	if(cmnd->num == 1 && getFullstrText(cmnd->fullstr, 1) != "")
		cmnd->num = 2;

	if(cmnd->num > 1) {
		// findRecipe handles digit, #, and object, so we need to check for filter
		filter = getFullstrText(cmnd->fullstr, 1);
		if(filter == "<none>")
			filter = "none";
		if(	filter != "none" &&
			filter != "cooking" &&
			filter != "alchemy" &&
			filter != "tailoring" &&
			filter != "brewing" &&
			filter != "smithing"
		) {
			// it's not something we want to filter on
			bool ignore=false;
			recipe = player->findRecipe(cmnd, "", &ignore);
			if(recipe)
				oStr << recipe;
			player->printColor("%s\n", oStr.str().c_str());
			return(0);
		}
	}

	if(!player->recipes.empty())
		oStr << "Recipes you know: type ^yrecipes <skill>^x or ^yrecipes <num>^x.\n";
	oStr << "  __________________________________________________\n"
		 << " /\\                                                 \\\n";

	for(it = player->recipes.begin(); it != player->recipes.end() ; it++) {
		recipe = gConfig->getRecipe(*it);
		i++;

		if(	filter != "" &&
			!(	filter == recipe->getSkill() ||
				(filter == "none" && recipe->getSkill() == "")
			)
		)
			continue;

		shown++;

		if(truncate)
			continue;

		if(shown==1)
			oStr << " \\_| ";
		else
			oStr << "   | ";

		oStr << "^c#" << std::setw(3) << i << "^x "
			 << std::setw(31) << recipe->getResultName()
			 << std::setw(14) << (recipe->getSkill() == "" ? "none" : recipe->getSkill())
			 << "|\n";

		// don't spam them if they have too many recipes
		// if they filter, we must show them all
		if(shown > 80 && filter == "")
			truncate = shown;
	}

	if(truncate) {
		oStr.setf(std::ios::right, std::ios::adjustfield);
		truncate = shown - truncate;
		oStr << "   |                                                 |\n"
			 << "   |" << std::setw(16 + (truncate==1 ? 1 : 0)) << truncate << " recipe" << (truncate != 1 ? "s" : "") << " not shown.              |\n";
	}

	if(!shown) {
		oStr << " \\_|                                                 |\n";
		if(filter == "")
			oStr << "   |             You know no recipes.                |\n";
		else
			oStr << "   |         No recipes matched that filter.         |\n";
		shown += 2;
	}

	// the list looks funny when long but not tall
	while(shown < 6) {
		oStr << "   |                                                 |\n";
		shown++;
	}

	oStr << "   |   ______________________________________________|_\n"
		 << "    \\_/_______________________________________________/\n";

	player->printColor("%s\n", oStr.str().c_str());
	return(0);
}

//**********************************************************************
//						findRecipe
//**********************************************************************

Recipe* Player::findRecipe(cmd* cmnd, bstring skill, bool* searchRecipes, Size recipeSize, int numIngredients) const {
	std::list<int>::const_iterator it;
	bstring txt = getFullstrText(cmnd->fullstr, 1);

	if(txt.getAt(0) == '#' || isdigit(txt.getAt(0))) {
		int id=0, n=0;
		if(txt.getAt(0) == '#')
			txt.Delete(0, 1);
		id = atoi(txt.c_str());

		for(it = recipes.begin(); it != recipes.end() ; it++) {
			n++;
			if(id == n)
				return(gConfig->getRecipe(*it));
		}
		print("You don't know that recipe.\n");
	} else {
		Recipe* recipe = 0;
		Object* object = findObject(this, first_obj, cmnd);
		if(object) {
			// if the object they're using is a recipe
			if(object->getRecipe()) {
				recipe = gConfig->getRecipe(object->getRecipe());
				if(recipe)
					return(recipe);
			}
			// if the object they're using is the only ingredient in the recipe
			recipe = gConfig->searchRecipes(this, skill, recipeSize, numIngredients, object);
			// the calling function needs to know we ran searchRecipes
			(*searchRecipes)=true;
			if(recipe) {
				object->setFlag(O_BEING_PREPARED);
				return(recipe);
			}
			printColor("%O is not a recipe and is not the sole ingredient in a recipe.\n", object);
			printColor("Note that some recipes may require equipment in the room.\n");
		} else {
			print("Object not found.\n");
		}
		// match object based on string
		//for(it = recipes.begin(); it != recipes.end() ; it++) {
		//}
	}
	return(0);
}

//**********************************************************************
//						dmCombine
//**********************************************************************

int dmCombine(Player* player, cmd* cmnd) {
	Recipe *recipe=0;
	std::list<CatRef> objects;
	std::list<CatRef>* list;
	otag* op = player->first_obj;
	bstring txt = "";

	if(!player->canBuildObjects())
		return(cmdNoAuth(player));

	if(cmnd->num < 2) {
		player->print("Type *combine <list> [new | #]\n");
		player->printColor("   Acceptable list types are ^yingredients^x, ^yreusables^x, and ^yequipment^x.\n");
		player->printColor("   Don't forget to use ^y*recsave^x to save your work.\n");
		return(0);
	} else if(cmnd->num == 2) {
		recipe = gConfig->getRecipe(cmnd->val[1]);
		if(!recipe->canBeEdittedBy(player)) {
			player->print("Sorry, you are not allowed to edit that recipe.\n");
			return(0);
		}
	} else {
		recipe = new Recipe;
		recipe->setCreator(player->name);
		gConfig->addRecipe(recipe);
		player->printColor("\nNew recipe ^y#%d^x created.\n\n", recipe->getId());
	}

	if(!strncmp(cmnd->str[1], "ingredients", strlen(cmnd->str[1]))) {
		list = &recipe->ingredients;
		txt = "ingredients";
	} else if(!strncmp(cmnd->str[1], "reusables", strlen(cmnd->str[1]))) {
		list = &recipe->reusables;
		txt = "reusables";
	} else if(!strncmp(cmnd->str[1], "equipment", strlen(cmnd->str[1]))) {
		list = &recipe->equipment;
		txt = "equipment";
	} else {
		player->print("That's not a valid list type.\n");
		return(0);
	}

	while(op) {
		if(!op->obj->info.id)
			player->printColor("Skipping %P, no index found.\n", op->obj);
		else if(op->obj->flagIsSet(O_BEING_PREPARED)) {
			if(!Recipe::goodObject(player, op->obj))
				player->printColor("Skipping %P, it failed goodObject check.\n", op->obj);
			else
				objects.push_back(op->obj->info);
		}
		op = op->next_tag;
	}

	// supply the new list
	list->clear();
	list->assign(objects.begin(), objects.end());

	player->print("Recipe #%d %s %s.\n", recipe->getId(), txt.c_str(),
		list->size() ? "set" : "cleared");
	player->unprepareAllObjects();
	return(0);
}

//**********************************************************************
//						dmSetRecipe
//**********************************************************************

int dmSetRecipe(Player* player, cmd* cmnd) {
	Recipe* recipe = gConfig->getRecipe(cmnd->val[1]);
	bstring txt = "";

	if(!player->canBuildObjects())
		return(cmdNoAuth(player));

	if(!recipe->canBeEdittedBy(player)) {
		player->print("Sorry, you are not allowed to edit that recipe.\n");
		return(0);
	}

	switch(low(cmnd->str[2][0])) {
	case 'd':
		if(strcmp(cmnd->str[3], "confirm")) {
			player->printColor("Are you sure you want to delete recipe #%d?\nType ^y*set rec %d del confirm^x to delete.\n",
				recipe->getId(), recipe->getId());
			return(0);
		}

		player->print("Recipe #%d deleted.\n", recipe->getId());
		log_immort(true, player, "%s deleted recipe #%d.\n",
			player->name, recipe->getId());

		gConfig->remRecipe(recipe);
		delete recipe;

		break;
	case 'e':
		recipe->setExperience(cmnd->val[3]);

		player->print("Recipe #%d's Experience set to %d.\n", recipe->getId(), recipe->getExperience());
		log_immort(true, player, "%s set recipe #%d's %s to %d.\n",
			player->name, recipe->getId(), "Experience", recipe->getExperience());
		break;
	case 'r':
		if(low(cmnd->str[2][1] == 'e' && low(cmnd->str[2][2]) == 'q')) {
			recipe->setRequiresRecipe(cmnd->val[2]);

			player->print("Recipe #%d's RequireRecipe set to %s.\n", recipe->getId(), recipe->requiresRecipe() ? "true" : "false");
			log_immort(true, player, "%s set recipe #%d's %s to %s.\n",
				player->name, recipe->getId(), "RequireRecipe", recipe->requiresRecipe() ? "true" : "false");
		} else {
			if(cmnd->val[2] > 1 && cmnd->val[2] < 20000) {
				CatRef	cr;
				cr.id = cmnd->val[2];
				recipe->setResult(cr);
		} else {
				Object* object = findObject(player, player->first_obj, cmnd, 3);
				if(!object) {
					player->print("Set result to what object?\n");
					return(0);
				} else if(!object->info.id) {
					player->printColor("%O does not have an index set.\n", object);
					return(0);
				} else {
					recipe->setResult(object->info);
				}
			}
			player->print("Recipe #%d's Result set to %s.\n", recipe->getId(), recipe->getResult().str().c_str());
			log_immort(true, player, "%s set recipe #%d's %s to %s.\n",
				player->name, recipe->getId(), "Result", recipe->getResult().str().c_str());
		}
		break;
	case 's':
		if(cmnd->str[2][1] == 'i') {
		case 'i':
			recipe->setSizable(cmnd->val[2]);
			player->print("Recipe #%d's Sizable set to \"%s\".\n", recipe->getId(), recipe->isSizable() ? "true" : "false");
			log_immort(true, player, "%s set recipe #%d's %s to \"%s\".\n",
				player->name, recipe->getId(), "Sizable", recipe->isSizable() ? "true" : "false");
		} else {
			txt = cmnd->str[3];
			if(txt != "" && !gConfig->getSkill(txt)) {
				player->print("The skill \"%s\" doesn't exist.\n", txt.c_str());
				return(0);
			}
			recipe->setSkill(txt);

			player->print("Recipe #%d's Skill set to \"%s\".\n", recipe->getId(), recipe->getSkill().c_str());
			log_immort(true, player, "%s set recipe #%d's %s to \"%s\".\n",
				player->name, recipe->getId(), "Skill", recipe->getSkill().c_str());
		}
		break;
	default:
		player->print("Unknown setting for recipe.\nAcceptable parameters are:\n");
		player->printColor("    ^ydelete^x, ^yexperience^x, ^yrequire^x, ^yresult^x, ^yskill\n");
		player->printColor("    use ^y*combine^x to set ingredients, reusables, and equipment.\n");
	}
	return(0);
}

//**********************************************************************
//						dmRecipes
//**********************************************************************

int dmRecipes(Player* player, cmd* cmnd) {
	std::list<CatRef>::iterator lIt;
	Recipe* recipe=0;
	std::ostringstream oStr;
	bstring txt = getFullstrText(cmnd->fullstr, 1);

	if(!player->canBuildObjects())
		return(cmdNoAuth(player));

	oStr.setf(std::ios::left, std::ios::adjustfield);

	if(txt != "" && isdigit(txt.getAt(0))) {
		bool	i=false;

		recipe = gConfig->getRecipe(atoi(txt.c_str()));

		if(!recipe) {
			player->print("That is not a valid recipe!\n");
			return(0);
		}
		if(!recipe->canBeEdittedBy(player)) {
			player->print("You are not allowed to view that recipe.\n");
			return(0);
		}

		oStr << recipe;


		oStr << "Recipe: ^c" << recipe->getId() << "^w"
			 << "      Skill: ";
		if(recipe->getSkill() != "")
			oStr << "^c" << recipe->getSkill() << "^x";
		else
			oStr << "<none>";

		oStr << "    RequireRecipe: " << (recipe->requiresRecipe() ? "^gYes" : "^rNo") << "^x";

		if(!recipe->ingredients.empty()) {
			oStr << "\nIngredients: ";
			i = false;
			for(lIt = recipe->ingredients.begin(); lIt != recipe->ingredients.end() ; lIt++) {
				if(i)
					oStr << ", ";
				i = true;
				oStr << "^c" << (*lIt).rstr() << "^x";
			}
		}

		if(!recipe->reusables.empty()) {
			oStr << "\nReusables: ";
			i = false;
			for(lIt = recipe->reusables.begin(); lIt != recipe->reusables.end() ; lIt++) {
				if(i)
					oStr << ", ";
				i = true;
				oStr << "^c" << (*lIt).rstr() << "^x";
			}
		}

		if(!recipe->equipment.empty()) {
			oStr << "\nEquipment: ";
			i = false;
			for(lIt = recipe->equipment.begin(); lIt != recipe->equipment.end() ; lIt++) {
				if(i)
					oStr << ", ";
				i = true;
				oStr << "^c" << (*lIt).rstr() << "^x";
			}
		}

		oStr << "\nResult: ^c" << recipe->getResult().rstr() << "^x\n"
		 	 << "Creator: ^c" << recipe->getCreator().c_str() << "^x\n"
		 	 << "Experience: ^c" << recipe->getExperience() << "^x\n"
		 	 << "MinSkill: ^c" << recipe->getMinSkill() << "^x\n"
		 	 << "Sizable: " << (recipe->isSizable() ? "^gYes" : "^rNo") << "^x\n";

	} else {

		std::map<int, Recipe*>::iterator it;
		if(cmnd->num > 1)
			txt = getFullstrText(cmnd->fullstr, 1);

		oStr << "^yListing Recipes; type *recipe <num> for more detailed information.^x\n";

		if(txt == "")
			 oStr << "^y                 or *recipe <skill> to filter by skill^x\n";
		else
			 oStr << "^y        Filtering on skill: " << txt << "^x\n";

		for(it = gConfig->recipes.begin(); it != gConfig->recipes.end() ; it++) {
			recipe = (*it).second;

			if(	txt != "" &&
				!(	txt == recipe->getSkill() ||
					(txt == "none" && recipe->getSkill() == "")
				)
			)
				continue;
			if(!recipe->canBeEdittedBy(player))
				continue;

			oStr << "   Recipe: ^c" << std::setw(4) << recipe->getId() << "^w"
				 << " Result: ^c" << std::setw(50) << recipe->getResultName(true) << "^xSkill: ";
			if(recipe->getSkill() != "")
				oStr << "^c" << recipe->getSkill() << "^x";
			else
				oStr << "<none>";
			oStr << "\n";
		}

	}
	player->printColor("%s\n", oStr.str().c_str());
	return(0);
}


//**********************************************************************
//						learnRecipe
//**********************************************************************

void Player::learnRecipe(int id) {
	if(knowsRecipe(id))
		return;
	recipes.push_back(id);
}
void Player::learnRecipe(Recipe* recipe) {
	learnRecipe(recipe->getId());
}


//**********************************************************************
//						knowsRecipe
//**********************************************************************

bool Player::knowsRecipe(int id) const {
	std::list<int>::const_iterator it;
	for(it = recipes.begin(); it != recipes.end() ; it++) {
		if((*it) == id)
			return(true);
	}
	return(false);
}
bool Player::knowsRecipe(Recipe* recipe) const {
	return(knowsRecipe(recipe->getId()));
}

//**********************************************************************
//						loadRecipes
//**********************************************************************

bool Config::loadRecipes() {
	xmlDocPtr xmlDoc;
	xmlNodePtr curNode;
	int		i=0;

	char filename[80];
	snprintf(filename, 80, "%s/recipes.xml", Path::Game);
	xmlDoc = xml::loadFile(filename, "Recipes");

	if(xmlDoc == NULL)
		return(false);

	curNode = xmlDocGetRootElement(xmlDoc);

	curNode = curNode->children;
	while(curNode && xmlIsBlankNode(curNode))
		curNode = curNode->next;

	if(curNode == 0) {
		xmlFreeDoc(xmlDoc);
		return(false);
	}

	clearRecipes();
	while(curNode != NULL) {
		if(NODE_NAME(curNode, "Recipe")) {
			i = xml::getIntProp(curNode, "Id");

			if(recipes.find(i) == recipes.end()) {
				recipes[i] = new Recipe;
				recipes[i]->load(curNode);
			}
		}
		curNode = curNode->next;
	}
	xmlFreeDoc(xmlDoc);
	xmlCleanupParser();
	return(true);
}


//**********************************************************************
//						saveRecipes
//**********************************************************************

bool Config::saveRecipes() const {
	std::map<int, Recipe*>::const_iterator it;
	xmlDocPtr	xmlDoc;
	xmlNodePtr	rootNode;
	char			filename[80];

	xmlDoc = xmlNewDoc(BAD_CAST "1.0");
	rootNode = xmlNewDocNode(xmlDoc, NULL, BAD_CAST "Recipes", NULL);
	xmlDocSetRootElement(xmlDoc, rootNode);

	for(it = recipes.begin() ; it != recipes.end() ; it++)
		(*it).second->save(rootNode);

	sprintf(filename, "%s/recipes.xml", Path::Game);
	xml::saveFile(filename, xmlDoc);
	xmlFreeDoc(xmlDoc);
	return(true);
}

//**********************************************************************
//						clearRecipes
//**********************************************************************

void Config::clearRecipes() {
	std::map<int, Recipe*>::iterator it;

	for(it = recipes.begin() ; it != recipes.end() ; it++) {
		Recipe* r = (*it).second;
		delete r;
	}
	recipes.clear();
}


//**********************************************************************
//						getRecipe
//**********************************************************************

Recipe* Config::getRecipe(int id) {
	if(recipes.find(id) != recipes.end())
		return(recipes[id]);
	return(0);
}


//**********************************************************************
//						unprepareAllObjects
//**********************************************************************

void Player::unprepareAllObjects() const {
	otag* op = first_obj;
	while(op) {
		op->obj->clearFlag(O_BEING_PREPARED);
		op = op->next_tag;
	}
}

//**********************************************************************
//						removeItems
//**********************************************************************

void Player::removeItems(const std::list<CatRef>* list, int numIngredients) {
	std::list<CatRef>::const_iterator it;
	otag* op=0;
	int num=0;

	for(it = list->begin(); it != list->end() ; it++) {
		num = 0;
		while(num < numIngredients) {
			op = first_obj;
			while(op) {
				if(Recipe::goodObject(this, op->obj, &(*it)) && op->obj->flagIsSet(O_BEING_PREPARED)) {
					delObj(op->obj, true, false, true, false);
					break;
				}
				op = op->next_tag;
			}
			// updating here is safer, rather than conditionally inside the loop
			num++;
		}
	}
	checkDarkness();
}


//**********************************************************************
//						cmdPrepareObject
//**********************************************************************

int cmdPrepareObject(Player* player, cmd* cmnd) {
	Object	*object=0;

	if(cmnd->num < 2) {
		player->print("Prepare what?\n");
		return(0);
	}

	object = findObject(player, player->first_obj, cmnd);
	if(!object) {
		player->print("You don't have that in your inventory.\n");
		return(0);
	}

	if(object->getType() == CONTAINER && object->first_obj) {
		player->printColor("You need to empty %P in order to prepare it.\n", object);
		return(0);
	}

	if(object->flagIsSet(O_BEING_PREPARED)) {
		player->print("That's already being prepared.\n");
		return(0);
	}

	player->printColor("You prepare %P.\n", object);
	broadcast(player->getSock(), player->getRoom(), "%M prepares %P.", player, object);

	object->setFlag(O_BEING_PREPARED);
	return(0);
}

//**********************************************************************
//						cmdUnprepareObject
//**********************************************************************

int cmdUnprepareObject(Player* player, cmd* cmnd) {
	Object	*object=0;
	if(cmnd->num < 2) {
		player->print("Unprepare what?\n");
		return(0);
	}

	if(!strcmp(cmnd->str[1], "all")) {
		player->unprepareAllObjects();
		player->print("All prepared objects now unprepared.\n");
		return(0);
	}

	object = findObject(player, player->first_obj, cmnd);
	if(!object || !object->flagIsSet(O_BEING_PREPARED)) {
		player->print("You are not preparing that.\n");
		return(0);
	}

	player->printColor("You no longer are preparing %P.\n", object);
	broadcast(player->getSock(), player->getRoom(), "%M stops preparing %P.", player, object);

	object->clearFlag(O_BEING_PREPARED);
	return(0);
}

void Player::checkFreeSkills(bstring skill) {
	if(	skill == "smithing" ||
		skill == "cooking" ||
		skill == "fishing" ||
		skill == "tailoring" ||
		skill == "carpentry"
	) {
		if(!knowsSkill(skill))
			addSkill(skill, 1);
	}
}

//**********************************************************************
//						cmdCraft
//**********************************************************************

int cmdCraft(Player* player, cmd* cmnd) {
	Object*	object=0;
	const Recipe* recipe=0;
	bool succeed=true, searchRecipes=false;
	std::list<int>::iterator it;
	bstring skill = "", action = cmnd->myCommand->getName(), reqSize = "";
	bstring result = "created", fail = "create";
	long t = time(0);
	Size size = player->getSize();
	int numIngredients = 1;

	player->unhide();

	// the parser doesn't understand "cook 5" as two strings. this will force it to.
	if(cmnd->num == 1 && getFullstrText(cmnd->fullstr, 1) != "")
		cmnd->num = 2;

	if(!player->isStaff()) {
		if(!player->ableToDoCommand())
			return(0);
		if(player->isBlind()) {
			player->printColor("^CYou can't do that! You're blind!\n");
			return(0);
		}
		if(player->inCombat()) {
			player->printColor("You are too busy to do that now!\n");
			return(0);
		}
		if(player->flagIsSet(P_MISTED)) {
			player->printColor("You must be in corporeal form to work with items.\n");
			return(0);
		}
		if(!player->canSee(player->getRoom(), true))
			return(0);

		if(!player->checkAttackTimer())
			return(0);
	}

	// this info will only be used for sizable recipes
	reqSize = getFullstrText(cmnd->fullstr, 2);
	if(reqSize != "") {
		size = getSize(reqSize);
		if(size == NO_SIZE)
			size = player->getSize();
		numIngredients = ::numIngredients(size);
	}

	// translate the command into an actual skill
	if(action == "smith") {
		skill = "smithing";
		result = "smithed";
		fail = "smith";
	} else if(action == "alchemy") {
		skill = "alchemy";
	} else if(action == "tailor") {
		skill = "tailoring";
		result = "tailored";
		fail = "tailor";
	} else if(action == "brew") {
		skill = "brewing";
		result = "brewed";
		fail = "brew";
	} else if(action == "cook") {
		skill = "cooking";
		result = "cooked";
		fail = "cook";
	} else if(action == "carpenter") {
		skill = "carpentry";
		result = "carpentered";
		fail = "carpenter";
	}
	// all other commands are skill-less


	// there is currently know way to train skills, so let everying be an amatuer (for now)
	player->checkFreeSkills(skill);



	// initial skill check
	if(skill != "" && !player->knowsSkill(skill)) {
		player->print("You lack the training in %s.\n", skill.c_str());
		return(0);
	}

	// using a recipe they know or a single object
	if(cmnd->num > 1) {
		// we are crafting from a recipe and need that recipe to go any further
		recipe = player->findRecipe(cmnd, skill, &searchRecipes, size, numIngredients);
		if(!recipe)
			return(0);

		if(!recipe->isSkilled(player, size)) {
			if(size != player->getSize()) {
				player->print("Sorry, you are not skilled enough in %s to use this recipe for the specified size.\n", recipe->getSkill().c_str());
			} else {
				player->print("Sorry, you are not skilled enough in %s to use this recipe.\n", recipe->getSkill().c_str());
			}
			return(0);
		}
		if(recipe->getSkill() != skill) {
			if(recipe->getSkill() == "")
				player->print("Sorry, that recipe is not a %s recipe.\n", skill.c_str());
			else
				player->print("Sorry, that recipe is a %s recipe.\n", recipe->getSkill().c_str());
			return(0);
		}

	} else {
		// we should have prepared all objects ourselves; recipe object not needed.
		// searching handles all error messages, so if bad, just exit
		if(!(recipe = gConfig->searchRecipes(player, skill, size, numIngredients)))
			return(0);
		searchRecipes = true;
	}

	if(!recipe->isValid()) {
		player->print("Sorry, that recipe is faulty.\n");
		return(0);
	}

	// searchRecipes will find everything we need - don't do it again
	if(!searchRecipes) {
		if(!recipe->canUseEquipment(player, skill))
			return(0);

		player->unprepareAllObjects();

		// checking handles all error messages, so if bad, just exit
		if(!recipe->check(player, &recipe->ingredients, "ingredients", numIngredients))
			return(0);
		if(!recipe->check(player, &recipe->reusables, "reusables", 1))
			return(0);
		if(skill == "cooking" && recipe->equipment.empty()) {
			object = findHot(player);
			if(!object) {
				player->unprepareAllObjects();
				player->print("You lack the necessary cooking equipment for this recipe.\n");
				return(0);
			} else {
				player->printColor("You prepare %1P.\n", object);
			}
		} else if(!recipe->check(player, &recipe->equipment, "equipment", 1))
			return(0);
	}


	// just check to make sure the item exists and we are set
	if(!loadObject(recipe->getResult(), &object)) {
		player->unprepareAllObjects();
		player->print("Sorry, that recipe is faulty.\n");
		return(0);
	}

	if(recipe->isSizable())
		object->setSize(size);

	if( object->getQuestnum() &&
		player->questIsSet(object->getQuestnum()-1) &&
		!player->checkStaff("You complete that recipe. You have already fulfilled that quest.\n")
	) {
		delete object;
		return(0);
	}

	if(!Lore::canHave(player, object, false)) {
		player->print("You already have enough of those.\nYou cannot currently have any more.\n");
		delete object;
		return(0);
	}
	if(!Unique::canGet(player, object)) {
		player->print("That recipe results in a limited object.\nYou cannot currently have any more.\n");
		delete object;
		return(0);
	}


	player->updateAttackTimer(true, 50);
	player->lasttime[LT_SPELL].ltime = t;
	player->lasttime[LT_SPELL].interval = 5L;
	player->lasttime[LT_MOVED].ltime = t;
	player->lasttime[LT_MOVED].misc = 5;


	player->removeItems(&recipe->ingredients, recipe->isSizable() ? numIngredients : 1);
	player->unprepareAllObjects();

	// check success: skillless recipes are always successful
	if(skill != "") {
		double chance = 30 + 20 * log(player->getSkillLevel(skill));
		if(chance < mrand(0,100))
			succeed = false;
		player->checkImprove(skill, succeed);
	}

	if(succeed) {
		player->statistics.craft();
		object->setDroppedBy(player, "Craft:"+skill);
		doGetObject(object, player);
		player->printColor("^yYou have successfully %s %1P^y.\n", result.c_str(), object);
		if(recipe && recipe->getExperience()) {
			if(!player->halftolevel()) {
				player->print("You %s %d experience.\n", gConfig->isAprilFools() ? "lose" : "gain", recipe->getExperience());
				player->addExperience(recipe->getExperience());
			}
		}
		broadcast(player->getSock(), player->getRoom(), "%M successfully %s %1P.^x", player, result.c_str(), object);
	} else {
		player->printColor("^yYou attempted to %s %1P^y, but failed.\n", fail.c_str(), object);
		broadcast(player->getSock(), player->getRoom(), "%M tried to %s an object.", player, fail.c_str());
		delete object;
	}

	return(0);
}
