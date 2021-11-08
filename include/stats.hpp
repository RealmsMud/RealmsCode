/*
 * stats.h
 *   Header file for stats
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
#ifndef STAT_H_
#define STAT_H_

#include <map>
#include <libxml/parser.h>  // for xmlNodePtr

#include "alphanum.hpp"

enum ModifierType {
    MOD_NONE = 0,
    MOD_CUR = 1,
    MOD_MAX = 2,
    MOD_CUR_MAX = 3,
    MOD_ALL = MOD_CUR_MAX
};
class StatModifier {
public:
    StatModifier();
    StatModifier(std::string_view pName, int pModAmt, ModifierType pModType);
    StatModifier(xmlNodePtr curNode);
    StatModifier(StatModifier &sm);
    void save(xmlNodePtr parentNode);

    void adjust(int adjAmount);
    void set(int newAmt);
    void setType(ModifierType newType);
    std::string getName();
    int getModAmt();
    ModifierType getModType();

    std::string toString();
private:
    std::string         name;
    int             modAmt{};
    ModifierType    modType;

};

typedef std::map<std::string, StatModifier*, alphanum_less<std::string> > ModifierMap;

class Stat
{
public:
    Stat();
    Stat& operator=(const Stat& cr);
    ~Stat();
    void doCopy(const Stat& st);

    
    std::string toString();
    friend std::ostream& operator<<(std::ostream& out, Stat& stat);

    void setName(std::string_view pName);

    bool load(xmlNodePtr curNode, std::string_view statName);
    bool loadModifiers(xmlNodePtr curNode);
    void save(xmlNodePtr parentNode, const char* statName) const;
    
    unsigned int increase(unsigned int amt);
    unsigned int decrease(unsigned int amt);
    unsigned int adjust(int amt);
    
    unsigned int getCur(bool recalc = true);
    unsigned int getMax();
    unsigned int getInitial() const;

    void addInitial(unsigned int a);
    void setMax(unsigned int newMax, bool allowZero=false);
    void setCur(unsigned int newCur);
    void setInitial(unsigned int i);
    void setDirty();

    void setInfluences(Stat* pInfluences);
    void setInfluencedBy(Stat* pInfluencedBy);
    unsigned int restore(); // Set a stat to it's maximum value

    void reCalc();

    bool addModifier(StatModifier* toAdd);
    bool addModifier(const std::string &pName, int modAmt, ModifierType modType);

    bool removeModifier(const std::string &pName);
    bool adjustModifier(const std::string &pName, int modAmt, ModifierType modType = MOD_CUR);
    bool setModifier(const std::string &pName, int newAmt, ModifierType modType = MOD_CUR);

    void clearModifiers();

    StatModifier* getModifier(const std::string &pName);
    int getModifierAmt(const std::string &pName);

    void upgradeSetCur(unsigned int newCur);  // Used only in upgrading to new stats
protected:

    std::string name;
    ModifierMap modifiers;
    bool dirty;


    unsigned int cur;
    unsigned int max;
    unsigned int initial;

    Stat* influences;
    Stat* influencedBy;
};

#endif /*STAT_H_*/
