/*
 * magic.h
 *   Header file for magic
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2016 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#ifndef MAGIC_H_
#define MAGIC_H_

#include <list>

#include "global.h"
#include "realm.h"
#include "structs.h"

enum MagicType {
    Arcane,
    Divine,
    NO_MAGIC_TYPE
};

enum SchoolOfMagic {
    MIN_SCHOOL,

    ABJURATION,
    CONJURATION,
    DIVINATION,
    ENCHANTMENT,
    EVOCATION,
    ILLUSION,
    NECROMANCY,
    TRANSLOCATION,
    TRANSMUTATION,

    NO_SCHOOL,

    SCHOOL_CANNOT_CAST,
    MAX_SCHOOL
};

enum DomainOfMagic {
    MIN_DOMAIN,

    HEALING,
    DESTRUCTION,
    EVIL,
    GOOD,
    KNOWLEDGE,
    PROTECTION,
    NATURE,
    AUGMENTATION,
    TRICKERY,
    TRAVEL,
    CREATION,

    NO_DOMAIN,

    DOMAIN_CANNOT_CAST,
    MAX_DOMAIN
};

class SpellData {
public:
    SpellData();
    CastType how;
    int splno;
    unsigned int level;
    Object* object;
    SchoolOfMagic school;
    DomainOfMagic domain;
    bstring skill;

    void set(CastType h, SchoolOfMagic s, DomainOfMagic d, Object* obj, const Creature* caster);
    bool check(const Creature* player, bool skipKnowCheck=false) const;
};

bstring spellSkill(SchoolOfMagic school);
bstring spellSkill(DomainOfMagic domain);
bstring realmSkill(Realm realm);


// Spell flags
#define S_VIGOR                 0 // vigor
#define S_ZEPHYR                1 // zephyr
#define S_INFRAVISION           2 // infravision
#define S_SLOW_POISON           3 // slow-poison
#define S_BLESS                 4 // bless
#define S_PROTECTION            5 // protection
#define S_FIREBALL              6 // fireball
#define S_INVISIBILITY          7 // invisibility
#define S_RESTORE               8 // restore
#define S_DETECT_INVISIBILITY   9 // detect-invisibility
#define S_DETECT_MAGIC          10 // detect-magic
#define S_TELEPORT              11 // teleport
#define S_STUN                  12 // stun
#define S_CYCLONE               13 // cyclone
#define S_ATROPHY               14 // iceblade
#define S_ENCHANT               15 // enchant
#define S_WORD_OF_RECALL        16 // word-of-recall
#define S_SUMMON                17 // summon
#define S_MEND_WOUNDS           18 // mend-wounds
#define S_HEAL                  19 // heal
#define S_TRACK                 20 // track
#define S_LEVITATE              21 // levitation
#define S_HEAT_PROTECTION       22 // heat-protection
#define S_FLY                   23 // fly
#define S_RESIST_MAGIC          24 // resist-magic
#define S_WHIRLWIND             25 // whirlwind
#define S_RUMBLE                26 // rumble
#define S_BURN                  27 // burn
#define S_BLISTER               28 // blister
#define S_DUSTGUST              29 // dustgust
#define S_WATERBOLT             30 // waterbolt
#define S_CRUSH                 31 // stonecrush
#define S_ENGULF                32 // engulf
#define S_BURSTFLAME            33 // burstflame
#define S_STEAMBLAST            34 // steamblast
#define S_SHATTERSTONE          35 // shatterstone
#define S_IMMOLATE              36 // immolate
#define S_BLOODBOIL             37 // bloodboil
#define S_TEMPEST               38 // thunderbolt
#define S_EARTH_TREMOR          39 // earthquake
#define S_FLAMEFILL             40 // flamefill
#define S_KNOW_AURA             41 // know-alignment
#define S_REMOVE_CURSE          42 // remove-curse
#define S_RESIST_COLD           43 // resist-cold
#define S_BREATHE_WATER         44 // breathe water
#define S_STONE_SHIELD          45 // stone shield
#define S_CLAIRVOYANCE          46 // locate player
#define S_DRAIN_EXP             47 // drain energy (exp)
#define S_CURE_DISEASE          48 // cure disease
#define S_CURE_BLINDNESS        49 // cure blindess
#define S_FEAR                  50 // fear
#define S_ROOM_VIGOR            51 // room vigor
#define S_TRANSPORT             52 // item transport
#define S_BLINDNESS             53 // cause blindness
#define S_SILENCE               54 // cause silence
#define S_FORTUNE               55 // fortune
#define S_CURSE                 56 // curse
#define S_EARTHQUAKE            57 // quake spell
#define S_FLOOD                 58 // flood spell
#define S_FIRESTORM             59 // firestorm spell
#define S_HURRICANE             60 // hurricane spell
#define S_CONJURE               61 // summon elemental spell
#define S_PLANE_SHIFT           62 // plane shift
#define S_JUDGEMENT             63 // word-of-judgement
#define S_RESIST_WATER          64 // Resist-Water
#define S_RESIST_FIRE           65 // Resist-Fire
#define S_RESIST_AIR            66 // Resist-Air
#define S_RESIST_EARTH          67 // Resist-Earth
#define S_REFLECT_MAGIC         68 // Fire-Shield
#define S_ANIMIATE_DEAD         69 // Animate-Dead
#define S_ANNUL_MAGIC           70 // Annul-Magic
#define S_TRUE_SIGHT            71 // True-sight
#define S_REMOVE_FEAR           72 // Remove-fear
#define S_REMOVE_SILENCE        73 // Remove-silence
#define S_CAMOUFLAGE            74 // Camouflage
#define S_ETHREAL_TRAVEL        75 // Ethereal-travel
#define S_DRAIN_SHIELD          76 // Drain-shield
#define S_DISPEL_EVIL           77 // Dispel-evil
#define S_DISPEL_GOOD           78 // Dispel-good
#define S_UNDEAD_WARD           79 // undead-ward
// New spell realms *****************
#define S_ZAP                   80 // Zap
#define S_LIGHTNING_BOLT        81 // Lightning-bolt
#define S_SHOCKBOLT             82 // Shockbolt
#define S_ELECTROCUTE           83 // Electrocute
#define S_THUNDERBOLT           84 // Thunderbolt
#define S_CHAIN_LIGHTNING       85 // Chain-Lightning
#define S_CHILL                 86 // Chill
#define S_FROSTBITE             87 // Frostbite
#define S_SLEET                 88 // Sleet
#define S_COLD_CONE             89 // Cold-cone
#define S_ICEBLADE              90 // Iceblade
#define S_BLIZZARD              91 // Blizzard
//************************************
#define S_RESIST_ELEC           92 // Resist-lightning
#define S_WARMTH                93 // Warmth
#define S_CURE_POISON           94 // Curepoison
#define S_HASTE                 95 // Haste
#define S_SLOW                  96 // Slow
#define S_COMPREHEND_LANGUAGES  97 // Comprehend-Languages
#define S_STONE_TO_FLESH        98 // Stone-to-Flesh
#define S_SCARE                 99 // Scare
#define S_HOLD_PERSON           100 // hold-person
#define S_ENTANGLE              101 // entangle
#define S_DIMENSIONAL_ANCHOR    102 // dimensional-anchor
#define S_STRENGTH              103 // strength spell
#define S_ENFEEBLEMENT          104 // enfeeblement spell
#define S_ARMOR                 105 // armor spell
#define S_STONESKIN             106 // stoneskin spell
#define S_FREE_ACTION           107 // free-action spell
#define S_REJUVENATE            108 // rejuvinate spell
#define S_DISINTEGRATE          109 // disintegrate spell
#define S_RESURRECT             110 // resurrect spell
#define S_COURAGE               111 // courage spell

#define S_BLOODFUSION           112 // bloodfusion spell
#define S_MAGIC_MISSILE         113 // magic-missile spell
#define S_KNOCK                 114 // knock spell
#define S_BLINK                 115 // blink spell
#define S_HARM                  116 // harm spell
#define S_BIND                  117 // bind spell
#define S_DARKNESS              118 // darkness spell
#define S_TONGUES               119 // tongues spell
#define S_ENLARGE               120 // enlarge spell
#define S_REDUCE                121 // reduce spell

#define S_INSIGHT               122 // insight spell
#define S_FEEBLEMIND            123 // feeblemind spell
#define S_PRAYER                124 // prayer spell
#define S_DAMNATION             125 // damnation spell
#define S_FORTITUDE             126 // fortitude spell
#define S_WEAKNESS              127 // weakness spell
#define S_PASS_WITHOUT_TRACE    128 // pass-without-trace spell
#define S_FARSIGHT              129 // farsight spell
#define S_WIND_PROTECTION       130 // wind-protection spell
#define S_STATIC_FIELD          131 // static-field spell
#define S_PORTAL                132 // portal spell
// anything after this line has been coded for the new magic system and needs player-testing
#define S_DETECT_HIDDEN         133 // detect-hidden spell
#define S_ILLUSION              134 // illusion spell
#define S_DEAFNESS              135 // deafness spell
#define S_RADIATION             136 // radiation spell
#define S_FIERY_RETRIBUTION     137 // fiery retribution spell
#define S_AURA_OF_FLAME         138 // aura of flame spell
#define S_BARRIER_OF_COMBUSTION 139 // barier of combustion spell
#define S_BOUNCE_MAGIC          140 // bounce magic spell
#define S_REBOUND_MAGIC         141 // rebound magic spell
#define S_DENSE_FOG             142 // dense fog spell
#define S_WALL_OF_FIRE          143 // wall-of-fire spell
#define S_WALL_OF_FORCE         144 // wall-of-force spell
#define S_WALL_OF_THORNS        145 // wall-of-thorns spell
#define S_GREATER_INVIS         146 // greater-invisibility spell
#define S_HALLOW                147 // hallow spell
#define S_UNHALLOW              148 // unhallow spell
#define S_TOXIC_CLOUD           149 // toxic cloud spell
#define S_BLUR                  150 // blur spell
#define S_ILLUSORY_WALL         151 // illusory wall spell
#define S_GLOBE_OF_SILENCE      152 // globe of silence spell
#define S_CANCEL_MAGIC          153 // cancel-magic spell
#define S_DISPEL_MAGIC          154 // dispel-magic spell
#define S_SAP_LIFE              155 // sap-life spell
#define S_LIFETAP               156 // lifetap spell
#define S_LIFEDRAW              157 // lifedraw spell
#define S_DRAW_SPIRIT           158 // draw-spirit spell
#define S_SIPHON_LIFE           159 // siphon-life spell
#define S_SPIRIT_STRIKE         160 // spirit-strike spell
#define S_SOULSTEAL             161 // soulsteal spell
#define S_TOUCH_OF_KESH         162 // touch-of-kesh spell
#define S_REGENERATION          163 // regeneration spell
#define S_WELLOFMAGIC           164 // well-of-magic spell


#define MAXSPELL                165 // Increment when you add a spell


#define SpellFn Creature*, cmd*, SpellData*
typedef int (*SpellRet)(SpellFn);

SpellRet get_spell_function(int nIndex);
SchoolOfMagic get_spell_school(int nIndex);
DomainOfMagic get_spell_domain(int nIndex);


// spells
int splAnnulMagic(Creature* player, cmd* cmnd, SpellData* spellData);
int splArmor(Creature* player, cmd* cmnd, SpellData* spellData);
int splAuraOfFlame(Creature* player, cmd* cmnd, SpellData* spellData);
int splBarrierOfCombustion(Creature* player, cmd* cmnd, SpellData* spellData);
int splBind(Creature* player, cmd* cmnd, SpellData* spellData);
int splBless(Creature* player, cmd* cmnd, SpellData* spellData);
int splBlind(Creature* player, cmd* cmnd, SpellData* spellData);
int splBlink(Creature* player, cmd* cmnd, SpellData* spellData);
int splBloodfusion(Creature* player, cmd* cmnd, SpellData* spellData);
int splBlur(Creature* player, cmd* cmnd, SpellData* spellData);
int splBounceMagic(Creature* player, cmd* cmnd, SpellData* spellData);
int splBreatheWater(Creature* player, cmd* cmnd, SpellData* spellData);
int splCamouflage(Creature* player, cmd* cmnd, SpellData* spellData);
int splCancelMagic(Creature* player, cmd* cmnd, SpellData* spellData);
int splClairvoyance(Creature* player, cmd* cmnd, SpellData* spellData);
int splComprehendLanguages(Creature* player, cmd* cmnd, SpellData* spellData);
int splCourage(Creature* player, cmd* cmnd, SpellData* spellData);
int splCureBlindness(Creature* player, cmd* cmnd, SpellData* spellData);
int splCureDisease(Creature* player, cmd* cmnd, SpellData* spellData);
int splCurePoison(Creature* player, cmd* cmnd, SpellData* spellData);
int splCurse(Creature* player, cmd* cmnd, SpellData* spellData);
int splDamnation(Creature* player, cmd* cmnd, SpellData* spellData);
int splDarkness(Creature* player, cmd* cmnd, SpellData* spellData);
int splDeafness(Creature* player, cmd* cmnd, SpellData* spellData);
int splDenseFog(Creature* player, cmd* cmnd, SpellData* spellData);
int splDetectHidden(Creature* player, cmd* cmnd, SpellData* spellData);
int splDetectInvisibility(Creature* player, cmd* cmnd, SpellData* spellData);
int splDetectMagic(Creature* player, cmd* cmnd, SpellData* spellData);
int splDimensionalAnchor(Creature* player, cmd* cmnd, SpellData* spellData);
int splDisintegrate(Creature* player, cmd* cmnd, SpellData* spellData);
int splDispelEvil(Creature* player, cmd* cmnd, SpellData* spellData);
int splDispelGood(Creature* player, cmd* cmnd, SpellData* spellData);
int splDispelMagic(Creature* player, cmd* cmnd, SpellData* spellData);
int splDrainShield(Creature* player, cmd* cmnd, SpellData* spellData);
int splEarthShield(Creature* player, cmd* cmnd, SpellData* spellData);
int splEnchant(Creature* player, cmd* cmnd, SpellData* spellData);
int splEnfeeblement(Creature* player, cmd* cmnd, SpellData* spellData);
int splEnlarge(Creature* player, cmd* cmnd, SpellData* spellData);
int splEntangle(Creature* player, cmd* cmnd, SpellData* spellData);
int splEtherealTravel(Creature* player, cmd* cmnd, SpellData* spellData);
int splFarsight(Creature* player, cmd* cmnd, SpellData* spellData);
int splFear(Creature* player, cmd* cmnd, SpellData* spellData);
int splFeeblemind(Creature* player, cmd* cmnd, SpellData* spellData);
int splFieryRetribution(Creature* player, cmd* cmnd, SpellData* spellData);
int splFly(Creature* player, cmd* cmnd, SpellData* spellData);
int splFortitude(Creature* player, cmd* cmnd, SpellData* spellData);
int splFortune(Creature* player, cmd* cmnd, SpellData* spellData);
int splFreeAction(Creature* player, cmd* cmnd, SpellData* spellData);
int splGlobeOfSilence(Creature* player, cmd* cmnd, SpellData* spellData);
int splGreaterInvisibility(Creature* player, cmd* cmnd, SpellData* spellData);
int splHallow(Creature* player, cmd* cmnd, SpellData* spellData);
int splHarm(Creature* player, cmd* cmnd, SpellData* spellData);
int splHaste(Creature* player, cmd* cmnd, SpellData* spellData);
int splHeal(Creature* player, cmd* cmnd, SpellData* spellData);
int splHeatProtection(Creature* player, cmd* cmnd, SpellData* spellData);
int splHoldPerson(Creature* player, cmd* cmnd, SpellData* spellData);
int splIllusion(Creature* player, cmd* cmnd, SpellData* spellData);
int splIllusoryWall(Creature* player, cmd* cmnd, SpellData* spellData);
int splInfravision(Creature* player, cmd* cmnd, SpellData* spellData);
int splInsight(Creature* player, cmd* cmnd, SpellData* spellData);
int splInvisibility(Creature* player, cmd* cmnd, SpellData* spellData);
int splJudgement(Creature* player, cmd* cmnd, SpellData* spellData);
int splKnock(Creature* player, cmd* cmnd, SpellData* spellData);
int splKnowAura(Creature* player, cmd* cmnd, SpellData* spellData);
int splLevitate(Creature* player, cmd* cmnd, SpellData* spellData);
int splMagicMissile(Creature* player, cmd* cmnd, SpellData* spellData);
int splMendWounds(Creature* player, cmd* cmnd, SpellData* spellData);
int splNecroDrain(Creature* player, cmd* cmnd, SpellData* spellData);
int splPassWithoutTrace(Creature* player, cmd* cmnd, SpellData* spellData);
int splPlaneShift(Creature* player, cmd* cmnd, SpellData* spellData);
int splPortal(Creature* player, cmd* cmnd, SpellData* spellData);
int splPrayer(Creature* player, cmd* cmnd, SpellData* spellData);
int splProtection(Creature* player, cmd* cmnd, SpellData* spellData);
int splRadiation(Creature* player, cmd* cmnd, SpellData* spellData);
int splReboundMagic(Creature* player, cmd* cmnd, SpellData* spellData);
int splReduce(Creature* player, cmd* cmnd, SpellData* spellData);
int splReflectMagic(Creature* player, cmd* cmnd, SpellData* spellData);
int splRegeneration(Creature* player, cmd* cmnd, SpellData* spellData);
int splRejuvenate(Creature* player, cmd* cmnd, SpellData* spellData);
int splRemoveCurse(Creature* player, cmd* cmnd, SpellData* spellData);
int splRemoveFear(Creature* player, cmd* cmnd, SpellData* spellData);
int splRemoveSilence(Creature* player, cmd* cmnd, SpellData* spellData);
int splResistAir(Creature* player, cmd* cmnd, SpellData* spellData);
int splResistCold(Creature* player, cmd* cmnd, SpellData* spellData);
int splResistEarth(Creature* player, cmd* cmnd, SpellData* spellData);
int splResistFire(Creature* player, cmd* cmnd, SpellData* spellData);
int splResistLightning(Creature* player, cmd* cmnd, SpellData* spellData);
int splResistMagic(Creature* player, cmd* cmnd, SpellData* spellData);
int splResistWater(Creature* player, cmd* cmnd, SpellData* spellData);
int splRestore(Creature* player, cmd* cmnd, SpellData* spellData);
int splResurrect(Creature* player, cmd* cmnd, SpellData* spellData);
int splRoomVigor(Creature* player, cmd* cmnd, SpellData* spellData);
int splScare(Creature* player, cmd* cmnd, SpellData* spellData);
int splSilence(Creature* player, cmd* cmnd, SpellData* spellData);
int splSlow(Creature* player, cmd* cmnd, SpellData* spellData);
int splSlowPoison(Creature* player, cmd* cmnd, SpellData* spellData);
int splStaticField(Creature* player, cmd* cmnd, SpellData* spellData);
int splStoneToFlesh(Creature* player, cmd* cmnd, SpellData* spellData);
int splStoneskin(Creature* player, cmd* cmnd, SpellData* spellData);
int splStrength(Creature* player, cmd* cmnd, SpellData* spellData);
int splStun(Creature* player, cmd* cmnd, SpellData* spellData);
int splSummon(Creature* player, cmd* cmnd, SpellData* spellData);
int splTeleport(Creature* player, cmd* cmnd, SpellData* spellData);
int splTongues(Creature* player, cmd* cmnd, SpellData* spellData);
int splToxicCloud(Creature* player, cmd* cmnd, SpellData* spellData);
int splTrack(Creature* player, cmd* cmnd, SpellData* spellData);
int splTransport(Creature* player, cmd* cmnd, SpellData* spellData);
int splTrueSight(Creature* player, cmd* cmnd, SpellData* spellData);
int splUndeadWard(Creature* player, cmd* cmnd, SpellData* spellData);
int splUnhallow(Creature* player, cmd* cmnd, SpellData* spellData);
int splVigor(Creature* player, cmd* cmnd, SpellData* spellData);
int splWallOfFire(Creature* player, cmd* cmnd, SpellData* spellData);
int splWallOfForce(Creature* player, cmd* cmnd, SpellData* spellData);
int splWallOfThorns(Creature* player, cmd* cmnd, SpellData* spellData);
int splWarmth(Creature* player, cmd* cmnd, SpellData* spellData);
int splWindProtection(Creature* player, cmd* cmnd, SpellData* spellData);
int splWeakness(Creature* player, cmd* cmnd, SpellData* spellData);
int splWellOfMagic(Creature* player, cmd* cmnd, SpellData* spellData);
int splWordOfRecall(Creature* player, cmd* cmnd, SpellData* spellData);

int doOffensive(Creature *caster, Creature* target, SpellData* spellData, const char *spellname, osp_t *osp, bool multi=false);
int splOffensive(Creature* player, cmd* cmnd, SpellData* spellData, char *spellname, osp_t *osp);
int splMultiOffensive(Creature* player, cmd* cmnd, SpellData* spellData, char *spellname, osp_t *osp);


void bringDownTheWall(EffectInfo* effect, BaseRoom* room, Exit* exit);
int conjure(Creature* player, cmd* cmnd, SpellData* spellData);
int animate_dead(Creature* player, cmd* cmnd, SpellData* spellData);
int drain_exp(Creature* player, cmd* cmnd, SpellData* spellData);


int getPetTitle(CastType how, int skLevel, bool weaker, bool undead);
void petTalkDesc(Monster* pet, Creature* owner);

bool canEnchant(Player* player, SpellData* spellData);
bool canEnchant(Creature* player, Object* object);
bool decEnchant(Player* player, CastType how);

int splGeneric(Creature* player, cmd* cmnd, SpellData* spellData, const char* article, const char* spell, bstring effect, int strength=-2, long duration=-2);

bool checkRefusingMagic(Creature* player, Creature* target, bool healing=false, bool print=true);

#endif /*MAGIC_H_*/
