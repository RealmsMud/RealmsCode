/*
 * creationSession.hpp
 *   Per-socket wizard cursor; choices live in CharacterDraft.
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

struct CreationSession {
    enum class Step {
        DmPassword, // DM account: verify the DM password before creating
        Race,
        SubRace,
        Sex,
        Class,
        Deity,
        Location,
        StatsChoice, // roll-your-own vs class predefined
        Stats,
        BonusStat,
        PenaltyStat,
        Prof,
        SecondProf,
        Name,
        Done, // policy screen shown; next input commits the character
    };
    Step step = Step::Race;
};
