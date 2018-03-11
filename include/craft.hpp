/*
 * craft.h
 *   Header file for crafting
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___ 
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *  
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef CRAFT_H_
#define CRAFT_H_

#include <list>

#include "catRef.hpp"
#include "common.hpp"
#include "size.hpp"

class Player;
class Object;


class Recipe {
public:
    Recipe();
    friend std::ostream& operator<<(std::ostream& out, Recipe& recipe);
    friend std::ostream& operator<<(std::ostream& out, Recipe* recipe);
    void    save(xmlNodePtr rootNode) const;
    void    load(xmlNodePtr curNode);
    void    saveList(xmlNodePtr curNode, bstring name, const std::list<CatRef>* list) const;
    void    loadList(xmlNodePtr curNode, std::list<CatRef>* list);
    bool    isValid() const;
    bool    check(const Player* player, const std::list<CatRef>* list, bstring type, int numIngredients) const;
    bool    check(std::list<CatRef>* list, const std::list<CatRef>* require, int numIngredients) const;
    bool    isSkilled(const Player* player, Size recipeSize) const;
    bstring listIngredients(const std::list<CatRef>* list) const;
    bstring display();
    bool    canUseEquipment(const Player* player, bstring skill) const;
    bool    canBeEdittedBy(const Player* player) const;

    int     getId() const;
    void    setId(int i);
    int     getExperience() const;
    void    setExperience(int exp);
    bool    isSizable() const;
    void    setSizable(bool size);
    CatRef  getResult() const;
    void    setResult(CatRef cr);
    bstring getResultName(bool appendCr=false);
    bstring getSkill() const;
    void    setSkill(bstring s);
    bstring getCreator() const;
    void    setCreator(bstring c);
    int     getMinSkill() const;
    bool    requiresRecipe() const;
    void    setRequiresRecipe(bool r);

    static bool goodObject(const Player* player, const Object* object, const CatRef* cr=0);

    std::list<CatRef> ingredients;
    std::list<CatRef> reusables;
    std::list<CatRef> equipment;

protected:
    int     id;
    CatRef  result;
    bstring resultName;
    bstring skill;
    bstring creator;
    bool    requireRecipe;
    int     experience;
    bool    sizable;

    int     minSkill;
};


#endif /*CRAFT_H_*/
