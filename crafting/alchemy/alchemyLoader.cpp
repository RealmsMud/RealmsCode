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
#include <map>


#include "config.hpp"
#include "alchemy.hpp"
#include "builders/alchemyBuilder.hpp"

void addToSet(AlchemyInfo &&alc, AlchemyMap &alchemyMap) {
    alchemyMap.emplace(alc.getName(), std::move(alc));
}

bool Config::loadAlchemy() {
 	addToSet(AlchemyBuilder()
		.name("fly")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("levitate")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("invisibility")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("detect-invisibility")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("breathe-water")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("strength")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("haste")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("fortitude")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("insight")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("prayer")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("detect-magic")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("protection")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("bless")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("true-sight")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("resist-magic")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("enlarge")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("reduce")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("blur")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("illusion")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("greater-invisibility")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("camouflage")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("pass-without-trace")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("know-aura")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("farsight")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("undead-ward")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("drain-shield")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("warmth")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("heat-protection")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("wind-protection")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("static-field")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("earth-shield")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("reflect-magic")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("fire-shield")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("armor")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("stonestkin")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("barkskin")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("darkness")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("infravision")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("comprehend-languages")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("tongues")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("resist-cold")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("resist-fire")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("resist-earth")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("resist-air")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("resist-water")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("resist-electric")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("courage")
		.positive(true)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("heal")
		.positive(true)
		.action("python")
		.throwable(false)
		.pythonScript("alchemyLib.potionHeal(actor)")
	, alchemy
    );
    addToSet(AlchemyBuilder()
         .name("harm")
         .positive(false)
         .action("python")
         .throwable(false)
         .pythonScript("alchemyLib.potionHarm(actor)") // This is wrong
    , alchemy
    );
	addToSet(AlchemyBuilder()
		.name("poison")
		.positive(false)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("disease")
		.positive(false)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("blindness")
		.positive(false)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("silence")
		.positive(false)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("slow")
		.positive(false)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("enfeeblement")
		.positive(false)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("weakness")
		.positive(false)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("feeblemind")
		.positive(false)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("damnation")
		.positive(false)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("deafness")
		.positive(false)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("vuln-cold")
		.positive(false)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("vuln-fire")
		.positive(false)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("vuln-earth")
		.positive(false)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("vuln-air")
		.positive(false)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("vuln-water")
		.positive(false)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("vuln-electric")
		.positive(false)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("vuln-crushing")
		.positive(false)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("vuln-chopping")
		.positive(false)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("vuln-piercing")
		.positive(false)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("vuln-slashing")
		.positive(false)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("vuln-exotic")
		.positive(false)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("vuln-ranged")
		.positive(false)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("porphyria")
		.positive(false)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("petrification")
		.positive(false)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("confusion")
		.positive(false)
		.action("effect")
		.throwable(false)
	, alchemy
    );
	addToSet(AlchemyBuilder()
		.name("fear")
		.positive(false)
		.action("effect")
		.throwable(false)
	, alchemy
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