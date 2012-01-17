/*
 * Quests.h
 *	 New questing system
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

#ifndef QUESTS_H_
#define QUESTS_H_


class QuestCatRef : public CatRef {
public:
	QuestCatRef();
	QuestCatRef(xmlNodePtr rootNode);
	xmlNodePtr save(xmlNodePtr rootNode, bstring saveName = "QuestCatRef") const;
//	bstring area;
//	int index;
	int curNum;
	int reqNum;		// How many
};

// Descriptive information about a quest
class QuestInfo {
public:
	QuestInfo(xmlNodePtr rootNode);

	int getId() const;
	bstring getName() const;
	bstring getDisplayName() const;
	bstring getDisplayString() const;
	bool isRepeatable() const;
	int getTimesRepetable() const;
	bool canGetQuest(const Player* player, const Monster* giver) const;
	void printReceiveString(const Player* player, const Monster* giver) const;
	void printCompletionString(const Player* player, const Monster* giver) const;
	void giveInitialitems(const Monster* giver, Player* player) const;
	const QuestCatRef& getTurnInMob() const;
private:
	int questId;
	bstring name;
	bstring description;		// Description of the quest
	bstring receiveString;		// String that is output to the player when they receive this quest
	bstring completionString;	// String that is output to the player when they complete this quest

	bstring revision;	// This will be used for making sure all questcompletions
				// are synced up with the parent quest incase we make changes
				// to them

	bool repeatable;	// Can this quest be repeated multiple times?
	int timesRepetable;     // Number of times the quest can be repeated.  0 for infinite
	bool sharable;		// Can this quest be shared with other players?
	int minLevel;		// Minimum required level to get this quest
	int minFaction;		// Minimum requried faction to get this quest (Based on mob's primeFaction)
	int level;		// Level of this quest, used to adjust rewards

	std::list<int> preRequisites; 		// A list of quests that must have been completed
						// before someone is allowed to get this quest
	std::list<QuestCatRef> initialItems;	// Items the player is given when the quest begins
	std::list<QuestCatRef> mobsToKill; 	// A list of monsters that need to be killed for this quest
	std::list<QuestCatRef> itemsToGet;	// A list of items that need to be obtained for this quest
	std::list<QuestCatRef> roomsToVisit;	// Rooms they need to visit before they can finish this quest

	// Completion Info
	QuestCatRef turnInMob;			// Monster that we turn this quest into for completion
	Money cashReward;			// Coin Reward
	long expReward;				// Exp reward
	std::list<QuestCatRef> itemRewards;	// Items rewarded on completion
	std::map<bstring,long> factionRewards;  // Factions to be modified

	friend class QuestCompletion;
};

// Class to keep track of what has been completed on a given quest for a player so far
class QuestCompletion {
public:
	QuestCompletion(QuestInfo* parent, Player* player);
	QuestCompletion(xmlNodePtr rootNode, Player* player);
	xmlNodePtr save(xmlNodePtr rootNode) const;

	//int getQuestId();
	void resetParentQuest();
	QuestInfo* getParentQuest() const;
private:
	int questId;
	QuestInfo* parentQuest;	// What quest are we keeping track of?
	Player* parentPlayer;	// Parent Player for this quest
	bstring revision;

	std::list<QuestCatRef> mobsKilled;		// How many of the required monsters have we killed
	std::list<QuestCatRef> roomsVisited;	// How many of the required rooms have we visited.

	// NOTE: Don't keep track of items here, we'll calculate that whenever we
	//       look at the quest completion info, because they might dispose of
	//       items between checks, they can't however change what rooms they've
	//       been to, or what monsters they've killed

	bool mobsCompleted;
	bool itemsCompleted;
	bool roomsCompleted;
public:
	void updateMobKills(Monster* monster);
	void updateItems(Object* object);
	void updateRooms(UniqueRoom* room);
	bstring getStatusDisplay();

	bool checkQuestCompletion(bool showMessage = true);
	bool hasRequiredMobs() const;
	bool hasRequiredItems() const;
	bool hasRequiredRooms() const;

	bool complete(Monster* monster);
};


class TalkResponse {
public:
	TalkResponse();
	TalkResponse(xmlNodePtr rootNode);
	xmlNodePtr saveToXml(xmlNodePtr rootNode) const;

	std::list<bstring> keywords; // Multiple keywords!
	bstring response; // What he'll respond with, ^C type colors allowed
	bstring action; // Cast on a player, flip them off, give them an item, give them a quest....etc
};

#endif /*QUESTS_H_*/

