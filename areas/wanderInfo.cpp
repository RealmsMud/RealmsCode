/*
 * wanderInfo.cpp
 *   Random monster wandering info
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


#include <map>                      // for map, operator==, map<>::const_ite...
#include <string>                   // for string
#include <string_view>              // for string_view
#include <utility>                  // for pair
#include <filesystem>

#include "catRef.hpp"               // for CatRef
#include "mudObjects/monsters.hpp"  // for Monster
#include "mudObjects/players.hpp"   // for Player
#include "random.hpp"               // for Random
#include "wanderInfo.hpp"           // for WanderInfo
#include "xml.hpp"                  // for loadMonster

namespace fs = std::filesystem;

//*********************************************************************
//                      WanderInfo
//*********************************************************************

WanderInfo::WanderInfo() {
    traffic = 0;
}

short WanderInfo::getTraffic() const { return(traffic); }
void WanderInfo::setTraffic(short t) { traffic = std::max<short>(0, std::min<short>(100, t)); }

//*********************************************************************
//                      getRandom
//*********************************************************************
// This function used to be weighted - it would check 0-9 and if an entry didn't
// exist, no monster would come. If we pick a random entry and the list size is
// less than 10, then we have increased traffic.

CatRef WanderInfo::getRandom() const {
    std::map<int, CatRef>::const_iterator it;
    CatRef cr;
    int i=0;

    if(random.empty())
        return(cr);

    // past the end
    it = random.end();
    // the last element
    it--;

    if(it == random.begin())
        return(cr);

    // pick a random element, adding weight if <9
    i = (*it).first;
    i = Random::get(0, i < 9 ? 9 : i);

    it = random.find(i);
    if(it != random.end())
        cr = (*it).second;
    return(cr);
}

unsigned long WanderInfo::getRandomCount() const {
    return random.size();
}


//*********************************************************************
//                      show
//*********************************************************************

void WanderInfo::show(const std::shared_ptr<Player> player, std::string_view area) const {
    std::map<int, CatRef>::const_iterator it;
    std::shared_ptr<Monster>  monster=nullptr;

    player->print("Traffic: %d%%\n", traffic);
    player->print("Random Monsters:\n");
    for(it = random.begin(); it != random.end() ; it++) {
        loadMonster((*it).second, monster);

        player->printColor("^c%2d) ^x%14s ^c::^x %s\n", (*it).first+1,
                           (*it).second.displayStr("", 'c').c_str(), monster ? monster->getCName() : "");

        monster.reset();
    }
}
