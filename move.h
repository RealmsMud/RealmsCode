/*
 * move.h
 *   Namespace for movement.
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___ 
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *  
 *  Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

namespace Move {
    bool tooFarAway(BaseRoom* pRoom, BaseRoom* tRoom, bool track);
    bool tooFarAway(Creature *player, BaseRoom* room);
    bool tooFarAway(Creature *player, Creature *target, bstring action);
    void broadcast(Creature* player, Container* container, bool ordinal, bstring exit, bool hiddenExit);
    bstring formatFindExit(cmd* cmnd);
    bool isSneaking(cmd* cmnd);
    bool isOrdinal(cmd* cmnd);
    AreaRoom *recycle(AreaRoom* room, Exit* exit);
    UniqueRoom *getUniqueRoom(Creature* creature, Player* player, Exit* exit, MapMarker* tMapmarker);
    void update(Player* player);
    void broadMove(Creature* player, Exit* exit, cmd* cmnd, bool sneaking);
    bool track(UniqueRoom* room, MapMarker *mapmarker, Exit* exit, Player* player, bool reset);
    void track(UniqueRoom* room, MapMarker *mapmarker, Exit* exit, Player *leader, std::list<Creature*> *followers);
    bool sneak(Player* player, bool sneaking);
    bool canEnter(Player* player, Exit* exit, bool leader);
    bool canMove(Player* player, cmd* cmnd);
    Exit *getExit(Creature* player, cmd* cmnd);
    bstring getString(Creature* creature, bool ordinal=false, bstring exit = "");
    void checkFollowed(Player* player, Exit* exit, BaseRoom* room, std::list<Creature*> *followers);
    void finish(Creature* creature, BaseRoom* room, bool self, std::list<Creature*> *followers);
    bool getRoom(Creature* creature, const Exit* exit, BaseRoom **newRoom, bool justLooking=false, MapMarker* tMapmarker=0, bool recycle=true);
    BaseRoom* start(Creature* creature, cmd* cmnd, Exit **gExit, bool leader, std::list<Creature*> *followers, int* numPeople, bool& roomPurged);

    void createPortal(BaseRoom* room, BaseRoom* target, const Player* player, bool initial=true);
    bool usePortal(Creature* player, BaseRoom* room, Exit* exit, bool initial=true);
    bool deletePortal(BaseRoom* room, Exit* exit, const Creature* leader=0, std::list<Creature*> *followers=0, bool initial=true);
    bool deletePortal(BaseRoom* room, bstring name, const Creature* leader=0, std::list<Creature*> *followers=0, bool initial=true);
}
