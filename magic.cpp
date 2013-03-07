/*
 * magic.cpp
 *	 Additional spell-casting routines.
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  Creative Commons - Attribution - Non Commercial - Share Alike 3.0 License
 *    http://creativecommons.org/licenses/by-nc-sa/3.0/
 *
 * 	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include "mud.h"
#include "help.h"

#include <sstream>
#include <iomanip>
#include <locale>
#include <iostream>
#include <fstream>

// Spells with -1 mp will not check mp and will leave it up to
// the spell function to do this check.
// The order of this array is important! "splno" must equal the
// spot of the struct in the array. The mud will check this on
// startup and will crash if it's not properly defined
struct {
    const char	*splstr;
    int		splno;
    int		(*splfn)(SpellFn);
    int		mp;
    SchoolOfMagic school;
    DomainOfMagic domain;
} spllist[] = {
//	  Name					Spell Num				Spell Function				Mana		School of Magic
	{ "vigor",				S_VIGOR,				splVigor,					2,			NO_SCHOOL,			HEALING		},
	{ "zephyr",				S_ZEPHYR, 				(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "infravision",		S_INFRAVISION,			splInfravision,				-1,			TRANSMUTATION,		AUGMENTATION},
	{ "slow-poison",		S_SLOW_POISON,			splSlowPoison,				8,			NO_SCHOOL,			HEALING		},
	{ "bless",				S_BLESS,				splBless,					10,			ABJURATION,			PROTECTION	},
	{ "protection",			S_PROTECTION,			splProtection,				10,			ABJURATION,			PROTECTION	},
	{ "fireball",			S_FIREBALL, 			(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "invisibility",		S_INVISIBILITY,			splInvisibility,			15,			ILLUSION,			TRICKERY	},
	{ "restore",			S_RESTORE,				splRestore,					-1,			SCHOOL_CANNOT_CAST,	DOMAIN_CANNOT_CAST,	},
	{ "detect-invisibility",S_DETECT_INVISIBILITY,	splDetectInvisibility,		10,			DIVINATION,			KNOWLEDGE	},
	{ "detect-magic",		S_DETECT_MAGIC,			splDetectMagic,				10,			DIVINATION,			KNOWLEDGE	},
	{ "teleport",			S_TELEPORT,				splTeleport,				-1,			TRANSLOCATION,		DOMAIN_CANNOT_CAST	},
	{ "stun",				S_STUN,					splStun,					10,			ENCHANTMENT,		TRICKERY	},
	{ "cyclone",			S_CYCLONE, 				(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "atrophy",			S_ATROPHY, 				(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "enchant",			S_ENCHANT,				splEnchant,					25,			ENCHANTMENT,		DOMAIN_CANNOT_CAST	},
	{ "word-of-recall",		S_WORD_OF_RECALL,		splWordOfRecall,			25,			TRANSLOCATION,		TRAVEL		},
	{ "summon",				S_SUMMON,				splSummon,					28,			TRANSLOCATION,		TRAVEL		},
	{ "mend-wounds",		S_MEND_WOUNDS,			splMendWounds,				4,			NO_SCHOOL,			HEALING,	},
	{ "heal",				S_HEAL,					splHeal,					20,			SCHOOL_CANNOT_CAST,	HEALING,	},
	{ "track",				S_TRACK,				splTrack,					13,			SCHOOL_CANNOT_CAST,	NATURE		},
	{ "levitate",			S_LEVITATE,				splLevitate,				10,			TRANSMUTATION,		AUGMENTATION	},
	{ "heat-protection",	S_HEAT_PROTECTION,		splHeatProtection,			12,			ABJURATION,			PROTECTION	},
	{ "fly",				S_FLY,					splFly,						15,			TRANSMUTATION,		AUGMENTATION	},
	{ "resist-magic",		S_RESIST_MAGIC,			splResistMagic,				25,			ABJURATION,			DOMAIN_CANNOT_CAST	},
	{ "whirlwind",			S_WHIRLWIND, 			(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "rumble",				S_RUMBLE, 				(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "burn",				S_BURN, 				(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "blister",			S_BLISTER, 				(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "dustgust",			S_DUSTGUST, 			(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "waterbolt",			S_WATERBOLT, 			(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "crush",				S_CRUSH, 				(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "shatterstone",		S_ENGULF, 				(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "burstflame",			S_BURSTFLAME, 			(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "steamblast",			S_STEAMBLAST, 			(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "engulf",				S_SHATTERSTONE, 		(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "immolate",			S_IMMOLATE, 			(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "bloodboil",			S_BLOODBOIL, 			(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "tempest",			S_TEMPEST, 				(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "earth-tremor",		S_EARTH_TREMOR, 		(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "flamefill",			S_FLAMEFILL, 			(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "know-aura",			S_KNOW_AURA,			splKnowAura,				6,			DIVINATION,			KNOWLEDGE	},
	{ "remove-curse",		S_REMOVE_CURSE,			splRemoveCurse,				18,			ABJURATION,			GOOD		},
	{ "resist-cold",		S_RESIST_COLD,			splResistCold,				13,			ABJURATION,			PROTECTION	},
	{ "breathe-water",		S_BREATHE_WATER,		splBreatheWater,			12,			TRANSMUTATION,		AUGMENTATION},
	{ "earth-shield",		S_STONE_SHIELD,			splEarthShield,				12,			ABJURATION,			PROTECTION	},
	{ "clairvoyance",		S_CLAIRVOYANCE,			splClairvoyance,			15,			DIVINATION,			KNOWLEDGE	},
	{ "drain-exp",			S_DRAIN_EXP,			drain_exp,					-1,			SCHOOL_CANNOT_CAST,	DOMAIN_CANNOT_CAST	},
	{ "cure-disease",		S_CURE_DISEASE,			splCureDisease,				12,			SCHOOL_CANNOT_CAST,	HEALING,	},
	{ "cure-blindness",		S_CURE_BLINDNESS,		splCureBlindness,			15,			NO_SCHOOL,			HEALING,	},
	{ "fear",				S_FEAR,					splFear,					15,			ENCHANTMENT,		TRICKERY	},
	{ "room-vigor",			S_ROOM_VIGOR,			splRoomVigor,				8,			SCHOOL_CANNOT_CAST,	HEALING,	},
	{ "transport",			S_TRANSPORT,			splTransport,				-1,			TRANSLOCATION,		DOMAIN_CANNOT_CAST	},
	{ "blind",				S_BLINDNESS,			splBlind,					20,			TRANSMUTATION,		TRICKERY	},
	{ "silence",			S_SILENCE,				splSilence,					-1,			ENCHANTMENT,		TRICKERY	},
	{ "fortune",			S_FORTUNE,				splFortune,					12,			DIVINATION,			DOMAIN_CANNOT_CAST	},
	{ "curse",				S_CURSE,				splCurse,					25,			ENCHANTMENT,		DOMAIN_CANNOT_CAST	},
	{ "earthquake",			S_EARTHQUAKE, 			(int (*)(SpellFn))splMultiOffensive,-1,	EVOCATION,			DESTRUCTION	},
	{ "tsunami",			S_FLOOD, 				(int (*)(SpellFn))splMultiOffensive,-1,	EVOCATION,			DESTRUCTION	},
	{ "inferno",			S_FIRESTORM, 			(int (*)(SpellFn))splMultiOffensive,-1,	EVOCATION,			DESTRUCTION	},
	{ "hurricane",			S_HURRICANE, 			(int (*)(SpellFn))splMultiOffensive,-1,	EVOCATION,			DESTRUCTION	},
	{ "conjure",			S_CONJURE,				conjure,					-1,			CONJURATION,		CREATION	},
	{ "plane-shift",		S_PLANE_SHIFT,			splPlaneShift,				-1,			SCHOOL_CANNOT_CAST,	DOMAIN_CANNOT_CAST	},
	{ "judgement",			S_JUDGEMENT,			splJudgement,				-1,			SCHOOL_CANNOT_CAST,	DOMAIN_CANNOT_CAST	},
	{ "resist-water",		S_RESIST_WATER,			splResistWater,				13,			ABJURATION,			PROTECTION	},
	{ "resist-fire",		S_RESIST_FIRE,			splResistFire,				13,			ABJURATION,			PROTECTION	},
	{ "resist-air",			S_RESIST_AIR,			splResistAir,				13,			ABJURATION,			PROTECTION	},
	{ "resist-earth",		S_RESIST_EARTH,			splResistEarth,				13,			ABJURATION,			PROTECTION	},
	{ "reflect-magic",		S_REFLECT_MAGIC,		splReflectMagic,			30,			ABJURATION,			DOMAIN_CANNOT_CAST	},
	{ "animate-dead",		S_ANIMIATE_DEAD,		animate_dead,				-1,			NECROMANCY,			EVIL		},
	{ "annul-magic",		S_ANNUL_MAGIC,			splAnnulMagic,				30,			ABJURATION,			TRICKERY	},
	{ "true-sight",			S_TRUE_SIGHT,			splTrueSight,				18,			DIVINATION,			KNOWLEDGE	},
	{ "remove-fear",		S_REMOVE_FEAR,			splRemoveFear,				10,			ABJURATION,			TRICKERY	},
	{ "remove-silence",		S_REMOVE_SILENCE,		splRemoveSilence,			15,			ABJURATION,			TRICKERY	},
	{ "camouflage",			S_CAMOUFLAGE,			splCamouflage,				15,			ILLUSION,			NATURE		},
	{ "ethereal-travel",	S_ETHREAL_TRAVEL,		splEtherealTravel,			31,			TRANSLOCATION,		TRAVEL		},
	{ "drain-shield",		S_DRAIN_SHIELD,			splDrainShield,				15,			ABJURATION,			PROTECTION	},
	{ "dispel-evil",		S_DISPEL_EVIL,			splDispelEvil,				25,			SCHOOL_CANNOT_CAST,	GOOD		},
	{ "dispel-good",		S_DISPEL_GOOD,			splDispelGood,				25,			SCHOOL_CANNOT_CAST,	EVIL		},
	{ "undead-ward",		S_UNDEAD_WARD,			splUndeadWard,				25,			ABJURATION,			PROTECTION	},
	{ "zap",				S_ZAP, 					(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "lightning-bolt",		S_LIGHTNING_BOLT, 		(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "shockbolt",			S_SHOCKBOLT, 			(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "electrocution",		S_ELECTROCUTE, 			(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "thunderbolt",		S_THUNDERBOLT, 			(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "chain-lightning",	S_CHAIN_LIGHTNING, 		(int (*)(SpellFn))splMultiOffensive,-1,	EVOCATION,			DESTRUCTION	},
	{ "chill",				S_CHILL, 				(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "frostbite",			S_FROSTBITE, 			(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "sleet",				S_SLEET, 				(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "cold-cone",			S_COLD_CONE, 			(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "iceblade",			S_ICEBLADE, 			(int (*)(SpellFn))splOffensive,	-1,		EVOCATION,			DESTRUCTION	},
	{ "blizzard",			S_BLIZZARD, 			(int (*)(SpellFn))splMultiOffensive,-1,	EVOCATION,			DESTRUCTION	},
	{ "resist-electric",	S_RESIST_ELEC,			splResistLightning,			13,			ABJURATION,			PROTECTION	},
	{ "warmth",				S_WARMTH,				splWarmth,					12,			ABJURATION,			PROTECTION	},
	{ "cure-poison",		S_CURE_POISON,			splCurePoison,				10,			SCHOOL_CANNOT_CAST,	HEALING,	},
	{ "haste",				S_HASTE,				splHaste,					30,			TRANSMUTATION,		AUGMENTATION},
	{ "slow",				S_SLOW,					splSlow,					30,			TRANSMUTATION,		AUGMENTATION},
	{ "comprehend-languages",S_COMPREHEND_LANGUAGES,splComprehendLanguages,		20,			DIVINATION,			KNOWLEDGE	},
	{ "stone-to-flesh",		S_STONE_TO_FLESH,		splStoneToFlesh,			20,			TRANSMUTATION,		NO_DOMAIN	},
	{ "scare",				S_SCARE,				splScare,					25,			ENCHANTMENT,		TRICKERY	},
	{ "hold-person",		S_HOLD_PERSON,			splHoldPerson,				20,			ENCHANTMENT,		TRICKERY	},
	{ "entangle",			S_ENTANGLE,				splEntangle,				20,			TRANSMUTATION,		NATURE		},
	{ "anchor",				S_DIMENSIONAL_ANCHOR,	splDimensionalAnchor,		-1,			TRANSLOCATION,		DOMAIN_CANNOT_CAST	},
	{ "strength",			S_STRENGTH,				splStrength,				30,			TRANSMUTATION,		AUGMENTATION},
	{ "enfeeblement",		S_ENFEEBLEMENT,			splEnfeeblement,			30,			TRANSMUTATION,		AUGMENTATION},
	{ "armor",				S_ARMOR,				splArmor,					-1,			ABJURATION,			DOMAIN_CANNOT_CAST	},
	{ "stoneskin",			S_STONESKIN,			splStoneskin,				-1,			ABJURATION,			DOMAIN_CANNOT_CAST	},
	{ "free-action",		S_FREE_ACTION,			splFreeAction,				15,			ABJURATION,			PROTECTION	},
	{ "rejuvenate",			S_REJUVENATE,			splRejuvenate,				-1,			SCHOOL_CANNOT_CAST,	HEALING,	},
	{ "disintegrate",		S_DISINTEGRATE,			splDisintegrate,			50,			TRANSMUTATION,		DOMAIN_CANNOT_CAST	},
	{ "resurrect",			S_RESURRECT,			splResurrect,				-1,			SCHOOL_CANNOT_CAST,	HEALING,	},
	{ "courage",			S_COURAGE,				splCourage,					20,			ENCHANTMENT,		NO_DOMAIN	},
	{ "bloodfusion",		S_BLOODFUSION,			splBloodfusion,				-1,			SCHOOL_CANNOT_CAST,	EVIL,		},
	{ "magic-missile",		S_MAGIC_MISSILE,		splMagicMissile,			-1,			EVOCATION,			DOMAIN_CANNOT_CAST	},
	{ "knock",				S_KNOCK,				splKnock,					-1,			TRANSMUTATION,		TRICKERY	},
	{ "blink",				S_BLINK,				splBlink,					20,			TRANSLOCATION,		DOMAIN_CANNOT_CAST	},
	{ "harm",				S_HARM,					splHarm,					30,			SCHOOL_CANNOT_CAST,	EVIL		},
	{ "bind",				S_BIND,					splBind,					25,			TRANSLOCATION,		TRAVEL		},
	{ "darkness",			S_DARKNESS,				splDarkness,				-1,			EVOCATION,			TRICKERY	},
	{ "tongues",			S_TONGUES,				splTongues,					25,			DIVINATION,			KNOWLEDGE	},
	{ "enlarge",			S_ENLARGE,				splEnlarge,					15,			TRANSMUTATION,		AUGMENTATION},
	{ "reduce",				S_REDUCE,				splReduce,					15,			TRANSMUTATION,		AUGMENTATION},
	{ "insight",			S_INSIGHT,				splInsight,					30,			TRANSMUTATION,		AUGMENTATION},
	{ "feeblemind",			S_FEEBLEMIND,			splFeeblemind,				30,			TRANSMUTATION,		AUGMENTATION},
	{ "prayer",				S_PRAYER,				splPrayer,					30,			TRANSMUTATION,		AUGMENTATION},
	{ "damnation",			S_DAMNATION,			splDamnation,				30,			TRANSMUTATION,		AUGMENTATION},
	{ "fortitude",			S_FORTITUDE,			splFortitude,				30,			TRANSMUTATION,		AUGMENTATION},
	{ "weakness",			S_WEAKNESS,				splWeakness,				30,			TRANSMUTATION,		AUGMENTATION},
	{ "pass-without-trace",	S_PASS_WITHOUT_TRACE,	splPassWithoutTrace,		8,			TRANSMUTATION,		NATURE		},
	{ "farsight",			S_FARSIGHT,				splFarsight,				10,			DIVINATION,			KNOWLEDGE	},
	{ "wind-protection",	S_WIND_PROTECTION,		splWindProtection,			12,			ABJURATION,			PROTECTION	},
	{ "static-field",		S_STATIC_FIELD,			splStaticField,				12,			ABJURATION,			PROTECTION	},
	{ "portal",				S_PORTAL,				splPortal,					100,		TRANSLOCATION,		DOMAIN_CANNOT_CAST	},
	// anything after this line has been coded for the new magic system and needs player-testing
	{ "detect-hidden",		S_DETECT_HIDDEN,		splDetectHidden,			5,			DIVINATION,			KNOWLEDGE	},
	{ "illusion",			S_ILLUSION,				splIllusion,				25,			ILLUSION,			DOMAIN_CANNOT_CAST	},
	{ "deafness",			S_DEAFNESS,				splDeafness,				30,			TRANSMUTATION,		TRICKERY	},
	{ "radiation",			S_RADIATION,			splRadiation,				6,			ABJURATION,			DOMAIN_CANNOT_CAST	},
	{ "fiery-retribution",	S_FIERY_RETRIBUTION,	splFieryRetribution,		12,			ABJURATION,			DOMAIN_CANNOT_CAST	},
	{ "aura-of-flame",		S_AURA_OF_FLAME,		splAuraOfFlame,				18,			ABJURATION,			DOMAIN_CANNOT_CAST	},
	{ "barrier-of-combustion",S_BARRIER_OF_COMBUSTION,splBarrierOfCombustion,	24,			ABJURATION,			DOMAIN_CANNOT_CAST	},
 	{ "bounce-magic",		S_BOUNCE_MAGIC,			splBounceMagic,				10,			ABJURATION,			DOMAIN_CANNOT_CAST	},
 	{ "rebound-magic",		S_REBOUND_MAGIC,		splReboundMagic,			20,			ABJURATION,			DOMAIN_CANNOT_CAST	},
 	{ "dense-fog",			S_DENSE_FOG,			splDenseFog,				10,			CONJURATION,		NATURE		},
 	{ "wall-of-fire",		S_WALL_OF_FIRE,			splWallOfFire,				30,			CONJURATION,		CREATION	},
 	{ "wall-of-force",		S_WALL_OF_FORCE,		splWallOfForce,				50,			CONJURATION,		CREATION	},
 	{ "wall-of-thorns",		S_WALL_OF_THORNS,		splWallOfThorns,			30,			CONJURATION,		NATURE		},
	{ "greater-invisibility",S_GREATER_INVIS,		splGreaterInvisibility,		30,			ILLUSION,			DOMAIN_CANNOT_CAST	},
	{ "hallow",				S_HALLOW,				splHallow,					20,			SCHOOL_CANNOT_CAST,	GOOD		},
	{ "unhallow",			S_UNHALLOW,				splUnhallow,				20,			SCHOOL_CANNOT_CAST,	EVIL		},
	{ "toxic-cloud",		S_TOXIC_CLOUD,			splToxicCloud,				20,			CONJURATION,		DESTRUCTION	},
	{ "blur",				S_BLUR,					splBlur,					10,			ILLUSION,			TRICKERY	},
	{ "illusory-wall",		S_ILLUSORY_WALL,		splIllusoryWall,			24,			ILLUSION,			TRICKERY	},
	{ "globe-of-silence",	S_GLOBE_OF_SILENCE,		splGlobeOfSilence,			45,			ENCHANTMENT,		TRICKERY	},
	{ "cancel-magic",		S_CANCEL_MAGIC,			splCancelMagic,				15,			ABJURATION,			TRICKERY	},
	{ "dispel-magic",		S_DISPEL_MAGIC,			splDispelMagic,				20,			ABJURATION,			TRICKERY	},
	{ "sap-life",			S_SAP_LIFE,				splNecroDrain,				-1,			NECROMANCY,			EVIL		},
	{ "lifetap",			S_LIFETAP,				splNecroDrain,				-1,			NECROMANCY,			EVIL		},
	{ "lifedraw",			S_LIFEDRAW,				splNecroDrain,				-1,			NECROMANCY,			EVIL		},
	{ "draw-spirit",		S_DRAW_SPIRIT,			splNecroDrain,				-1,			NECROMANCY,			EVIL		},
	{ "siphon-life",		S_SIPHON_LIFE,			splNecroDrain,				-1,			NECROMANCY,			EVIL		},
	{ "spirit-strike",		S_SPIRIT_STRIKE,		splNecroDrain,				-1,			NECROMANCY,			EVIL		},
	{ "soulsteal",			S_SOULSTEAL,			splNecroDrain,				-1,			NECROMANCY,			EVIL		},
	{ "touch-of-kesh",		S_TOUCH_OF_KESH,		splNecroDrain,				-1,			NECROMANCY,			EVIL		},
	{ "regeneration",		S_REGENERATION,			splRegeneration,			40,			SCHOOL_CANNOT_CAST,	HEALING		},
	{ "well-of-magic",		S_WELLOFMAGIC,			splWellOfMagic,				40,			TRANSMUTATION,		HEALING		},
	{ "@",					-1,						0,							0,			NO_SCHOOL,			NO_DOMAIN	}
};
int spllist_size = sizeof(spllist)/sizeof(*spllist);


//**********************************************************************
//						writeSchoolDomainFiles
//**********************************************************************

void writeSchoolDomainFiles(MagicType type, int min, int max, const char* seeAlso) {
	char	spl[max][MAXSPELL][24], list[MAXSPELL][24];
	int		j[max], i=0, n=0, l = sizeof(list);
	bstring skill = "";
	char	filename[80];

	zero(j, sizeof(j));
	zero(spl, sizeof(spl));

	for(i = 0; i < get_spell_list_size() ; i++) {
		if(type == Divine)
			n = (int)get_spell_domain(i);
		else
			n = (int)get_spell_school(i);

		// If this spells belongs to a realm (air, fire), we need to skip it.
		// This is best handled by checking what function the spell uses to cast.
		if(	spllist[i].splfn == (int (*)(SpellFn))splOffensive ||
			spllist[i].splfn == (int (*)(SpellFn))splMultiOffensive
		)
			continue;

		strcpy(spl[n][j[n]++], get_spell_name(i));
	}

	// skip NO_SCHOOL/NO_DOMAIN and SCHOOL_NOT_CASTABLE/DOMAIN_NOT_CASTABLE
	for(n = min+1; n<max-2; n++) {
		// get the existing contents from the file
		if(type == Divine)
			skill = spellSkill((DomainOfMagic)n);
		else
			skill = spellSkill((SchoolOfMagic)n);

		if(skill == "")
			continue;


		// prepare to write the help file
		sprintf(filename, "%s%s.txt", Path::Help, skill.c_str());
		std::ofstream out(filename);
		out.setf(std::ios::left, std::ios::adjustfield);
		out.imbue(std::locale(""));

		out << Help::loadHelpTemplate(skill.c_str());

		// append all the spells for this school/domain to the helpfile
		out << "^WSpells:^x\n";

		// sort alphabetically
		memcpy(list, spl[n], l);
		qsort((void *) list, j[n], 24, (PFNCOMPARE) strcmp);

		// append
		for(i = 0; i < j[n]; i++) {
			out << "    " << list[i] << "\n";
		}

		// write file to disk
		out << "\nSee also:\n    HELP " << seeAlso << "\n\n";
		out.close();
	}
}

//**********************************************************************
//						writeSpellFiles
//**********************************************************************
// This creates spells.xml, which the Web Editor requests when it wants
// to know what spells it can use. It also updates the "sflags" helpfile
// for staff and the "spells" filefile for players.

bool Config::writeSpellFiles() const {
	xmlDocPtr	xmlDoc;
	xmlNodePtr	rootNode, curNode;
	char	filename[80];
	char	dmfile[100], dmfileLink[100];
	char	bhfile[100], bhfileLink[100];

	// Figure out pathing information for the helpfiles
	sprintf(dmfile, "%s/sflags.txt", Path::DMHelp);
	sprintf(dmfileLink, "%s/sflag.txt", Path::DMHelp);
	sprintf(bhfile, "%s/sflags.txt", Path::BuilderHelp);
	sprintf(bhfileLink, "%s/sflag.txt", Path::BuilderHelp);

	xmlDoc = xmlNewDoc(BAD_CAST "1.0");
	rootNode = xmlNewDocNode(xmlDoc, NULL, BAD_CAST "Spells", NULL);
	xmlDocSetRootElement(xmlDoc, rootNode);

	// set left aligned
	std::ofstream out(dmfile);
	out.setf(std::ios::left, std::ios::adjustfield);
	out.imbue(std::locale(""));
	out << "^WSpell Flags^x\n";

	for(int i=0 ; i < spllist_size-1; i++) {
		curNode = xml::newStringChild(rootNode, "Spell", spllist[i].splstr);
		xml::newNumProp(curNode, "Id", i+1);

		out << " " << std::setw(10) << (i+1) << std::setw(20) << spllist[i].splstr << "\n";
	}

	sprintf(filename, "%s/spells.xml", Path::Code);
	xml::saveFile(filename, xmlDoc);
	xmlFreeDoc(xmlDoc);

	out << "\n";
	out.close();
	link(dmfile, dmfileLink);
	link(dmfile, bhfile);
	link(dmfile, bhfileLink);

	// write help files for each school and domain
	writeSchoolDomainFiles(Arcane, (int)MIN_SCHOOL, (int)MAX_SCHOOL, "SCHOOLS");
	writeSchoolDomainFiles(Divine, (int)MIN_DOMAIN, (int)MAX_DOMAIN, "DOMAINS");

	return(true);
}

//**********************************************************************
//						initSpellList
//**********************************************************************
// Make sure someone hasn't messed up the spell table :)

void initSpellList() {
	for(int i=0 ; i<spllist_size; i++) {
		if(spllist[i].splno != i) {
			if(spllist[i].splno != -1) {
				printf("Error: Spell list mismatch: %d != %d\n", i, spllist[i].splno);
				abort();
			}
		}
	}
}

//*********************************************************************
//						get_spell_name
//*********************************************************************

const char *get_spell_name(int nIndex) {
    // do bounds checking
    ASSERTLOG(nIndex >= 0);
    ASSERTLOG(nIndex < spllist_size);

    nIndex = MAX(0, MIN(nIndex, spllist_size));
    return( spllist[nIndex].splstr );
}


//*********************************************************************
//						get_spell_num()
//*********************************************************************

int get_spell_num(int nIndex) {
    // do bounds checking
    ASSERTLOG(nIndex >= 0);
    ASSERTLOG(nIndex < spllist_size);

    nIndex = MAX(0, MIN(nIndex, spllist_size));
    return(spllist[nIndex].splno);
}

//*********************************************************************
//						get_spell_lvl
//*********************************************************************
// this also serves as a list of offensive spells to cast during combat
// (for pets - see Monster::castSpell)

int get_spell_lvl(int sflag) {
	int slvl=0;

	ASSERTLOG(sflag >= 0 );
	ASSERTLOG(sflag < spllist_size );

	switch(sflag) {
	case S_RUMBLE:
	case S_ZEPHYR:
	case S_BURN:
	case S_BLISTER:
	case S_ZAP:
	case S_CHILL:
	case S_SAP_LIFE:
	case S_LIFETAP:
		slvl = 1;
		break;
	case S_CRUSH:
	case S_DUSTGUST:
	case S_FIREBALL:
	case S_WATERBOLT:
	case S_LIGHTNING_BOLT:
	case S_FROSTBITE:
	case S_LIFEDRAW:
		slvl = 2;
		break;
	case S_ENGULF:
	case S_WHIRLWIND:
	case S_BURSTFLAME:
	case S_STEAMBLAST:
	case S_SHOCKBOLT:
	case S_SLEET:
	case S_DRAW_SPIRIT:
		slvl = 3;
		break;
	case S_SHATTERSTONE:
	case S_CYCLONE:
	case S_IMMOLATE:
	case S_BLOODBOIL:
	case S_ELECTROCUTE:
	case S_COLD_CONE:
	case S_SIPHON_LIFE:
		slvl = 4;
		break;
	case S_EARTH_TREMOR:
	case S_TEMPEST:
	case S_FLAMEFILL:
	case S_ATROPHY:
	case S_THUNDERBOLT:
	case S_ICEBLADE:
	case S_SPIRIT_STRIKE:
		slvl = 5;
		break;
	case S_EARTHQUAKE:
	case S_HURRICANE:
	case S_FIRESTORM:
	case S_FLOOD:
	case S_CHAIN_LIGHTNING:
	case S_BLIZZARD:
	case S_SOULSTEAL:
	case S_TOUCH_OF_KESH:
		slvl = 6;
		break;
	default:
		slvl = 0;
		break;
	}

	return(slvl);
}

//*********************************************************************
//						get_spell_function
//*********************************************************************

SpellRet get_spell_function(int nIndex) {
    // do bounds checking
    ASSERTLOG(nIndex >= 0);
    ASSERTLOG(nIndex < spllist_size);

    nIndex = MAX(0, MIN(nIndex, spllist_size));

    return( spllist[nIndex].splfn );
}

//*********************************************************************
//						get_spell_school
//*********************************************************************

SchoolOfMagic get_spell_school(int nIndex) {
    // do bounds checking
    ASSERTLOG(nIndex >= 0);
    ASSERTLOG(nIndex < spllist_size);

    nIndex = MAX(0, MIN(nIndex, spllist_size));

    return( spllist[nIndex].school );
}

//*********************************************************************
//						get_spell_school
//*********************************************************************

DomainOfMagic get_spell_domain(int nIndex) {
    // do bounds checking
    ASSERTLOG(nIndex >= 0);
    ASSERTLOG(nIndex < spllist_size);

    nIndex = MAX(0, MIN(nIndex, spllist_size));

    return( spllist[nIndex].domain );
}

//*********************************************************************
//						get_spell_list_size
//*********************************************************************

int get_spell_list_size() {
    return(spllist_size);
}

//*********************************************************************
//						getSpellMp
//*********************************************************************
// Parameters:  <splNo>
int getSpellMp(int spellNum) {
    // do bounds checking
    ASSERTLOG(spellNum >= 0);
    ASSERTLOG(spellNum < spllist_size);

	return(spllist[spellNum].mp);
}


//*********************************************************************
//						checkMp
//*********************************************************************
// Parameters:  <target> The player casting
//				<reqMp>  The required amount of mp
// Return value: <true> - Has enough mp
//				 <false> - Doesn't have enough mp

bool Creature::checkMp(int reqMp) {
	if(getClass() != LICH && mp.getCur() < reqMp) {
		if(!isPet())
			print("You need %d magic points to cast that spell.\n", reqMp);
		else
			print("%M needs %d magic points to cast that spell.\n", this, reqMp);
		return(false);
	} else if(getClass() == LICH && hp.getMax() <= reqMp) {
		if(!isPet())
			print("You are not experienced enough to cast that spell.\n");
		else
			print("%M is not experienced enough to cast that spell.\n", this);
		return(false);
	} else if(getClass() == LICH && hp.getCur() <= reqMp) {
		if(!isPet())
			print("Sure, and kill yourself in the process?\n");
		else
			print("Sure, and have %N kill %sself in the process?\n", this, himHer());
		return(false);
	}
	return(true);
}

//*********************************************************************
//						subMp
//*********************************************************************
// Parameters:  <target> The player casting
//				<reqMp>  The amount of mp to deduct

void Creature::subMp(int reqMp) {
	if(getClass() == LICH) {
		hp.decrease(reqMp);
	} else {
		mp.decrease(reqMp);
	}
}

//*********************************************************************
//						splLevitate
//*********************************************************************

bool checkRefusingMagic(Creature* player, Creature* target, bool healing, bool print) {
	if(player->isStaff() || !target->isPlayer())
		return(false);
	// linkdead players always want healing
	if(!healing && target->flagIsSet(P_LINKDEAD)) {
		if(print)
			player->print("%M doesn't want that cast on them right now.\n", target);
		return(0);
	}
	if(target->getAsPlayer()->isRefusing(player->getName())) {
		if(print)
			player->print("%M is refusing your magical services.\n", target);
		return(true);
	}
	return(false);
}
