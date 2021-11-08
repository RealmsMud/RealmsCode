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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef CRAFT_H_
#define CRAFT_H_

#include <list>

#include "catRef.hpp"
#include "size.hpp"

class Player;
class Object;


class Recipe {
public:
    Recipe();
    friend std::ostream &operator<<(std::ostream &out, Recipe &recipe);
    friend std::ostream &operator<<(std::ostream &out, Recipe *recipe);
    void save(xmlNodePtr rootNode) const;
    void load(xmlNodePtr curNode);
    void saveList(xmlNodePtr curNode, const std::string &name, const std::list<CatRef> *list) const;
    void loadList(xmlNodePtr curNode, std::list<CatRef> *list);
    [[nodiscard]] bool isValid() const;
    bool check(const Player *player, const std::list<CatRef> *list, std::string_view type, int numIngredients) const;
    bool check(std::list<CatRef> *list, const std::list<CatRef> *require, int numIngredients) const;
    bool isSkilled(const Player *player, Size recipeSize) const;
    std::string listIngredients(const std::list<CatRef> *list) const;
    std::string display();
    bool canUseEquipment(const Player *player, std::string_view skill) const;
    bool canBeEdittedBy(const Player *player) const;

    void setId(unsigned int i);
    void setExperience(int exp);
    void setSizable(bool size);
    void setResult(const CatRef &cr);
    void setSkill(std::string_view s);
    void setRequiresRecipe(bool r);
    void setCreator(std::string_view c);
    [[nodiscard]] unsigned int getId() const;
    [[nodiscard]] int getExperience() const;
    [[nodiscard]] bool isSizable() const;
    [[nodiscard]] CatRef getResult() const;
    [[nodiscard]] std::string getResultName(bool appendCr = false);
    [[nodiscard]] std::string getSkill() const;
    [[nodiscard]] std::string getCreator() const;
    [[nodiscard]] int getMinSkill() const;
    [[nodiscard]] bool requiresRecipe() const;

    static bool goodObject(const Player *player, const Object *object, const CatRef *cr = nullptr);

    std::list<CatRef> ingredients;
    std::list<CatRef> reusables;
    std::list<CatRef> equipment;

protected:
    unsigned int id;
    CatRef result;
    std::string resultName;
    std::string skill;
    std::string creator;
    bool requireRecipe;
    int experience;
    bool sizable;

    int minSkill;
};


#endif /*CRAFT_H_*/
