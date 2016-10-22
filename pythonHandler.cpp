/*
 * pythonHandler.h
 *   Class for handling Python <--> mud interactions
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

// Mud Includes
#include "mud.h"
#include "commands.h"
#include "pythonHandler.h"
#include "effects.h"
#include "msdp.h"
#include <indexing_suite/container_suite.hpp>
#include <indexing_suite/set.hpp>

int pythonRand(int a, int b) {
    return (mrand(a,b));
}

namespace bp = boost::python;

struct BaseRoom_wrapper: BaseRoom, bp::wrapper<BaseRoom> {

    BaseRoom_wrapper() :
            BaseRoom(), bp::wrapper<BaseRoom>() {
        // null constructor

    }

    void BaseDestroy() {
        BaseRoom::BaseDestroy();
    }

    virtual bool flagIsSet(int flag) const {
        bp::override func_flagIsSet = this->get_override("flagIsSet");
        return func_flagIsSet(flag);
    }
    virtual void setFlag(int flag) {
        bp::override func_setFlag = this->get_override("setFlag");
        func_setFlag(flag);
        return;
    }

    virtual Size getSize() const {
        bp::override func_getSize = this->get_override("getSize");
        return func_getSize();
    }

    virtual ::Fishing const * getFishing() const {
        bp::override func_getFishing = this->get_override("getFishing");
        return func_getFishing();
    }

};

struct MudObject_wrapper: MudObject, bp::wrapper<MudObject> {
    virtual bool pulseEffects(::time_t t) {
        bp::override func_pulseEffects = this->get_override("pulseEffects");
        return func_pulseEffects(t);
    }

};

BOOST_PYTHON_MODULE(mud)
{

    class_<Config>("Config", no_init).def("getVersion", &Config::getVersion)
            .def("getMudName", &Config::getMudName)
            .def("getMudNameAndVersion",&Config::getMudNameAndVersion)
            .def("effectExists", &Config::effectExists)
            ;

    class_<Server>("Server", no_init).def("findPlayer", &Server::findPlayer,
            return_internal_reference<>())

//      .def("findPlayer",&Server::findPlayer,
//          return_value_policy<reference_existing_object>())
            ;

    { //::MudObject
        typedef bp::class_<MudObject_wrapper, boost::noncopyable> MudObject_exposer_t;
        MudObject_exposer_t MudObject_exposer = MudObject_exposer_t("MudObject",
                bp::no_init);
        bp::scope MudObject_scope(MudObject_exposer);

//      { //::MudObject::getName
//          typedef bstring& (::MudObject::*getName_function_type)();
//
//          MudObject_exposer.def("getName"
//                  , getName_function_type(&::MudObject::getName)
//                  , return_value_policy<reference_existing_object>() );
//
//      }

        { //::MudObject::isEffected

            typedef bool ( ::MudObject::*isEffected_function_type )( ::bstring const &,bool ) const;

            MudObject_exposer.def(
                "isEffected"
                , isEffected_function_type( &::MudObject::isEffected )
                , ( bp::arg("effect"), bp::arg("exactMatch")=(bool)(false) ) );

        }
        { //::MudObject::isEffected

            typedef bool ( ::MudObject::*isEffected_function_type )( ::EffectInfo * ) const;

            MudObject_exposer.def(
                "isEffected"
                , isEffected_function_type( &::MudObject::isEffected )
                , ( bp::arg("effect") ) );

        }
        { //::MudObject::addEffect

            typedef ::EffectInfo * ( ::MudObject::*addEffect_function_type )( ::bstring const &,long int,int,::MudObject *,bool,::Creature const *,bool ) ;

            MudObject_exposer.def(
                "addEffect"
                , addEffect_function_type( &::MudObject::addEffect )
                , ( bp::arg("effect"), bp::arg("duration")=(long int)(-0x00000000000000002), bp::arg("strength")=(int)(-0x00000000000000002), bp::arg("applier")=0l, bp::arg("show")=(bool)(true), bp::arg("owner")=bp::object(), bp::arg("keepApplier")=(bool)(false) )
                ,return_value_policy<reference_existing_object>() );

        }


        MudObject_exposer.def("removeOppositeEffect", &::MudObject::removeOppositeEffect)
                .def("getName", &MudObject::getName, return_value_policy<copy_const_reference>())
                .def("getPlayer",&MudObject::getAsPlayer, return_value_policy<reference_existing_object>())
                .def("getMonster", &MudObject::getAsMonster, return_value_policy<reference_existing_object>())
                .def("getObject", &MudObject::getAsObject, return_value_policy<reference_existing_object>())
                .def("getExit",&MudObject::getAsExit, return_value_policy<reference_existing_object>())
                .def("equals", &MudObject::equals)
                .def("getId", &MudObject::getIdPython)
                ;
    }

//  class_<MudObject>("MudObject", no_init)
//      .def("getName", &MudObject::getName)
//  ;


    bp::enum_< crtClasses>("crtClasses")
        .value("ASSASSIN", ASSASSIN)
        .value("BERSERKER", BERSERKER)
        .value("CLERIC", CLERIC)
        .value("FIGHTER", FIGHTER)
        .value("MAGE", MAGE)
        .value("PALADIN", PALADIN)
        .value("RANGER", RANGER)
        .value("THIEF", THIEF)
        .value("PUREBLOOD", PUREBLOOD)
        .value("MONK", MONK)
        .value("DEATHKNIGHT", DEATHKNIGHT)
        .value("DRUID", DRUID)
        .value("LICH", LICH)
        .value("WEREWOLF", WEREWOLF)
        .value("BARD", BARD)
        .value("ROGUE", ROGUE)
        .value("BUILDER", BUILDER)
        .value("CARETAKER", CARETAKER)
        .value("DUNGEONMASTER", DUNGEONMASTER)
        .value("CLASS_COUNT", CLASS_COUNT)
        .export_values()
        ;


    bp::enum_< religions>("religions")
        .value("ATHEIST", ATHEIST)
        .value("ARAMON", ARAMON)
        .value("CERIS", CERIS)
        .value("ENOCH", ENOCH)
        .value("GRADIUS", GRADIUS)
        .value("ARES", ARES)
        .value("KAMIRA", KAMIRA)
        .value("LINOTHAN", LINOTHAN)
        .value("ARACHNUS", ARACHNUS)
        .value("MARA", MARA)
        .value("JAKAR", JAKAR)
        .value("MAX_DEITY", MAX_DEITY)
        .export_values()
        ;

    bp::enum_< DeathType>("DeathType")
         .value("DT_NONE", DT_NONE)
         .value("FALL", FALL)
         .value("POISON_MONSTER", POISON_MONSTER)
         .value("POISON_GENERAL", POISON_GENERAL)
         .value("DISEASE", DISEASE)
         .value("SMOTHER", SMOTHER)
         .value("FROZE", FROZE)
         .value("BURNED", BURNED)
         .value("DROWNED", DROWNED)
         .value("DRAINED", DRAINED)
         .value("ZAPPED", ZAPPED)
         .value("SHOCKED", SHOCKED)
         .value("WOUNDED", WOUNDED)
         .value("CREEPING_DOOM", CREEPING_DOOM)
         .value("SUNLIGHT", SUNLIGHT)
         .value("PIT", PIT)
         .value("BLOCK", BLOCK)
         .value("DART", DART)
         .value("ARROW", ARROW)
         .value("SPIKED_PIT", SPIKED_PIT)
         .value("FIRE_TRAP", FIRE_TRAP)
         .value("FROST", FROST)
         .value("ELECTRICITY", ELECTRICITY)
         .value("ACID", ACID)
         .value("ROCKS", ROCKS)
         .value("ICICLE_TRAP", ICICLE_TRAP)
         .value("SPEAR", SPEAR)
         .value("CROSSBOW_TRAP", CROSSBOW_TRAP)
         .value("VINES", VINES)
         .value("COLDWATER", COLDWATER)
         .value("EXPLODED", EXPLODED)
         .value("BOLTS", BOLTS)
         .value("SPLAT", SPLAT)
         .value("POISON_PLAYER", POISON_PLAYER)
         .value("BONES", BONES)
         .value("EXPLOSION", EXPLOSION)
         .value("PETRIFIED", PETRIFIED)
         .value("LIGHTNING", LIGHTNING)
         .value("WINDBATTERED", WINDBATTERED)
         .value("PIERCER", PIERCER)
         .value("ELVEN_ARCHERS", ELVEN_ARCHERS)
         .value("DEADLY_MOSS", DEADLY_MOSS)
         .value("THORNS", THORNS)
         .export_values()
         ;

    bp::enum_< mType>("mType")
        .value("INVALID", INVALID)
        .value("PLAYER", PLAYER)
        .value("MONSTER", MONSTER)
        .value("NPC", NPC)
        .value("HUMANOID", HUMANOID)
        .value("GOBLINOID", GOBLINOID)
        .value("MONSTROUSHUM", MONSTROUSHUM)
        .value("GIANTKIN", GIANTKIN)
        .value("ANIMAL", ANIMAL)
        .value("DIREANIMAL", DIREANIMAL)
        .value("INSECT", INSECT)
        .value("INSECTOID", INSECTOID)
        .value("ARACHNID", ARACHNID)
        .value("REPTILE", REPTILE)
        .value("DINOSAUR", DINOSAUR)
        .value("AUTOMATON", AUTOMATON)
        .value("AVIAN", AVIAN)
        .value("FISH", FISH)
        .value("PLANT", PLANT)
        .value("DEMON", DEMON)
        .value("DEVIL", DEVIL)
        .value("DRAGON", DRAGON)
        .value("BEAST", BEAST)
        .value("MAGICALBEAST", MAGICALBEAST)
        .value("GOLEM", GOLEM)
        .value("ETHEREAL", ETHEREAL)
        .value("ASTRAL", ASTRAL)
        .value("GASEOUS", GASEOUS)
        .value("ENERGY", ENERGY)
        .value("FAERIE", FAERIE)
        .value("DEVA", DEVA)
        .value("ELEMENTAL", ELEMENTAL)
        .value("PUDDING", PUDDING)
        .value("SLIME", SLIME)
        .value("UNDEAD", UNDEAD)
        .value("MAX_MOB_TYPES", MAX_MOB_TYPES)
        .export_values()
        ;


    def("dice", &::dice);
    def("rand", &::pythonRand);
    def("spawnObjects", &::spawnObjects);
    def("isBadSocial", &::isBadSocial);
    def("isSemiBadSocial", &::isSemiBadSocial);
    def("isGoodSocial", &::isGoodSocial);
    def("getConBonusPercentage", &::getConBonusPercentage);
    //void doCastPython(MudObject* caster, Creature* target, bstring spell, int strength)
    def("doCast", &::doCastPython, ( bp::arg("caster"), bp::arg("target"), bp::arg("spell"), bp::arg("strength")=(int)(130) ) );
}

BOOST_PYTHON_MODULE(MudObjects)
{

    class_<MonsterSet> ("MonsterSet")
            .def (indexing::container_suite< MonsterSet >::with_policies(return_internal_reference<>()));

    class_<PlayerSet> ("PlayerSet")
                .def (indexing::container_suite< PlayerSet >::with_policies(return_internal_reference<>()));

    class_<Containable, boost::noncopyable, bases<MudObject> >("Containable", no_init)
            .def("getParent", &Containable::getParent, return_value_policy<reference_existing_object>())
            .def("addTo", &Containable::addTo);

    typedef ::Container * ( ::Container::*findCreature_function_type )( ::Creature *,::bstring const &,bool,bool,bool );

    class_<Container, boost::noncopyable, bases<MudObject> > ("Container", no_init)
            .def_readwrite("monsters", &Container::monsters)
            .def_readwrite("players", &Container::players)
            .def("wake", &Container::wake)
            .def(
                    "findCreature"
                    , findCreature_function_type( &::Container::findCreaturePython )
                    , ( bp::arg("searcher"), bp::arg("name"), bp::arg("monFirst")=(bool)(true), bp::arg("firstAggro")=(bool)(false), bp::arg("exactMatch")=(bool)(false) )
                    ,return_value_policy<reference_existing_object>())
            ;


    bp::class_<Stat, boost::noncopyable>("Stat", bp::init<>())

        .def("getModifierAmt", &Stat::getModifierAmt)

    .def(
            "adjust"
            , (int ( ::Stat::* )( int,bool ) )( &::Stat::adjust )
            , ( bp::arg("amt") ) )
    .def(
            "decrease"
            , (int ( ::Stat::* )( int ) )( &::Stat::decrease )
            , ( bp::arg("amt") ) )
    .def(
             "getCur"
             , (int (::Stat::* ) ( bool ) const)( &::Stat::getCur )
             , ( bp::arg("recalc")=(bool)(true) ) )
    .def(
            "getInitial"
            , (short int ( ::Stat::* )( ) const)( &::Stat::getInitial ) )
    .def(
            "getMax"
            , (short int ( ::Stat::* )( ) const)( &::Stat::getMax ) )
    .def(
            "increase"
            , (int ( ::Stat::* )( int,bool ) )( &::Stat::increase )
            , ( bp::arg("amt"), bp::arg("overMaxOk")=(bool)(false) ) )
    .def(
            "load"
            , (bool ( ::Stat::* )( ::xmlNodePtr ) )( &::Stat::load )
            , ( bp::arg("curNode") ) )
    .def(
            "restore"
            , (int ( ::Stat::* )( ) )( &::Stat::restore ) )
    .def(
            "save"
            , (void ( ::Stat::* )( ::xmlNodePtr,char const * ) const)( &::Stat::save )
            , ( bp::arg("parentNode"), bp::arg("statName") ) )
    .def(
            "setCur"
            , (int ( ::Stat::* )( short int ) )( &::Stat::setCur )
            , ( bp::arg("newCur") ) )
    .def(
            "setInitial"
            , (void ( ::Stat::* )( short int ) )( &::Stat::setInitial )
            , ( bp::arg("i") ) )
    .def(
            "setMax"
            , (int ( ::Stat::* )( short int,bool ) )( &::Stat::setMax )
            , ( bp::arg("newMax"), bp::arg("allowZero")=(bool)(false) ) )

    ;

    { //::BaseRoom
        typedef bp::class_< BaseRoom_wrapper, bp::bases< Container >, boost::noncopyable > BaseRoom_exposer_t;
        BaseRoom_exposer_t BaseRoom_exposer = BaseRoom_exposer_t( "BaseRoom", bp::no_init );
        bp::scope BaseRoom_scope( BaseRoom_exposer );
        BaseRoom_exposer.def( bp::init< >() );
        { //::BaseRoom::BaseDestroy

            typedef void ( BaseRoom_wrapper::*BaseDestroy_function_type )( );

            BaseRoom_exposer.def(
                    "BaseDestroy"
                    , BaseDestroy_function_type( &BaseRoom_wrapper::BaseDestroy ) );

        }
        { //::BaseRoom::findCreature

            typedef ::Creature * ( ::BaseRoom::*findCreature_function_type )( ::Creature *,::bstring const &,bool,bool,bool );

            BaseRoom_exposer.def(
                    "findCreature"
                    , findCreature_function_type( &::BaseRoom::findCreaturePython )
                    , ( bp::arg("searcher"), bp::arg("name"), bp::arg("monFirst")=(bool)(true), bp::arg("firstAggro")=(bool)(false), bp::arg("exactMatch")=(bool)(false) )
                    ,return_value_policy<reference_existing_object>());

        }
        { //::BaseRoom::killMortalObjects

            typedef void ( ::BaseRoom::*killMortalObjects_function_type )( bool );

            BaseRoom_exposer.def(
                    "killMortalObjects"
                    , killMortalObjects_function_type( &::BaseRoom::killMortalObjects )
                    , ( bp::arg("floor")=(bool)(true) ) );

        }

        BaseRoom_exposer.def("hasMagicBonus", &::BaseRoom::magicBonus)
        .def("isForest", &::BaseRoom::isForest)
        .def("setTempNoKillDarkmetal", &::BaseRoom::setTempNoKillDarkmetal)
        .def("isSunlight", &::BaseRoom::isSunlight)
        ;
    }
    bp::class_<Skill, boost::noncopyable >( "Skill", bp::no_init )
        ;

    bp::class_<SkillInfo, boost::noncopyable >( "SkillInfo", bp::no_init )
        ;

    bp::class_<SkillCommand, boost::noncopyable, bases<SkillInfo> >( "SkillCommand", bp::no_init )
        ;

    bp::class_<EffectInfo, boost::noncopyable >( "EffectInfo", bp::no_init )
    .def("add", &EffectInfo::add )
    .def("compute", &EffectInfo::compute)
    .def("pulse", &EffectInfo::pulse)
    .def("remove", &EffectInfo::remove)
    .def("runScript", &EffectInfo::runScript, (bp::arg("pyScript"), bp::arg("applier")=0l ) )
    .def("getDisplayName", &EffectInfo::getDisplayName)
    .def("getDuration", &EffectInfo::getDuration)
    .def("getEffect", &EffectInfo::getEffect, return_value_policy<reference_existing_object>())

    .def("getExtra", &EffectInfo::getExtra)
    .def("getLastMod", &EffectInfo::getLastMod)
    .def("getName", &EffectInfo::getName)
    .def("getOwner", &EffectInfo::getOwner)
    .def("getParent", &EffectInfo::getParent, return_value_policy<reference_existing_object>())
    .def("getStrength", &EffectInfo::getStrength)
    .def("isCurse", &EffectInfo::isCurse)
    .def("isDisease", &EffectInfo::isDisease)
    .def("isPoison", &EffectInfo::isPoison)
    .def("isOwner", &EffectInfo::isOwner)
    .def("isPermanent", &EffectInfo::isPermanent)
    .def("willOverWrite", &EffectInfo::willOverWrite)

    .def("setDuration", &EffectInfo::setDuration)
    .def("setExtra", &EffectInfo::setExtra)
    .def("setOwner", &EffectInfo::setOwner)
    .def("setParent", &EffectInfo::setParent)
    .def("setStrength", &EffectInfo::setStrength)
    .def("updateLastMod", &EffectInfo::updateLastMod)
    ;

    class_<Exit, boost::noncopyable, bases<MudObject> >("Exit", no_init)
    .def("getRoom", &Exit::getRoom, return_value_policy<reference_existing_object>())
    ;

    class_<Effect, boost::noncopyable >("Effect", no_init)
    .def("getPulseScript", &Effect::getPulseScript)
    .def("getUnApplyScript", &Effect::getUnApplyScript)
    .def("getApplyScript", &Effect::getApplyScript)
    .def("getPreApplyScript", &Effect::getPreApplyScript)
    .def("getPostApplyScript", &Effect::getPostApplyScript)
    .def("getComputeScript", &Effect::getComputeScript)
    .def("getType", &Effect::getType)
    .def("getRoomDelStr", &Effect::getRoomDelStr)
    .def("getRoomAddStr", &Effect::getRoomAddStr)
    .def("getSelfDelStr", &Effect::getSelfDelStr)
    .def("getSelfAddStr", &Effect::getSelfAddStr)
    .def("getOppositeEffect", &Effect::getOppositeEffect)
    .def("getDisplay", &Effect::getDisplay)
    .def("getName", &Effect::getName)

    .def("getPulseDelay", &Effect::getPulseDelay)

    .def("isPulsed", &Effect::isPulsed)
    ;

    class_<Creature, boost::noncopyable, bases<Container>, bases<Containable> >("Creature", no_init)
    .def("send", &Creature::bPrint)
    .def("getCrtStr", &Creature::getCrtStr, ( bp::arg("viewer")=0l, bp::arg("flags")=(int)(0), bp::arg("num")=(int)(0) ))
    .def("getParent", &Creature::getParent, return_value_policy<reference_existing_object>())
    .def("hisHer", &Creature::hisHer)
    .def("upHisHer", &Creature::upHisHer)
    .def("himHer", &Creature::himHer)
    .def("getRoom", &Creature::getRoomParent, return_value_policy<reference_existing_object>())
    .def("getTarget", &Creature::getTarget, return_value_policy<reference_existing_object>())
    .def("getDeity", &Creature::getDeity)
    .def("getClass", &Creature::getClass)
    .def("setDeathType", &Creature::setDeathType)
    .def("poisonedByMonster", &Creature::poisonedByMonster)
    .def("poisonedByPlayer", &Creature::poisonedByPlayer)
    .def("getLevel", &Creature::getLevel)
    .def("getAlignment", &Creature::getAlignment)
    .def("getArmor", &Creature::getArmor)
    .def("getDamageReduction", &Creature::getDamageReduction)
    .def("getExperience", &Creature::getExperience)
    .def("getPoisonedBy", &Creature::getPoisonedBy)
    .def("getClan", &Creature::getClan)
    //.def("getType", &Creature::getType)
    .def("getRace", &Creature::getRace)
    .def("getSize", &Creature::getSize)
    .def("getAttackPower", &Creature::getAttackPower)
    .def("getDescription", &Creature::getDescription)
    .def("checkMp", &Creature::checkMp)
    .def("subMp", &Creature::subMp)
    .def("smashInvis", &Creature::smashInvis)
    .def("unhide", &Creature::unhide, (bp::arg("show")=(bool)(true) ))
    .def("unmist", &Creature::unmist)
    .def("stun", &Creature::stun)
    .def("wake", &Creature::wake, ( bp::arg("str")="", bp::arg("noise")=(bool)(false) ))

    .def("addStatModEffect", &Creature::addStatModEffect)
    .def("remStatModEffect", &Creature::remStatModEffect)
    .def("unApplyTongues", &Creature::unApplyTongues)
    .def("unBlind", &Creature::unBlind)
    .def("unSilence", &Creature::unSilence)
    .def("changeSize", &Creature::changeSize)

    .def("flagIsSet", &Creature::flagIsSet)
    .def("setFlag", &Creature::setFlag)
    .def("clearFlag", &Creature::clearFlag)
    .def("toggleFlag", &Creature::toggleFlag)

    .def("isPlayer", &Creature::isPlayer)
    .def("isMonster", &Creature::isMonster)

    .def("learnSpell", &Creature::setFlag)
    .def("forgetSpell", &Creature::clearFlag)
    .def("spellIsKnown", &Creature::spellIsKnown)

    .def("learnLanguage", &Creature::learnLanguage)
    .def("forgetLanguage", &Creature::forgetLanguage)
    .def("languageIsKnown", &Creature::languageIsKnown)

//      .def("isEffected", &Creature::isEffected, ( bp::arg("effect")=""))
//      .def("hasPermEffect", &Creature::hasPermEffect)

    .def("knowsSkill", &Creature::knowsSkill)
    .def("getSkillLevel", &Creature::getSkillLevel)
    .def("getSkillGained", &Creature::getSkillGained)
    .def("addSkill", &Creature::addSkill)
    .def("remSkill", &Creature::remSkill)
    .def("setSkill", &Creature::setSkill)

    .def("addExperience", &Creature::addExperience)
    .def("subExperience", &Creature::subExperience)
    .def("subAlignment", &Creature::subAlignment)

    .def("setClass", &Creature::setClass)
    .def("setClan", &Creature::setClan)
    .def("setRace", &Creature::setRace)
    .def("setLevel", &Creature::setLevel)

    .def("subAlignment", &Creature::subAlignment)
    .def("setSize", &Creature::setSize)
    .def("getWeight", &Creature::getWeight)
    .def("maxWeight", &Creature::maxWeight)

    .def("isVampire", &Creature::isNewVampire)
    .def("isWerewolf", &Creature::isNewWerewolf)
    .def("isUndead", &Creature::isUndead)
    .def("willBecomeVampire", &Creature::willBecomeVampire)
    .def("makeVampire", &Creature::makeVampire)
    .def("willBecomeWerewolf", &Creature::willBecomeWerewolf)
    .def("makeWerewolf", &Creature::makeWerewolf)
    .def("immuneCriticals", &Creature::immuneCriticals)
    .def("immuneToPoison", &Creature::immuneToPoison)
    .def("immuneToDisease", &Creature::immuneToDisease)

    .def("isRace", &Creature::isRace)
    .def("getSex", &Creature::getSex)

    .def("isBrittle", &Creature::isBrittle)
    .def("isBlind", &Creature::isBlind)
    .def("isUnconscious", &Creature::isUnconscious)
    .def("isBraindead", &Creature::isBraindead)

    .def("isHidden", &Creature::isHidden)
    .def("isInvisible", &Creature::isInvisible)

    .def("isWatcher", &Creature::isWatcher)
    .def("isStaff", &Creature::isStaff)
    .def("isCt", &Creature::isCt)
    .def("isDm", &Creature::isDm)
    .def("isAdm", &Creature::isAdm)
    .def("isPet", &Creature::isPet)
    .def_readonly( "hp", &Creature::hp )
    .def_readonly( "mp", &Creature::mp )
    .def_readonly( "strength", &Creature::strength )
    .def_readonly( "dexterity", &Creature::dexterity )
    .def_readonly( "constitution", &Creature::constitution )
    .def_readonly( "intelligence", &Creature::intelligence )
    .def_readonly( "piety", &Creature::piety )

    .def("delayedAction", &Creature::delayedAction)
    .def("delayedScript", &Creature::delayedScript)

    ;

    class_<MsdpVariable, boost::noncopyable >("MsdpVariable", no_init)
    .def("getName", &MsdpVariable::getName)
    ;

    class_<ReportedMsdpVariable, boost::noncopyable, bases<MsdpVariable> >("ReportedMsdpVariable", no_init)
    .def("getValue", &ReportedMsdpVariable::getValue)
    .def("setDirty", &ReportedMsdpVariable::setDirty)
    .def(
            "setValue"
            , (void ( ::ReportedMsdpVariable::* )( bstring ) )( &::ReportedMsdpVariable::setValue )
            , ( bp::arg("newValue") ) )
    .def(
            "setValue"
            , (void ( ::ReportedMsdpVariable::* )( int ) )( &::ReportedMsdpVariable::setValue )
            , ( bp::arg("newValue") ) )
    .def(
            "setValue"
            , (void ( ::ReportedMsdpVariable::* )( long int ) )( &::ReportedMsdpVariable::setValue )
            , ( bp::arg("newValue") ) )
    ;

    class_<Socket, boost::noncopyable >("Socket", no_init)
    .def("getPlayer", &Socket::getPlayer, return_value_policy<reference_existing_object>())
    .def("bprint", &Socket::bprint)
    .def("msdpSendPair", &Socket::msdpSendPair)
    .def("msdpSendList", &Socket::msdpSendList)
    ;

//  .def(
//      "getMax"
//      , (short int ( ::Stat::* )(  ) const)( &::Stat::getMax ) )

    class_<Player, boost::noncopyable, bases<Creature> >("Player", no_init)
    .def("getSock", &Player::getSock, return_value_policy<reference_existing_object>())
    .def("customColorize", &Player::customColorize, ( bp::arg("text"), bp::arg("caret")=(bool)(true) ))
    .def("expToLevel", (unsigned long ( ::Player::* )() const)( &Player::expToLevel ) )
    .def("expForLevel", (bstring ( ::Player::* )( ) const)( &Player::expForLevel ) )
    .def("getCoinDisplay", &Player::getCoinDisplay)
    .def("getBankDisplay", &Player::getBankDisplay)
    .def("getWimpy", &Player::getWimpy)
    .def("getAfflictedBy", &Player::getAfflictedBy)
    ;

    class_<Monster, boost::noncopyable, bases<Creature> >("Monster", no_init)
    .def("addEnemy", &Monster::addEnemy)
    .def("adjustThreat", &Monster::adjustThreat)
    .def("customColorize", &Monster::customColorize, ( bp::arg("text"), bp::arg("caret")=(bool)(true) ))
    ;

    class_<Object, bases<MudObject> >("Object", no_init)
    .def("getType", &Object::getType)
    .def("getWearflag", &Object::getWearflag)
    .def("getShotsmax", &Object::getShotsMax)
    .def("getShotscur", &Object::getShotsCur)

    .def("flagIsSet", &Object::flagIsSet)
    .def("setFlag", &Object::setFlag)
    .def("clearFlag", &Object::clearFlag)
    .def("toggleFlag", &Object::toggleFlag)
    .def("getEffect", &Object::getEffect)
    .def("getEffectDuration", &Object::getEffectDuration)
    .def("getEffectStrength", &Object::getEffectStrength)

    ;

}

// This handles bstring to PythonString conversions
struct bstringToPythonStr {
    static PyObject* convert(bstring const& s) {
        return boost::python::incref(
                boost::python::object((std::string) (s)).ptr());
    }
};

struct bstringFromPythonStr {
    bstringFromPythonStr() {
        boost::python::converter::registry::push_back(&convertible, &construct,
                boost::python::type_id<bstring>());
    }

    static void* convertible(PyObject* objPtr) {
        if (!PyUnicode_Check(objPtr))
            return 0;
        return objPtr;
    }

    static void construct(PyObject* objPtr,
            boost::python::converter::rvalue_from_python_stage1_data* data) {
        const char* value = _PyUnicode_AsString(objPtr);
        if (value == 0)
            boost::python::throw_error_already_set();
        void* storage = ((boost::python::converter::rvalue_from_python_storage<
                bstring>*) data)->storage.bytes;
        new (storage) bstring(value);
        data->convertible = storage;
    }
};

// This will just test python for now
bool Server::initPython() {
    try {
        // Add in our python lib to the python path for importing modules
        setenv("PYTHONPATH", Path::Python, 1);

        std::cout << " ====> PythonPath: " << getenv("PYTHONPATH") << std::endl;

        pythonHandler = new PythonHandler();

        // Setup the bstring --> pythonStr converter
        boost::python::to_python_converter<bstring, bstringToPythonStr>();

        // And the pythonStr --> bstring converter
        bstringFromPythonStr();

        // ********************************************************
        // * Module Initializations
        // *   Put any modules you would like available system wide
        // *   here, before Python is initialized below
        PyImport_AppendInittab("mud", &PyInit_mud);
        PyImport_AppendInittab("MudObjects", &PyInit_MudObjects);

        // Now that we've imported the modules we want, initalize python and setup the main module
        Py_Initialize();
        object main_module(
                (handle<>(borrowed(PyImport_AddModule("__main__")))));
        pythonHandler->mainNamespace = main_module.attr("__dict__");

        object IOModule((handle<>(PyImport_ImportModule("io"))));
        pythonHandler->mainNamespace["io"] = IOModule;

        // Import sys
        object sysModule((handle<>(PyImport_ImportModule("sys"))));
        pythonHandler->mainNamespace["sys"] = sysModule;
        object mudLibModule((handle<>(PyImport_ImportModule("mudLib"))));
        pythonHandler->mainNamespace["mudLib"] = mudLibModule;

        // Now import the modules we setup above
        object mudModule((handle<>(PyImport_ImportModule("mud"))));
        pythonHandler->mainNamespace["mud"] = mudModule;

        object mudObjectsModule(
                (handle<>(PyImport_ImportModule("MudObjects"))));
        pythonHandler->mainNamespace["MudObjectsSystem"] = mudObjectsModule;

        // Add in any objects we want
        scope(mudModule).attr("gConfig") = ptr(gConfig); // Make the gConfig object available
        scope(mudModule).attr("gServer") = ptr(gServer); // Make the gServer object available

        // Run a python test command here
        runPython(
                "print(\"Python Initialized! Running Version \" + mud.gConfig.getVersion())");

    } catch (error_already_set) {
        PyErr_Print();
    } catch (...) {
        PyErr_Print();
    }

    return (true);
}

bool Server::cleanUpPython() {
    Py_Finalize();
    // Causing a crash for some reason on shutdown
    //delete pythonHandler;
    return (true);
}

bool addMudObjectToDictionary(object& dictionary, bstring key,
        MudObject* myObject) {
    // If null, we still want it!
    if (!myObject) {
        dictionary[key.c_str()] = ptr(myObject);
        return (true);
    }

    Monster* mPtr = myObject->getAsMonster();
    Player* pPtr = myObject->getAsPlayer();
    Object* oPtr = myObject->getAsObject();
    UniqueRoom* rPtr = myObject->getAsUniqueRoom();
    AreaRoom* aPtr = myObject->getAsAreaRoom();
    Exit* xPtr = myObject->getAsExit();

    if (mPtr) {
        dictionary[key.c_str()] = ptr(mPtr);
    } else if (pPtr) {
        dictionary[key.c_str()] = ptr(pPtr);
    } else if (oPtr) {
        dictionary[key.c_str()] = ptr(oPtr);
    } else if (rPtr) {
        dictionary[key.c_str()] = ptr(rPtr);
    } else if (aPtr) {
        dictionary[key.c_str()] = ptr(aPtr);
    } else if (xPtr) {
        dictionary[key.c_str()] = ptr(xPtr);
    } else {
        dictionary[key.c_str()] = ptr(myObject);
    }
    return (true);
}

bool Server::runPython(const bstring& pyScript, object& localNamespace) {
    try {
        // Run a python test command here
        handle<> ignored((

        PyRun_String( pyScript.c_str(),

                Py_file_input,
                pythonHandler->mainNamespace.ptr(),
                localNamespace.ptr() )));

    } catch (error_already_set) {
        handlePythonError();
        return (false);
    }
    return (true);
}

//==============================================================================
// RunPython:
//==============================================================================
//  pyScript:   The script to be run
//  actor:      The actor of the script.
//  target:     The target of the script

bool Server::runPython(const bstring& pyScript, bstring args, MudObject *actor, MudObject *target) {
    object localNamespace((handle<>(PyDict_New())));

    localNamespace["args"] = args;

    addMudObjectToDictionary(localNamespace, "actor", actor);
    addMudObjectToDictionary(localNamespace, "target", target);

    return (runPython(pyScript, localNamespace));
}

//==============================================================================
// RunPythonWithReturn:
//==============================================================================
//  pyScript:   The script to be run
//  actor:      The actor of the script.
//  target:     The target of the script
bool Server::runPythonWithReturn(const bstring& pyScript, bstring args, MudObject *actor, MudObject *target) {

    try {

        object localNamespace((handle<>(PyDict_New())));

        localNamespace["args"] = args;

        localNamespace["retVal"] = true;

//      object effectModule( (handle<>(PyImport_ImportModule("hookLib"))) );

        addMudObjectToDictionary(localNamespace, "actor", actor);
        addMudObjectToDictionary(localNamespace, "target", target);

        gServer->runPython(pyScript, localNamespace);

        bool retVal = extract<bool>(localNamespace["retVal"]);

        return(retVal);
    }
    catch( error_already_set) {
        gServer->handlePythonError();
    }

    return(true);
}

//==============================================================================
// RunPython:
//==============================================================================
//  pyScript:   The script to be run
//  actor:      The actor of the script.
//  target:     The target of the script
bool Server::runPython(const bstring& pyScript, bstring args, Socket *sock, Player *actor, MsdpVariable* msdpVar) {
    object localNamespace((handle<>(PyDict_New())));

    localNamespace["args"] = args;

    if (sock != nullptr)
        localNamespace["sock"] = ptr(sock);
    if (actor != nullptr)
        localNamespace["actor"] = ptr(actor);

    ReportedMsdpVariable* reportedVar =
            dynamic_cast<ReportedMsdpVariable*>(msdpVar);

    if (msdpVar != nullptr)
        localNamespace["msdpVar"] = ptr(msdpVar);
    if (reportedVar != nullptr)
        localNamespace["reportedVar"] = ptr(reportedVar);

    return (runPython(pyScript, localNamespace));
}
namespace py = boost::python;
void Server::handlePythonError() {
    object sys = pythonHandler->mainNamespace["sys"];
    object io = pythonHandler->mainNamespace["io"];
    object err = sys.attr("stderr");
    std::string errText("Unknown");
    try {
        PyObject *type_ptr = nullptr, *value_ptr = nullptr, *traceback_ptr = nullptr;
        PyErr_Fetch(&type_ptr, &value_ptr, &traceback_ptr);
        if(type_ptr != nullptr){
            py::handle<> h_type(type_ptr);
            py::str type_pstr(h_type);
            py::extract<std::string> e_type_pstr(type_pstr);
            if(e_type_pstr.check())
                errText = e_type_pstr();
            else
                errText = "Unknown exception type";
        }
        if(value_ptr != nullptr){
            py::handle<> h_val(value_ptr);
            py::str a(h_val);
            py::extract<std::string> returned(a);
            if(returned.check())
                errText +=  ": " + returned();
            else
                errText += std::string(": Unparseable Python error: ");
        }
         if(traceback_ptr != nullptr){
            py::handle<> h_tb(traceback_ptr);
            py::object tb(py::import("traceback"));
            py::object fmt_tb(tb.attr("format_tb"));
            py::object tb_list(fmt_tb(h_tb));
            py::object tb_str(py::str("\n").join(tb_list));
            py::extract<std::string> returned(tb_str);
            if(returned.check())
                errText += ": " + returned();
            else
                errText += std::string(": Unparseable Python traceback");
            }
    } catch (...) {
        errText = "Exception getting exception info!";
    }
    broadcast(isDm, "^GPython Error: %s", errText.c_str());
    PyErr_Print();

//  PyRun_SimpleString("sys.stderr = io.StringIO.StringIO()");
}

