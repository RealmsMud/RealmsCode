/*
 * threat.h
 *	 Classes for threat
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

#ifndef THREAT_H_
#define THREAT_H_

#include <set>
#include <functional>


class ThreatEntry {
public:
    friend std::ostream& operator<<(std::ostream& out, const ThreatEntry& threat);
    friend std::ostream& operator<<(std::ostream& out, const ThreatEntry* threat);

public:
	ThreatEntry(bstring pUid);

	long getThreatValue();
	long getContributionValue();
	long adjustThreat(long modAmt);
	long adjustContribution(long modAmt);
//protected:
public:
	bstring uId;        // ID of the target this monster is mad at

	time_t lastMod;     // When was this threat entry last updated

	long threatValue;       // How mad they are at this target
	long contributionValue; // How much this target has contributed to killing this monster
	bool operator< (const ThreatEntry& t) const;
	const bstring& getUid() const;
};

struct ThreatPtrLess : public std::binary_function<const ThreatEntry*, const ThreatEntry*, bool> {
    bool operator()(const ThreatEntry* lhs, const ThreatEntry* rhs) const;
};

typedef std::map<bstring, ThreatEntry*> ThreatMap;
typedef std::multiset<ThreatEntry*, ThreatPtrLess> ThreatSet;


class ThreatTable {
    // Operators
public:
    friend std::ostream& operator<<(std::ostream& out, const ThreatTable& table);
    friend std::ostream& operator<<(std::ostream& out, const ThreatTable* table);
public:
	ThreatTable(Creature* parent);
	~ThreatTable();

	bool isEnemy(const Creature* target);
	long getTotalThreat();
	long getThreat(Creature* target);
	long adjustThreat(Creature* target, long modAmt, double threatFactor = 1.0);
	long removeThreat(const bstring& pUid);
	long removeThreat(Creature* target);
	void setParent(Creature* pParent);
	bool hasEnemy() const;

	ThreatMap::size_type size();
	void clear();

	Creature* getTarget(bool sameRoom=true);

protected:
	ThreatSet::iterator removeFromSet(ThreatEntry* threat);

public:
	ThreatMap threatMap;
	ThreatSet threatSet;

	long totalThreat;
    Creature* myParent;
};


#endif //THREAT_H
