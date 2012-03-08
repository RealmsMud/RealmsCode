/*
 * stats.h
 *	 Header file for stats
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
#ifndef STAT_H_
#define STAT_H_

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
	StatModifier(bstring pName, int pModAmt, ModifierType pModType);
	StatModifier(xmlNodePtr curNode);
	StatModifier(StatModifier &sm);
	void save(xmlNodePtr parentNode);

	void adjust(int adjAmount);
	void set(int newAmt);
	void setType(ModifierType newType);
	bstring getName();
	int getModAmt();
	ModifierType getModType();

	bstring toString();
private:
	bstring 		name;
	int				modAmt;
	ModifierType	modType;

};

#ifndef PYTHON_CODE_GEN
typedef std::map<bstring, StatModifier*, alphanum_less<bstring> > ModifierMap;
#endif

class Stat
{
public:
	Stat();
	Stat& operator=(const Stat& cr);
	~Stat();
	void doCopy(const Stat& st);

	
	bstring toString();
	friend std::ostream& operator<<(std::ostream& out, Stat& stat);

	void setName(bstring pName);

	bool load(xmlNodePtr curNode, bstring statName);
	bool loadModifiers(xmlNodePtr curNode);
	void save(xmlNodePtr parentNode, const char* statName) const;
	
	int increase(int amt);
	int decrease(int amt);
	int adjust(int amt);
	
	int getCur(bool recalc = true);
	int getMax();
	int getInitial() const;

	void addInitial(int a);
	void setMax(int newMax, bool allowZero=false);
	void setCur(int newCur);
	void setInitial(int i);
	void setDirty();

	void setInfluences(Stat* pInfluences);
	void setInfluencedBy(Stat* pInfluencedBy);
	int restore(); // Set a stat to it's maximum value

	void reCalc();

	bool addModifier(StatModifier* toAdd);
	bool addModifier(bstring name, int modAmt, ModifierType modType);

	bool removeModifier(bstring name);
	bool adjustModifier(bstring name, int modAmt, ModifierType modType = MOD_CUR);
	bool setModifier(bstring name, int newAmt, ModifierType modType = MOD_CUR);

	void clearModifiers();

	StatModifier* getModifier(bstring name);
	int getModifierAmt(bstring name);

	void upgradeSetCur(int newCur);  // Used only in upgrading to new stats
protected:

	bstring name;
#ifndef PYTHON_CODE_GEN
	ModifierMap modifiers;
#endif
	bool dirty;


	int	cur;
	int	max;
	int	initial;

	Stat* influences;
	Stat* influencedBy;
};

#endif /*STAT_H_*/
