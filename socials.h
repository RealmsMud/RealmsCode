/*
 * socials.h
 *   Class for social(s)
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

#ifndef SOCIAL_H_
#define SOCIAL_H_

#include <list>

#include "common.h"
#include "global.h"
#include "structs.h"

class SocialCommand: public Command {
public:
    SocialCommand(xmlNodePtr rootNode);
    bool saveToXml(xmlNodePtr rootNode) const;
    int execute(Creature* player, cmd* cmnd);

    bool getWakeTarget() const;
    bool getRudeWakeTarget() const;
    bool getWakeRoom() const;

    const bstring& getSelfNoTarget() const;
    const bstring& getRoomNoTarget() const;

    const bstring& getSelfOnTarget() const;
    const bstring& getRoomOnTarget() const;
    const bstring& getVictimOnTarget() const;

    const bstring& getSelfOnSelf() const;
    const bstring& getRoomOnSelf() const;

private:
    int (*fn)(Creature* player, cmd* cmnd);
    bstring script;

    bstring selfNoTarget;
    bstring roomNoTarget;

    bstring selfOnTarget;
    bstring roomOnTarget;
    bstring victimOnTarget;

    bstring selfOnSelf;
    bstring roomOnSelf;

    bool wakeTarget;
    bool rudeWakeTarget;
    bool wakeRoom;


};


#endif /* SOCIAL_H_ */
