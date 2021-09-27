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
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef SOCIAL_H_
#define SOCIAL_H_

#include <list>
#include <libxml/parser.h>  // for xmlNodePtr

#include "global.hpp"
#include "structs.hpp"

class SocialCommand: public Command {
public:
    explicit SocialCommand(xmlNodePtr rootNode);
    SocialCommand(std::string_view pCmdStr) {
        name = pCmdStr;
    }
    bool saveToXml(xmlNodePtr rootNode) const;
    int execute(Creature* player, cmd* cmnd) const;

    [[nodiscard]] bool getWakeTarget() const;
    [[nodiscard]] bool getRudeWakeTarget() const;
    [[nodiscard]] bool getWakeRoom() const;

    [[nodiscard]] std::string_view getSelfNoTarget() const;
    [[nodiscard]] std::string_view getRoomNoTarget() const;

    [[nodiscard]] std::string_view getSelfOnTarget() const;
    [[nodiscard]] std::string_view getRoomOnTarget() const;
    [[nodiscard]] std::string_view getVictimOnTarget() const;

    [[nodiscard]] std::string_view getSelfOnSelf() const;
    [[nodiscard]] std::string_view getRoomOnSelf() const;

private:
    int (*fn)(Creature* player, cmd* cmnd){};
    bstring script;

    bstring selfNoTarget;
    bstring roomNoTarget;

    bstring selfOnTarget;
    bstring roomOnTarget;
    bstring victimOnTarget;

    bstring selfOnSelf;
    bstring roomOnSelf;

    bool wakeTarget{};
    bool rudeWakeTarget{};
    bool wakeRoom{};


};


#endif /* SOCIAL_H_ */
