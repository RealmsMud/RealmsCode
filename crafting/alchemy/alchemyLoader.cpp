/*
 * alchemyLoader.cpp
 *   Load all effectgs
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
#include <map>                          // for allocator
#include <utility>                      // for move

#include "alchemy.hpp"                  // for AlchemyInfo
#include "builders/alchemyBuilder.hpp"  // for AlchemyBuilder
#include "config.hpp"                   // for Config, AlchemyMap

void addToMap(AlchemyInfo &&alc, AlchemyMap &alchemyMap) {
    alchemyMap.emplace(alc.getName(), std::move(alc));
}

bool Config::loadAlchemy() {
    addToMap(AlchemyBuilder()
        .name("fly")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("levitate")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("invisibility")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("detect-invisibility")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("breathe-water")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("strength")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("haste")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("fortitude")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("insight")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("prayer")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("detect-magic")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("protection")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("bless")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("true-sight")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("resist-magic")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("enlarge")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("reduce")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("blur")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("illusion")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("greater-invisibility")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("camouflage")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("pass-without-trace")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("know-aura")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("farsight")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("undead-ward")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("drain-shield")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("warmth")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("heat-protection")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("wind-protection")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("static-field")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("earth-shield")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("reflect-magic")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("fire-shield")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("armor")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("stonestkin")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("barkskin")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("darkness")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("infravision")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("comprehend-languages")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("tongues")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("resist-cold")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("resist-fire")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("resist-earth")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("resist-air")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("resist-water")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("resist-electric")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("courage")
        .positive(true)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("heal")
        .positive(true)
        .action("python")
        .throwable(false)
        .pythonScript("alchemyLib.potionHeal(actor)"), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("harm")
        .positive(false)
        .action("python")
        .throwable(false)
        .pythonScript("alchemyLib.potionHarm(actor)") // This is wrong
            , alchemy
    );
    addToMap(AlchemyBuilder()
        .name("poison")
        .positive(false)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("disease")
        .positive(false)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("blindness")
        .positive(false)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("silence")
        .positive(false)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("slow")
        .positive(false)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("enfeeblement")
        .positive(false)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("weakness")
        .positive(false)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("feeblemind")
        .positive(false)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("damnation")
        .positive(false)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("deafness")
        .positive(false)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("vuln-cold")
        .positive(false)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("vuln-fire")
        .positive(false)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("vuln-earth")
        .positive(false)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("vuln-air")
        .positive(false)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("vuln-water")
        .positive(false)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("vuln-electric")
        .positive(false)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("vuln-crushing")
        .positive(false)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("vuln-chopping")
        .positive(false)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("vuln-piercing")
        .positive(false)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("vuln-slashing")
        .positive(false)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("vuln-exotic")
        .positive(false)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("vuln-ranged")
        .positive(false)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("porphyria")
        .positive(false)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("petrification")
        .positive(false)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("confusion")
        .positive(false)
        .action("effect")
        .throwable(false), alchemy
    );
    addToMap(AlchemyBuilder()
        .name("fear")
        .positive(false)
        .action("effect")
        .throwable(false), alchemy
    );

    // This needs a python script!
//	addToSet(AlchemyBuilder()
//		.name("knock")
//		.positive(true)
//		.action("python")
//		.throwable(true)
//	, alchemy
//    );

    return true;
}