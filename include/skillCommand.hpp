/*
 * skills.h
 *   Header file for skills
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

#include "skills.hpp"

#include <functional>

class SkillCost {
public:
    ResourceType resource;  // What type of resource
    int cost;               // How much of the resource

};

//**********************************************************************
// SkillCommand - Subclass of SkillInfo - A skill that is a command
//**********************************************************************

class SkillCommand : public virtual SkillInfo, public virtual Command {
    friend class Config;
public:
    SkillCommand();
    SkillCommand(std::string_view pCmdStr) {
        name = pCmdStr;
    }
    int execute(const std::shared_ptr<Creature>& player, cmd* cmnd) const;

protected:
    TargetType targetType;            // What sort of target?
    bool offensive{};                 // Is this an offensive skill? Default: Yes         // *

    bool usesAttackTimer{};           // Delay/cooldown is also affected by the attack timer (True by default)
    int cooldown{};                   // Delay/cooldown on this skill * 10.  (10 = 1.0s delay)
    int failCooldown{};               // Delay/cooldown on this skill on failure
    std::list<SkillCost> resources;   // Resources this skill uses
    std::string pyScript;             // Python script for this skillCommand

    std::list<std::string> aliases;

private:
    std::function<int(const std::shared_ptr<Creature>& creature, cmd* cmnd)> fn{nullptr};

public:
    bool checkResources(std::shared_ptr<Creature> creature) const;
    void subResources(std::shared_ptr<Creature> creature);

    TargetType getTargetType() const;
    bool isOffensive() const;
    bool runScript(std::shared_ptr<Creature> actor, std::shared_ptr<MudObject> target, Skill* skill) const;

    bool getUsesAttackTimer() const;
    bool hasCooldown() const;
    int getCooldown() const;
    int getFailCooldown() const;

};
