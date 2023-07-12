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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#pragma once

#include <list>
#include <functional>
#include <libxml/parser.h>  // for xmlNodePtr

#include "global.hpp"
#include "structs.hpp"

class SocialCommand: public Command {
public:
    SocialCommand(xmlNodePtr rootNode);
    SocialCommand(std::string_view pCmdStr) {
        name = pCmdStr;
    }
    ~SocialCommand() override = default;
    int execute(const std::shared_ptr<Creature>& player, cmd* cmnd) const override;

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
    std::function<int(const std::shared_ptr<Creature>& player, cmd* cmnd)> fn;

    std::string script;

    std::string selfNoTarget;
    std::string roomNoTarget;

    std::string selfOnTarget;
    std::string roomOnTarget;
    std::string victimOnTarget;

    std::string selfOnSelf;
    std::string roomOnSelf;

    bool wakeTarget{};
    bool rudeWakeTarget{};
    bool wakeRoom{};


};
