/*
 * alignment.cpp
 *   Alignment-related functions
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
#include <strings.h>                 // for strcasecmp
#include <cstdlib>                   // for abs
#include <string>                    // for allocator, string

#include "cmd.hpp"                   // for cmd
#include "flags.hpp"                 // for P_CHAOTIC, P_CHOSEN_ALIGNMENT
#include "global.hpp"                // for NEUTRAL, CreatureClass, BLUISH
#include "mud.hpp"                   // for ALIGNMENT_LEVEL
#include "mudObjects/creatures.hpp"  // for Creature
#include "mudObjects/monsters.hpp"   // for Monster
#include "mudObjects/players.hpp"    // for Player
#include "proto.hpp"                 // for broadcast, logn, isStaff, antiGr...



//***********************************************************************
//                      getAlignDiff
//***********************************************************************
// This function returns the distance on the alignment scale between
// any two different creatures. If either creature is neutral, or if
// both creatures are of the same moral character (i.e. both good or
// both evil) it will return 0. It is to be used for when opposing
// ethos has an effect on anything.

int getAlignDiff(std::shared_ptr<Creature>crt1, std::shared_ptr<Creature>crt2) {

    int     alignDiff = 0;

    if( (crt1->getAdjustedAlignment() < NEUTRAL && crt2->getAdjustedAlignment() > NEUTRAL)  ||
        (crt2->getAdjustedAlignment() < NEUTRAL && crt1->getAdjustedAlignment() > NEUTRAL)
    ) {
        alignDiff = abs(crt1->getAdjustedAlignment()) + abs(crt2->getAdjustedAlignment());
    }

    return(alignDiff);
}

//***********************************************************************
//                      getAdjustedAlignment
//***********************************************************************

int Monster::getAdjustedAlignment() const {

    if (alignment <= M_TOP_BLOODRED)
        return(BLOODRED);
    else if (alignment >= M_BOTTOM_RED && alignment <= M_TOP_RED)
        return(RED);
    else if (alignment >= M_BOTTOM_REDDISH && alignment <= M_TOP_REDDISH)
        return(REDDISH);
    else if (alignment >= M_BOTTOM_PINKISH && alignment <= M_TOP_PINKISH)
        return(PINKISH);
    else if (alignment == 0)
        return(NEUTRAL);
    else if (alignment >= M_BOTTOM_LIGHTBLUE && alignment <= M_TOP_LIGHTBLUE)
        return(LIGHTBLUE);
    else if (alignment >= M_BOTTOM_BLUISH && alignment <= M_TOP_BLUISH)
        return(BLUE);
    else if (alignment >= M_BOTTOM_BLUE && alignment <= M_TOP_BLUE)
        return(BLUE);
    else if (alignment >= M_BOTTOM_ROYALBLUE)
        return(ROYALBLUE);

    return(NEUTRAL);

}

int Player::getAdjustedAlignment() const {

    if (alignment <= P_TOP_BLOODRED)
        return(BLOODRED);
    else if (alignment >= P_BOTTOM_RED && alignment <= P_TOP_RED)
        return(RED);
    else if (alignment >= P_BOTTOM_REDDISH && alignment <= P_TOP_REDDISH)
        return(REDDISH);
    else if (alignment >= P_BOTTOM_PINKISH && alignment <= P_TOP_PINKISH)
        return(PINKISH);
    else if (alignment >= P_BOTTOM_NEUTRAL && alignment <= P_TOP_NEUTRAL)
        return(NEUTRAL);
    else if (alignment >= P_BOTTOM_LIGHTBLUE && alignment <= P_TOP_LIGHTBLUE)
        return(LIGHTBLUE);
    else if (alignment >= P_BOTTOM_BLUISH && alignment <= P_TOP_BLUISH)
        return(BLUISH);
    else if (alignment >= P_BOTTOM_BLUE && alignment <= P_TOP_BLUE)
        return(BLUE);
    else if (alignment >= P_BOTTOM_ROYALBLUE)
        return(ROYALBLUE);

    return(NEUTRAL);
}

int Creature::getDeityAlignment() const {
    if (!deity)
        return(0);

    switch(deity) {
    case ENOCH:
    case GRADIUS:
        return(ROYALBLUE);
    case CERIS:
    case LINOTHAN:
    case MARA:
        return(BLUE);
    break;
    case KAMIRA:
        return(LIGHTBLUE);
    case ARACHNUS:
    case ARAMON:
        return(BLOODRED);
    case ARES:
    case JAKAR:
        return(NEUTRAL);
    default:
    break; 
    }
    return(NEUTRAL);
}

// *************************************************************************************************************
//                                              shiftAlignment
// *************************************************************************************************************
// This function will shift a creature's adjusted alignment a number of scale places specified by shift. The resulting 
// alignment value will be set in the middle of the range of the new adjusted alignment. This is primarily meant for
// players, but there could be some situations where we might want to shift monster alignments in a similar manner.
// If silent is false, a player will not get a report that their alignment shifted.
void Creature::shiftAlignment(short shift, bool silent) {

    int newAdjustedAlign = std::max((int)BLOODRED,(std::min((int)ROYALBLUE,(getAdjustedAlignment()+shift))));
    bool cPlayer = isPlayer();

    
    // A resulting shift of 0 (such as shifting past min/max BLOODRED/ROYALBLUE) doesn't do anything
    // to the creature's current alignment value, and it stays unchanged. The same thing happens if
    // this function is called with shift set to 0 for some reason, which shouldn't happen, but could.
    if (newAdjustedAlign == getAdjustedAlignment())
        return;

    switch (newAdjustedAlign) {
    case BLOODRED:
        setAlignment((cPlayer?((P_TOP_BLOODRED+P_BOTTOM_BLOODRED)/2):((M_TOP_BLOODRED+M_BOTTOM_BLOODRED)/2)));
        break;
    case RED:
        setAlignment((cPlayer?((P_TOP_RED+P_BOTTOM_RED)/2):((M_TOP_RED+M_BOTTOM_RED)/2)));
        break;
    case REDDISH:
        setAlignment((cPlayer?((P_TOP_REDDISH+P_BOTTOM_REDDISH)/2):((M_TOP_REDDISH+M_BOTTOM_REDDISH)/2)));
        break;
    case PINKISH:
        setAlignment((cPlayer?((P_TOP_PINKISH+P_BOTTOM_PINKISH)/2):((M_TOP_PINKISH+M_BOTTOM_PINKISH)/2)));
        break;
    case NEUTRAL:
        setAlignment((cPlayer?((P_TOP_NEUTRAL+P_BOTTOM_NEUTRAL)/2):0));
        break;
    case LIGHTBLUE:
        setAlignment((cPlayer?((P_TOP_LIGHTBLUE+P_BOTTOM_LIGHTBLUE)/2):((M_TOP_LIGHTBLUE+M_BOTTOM_LIGHTBLUE)/2)));
        break;
    case BLUISH:
        setAlignment((cPlayer?((P_TOP_BLUISH+P_BOTTOM_BLUISH)/2):((M_TOP_BLUISH+M_BOTTOM_BLUISH)/2)));
        break;
    case BLUE:
        setAlignment((cPlayer?((P_TOP_BLUE+P_BOTTOM_BLUE)/2):((M_TOP_BLUE+M_BOTTOM_BLUE)/2)));
        break;
    case ROYALBLUE:
        setAlignment((cPlayer?((P_TOP_ROYALBLUE+P_BOTTOM_ROYALBLUE)/2):((M_TOP_ROYALBLUE+M_BOTTOM_ROYALBLUE)/2)));
        break;
    default:
        setAlignment(NEUTRAL);
        break;
    }
    

    if (cPlayer && !silent) {
        *this << ColorOn << "Your alignment has been shifted " << intToText(abs(shift),false) << " tier" << (abs(shift)>1?"s":"") << " in the direction of " << ((shift<0)?"^R":"^B") << ((shift<0)?"evil":"good") << "^x." << ColorOff;
        *this << ColorOn << " It is now " << alignColor() << alignString() << ((shift<0)?"^r":"^b") << "^x.\n" << ColorOff;
    }
    
    return;
}

void Creature::setOppositeAlignment(bool silent) {

    if (getAdjustedAlignment() == NEUTRAL)
        return;
    else 
    {
        if (isPlayer() && !silent) {
            *this << ColorOn << alignColor() << "^#Your alignment has been diametrically shifted!\n" << ColorOff;
            *this << ColorOn << alignColor() << "Your alignment is now " << alignString() << ".\n" << ColorOff;
            *this << ColorOn << alignColor() << "You suddenly feel rather " << ((getAdjustedAlignment()>NEUTRAL)?"^rwicked":"^bvirtuous") << "^x.\n" << ColorOff;
            shiftAlignment((getAdjustedAlignment()*-2), true);
        }
        else
            shiftAlignment((getAdjustedAlignment()*-2), true);
    }

    return;
}


//***********************************************************************
//                      alignColor
//***********************************************************************

std::string Creature::alignColor() const {

    switch(getAdjustedAlignment()) {
    case PINKISH:
    case REDDISH:
    case RED:
        return("^R");
    case LIGHTBLUE:
    case BLUISH:
    case BLUE:
        return("^B");
    case BLOODRED:
        return("^r");
    case ROYALBLUE:
        return("^b");
    default:
        return("^x");
    }

}

std::string Creature::alignString() const {
    switch (getAdjustedAlignment()) {
    case BLOODRED:
        return("blood red");
    case RED:
        return("red");
    case REDDISH:
        return("reddish");
    case PINKISH:
        return("pinkish");
    case NEUTRAL:
        return("grey");
    case LIGHTBLUE:
        return("light blue");
    case BLUISH:
        return("bluish");
    case BLUE:
        return("blue");
    case ROYALBLUE:
        return("royal blue");
    default:
        return("grey");
    }
}

//********************************************************************
//                      cmdChooseAlignment
//********************************************************************

int cmdChooseAlignment(const std::shared_ptr<Player>& player, cmd* cmnd) {
    char syntax[] = "Syntax: alignment lawful\n"
                    "        alignment chaotic\n"
                    "Note: Tieflings must be chaotic.\n\n";

    if(player->isStaff() && !player->isCt())
        return(0);

    if(!player->ableToDoCommand())
        return(0);

    if(player->flagIsSet(P_CHOSEN_ALIGNMENT)) {
        *player << "You have already chosen your alignment.\n";
        if(player->flagIsSet(P_CHAOTIC))
            *player << "In order to convert to lawful, use the 'convert' command. HELP CONVERT.\n";
        return(0);
    } else if(player->getLevel() < ALIGNMENT_LEVEL) {
        *player << "You cannot choose your alignment until level " << ALIGNMENT_LEVEL << ".\n";
        return(0);
    } else if(player->getLevel() > ALIGNMENT_LEVEL) {
        *player << "Your alignment has already been chosen.\n";
        return(0);
    }

    if(cmnd->num < 2) {
        player->print(syntax);
        return(0);
    }


    if(!strcasecmp(cmnd->str[1], "lawful")) {
        if(player->getRace() == TIEFLING) {
            player->print("Tieflings are required to be chaotic.\n\n");
        } else {
            broadcast("^B### %s chooses to adhere to the order of LAW!", player->getCName());
            player->setFlag(P_CHOSEN_ALIGNMENT);
        }
    } else if(!strcasecmp(cmnd->str[1], "chaotic")) {
        broadcast("^R### %s chooses to embrace the whims of CHAOS!", player->getCName());
        player->setFlag(P_CHAOTIC);
        player->setFlag(P_CHOSEN_ALIGNMENT);
    } else {
        player->print(syntax);
        return(0);
    }
    return(0);
}

//*********************************************************************
//                      alignAdjustAcThaco
//*********************************************************************
// This function will be called whenever alignment changes,
// primarily in combat after killing a mob. It is initially
// designed to be only for monks and werewolves, but it can
// be applied to any class by altering the function. -TC

void Player::alignAdjustAcThaco() {
    if(cClass != CreatureClass::MONK && cClass != CreatureClass::WEREWOLF)
        return;

    computeAC();
    computeAttackPower();
}


bool Creature::mustRemainNeutral() {

    switch(getClass()) {
    case CreatureClass::DRUID:
        return(true);
        break;
    case CreatureClass::CLERIC:
        if (getDeity() == JAKAR || getDeity() == ARES)
            return(true);
        break;
    default:
        break;    
    }
    return(false);
}

bool Creature::mustRemainEvil() {

    switch(getClass()) {
    case CreatureClass::DEATHKNIGHT:
    case CreatureClass::LICH:
        return(true);
        break;
    case CreatureClass::CLERIC:
        if (deity == ARAMON || deity == ARACHNUS)
            return(true);
        break;   
    default:
        break;
    }
    return(false);
}

bool Creature::mustRamainGood() {
    switch(getClass()) {
    case CreatureClass::PALADIN:
    case CreatureClass::RANGER:
        return(true);
        break;
    case CreatureClass::CLERIC:
        if (deity == CERIS || deity == GRADIUS || deity == ENOCH ||
            deity == MARA || deity == LINOTHAN || deity == KAMIRA)
            return(true);
        break;   
    default:
        break;
    }
    return(false);
}

bool Creature::isGood() {
    return(getAdjustedAlignment() > NEUTRAL);
}

bool Creature::isEvil() {
    return(getAdjustedAlignment() < NEUTRAL);
}

//*********************************************************************
//                      alignInOrder
//*********************************************************************
// General alignment check for player class, deity, and/or races
// TODO: Add checks for races if necessary - no checks needed yet

bool Player::alignInOrder() const {

    switch(getClass()) {
    case CreatureClass::PALADIN:
        return(getAdjustedAlignment() > REDDISH);
    case CreatureClass::DEATHKNIGHT:
        return(getAdjustedAlignment() < BLUISH);
    case CreatureClass::DRUID:
        return(getAdjustedAlignment() > RED && getAdjustedAlignment() < BLUE);
    case CreatureClass::RANGER:
        return(getAdjustedAlignment() > BLOODRED);
    case CreatureClass::CLERIC:
        switch(deity) {
        case CERIS:
            return(getAdjustedAlignment() > REDDISH);
        case GRADIUS:
        case KAMIRA:
        case MARA:
        case LINOTHAN:
            return(getAdjustedAlignment() > RED);
        case ENOCH:
            return(getAdjustedAlignment() > REDDISH);
        case JAKAR:
            return(getAdjustedAlignment() > BLOODRED && getAdjustedAlignment() < ROYALBLUE);
        case ARES:
            return(getAdjustedAlignment() > RED && getAdjustedAlignment() < BLUE);
        case ARAMON:
        case ARACHNUS:
            return(getAdjustedAlignment() < BLUISH);
        default:
            break;
         }
    default:
        break;
    }
    
    return(true);
}

//*********************************************************************
//                      isAntiGradius
//*********************************************************************

bool Creature::isAntiGradius() const {
    return(antiGradius(race));
}

//*********************************************************************
//                      antiGradius
//*********************************************************************

bool antiGradius(int race) {
    return( race == ORC ||
            race == OGRE ||
            race == GOBLIN ||
            race == TROLL ||
            race == KOBOLD );
}


//********************************************************************
//                      adjustAlignment
//********************************************************************

void Player::adjustAlignment(std::shared_ptr<Monster> victim) {
    auto adjust = (short)(victim->getAlignment() / 8);

    if(victim->getAlignment() < 0 && victim->getAlignment() > -8)
        adjust = -1;
    if(victim->getAlignment() > 0 && victim->getAlignment() < 8)
        adjust = 1;

   // bool toNeutral = ((alignment > 0 && adjust > 0) ||
   //                  (alignment < 0 && adjust < 0) );

    bool hates = getAsCreature()->hatesEnemy(victim);

    if(hates && ((getAsCreature()->isGood() && victim->isGood()) ||
                 (getAsCreature()->isEvil() && victim->isEvil())) )
        hates=false;

    if (getAsCreature()->mustRemainNeutral() && hates)
        adjust=0; // no alignment change
    
    alignment -= adjust;
    alignment = std::max<short>(-1000, std::min<short>(1000, alignment));
}

//*********************************************************************
//                      cmdConvert
//*********************************************************************
// This function allows a player to convert from chaotic to lawful alignment.

int cmdConvert(const std::shared_ptr<Player>& player, cmd* cmnd) {
    if(!player->ableToDoCommand())
        return(0);


    if(!player->flagIsSet(P_CHAOTIC)) {
        *player << "You cannot convert. You're lawful.\n";
        return(0);
    } else if(player->getRace() == TIEFLING) {
        *player << "You cannot convert. Tieflings are required to be chaotic.\n";
        return(0);
    } else if(player->isNewVampire()) {
        *player << "You cannot convert. Vampires are required to be chaotic.\n";
        return(0);
    }

    if(cmnd->num < 2 || strcasecmp(cmnd->str[1], "yes") != 0) {
        *player << "To prevent accidental conversion you must confirm you want to convert.\n";
        *player << "To do so, you must type: 'convert yes'\n";
        *player << "\nRemember, you will NEVER be able to be chaotic again!\n";
        return(0);
    }

    if(player->isStaff()) 
        broadcast(isStaff, "^G### %s just converted to lawful alignment.", player->getCName());
    else
        broadcast("^G### %s just converted to lawful alignment.", player->getCName());

    logn("log.convert","%s converted to lawful.", player->getCName());
    player->clearFlag(P_CHAOTIC);

    if(player->getClass() == CreatureClass::CLERIC)
        player->clearFlag(P_PLEDGED);

    return(0);
}
