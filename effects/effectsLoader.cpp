/*
 * effectLoader.cpp
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

#include <map>                         // for allocator
#include <utility>                     // for move

#include "builders/effectBuilder.hpp"  // for EffectBuilder
#include "config.hpp"                  // for Config, EffectMap
#include "effects.hpp"                 // for Effect

void addToSet(Effect&& eff, EffectMap &effectMap) {
    effectMap.emplace(eff.getName(), std::move(eff));
}

bool Config::loadEffects() {
    // Replace with function/lambda that takes an Effect& and does `effects.emplace(eff.name(), eff)`
    addToSet(EffectBuilder()
         .name("alwayscold")
         .display("^BAlways Cold^x")
         .pulsed(false)
         .type("Positive")
         .selfAddStr("^bYour temperature drops.^x")
         .roomAddStr("^b*ACTOR*'s temperature drops.^x"),
       effects
    );
    addToSet(
      EffectBuilder()
        .name("alwayswarm")
        .addBaseEffect("warmth")
        .display("^RAlways Warm^x")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^rYour temperature raises.^x")
        .roomAddStr("^r*ACTOR*'s temperature raises.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("armor")
        .display("^BArmor^x")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^BYou are surrounded by magical armor.^x")
        .roomAddStr("^B*ACTOR* is surrounded by magical armor.^x")
        .selfDelStr("^BYour magical armor dissipates.^x")
        .roomDelStr("^B*ACTOR*'s magical armor dissipates.^x")
        .isSpellEffect(true)
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("avianaria")
        .addBaseEffect("fly")
        .addBaseEffect("levitate")
        .display("^cAvian Aria^x")
        .computeScript("effect.setStrength(int(actor.getLevel()/4))")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^cAvian Aria lifts you off your feet.^x")
        .roomAddStr("^c*ACTOR* is lifted off *A-HISHER* feet by Avian Aria.^x")
        .selfDelStr("^cYou slowly float to the ground.^x")
        .roomDelStr("^c*ACTOR* floats down to the ground.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("barkskin")
        .display("^yBarkskin^x")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^yYour skin toughens.^x")
        .roomAddStr("^y*ACTOR*'s skin toughens.^x")
        .selfDelStr("^yYour barkskin erodes away.^x")
        .roomDelStr("^y*ACTOR*'s barkskin erodes away.^x")
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("berserk")
        .display("^RBerserk^x")
        .pulsed(false)
        .type("Positive")
        .addBaseEffect("strength")
        .applyScript("actor.addStatModEffect(effect)")
        .unApplyScript("actor.remStatModEffect(effect)")
        .selfAddStr("^rThe rage inside you swells.^x")
        .roomAddStr("^r*ACTOR* swells with rage.^x")
        .selfDelStr("^rYour rage diminishes.^x")
        .roomDelStr("^r*ACTOR*'s rage diminishes.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("bless")
        .addBaseEffect("bless")
        .display("^YBlessed^x")
        .computeScript("effectLib.computeBeneficial(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^WYou feel holy.^x")
        .roomAddStr("^WA halo appears over *LOW-ACTOR*'s head.^x")
        .selfDelStr("^YYou feel less holy.^x")
        .roomDelStr("^Y*ACTOR* is no longer blessed.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("free-action")
        .addBaseEffect("free-action")
        .display("^gFree-Action^x")
        .computeScript("effectLib.computeBeneficial(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^GYou are now protected from hold magic and difficult movement.^x")
        .roomAddStr("^G*ACTOR* is now protected from hold magic and difficult movement.^x")
        .selfDelStr("^gYour protection from hold magic and difficult movement has ended.^x")
        .roomDelStr("^g*ACTOR*'s protection from hold magic and difficult movement has ended.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("blindness")
        .addBaseEffect("blindness")
        .display("^YBlinded!^x")
        .computeScript("effectLib.computeDisable(actor, effect, applier)")
        .applyScript("actor.unBlind()")
        .pulsed(false)
        .type("Negative")
        .selfAddStr("^yYou have gone blind!^x")
        .roomAddStr("^y*ACTOR* has gone blind.^x")
        .selfDelStr("^yYou can see again!^x")
        .roomDelStr("^y*ACTOR* can see again.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("bloodsac")
        .display("^RBlood Sacrifice^x")
        .computeScript("effectLib.computeBloodSac(actor, effect, applier)")
        .applyScript("actor.addStatModEffect(effect)")
        .unApplyScript("actor.remStatModEffect(effect)")
        .pulsed(false)
        .type("Negative")
        .selfAddStr("^rYour body is infused with increased vitality!^x")
        .roomAddStr("^r*ACTOR*'s body is infused with increased vitality.^x")
        .selfDelStr("^rYour demonic power leaves you.^x")
        .roomDelStr("^r*ACTOR*'s demonic power fades.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("blur")
        .display("^CBlur^x")
        .computeScript("effectLib.computeVisibility(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^CYour form suddenly blurs.^x")
        .roomAddStr("^C*ACTOR*'s outline begins to blur.^x")
        .selfDelStr("^CYour form comes into focus.^x")
        .roomDelStr("^W*ACTOR*'s outline comes into focus.^x")
        .isSpellEffect(true)
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("breathe-water")
        .addBaseEffect("breathe-water")
        .display("^BBreathe-Water^x")
        .computeScript("effectLib.computeBeneficial(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^bYour lungs increase in size.^x")
        .roomAddStr("^b*ACTOR*'s lungs increase in size.^x")
        .selfDelStr("^BYour lungs contract in size.^x")
        .roomDelStr("^B*ACTOR*'s lungs contract in size.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("camouflage")
        .display("^GCamouflage^x")
        .computeScript("effectLib.computeVisibility(actor, effect, applier)")
        .pulsed(true)
        .pulseScript("effectLib.pulseCamouflage(actor, effect)")
        .pulseDelay(20)
        .type("Positive")
        .selfAddStr("^gYou blend in with your surroundings.^x")
        .roomAddStr("^g*ACTOR* blends in with the surroundings.^x")
        .selfDelStr("^gYou no longer blend in with your surroundings.^x")
        .roomDelStr("^g*ACTOR* no longer blends in with the surroundings.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("comprehend-languages")
        .addBaseEffect("comprehend-languages")
        .display("^gComprehend-Languages^x")
        .computeScript("effectLib.computeLanguages(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^gYou feel understanding flow within you.^x")
        .roomAddStr("^g*ACTOR* gains newfound understanding.^x")
        .selfDelStr("^gYour understanding leaves you.^x")
        .roomDelStr("^g*ACTOR* looks dumbfounded.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("concealed"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("confusion")
        .addBaseEffect("confusion")
        .display("^CConfused!^x")
        .computeScript("effectLib.computeDisable(actor, effect, applier)")
        .pulsed(false)
        .type("Negative")
        .selfAddStr("^cYour mind clouds.^x")
        .roomAddStr("^cA confused look overcomes *LOW-ACTOR*.^x")
        .selfDelStr("^cThe fog over your mind lifts.^x")
        .roomDelStr("^cThe fog over your mind lifts.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("courage")
        .addBaseEffect("courage")
        .display("^YCourage^x")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^YYou suddenly feel very brave.^x")
        .roomAddStr("^Y*ACTOR* suddenly looks very brave.^x")
        .selfDelStr("^YYou are no longer unnaturally brave.^x")
        .roomDelStr("^Y*ACTOR* is no longer unnaturally brave.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("creeping-doom")
        .addBaseEffect("curse")
        .display("^DCreeping-Doom^x")
        .pulsed(true)
        .pulseDelay(20)
        .pulseScript("effectLib.pulseCreepingDoom(actor, effect)")
        .type("Negative")
        .roomAddStr("^WThe cursed spiders leave your body.^x")
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("damnation")
        .display("^rDamnation^x")
        .computeScript("effectLib.computeStatLower(actor, effect, applier)")
        .oppositeEffect("prayer")
        .applyScript("actor.addStatModEffect(effect)")
        .unApplyScript("actor.remStatModEffect(effect)")
        .pulsed(false)
        .type("Negative")
        .selfAddStr("^rYou are damned by the gods!^x")
        .roomAddStr("^r*ACTOR* has been damned by the gods!^x")
        .selfDelStr("^rYour spiritual punishment has ended.^x")
        .roomDelStr("^r*ACTOR*'s spiritual punishment has ended.^x")
        .isSpellEffect(true)
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("darkness")
        .addBaseEffect("darkness")
        .display("^DDarkness^x")
        .computeScript("effectLib.computeDarkInfra(actor, effect, applier)")
        .preApplyScript("actor.getRoom().setTempNoKillDarkmetal(True)")
        .postApplyScript("actor.getRoom().setTempNoKillDarkmetal(False)")
        .unApplyScript("actor.getRoom().killMortalObjects()")
        .pulsed(false)
        .type("Negative")
        .selfAddStr("^DYou are engulfed in darkness.")
        .roomAddStr("^D*ACTOR* is engulfed in darkness.^x")
        .selfDelStr("^YThe globe of darkness around you fades.^x")
        .roomDelStr("^YThe globe of darkness around *LOW-ACTOR* fades.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("deafness")
        .addBaseEffect("deafness")
        .display("^YDeafened!^x")
        .computeScript("effectLib.computeDisable(actor, effect, applier)")
        .pulsed(false)
        .type("Negative")
        .selfAddStr("^yYou lose your hearing!^x")
        .roomAddStr("^y*ACTOR* has gone deaf.^x")
        .selfDelStr("^yYou can hear again!^x")
        .roomDelStr("^yYou can hear again!^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("death-sickness")
        .addBaseEffect("death-sickness")
        .display("^#^DDeath Sickness!^x")
        .computeScript("effectLib.computeDeathSickness(actor, effect, applier)")
        .pulsed(true)
        .pulseScript("effectLib.pulseDeathSickness(actor, effect)")
        .pulseDelay(20)
        .type("Negative")
        .selfAddStr("^DYou have become afflicted with death sickness.^x")
        .selfDelStr("^WYou recover from your death sickness.^x")
        .roomDelStr("^W*ACTOR* looks much better.^x")
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("dense-fog")
        .display("^CDense Fog^x")
        .roomAddStr("^WFog begins to fill the area.^x")
        .roomDelStr("^WThe fog clears the area.^x")
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("detect-invisible")
        .addBaseEffect("detect-invisible")
        .display("^MDetect-Invisible^x")
        .computeScript("effectLib.computeDetect(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^yThe invisible slowly reveals itself to you.^x")
        .roomAddStr("^y*ACTOR*'s eyes glow yellow.^x")
        .selfDelStr("^WThe invisible fades from view.^x")
        .roomDelStr("^W*ACTOR*'s eyes lose their yellowish glow.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("detect-magic")
        .addBaseEffect("detect-magic")
        .display("^rDetect-Magic^x")
        .computeScript("effectLib.computeDetect(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^rYour eyes glow red.^x")
        .roomAddStr("^r*ACTOR*'s eyes glow red.^x")
        .selfDelStr("^WYou can no longer sense the magical.^x")
        .roomDelStr("^W*ACTOR*'s eyes lose their reddish glow.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("dimensional-anchor")
        .display("^mDimensional-Anchor^x")
        .computeScript("effectLib.computeBeneficial(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^mYou are now tethered in space and time.^x")
        .roomAddStr("^m*ACTOR* is now tethered in space and time.^x")
        .selfDelStr("^mYou are no longer tethered in space and time.^x")
        .roomDelStr("^m*ACTOR* is no longer tethered in space and time.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("disease")
        .addBaseEffect("disease")
        .pulseDelay(20)
        .pulseScript("effectLib.pulseDisease(actor, effect)")
        .display("^rDisease^x")
        .pulsed(true)
        .type("Negative"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("dkpray")
        .display("^RBerserk^x")
        .pulsed(false)
        .type("Positive")
        .addBaseEffect("strength")
        .applyScript("actor.addStatModEffect(effect)")
        .unApplyScript("actor.remStatModEffect(effect)")
        .selfAddStr("^rThe evil in your soul infuses your body with power.^x")
        .roomAddStr("^r*ACTOR* glows with evil.^x")
        .selfDelStr("^rYour demonic strength leaves you.^x")
        .roomDelStr("^rThe evil glow about *ACTOR* diminishes.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("drain-shield")
        .display("^WDrain-Shield^x")
        .computeScript("effectLib.computeBeneficial(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^WAn aura of positive energy surrounds you.^x")
        .roomAddStr("^WAn aura of positive energy surrounds *LOW-ACTOR*.^x")
        .selfDelStr("^DThe aura of positive energy around you dissipates.^x")
        .roomDelStr("^DThe aura of positive energy around *LOW-ACTOR* dissipates.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("drunkenness")
        .addBaseEffect("drunkenness")
        .display("^o")
        .computeScript("effectLib.computeDisable(actor, effect, applier)")
        .pulsed(false)
        .type("Negative")
        .selfAddStr("^oYou feel light-headed and your vision blurs.^x")
        .roomAddStr("^o*ACTOR* hiccups and staggers.^x")
        .selfDelStr("^CYou no longer feel light-headed.^x")
        .roomDelStr("^C*ACTOR* no longer feels light-headed.^x")
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("earth-shield")
        .display("^GEarth-Shield^x")
        .computeScript("effectLib.computeBeneficial(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^GYour flesh strengthens.^x")
        .roomAddStr("^G*ACTOR*'s flesh strengthens.^x")
        .selfDelStr("^GYour skin softens.^x")
        .roomDelStr("^G*ACTOR*'s skin softens.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("enfeeblement")
        .display("^GEnfeeblement^x")
        .computeScript("effectLib.computeStatLower(actor, effect, applier)")
        .oppositeEffect("strength")
        .applyScript("actor.addStatModEffect(effect)")
        .unApplyScript("actor.remStatModEffect(effect)")
        .pulsed(false)
        .type("Negative")
        .selfAddStr("^GYou feel weaker!^x")
        .roomAddStr("^G*ACTOR* looks much weaker.^x")
        .selfDelStr("^GYour weakness fades.^x")
        .roomDelStr("^G*ACTOR*'s weakness fades.^x")
        .isSpellEffect(true)
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("enlarge")
        .addBaseEffect("sizeChange")
        .display("^CEnlarge^x")
        .applyScript("actor.changeSize(0, effect.getStrength(), True)")
        .unApplyScript("actor.changeSize(0, effect.getStrength(), False)")
        .pulsed(false)
        .type("Neutral")
        .selfAddStr("^CYou grow in size!^x")
        .roomAddStr("^C*ACTOR* grows in size!^x")
        .selfDelStr("^CYou shrink in size.^x")
        .roomDelStr("^C*ACTOR* shrinks in size.^x")
        .isSpellEffect(true)
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("reduce")
        .addBaseEffect("sizeChange")
        .display("^cReduce^x")
        .applyScript("actor.changeSize(0, effect.getStrength(), False)")
        .unApplyScript("actor.changeSize(0, effect.getStrength(), True)")
        .pulsed(false)
        .type("Neutral")
        .selfAddStr("^CYou shrink in size!^x")
        .roomAddStr("^C*ACTOR* shrinks in size!^x")
        .selfDelStr("^CYou grow in size.^x")
        .roomDelStr("^C*ACTOR* grows in size.^x")
        .isSpellEffect(true)
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("farsight")
        .addBaseEffect("farsight")
        .display("^WFarsight^x")
        .computeScript("effectLib.computeDetect(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^WYour vision improves.^x")
        .roomAddStr("^W*ACTOR*'s vision improves.^x")
        .selfDelStr("^cYour vision returns to normal.^x")
        .roomDelStr("^c*ACTOR*'s vision returns to normal.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("fear")
        .addBaseEffect("fear")
        .display("^YFear^x")
        .pulsed(false)
        .type("Negative")
        .selfAddStr("^yYou suddenly feel very afraid.^x")
        .roomAddStr("^r*ACTOR* suddenly looks very afraid.^x")
        .selfDelStr("^yYou feel your courage return.^x")
        .roomDelStr("^y*ACTOR*'s courage returns.^x")
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("feeblemind")
        .display("^mFeeblemind^x")
        .computeScript("effectLib.computeStatLower(actor, effect, applier)")
        .oppositeEffect("insight")
        .applyScript("actor.addStatModEffect(effect)")
        .unApplyScript("actor.remStatModEffect(effect)")
        .pulsed(false)
        .type("Negative")
        .selfAddStr("^mYour minds becomes feeble.^x")
        .roomAddStr("^m*ACTOR* blankly stares off into space.^x")
        .selfDelStr("^mThe fog in your mind fades.^x")
        .roomDelStr("^mThe fog in *LOW-ACTOR*'s mind fades.^x")
        .isSpellEffect(true)
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("fire-shield")
        .display("^yFire Shield^x")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^RYou are protected by a fiery barrier.^x")
        .roomAddStr("^r*ACTOR* is protected by a fiery barrier.^x")
        .selfDelStr("^yYour shield of fire dissipates.^x")
        .roomDelStr("^y*ACTOR*'s shield of fire dissipates.^x")
        .isSpellEffect(true)
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("fly")
        .addBaseEffect("fly")
        .display("^yFly^x")
        .computeScript("effectLib.computeGravity(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^yYou begin to fly.^x")
        .roomAddStr("^y*ACTOR* begins to fly.^x")
        .selfDelStr("^yYou slowly float to the ground.^x")
        .roomDelStr("^y*ACTOR* floats down to the ground.^x")
        .isSpellEffect(true)
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("fortitude")
        .display("^GFortitude^x")
        .computeScript("effectLib.computeStatRaise(actor, effect, applier)")
        .oppositeEffect("weakness")
        .applyScript("actor.addStatModEffect(effect)")
        .unApplyScript("actor.remStatModEffect(effect)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^GYou are vitalized by magical health.^x")
        .roomAddStr("^G*ACTOR* is vitalized by magical health.^x")
        .selfDelStr("^gYour magical health fades.^x")
        .roomDelStr("^g*ACTOR*'s magical health fades.^x")
        .isSpellEffect(true)
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("frenzy")
        .display("^gFrenzy^x")
        .pulsed(false)
        .type("Positive")
        .addBaseEffect("dexterity")
        .addBaseEffect("haste")
        .applyScript("actor.addStatModEffect(effect)")
        .unApplyScript("actor.remStatModEffect(effect)")
        .selfAddStr("^gYour faster reflexes slow the world around you.^x")
        .roomAddStr("^g*ACTOR* attacks in a frenzy.^x")
        .selfDelStr("^gYou feel slower.^x")
        .roomDelStr("^r*ACTOR* calms down.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("globe-of-silence")
        .display("^DGlobe of Silence^x")
        .roomAddStr("^DSounds in the area begin to fade.^x")
        .roomDelStr("^WSounds in the area return.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("greater-invisibility")
        .display("^MGreater-Invisiblity^x")
        .computeScript("effectLib.computeVisibility(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^WYou fade from view.^x")
        .roomAddStr("^W*ACTOR* fades from view.^x")
        .selfDelStr("^WYou fade into view.^x")
        .roomDelStr("^W*ACTOR* fades into view.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("hallow")
        .display("^YHallow^x")
        .roomAddStr("^YA holy aura fills the room.^x")
        .roomDelStr("^WThe holy aura in the room fades.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("haste")
        .display("^CHaste^x")
        .computeScript("effectLib.computeStatRaise(actor, effect, applier)")
        .oppositeEffect("slow")
        .applyScript("actor.addStatModEffect(effect)")
        .unApplyScript("actor.remStatModEffect(effect)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^cThe world slows down around you.^x")
        .roomAddStr("^c*ACTOR* starts moving much faster.^x")
        .selfDelStr("^cThe world catches up to you.^x")
        .roomDelStr("^cThe world catches up to *LOW-ACTOR*.^x")
        .isSpellEffect(true)
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("heat-protection")
        .display("^RHeat-Protection^x")
        .computeScript("effectLib.computeBeneficial(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^MYour skin toughens.^x")
        .roomAddStr("^M*ACTOR*'s skin toughens.^x")
        .selfDelStr("^yYou are no longer protected from fire.^x")
        .roomDelStr("^y*ACTOR*'s skin softens.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("hold-person")
        .addBaseEffect("hold-person")
        .display("^YMagically Held!^x")
        .computeScript("effectLib.computeDisable(actor, effect, applier)")
        .pulsed(true)
        .pulseScript("effectLib.pulseHoldSpells(actor, effect)")
        .pulseDelay(12)
        .type("Negative")
        .selfAddStr("^yYou are frozen and unable to move.^x")
        .roomAddStr("^y*ACTOR* is frozen and unable to move.^x")
        .selfDelStr("^yYou are no longer magically held.^x")
        .roomDelStr("^y*ACTOR* is no longer magically held.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("hold-monster")
        .addBaseEffect("hold-monster")
        .display("^cMagically Held!^x")
        .computeScript("effectLib.computeDisable(actor, effect, applier)")
        .pulsed(true)
        .pulseScript("effectLib.pulseHoldSpells(actor, effect)")
        .pulseDelay(12)
        .type("Negative")
        .selfAddStr("^cYou are frozen and unable to move.^x")
        .roomAddStr("^c*ACTOR* is frozen and unable to move.^x")
        .selfDelStr("^cYou are no longer magically held.^x")
        .roomDelStr("^c*ACTOR* is no longer magically held.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("hold-undead")
        .addBaseEffect("hold-undead")
        .display("^DMagically Held!^x")
        .computeScript("effectLib.computeDisable(actor, effect, applier)")
        .pulsed(true)
        .pulseScript("effectLib.pulseHoldSpells(actor, effect)")
        .pulseDelay(15)
        .type("Negative")
        .selfAddStr("^DYou are frozen and unable to move.^x")
        .roomAddStr("^D*ACTOR* is frozen and unable to move.^x")
        .selfDelStr("^DYou are no longer magically held.^x")
        .roomDelStr("^D*ACTOR* is no longer magically held.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("hold-animal")
        .addBaseEffect("hold-animal")
        .display("^gMagically Held!^x")
        .computeScript("effectLib.computeDisable(actor, effect, applier)")
        .pulsed(true)
        .pulseScript("effectLib.pulseHoldSpells(actor, effect)")
        .pulseDelay(12)
        .type("Negative")
        .selfAddStr("^gYou are frozen and unable to move.^x")
        .roomAddStr("^g*ACTOR* is frozen and unable to move.^x")
        .selfDelStr("^gYou are no longer magically held.^x")
        .roomDelStr("^g*ACTOR* is no longer magically held.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("hold-plant")
        .addBaseEffect("hold-plant")
        .display("^GMagically Held!^x")
        .computeScript("effectLib.computeDisable(actor, effect, applier)")
        .pulsed(true)
        .pulseScript("effectLib.pulseHoldSpells(actor, effect)")
        .pulseDelay(12)
        .type("Negative")
        .selfAddStr("^GYou are frozen and unable to move.^x")
        .roomAddStr("^G*ACTOR* is frozen and unable to move.^x")
        .selfDelStr("^GYou are no longer magically held.^x")
        .roomDelStr("^G*ACTOR* is no longer magically held.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("hold-elemental")
        .addBaseEffect("hold-elemental")
        .display("^rMagically Held!^x")
        .computeScript("effectLib.computeDisable(actor, effect, applier)")
        .pulsed(true)
        .pulseScript("effectLib.pulseHoldSpells(actor, effect)")
        .pulseDelay(15)
        .type("Negative")
        .selfAddStr("^rYou are frozen and unable to move.^x")
        .roomAddStr("^r*ACTOR* is frozen and unable to move.^x")
        .selfDelStr("^rYou are no longer magically held.^x")
        .roomDelStr("^r*ACTOR* is no longer magically held.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("hold-fey")
        .addBaseEffect("hold-fey")
        .display("^MMagically Held!^x")
        .computeScript("effectLib.computeDisable(actor, effect, applier)")
        .pulsed(true)
        .pulseScript("effectLib.pulseHoldSpells(actor, effect)")
        .pulseDelay(15)
        .type("Negative")
        .selfAddStr("^MYou are frozen and unable to move.^x")
        .roomAddStr("^M*ACTOR* is frozen and unable to move.^x")
        .selfDelStr("^MYou are no longer magically held.^x")
        .roomDelStr("^M*ACTOR* is no longer magically held.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("illusion")
        .display("^CIllusion^x")
        .computeScript("effectLib.computeVisibility(actor, effect, applier)")
        .pulsed(false)
        .type("Neutral")
        .selfAddStr("^CYou are surrounded by an illusion.^x")
        .roomAddStr("^W*ACTOR* suddenly looks different.^x")
        .selfDelStr("^CYour illusion fades.^x")
        .roomDelStr("^W*ACTOR* suddenly looks different.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("immune-air")
        .display("^yImmune-Air^x")
        .computeScript("mudLib.noPlayer(actor)")
        .pulsed(false)
        .type("Positive"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("immune-chopping")
        .display("^cImmune-Chopping^x")
       .computeScript("mudLib.noPlayer(actor)")
        .pulsed(false)
        .type("Positive"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("immune-cold")
        .display("^bImmune-Cold^x")
        .computeScript("mudLib.noPlayer(actor)")
        .pulsed(false)
        .type("Positive"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("immune-crushing")
        .display("^cImmune-Crushing^x")
        .computeScript("mudLib.noPlayer(actor)")
        .pulsed(false)
        .type("Positive"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("immune-earth")
        .display("^YImmune-Earth^x")
        .computeScript("mudLib.noPlayer(actor)")
        .pulsed(false)
        .type("Positive"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("immune-electric")
        .display("^cImmune-Electric^x")
        .computeScript("mudLib.noPlayer(actor)")
        .pulsed(false)
        .type("Positive"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("immune-exotic")
        .display("^cImmune-Exotic^x")
        .computeScript("mudLib.noPlayer(actor)")
        .pulsed(false)
        .type("Positive"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("immune-fire")
        .display("^rImmune-Fire^x")
        .computeScript("mudLib.noPlayer(actor)")
        .pulsed(false)
        .type("Positive"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("immune-piercing")
        .display("^cImmune-Piercing^x")
        .computeScript("mudLib.noPlayer(actor)")
        .pulsed(false)
        .type("Positive"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("immune-ranged")
        .display("^cImmune-Ranged^x")
        .computeScript("mudLib.noPlayer(actor)")
        .pulsed(false)
        .type("Positive"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("immune-slashing")
        .display("^cImmune-Slashing^x")
        .computeScript("mudLib.noPlayer(actor)")
        .pulsed(false)
        .type("Positive"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("immune-water")
        .display("^BImmune-Water^x")
        .computeScript("mudLib.noPlayer(actor)")
        .pulsed(false)
        .type("Positive"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("incognito")
        .display("^gIncognito^x")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^gYou cloak your presence.^x")
        .roomAddStr("^W*ACTOR* cloaks *A-HISHER* presence.^x")
        .selfDelStr("^WYou uncloak your presence.^x")
        .roomDelStr("^W*ACTOR* uncloaks *A-HISHER* presence.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("infravision")
        .addBaseEffect("infravision")
        .display("^YInfravision^x")
        .computeScript("effectLib.computeDarkInfra(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^YYour eyes glow yellow.")
        .roomAddStr("^Y*ACTOR*'s eyes glow yellow.^x")
        .selfDelStr("^DThe world darkens around you.^x")
        .roomDelStr("^Y*ACTOR*'s magic vision fades.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("insight")
        .display("^mInsight^x")
        .computeScript("effectLib.computeStatRaise(actor, effect, applier)")
        .oppositeEffect("feeblemind")
        .applyScript("actor.addStatModEffect(effect)")
        .unApplyScript("actor.remStatModEffect(effect)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^mYou become filled with magical insight.^x")
        .roomAddStr("^Y*ACTOR* is filled with magical insight.^x")
        .selfDelStr("^mYour magical insight fades.^x")
        .roomDelStr("^m*ACTOR*'s magical insight fades.^x")
        .isSpellEffect(true)
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("invisibility")
        .display("^MInvisible^x")
        .computeScript("effectLib.computeVisibility(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^WYou fade from view.^x")
        .roomAddStr("^W*ACTOR* fades from view.^x")
        .selfDelStr("^WYou fade into view.^x")
        .roomDelStr("^W*ACTOR* fades into view.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("know-aura")
        .addBaseEffect("know-aura")
        .display("^cKnow-Aura^x")
        .computeScript("effectLib.computeDetect(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^cYou become more perceptive.^x")
        .roomAddStr("^c*ACTOR* becomes more perceptive.^x")
        .selfDelStr("^cYour perception is diminished.^x")
        .roomDelStr("^c*ACTOR*'s perception is diminished.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("levitate")
        .addBaseEffect("levitate")
        .display("^cLevitate^x")
        .computeScript("effectLib.computeGravity(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^cYou begin to float.^x")
        .roomAddStr("^c*ACTOR* begins to float.^x")
        .selfDelStr("^cYour feet hit the ground.^x")
        .roomDelStr("^c*ACTOR*'s feet hit the ground.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("lycanthropy")
        .addBaseEffect("disease")
        .computeScript("effectLib.computeLycanthropy(actor, effect, applier)")
        .pulseScript("effectLib.pulseLycanthropy(actor, effect)")
        .display("^oLycanthropy^x")
        .pulsed(true)
        .type("Neutral"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("mist")
        .display("^mMist^x")
        .pulsed(false)
        .selfAddStr("^mYou turn to mist.^x")
        .roomAddStr("^m*ACTOR* turns to mist.^x")
        .selfDelStr("^mYou reform.^x")
        .roomDelStr("^m*ACTOR* reforms.^x")
        .type("Positive"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("non-detection")
        .addBaseEffect("non-detection")
        .display("^DNon-Detection^x")
        .computeScript("effectLib.computeBeneficial(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^DYou are now hidden from magical detection.^x")
        .roomAddStr("^D*ACTOR*'s body seems to shimmer then reform.^x")
        .selfDelStr("^DYou are no longer hidden from magical detection.^x")
        .roomDelStr("^D*ACTOR*'s body seems to shimmer then reform.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("pass-without-trace")
        .display("^GPass-without-trace^x")
        .computeScript("effectLib.computeVisibility(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^gYou no longer leave any tracks.^x")
        .roomAddStr("^g*ACTOR* starts to step very softly.^x")
        .selfDelStr("^gYou start leaving tracks again.^x")
        .roomDelStr("^g*ACTOR* begins stepping heavier.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("petrification")
        .addBaseEffect("petrification")
        .display("^DPetrified!^x")
        .computeScript("effectLib.computeDisable(actor, effect, applier)")
        .pulsed(false)
        .type("Negative")
        .selfAddStr("^DYou turn to stone!^x")
        .roomAddStr("^D*ACTOR* has been turned to stone!")
        .selfDelStr("^YStone becomes flesh, you can move again!^x")
        .roomDelStr("^Y*ACTOR*'s skin slowly turns to flesh.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("poison")
        .addBaseEffect("poison")
        .display("^gPoison^x")
        .pulsed(true)
        .pulseDelay(20)
        .pulseScript("effectLib.pulsePoison(actor, effect)")
        .type("Negative")
        .selfDelStr("^yThe poison has run its course.")
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("porphyria")
        .addBaseEffect("curse")
        .display("^WPorphyria^x")
        .pulsed(true)
        .pulseDelay(20)
        .pulseScript("effectLib.pulsePorphyria(actor, effect)")
        .type("Negative")
        .selfAddStr("^WYour skin turns pale.^x")
        .roomAddStr("^W*ACTOR* looks pale.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("prayer")
        .display("^mPrayer^x")
        .computeScript("effectLib.computeStatRaise(actor, effect, applier)")
        .oppositeEffect("damnation")
        .applyScript("actor.addStatModEffect(effect)")
        .unApplyScript("actor.remStatModEffect(effect)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^yYou shine with divine favor!^x")
        .roomAddStr("^y*ACTOR* shines radiently.^x")
        .selfDelStr("^yYour divine favor fades.^x")
        .roomDelStr("^y*ACTOR*'s divine favor fades.^x")
        .isSpellEffect(true)
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("pray")
        .display("^yPrayed^x")
        .pulsed(false)
        .type("Positive")
        .addBaseEffect("prayer")
        .applyScript("actor.addStatModEffect(effect)")
        .unApplyScript("actor.remStatModEffect(effect)")
        .selfAddStr("^yYou feel extremely pious.^x")
        .roomAddStr("^y*ACTOR* looks more pious.^x")
        .selfDelStr("^yYou feel less pious.^x")
        .roomDelStr("^y*ACTOR* looks less pious.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("protection")
        .addBaseEffect("protection")
        .display("^WProtection^x")
        .computeScript("effectLib.computeBeneficial(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^WYou feel watched.^x")
        .roomAddStr("^W*ACTOR* is surrounded by a faint magical aura.^x")
        .selfDelStr("^yYour aura of protection fades.^x")
        .roomDelStr("^y*ACTOR* is no longer protected.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("benediction")
        .addBaseEffect("benediction")
        .display("^BBenediction^x")
        .computeScript("effectLib.computeBeneficial(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^BYour soul is fortified with the protective power of good.^x")
        .roomAddStr("^B*ACTOR* glows brightly from the protective power of good.^x")
        .selfDelStr("^DYour soul is no longer fortified with the protective power of good.^x")
        .roomDelStr("^D*ACTOR* no longer glows from the protective power of good.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("malediction")
        .addBaseEffect("malediction")
        .display("^RMalediction^x")
        .computeScript("effectLib.computeBeneficial(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^RYour soul is fortified with the spiteful power of evil.^x")
        .roomAddStr("^R*ACTOR* glows darkly from the spiteful power of evil.^x")
        .selfDelStr("^DYour soul is no longer fortified with the spiteful power of evil.^x")
        .roomDelStr("^D*ACTOR* no longer glows from the spiteful power of evil.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("reflect-magic")
        .addBaseEffect("reflect-magic")
        .display("^MReflect-Magic^x")
        .computeScript("effectLib.computeBeneficial(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^MA shield of magical force surrounds you.^x")
        .roomAddStr("^MA shield of magical force engulfs *LOW-ACTOR*.^x")
        .selfDelStr("^MYour shield of magic dissipates.^x")
        .roomDelStr("^M*ACTOR*'s shield of magic dissipates.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("regeneration")
        .addBaseEffect("regeneration")
        .display("^gRegeneration^x")
        .computeScript("effectLib.computeRegen(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^gYou begin to regenerate.^x")
        .roomAddStr("^g*ACTOR* begins to regenerate.^x")
        .selfDelStr("^gYour wounds no longer magically close.^x")
        .roomDelStr("^g*ACTOR*'s wounds no longer magically close.^x")
        .isSpellEffect(true)
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("resist-air")
        .display("^yResist-Air^x")
        .computeScript("effectLib.computeResist(actor, effect, applier)")
        .oppositeEffect("vuln-air")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^yYou feel resistant to air.^x")
        .roomAddStr("^y*ACTOR* looks resistant to air.^x")
        .selfDelStr("^yYou feel less resistant to air.^x")
        .roomDelStr("^y*ACTOR* looks less resistant to air.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("resist-chopping")
        .display("^mResist-Chopping^x")
        .computeScript("mudLib.noPlayer(actor)")
        .pulsed(false)
        .type("Positive"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("resist-cold")
        .display("^bResist-Cold^x")
        .computeScript("effectLib.computeResist(actor, effect, applier)")
        .oppositeEffect("vuln-cold")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^cYou feel resistant to cold.^x")
        .roomAddStr("^c*ACTOR* looks resistant to cold.^x")
        .selfDelStr("^cYou feel less resistant to cold.^x")
        .roomDelStr("^c*ACTOR* looks less resistant to cold.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("resist-crushing")
        .display("^mResist-Crushing^x")
        .computeScript("mudLib.noPlayer(actor)")
        .pulsed(false)
        .type("Positive"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("resist-earth")
        .display("^YResist-Earth^x")
        .computeScript("effectLib.computeResist(actor, effect, applier)")
        .oppositeEffect("vuln-earth")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^YYou feel resistant to earth.^x")
        .roomAddStr("^Y*ACTOR* looks resistant to earth.^x")
        .selfDelStr("^YYou feel less resistant to earth.^x")
        .roomDelStr("^Y*ACTOR* looks less resistant to earth.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("resist-electric")
        .display("^cResist-Electric^x")
        .computeScript("effectLib.computeResist(actor, effect, applier)")
        .oppositeEffect("vuln-electric")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^cYou feel resistant to electricity.^x")
        .roomAddStr("^c*ACTOR* looks resistant to electricity.^x")
        .selfDelStr("^cYou feel less resistant to electricity.^x")
        .roomDelStr("^c*ACTOR* looks less resistant to electricity.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("resist-exotic")
        .display("^mResist-Exotic^x")
        .computeScript("mudLib.noPlayer(actor)")
        .pulsed(false)
        .type("Positive"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("resist-fire")
        .display("^rResist-Fire^x")
        .computeScript("effectLib.computeResist(actor, effect, applier)")
        .oppositeEffect("vuln-fire")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^rYou feel resistant to fire.^x")
        .roomAddStr("^r*ACTOR* looks resistant to fire.^x")
        .selfDelStr("^rYou feel less resistant to fire.^x")
        .roomDelStr("^r*ACTOR* looks less resistant to fire.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("resist-magic")
        .display("^MResist-Magic^x")
        .computeScript("effectLib.computeResist(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^mYou are surrounded by a magical shield.^x")
        .roomAddStr("^m*ACTOR* is surrounded by a magical shield.^x")
        .selfDelStr("^MYour magical shield dissipates.^x")
        .roomDelStr("^M*ACTOR*'s magical shield dissipates.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("resist-piercing")
        .display("^mResist-Piercing^x")
        .computeScript("mudLib.noPlayer(actor)")
        .pulsed(false)
        .type("Positive"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("resist-ranged")
        .display("^mResist-Ranged^x")
        .computeScript("mudLib.noPlayer(actor)")
        .pulsed(false)
        .type("Positive"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("resist-slashing")
        .display("^mResist-Slashing^x")
        .computeScript("mudLib.noPlayer(actor)")
        .pulsed(false)
        .type("Positive"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("resist-water")
        .display("^BResist-Water^x")
        .computeScript("effectLib.computeResist(actor, effect, applier)")
        .oppositeEffect("vuln-water")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^BYou feel resistant to water.^x")
        .roomAddStr("^B*ACTOR* looks resistant to water.^x")
        .selfDelStr("^BYou feel less resistant to water.^x")
        .roomDelStr("^B*ACTOR* looks less resistant to water.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("silence")
        .addBaseEffect("silence")
        .display("^CSilent!^x")
        .computeScript("effectLib.computeDisable(actor, effect, applier)")
        .applyScript("actor.unSilence()")
        .pulsed(false)
        .type("Negative")
        .selfAddStr("^CYour voice fades!^x")
        .roomAddStr("^C*ACTOR*'s voice fades.^x")
        .selfDelStr("^gYour voice returns!^x")
        .roomDelStr("^g*ACTOR*'s voice returns!^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("slow")
        .display("^gSlow^x")
        .computeScript("effectLib.computeStatLower(actor, effect, applier)")
        .oppositeEffect("haste")
        .applyScript("actor.addStatModEffect(effect)")
        .unApplyScript("actor.remStatModEffect(effect)")
        .pulsed(false)
        .type("Negative")
        .selfAddStr("^gThe world speeds up around you.^x")
        .roomAddStr("^g*ACTOR* starts moving much slower.^x")
        .selfDelStr("^gYou catch up to the world.^x")
        .roomDelStr("^g*ACTOR* catches up to the world.^x")
        .isSpellEffect(true)
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("slow-poison")
        .addBaseEffect("slow-poison")
        .display("^gSlow-Poison^x")
        .pulsed(false)
        .type("Positive")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("static-field")
        .display("^cStatic-Field^x")
        .computeScript("effectLib.computeBeneficial(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^cYour hair stands on end.^x")
        .roomAddStr("^c*ACTOR*'s hair stands on end.^x")
        .selfDelStr("^cYour hair is no longer standing on end.^x")
        .roomDelStr("^x*ACTOR*'s hair is no longer standing on end.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("stoneskin")
        .display("^GStoneskin^x")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^yYour skin hardens.^x")
        .roomAddStr("^y*ACTOR*'s skin hardens.^x")
        .selfDelStr("^yYour stoneskin dissipates.^x")
        .roomDelStr("^y*ACTOR*'s stoneskin dissipates.^x")
        .isSpellEffect(true)
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("strength")
        .display("^YStrength^x")
        .computeScript("effectLib.computeStatRaise(actor, effect, applier)")
        .oppositeEffect("enfeeblement")
        .applyScript("actor.addStatModEffect(effect)")
        .unApplyScript("actor.remStatModEffect(effect)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^YYou feel stronger!^x")
        .roomAddStr("^Y*ACTOR* looks stronger.^x")
        .selfDelStr("^yYour strength fades.^x")
        .roomDelStr("^y*ACTOR*'s strength fades.^x")
        .isSpellEffect(true)
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("tongues")
        .addBaseEffect("tongues")
        .display("^gTongues^x")
        .computeScript("effectLib.computeLanguages(actor, effect, applier)")
        .unApplyScript("actor.unApplyTongues()")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^gYou being speaking in tongues.^x")
        .roomAddStr("^g*ACTOR* begins speaking in tongues.^x")
        .selfDelStr("^gYou stop speaking in tongues.^x")
        .roomDelStr("^g*ACTOR* stops speaking in tongues.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("toxic-cloud")
        .display("^GToxic Cloud^x")
        .roomAddStr("^GA toxic cloud begins to fill the area.^x")
        .roomDelStr("^WThe toxic cloud clears the area.^x")
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("true-sight")
        .addBaseEffect("true-sight")
        .display("^WTrue-Sight^x")
        .computeScript("effectLib.computeDetect(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^gThings become much clearer.^x")
        .roomAddStr("^g*ACTOR*'s eyes glow green.^x")
        .selfDelStr("^WYour vision returns to normal.^x")
        .roomDelStr("^W*ACTOR*'s eyes lose their greenish glow.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("undead-ward")
        .addBaseEffect("undead-ward")
        .display("^CUndead-Ward^x")
        .computeScript("effectLib.computeBeneficial(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^WYou feel guarded against the undead.^x")
        .roomAddStr("^W*ACTOR* is now guarded against the undead.^x")
        .selfDelStr("^CYour shiver and suddenly feel vulnerable to the undead.^x")
        .roomDelStr("^C*ACTOR* is no longer protected against the undead.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("unhallow")
        .display("^DUnhallow^x")
        .roomAddStr("^DAn unholy aura fills the room.^x")
        .roomDelStr("^WThe unholy aura in the room fades.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("vampirism")
        .display("^rVampirism^x")
        .pulsed(false)
        .type("Neutral"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("vuln-air")
        .display("^yVuln-Air^x")
        .computeScript("effectLib.computeResist(actor, effect, applier)")
        .oppositeEffect("resist-air")
        .pulsed(false)
        .type("Negative")
        .selfAddStr("^cYou feel vulnerable to air.^x")
        .roomAddStr("^c*ACTOR* looks vulnerable to air.^x")
        .selfDelStr("^cYou feel less vulnerable to air.^x")
        .roomDelStr("^c*ACTOR* looks less vulnerable to air.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("vuln-chopping")
        .display("^rVuln-Chopping^x")
        // This magic only works out of the box with exec of string literals, not ones that have already been converted to a string
        // so we need to replace these with one liners, or have the *script functions dedent it

        .computeScript("mudLib.noPlayer(actor)")
        .pulsed(false)
        .type("Negative"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("vuln-cold")
        .display("^bVuln-Cold^x")
        .computeScript("effectLib.computeResist(actor, effect, applier)")
        .oppositeEffect("resist-cold")
        .pulsed(false)
        .type("Negative")
        .selfAddStr("^cYou feel vulnerable to cold.^x")
        .roomAddStr("^c*ACTOR* looks vulnerable to cold.^x")
        .selfDelStr("^cYou feel less vulnerable to cold.^x")
        .roomDelStr("^c*ACTOR* looks less vulnerable to cold.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("vuln-crushing")
        .display("^rVuln-Crushing^x")
        .computeScript("mudLib.noPlayer(actor)")
        .pulsed(false)
        .type("Negative"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("vuln-earth")
        .display("^YVuln-Earth^x")
        .computeScript("effectLib.computeResist(actor, effect, applier)")
        .oppositeEffect("resist-earth")
        .pulsed(false)
        .type("Negative")
        .selfAddStr("^cYou feel vulnerable to earth.^x")
        .roomAddStr("^c*ACTOR* looks vulnerable to earth.^x")
        .selfDelStr("^cYou feel less vulnerable to earth.^x")
        .roomDelStr("^c*ACTOR* looks less vulnerable to earth.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("vuln-electric")
        .display("^cVuln-Electric^x")
        .computeScript("effectLib.computeResist(actor, effect, applier)")
        .oppositeEffect("resist-electric")
        .pulsed(false)
        .type("Negative")
        .selfAddStr("^cYou feel vulnerable to electric.^x")
        .roomAddStr("^c*ACTOR* looks vulnerable to electric.^x")
        .selfDelStr("^cYou feel less vulnerable to electric.^x")
        .roomDelStr("^c*ACTOR* looks less vulnerable to electric.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("vuln-exotic")
        .display("^rVuln-Exotic^x")
        .computeScript("mudLib.noPlayer(actor)")
        .pulsed(false)
        .type("Negative"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("vuln-fire")
        .display("^rVuln-Fire^x")
        .computeScript("effectLib.computeResist(actor, effect, applier)")
        .oppositeEffect("resist-fire")
        .pulsed(false)
        .type("Negative")
        .selfAddStr("^cYou feel vulnerable to fire.^x")
        .roomAddStr("^c*ACTOR* looks vulnerable to fire.^x")
        .selfDelStr("^cYou feel less vulnerable to fire.^x")
        .roomDelStr("^c*ACTOR* looks less vulnerable to fire.^x"),
      effects
    );
    addToSet(
    EffectBuilder()
      .name("vuln-piercing")
      .display("^rVuln-Piercing^x")
        .computeScript("mudLib.noPlayer(actor)")
      .pulsed(false)
      .type("Negative"),
    effects
    );
    addToSet(
      EffectBuilder()
        .name("vuln-ranged")
        .display("^rVuln-Ranged^x")
        .computeScript("mudLib.noPlayer(actor)")
        .pulsed(false)
        .type("Negative"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("vuln-slashing")
        .display("^rVuln-Slashing^x")
        .computeScript("mudLib.noPlayer(actor)")
        .pulsed(false)
        .type("Negative"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("vuln-water")
        .display("^BVuln-Water^x")
        .computeScript("effectLib.computeResist(actor, effect, applier)")
        .oppositeEffect("resist-water")
        .pulsed(false)
        .type("Negative")
        .selfAddStr("^cYou feel vulnerable to water.^x")
        .roomAddStr("^c*ACTOR* looks vulnerable to water.^x")
        .selfDelStr("^cYou feel less vulnerable to water.^x")
        .roomDelStr("^c*ACTOR* looks less vulnerable to water.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("wall-of-fire")
        .display("^rWall of Fire^x")
        .roomAddStr("^RA magical wall-of-fire ignites and blocks passage to the '*LOW-ACTOR*' exit.^x")
        .roomDelStr("^WThe magical wall-of-fire blocking the '*LOW-ACTOR*' exit abruptly extinquishes.^x")
        .pulsed(true)
        .pulseScript("effectLib.pulseWall(actor, effect)")
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("wall-of-lightning")
        .display("^cWall of Lightning^x")
        .roomAddStr("^cA magical wall-of-lightning forms and blocks passage to the '*LOW-ACTOR*' exit.^x")
        .roomDelStr("^WThe magical wall-of-lightning blocking the '*LOW-ACTOR*' exit has dissipated.^x")
        .pulsed(true)
        .pulseScript("effectLib.pulseWall(actor, effect)")
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("wall-of-sleet")
        .display("^CWall of Sleet^x")
        .roomAddStr("^CA magical wall-of-sleet forms and blocks passage to the '*LOW-ACTOR*' exit.^x")
        .roomDelStr("^CThe magical wall-of-sleet blocking the '*LOW-ACTOR*' exit has faded away.^x")
        .pulsed(true)
        .pulseScript("effectLib.pulseWall(actor, effect)")
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("wall-of-force")
        .display("^MWall of Force^x")
        .roomAddStr("^MA magical wall-of-force appears and blocks passage to the '*LOW-ACTOR*' exit.^x")
        .roomDelStr("^WThe magical wall-of-force blocking the '*LOW-ACTOR*' exit abruptly vanishes.^x")
        .pulsed(true)
        .pulseScript("effectLib.pulseWall(actor, effect)"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("wall-of-thorns")
        .display("^oWall of Thorns^x")
        .roomAddStr("^oA wall of thorns grows and blocks passage to the *LOW-ACTOR*.^x")
        .roomDelStr("^WThe wall of thorns blocking the *LOW-ACTOR* withers away.^x")
        .pulsed(true)
        .pulseScript("effectLib.pulseWall(actor, effect)")
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("warmth")
        .addBaseEffect("warmth")
        .display("^RWarmth^x")
        .computeScript("effectLib.computeBeneficial(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^RA warm aura surrounds you.^x")
        .roomAddStr("^RA warm aura surrounds *LOW-ACTOR*.^x")
        .selfDelStr("^CA cold chill runs through your body.^x")
        .roomDelStr("^C*ACTOR* shivers.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("weakness")
        .display("^YWeakness^x")
        .computeScript("effectLib.computeStatLower(actor, effect, applier)")
        .oppositeEffect("fortitude")
        .applyScript("actor.addStatModEffect(effect)")
        .unApplyScript("actor.remStatModEffect(effect)")
        .pulsed(false)
        .type("Negative")
        .selfAddStr("^YA magical force saps your health.^x")
        .roomAddStr("^Y*ACTOR* looks unnaturally pale.^x")
        .selfDelStr("^YYour health returns.^x")
        .roomDelStr("^Y*ACTOR*'s health returns.^x")
        .isSpellEffect(true)
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("well-of-magic")
        .addBaseEffect("well-of-magic")
        .display("^sWell-of-Magic^x")
        .computeScript("effectLib.computeRegen(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^sYou feel magically invigorated.^x")
        .roomAddStr("^s*ACTOR* feels magically invigorated.^x")
        .selfDelStr("^sYour magic vitality fades.^x")
        .roomDelStr("^s*ACTOR*'s magic vitality fades.^x")
        .isSpellEffect(true)
        .useStrength(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("wind-protection")
        .display("^cWind-Protection^x")
        .computeScript("effectLib.computeBeneficial(actor, effect, applier)")
        .pulsed(false)
        .type("Positive")
        .selfAddStr("^cYour skin toughens.^x")
        .roomAddStr("^c*ACTOR*'s skin toughens.^x")
        .selfDelStr("^cYou are no longer protected from the wind.^x")
        .roomDelStr("^x*ACTOR*'s skin softens.^x")
        .isSpellEffect(true),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("wounded")
        .addBaseEffect("curse")
        .display("^rFestering Wounds^x")
        .pulsed(true)
        .pulseDelay(20)
        .pulseScript("effectLib.pulseFestering(actor, effect)")
        .type("Negative")
        .selfDelStr("Your wounds are no longer unclean."),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("chorusofclarity")
        .display("^WChorus of Clarity^x")
        .type("Positive")
        .selfAddStr("^CA soft breeze slips into your mind.^x")
        .roomAddStr("^C*ACTOR* looks very tranquil.^x")
        .selfDelStr("^CThe soft breeze fades.^x")
        .roomDelStr("^C*ACTOR*'s tranquility fades.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("heartwarmingrapsody")
        .addBaseEffect("warmth")
        .display("^RHeartwarming Rapsody^x")
        .type("Positive")
        .selfAddStr("^RHeartwarming rapsody warms your heart.^x")
        .roomAddStr("^R*ACTOR* is warmed by a Heartwarming Rapsody.^x")
        .selfDelStr("^cThe warmth in your heart fades.^x")
        .roomDelStr("^cThe warmth in *ACTOR*'s heart fades.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("songofsight")
        .addBaseEffect("infravision")
        .addBaseEffect("detect-invisible")
        .addBaseEffect("detect-magic")
        .display("^WSong of Sight^x")
        .type("Positive")
        .selfAddStr("^WThe world reveals itself to you.^x")
        .roomAddStr("^WThe world is revealed to *ACTOR*.")
        .selfDelStr("^wThe world seems a little fuzzier.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("haileyshastinghymn")
        .addBaseEffect("haste")
        .display("^GHailey's Hasting Hymn^x")
        .type("Positive")
        .selfAddStr("^g*APPLIER-POS-SELF*'s hymns speed your movements.^x")
        .roomAddStr("^g*ACTOR* is hastened by *APPLIER-POS* hymns!^x")
        .selfDelStr("^gThe hasting hymn fades.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("battlechant")
        .display("^RBattle Chant^x")
        .type("Positive")
        .selfAddStr("^RThe battle chant aguments your battle readiness!^x")
        .roomAddStr("^R*ACTOR*'s battle readiness is augmented by the Battle Chant!^x")
        .selfDelStr("^RYour battle readiness fades.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("songofvitality")
        .display("^gSong of Vitality^x")
        .type("Positive")
        .selfAddStr("^gYour vitality feels augmented by the Song of Vitality^x")
        .roomAddStr("^g*ACTOR*'s vitality improves!^x")
        .selfDelStr("^gThe augmented vitality fades.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("aquaticayre")
        .addBaseEffect("breathe-water")
        .display("^bAquatic Ayre^x")
        .type("Positive")
        .selfAddStr("^bThe Aquatic Ayre fills your lungs with air.^x")
        .roomAddStr("^b*ACTOR*'s lungs are filled with air from the Aquatic Ayre.^x")
        .selfDelStr("^bThe Aquatic Ayre fades.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("sonorusserenade")
        .addBaseEffect("invisibility")
        .addBaseEffect("detect-invisibility")
        .display("^WSonorus Serenade^x")
        .type("Positive")
        .selfAddStr("^W*APPLIER-SELF-POS* Serenade hides you from view!^x")
        .roomAddStr("^W*ACTOR* is hidden from view by *LOW-APPLIER-POS* Serenade!^x")
        .selfDelStr("^wThe Serenade fades.^x")
        .roomDelStr("^wThe Seranade about *ACTOR* fades.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("hymnofrevitalization")
        .display("^gHymn of Revitalization^x")
        .type("Positive")
        .selfAddStr("^gThe song of revitalization hastens the healing of your wounds.^x")
        .roomAddStr("^gThe song of revitalization hastens the healing of *LOW-ACTOR*'s wounds.^x")
        .selfDelStr("^gThe revitalizing hymn fades.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("warsong")
        .addBaseEffect("haste")
        .display("^RWarsong^x")
        .type("Positive")
        .selfAddStr("^RYou feel ready for war!^x")
        .roomAddStr("^R*ACTOR* looks ready for war!^x")
        .selfDelStr("^RYour war-readiness fades.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("songofpurification")
        .addBaseEffect("slow-poison")
        .display("^gSong of Purification^x")
        .type("Positive")
        .selfAddStr("^gYou feel purified.^x")
        .roomAddStr("^g*ACTOR* looks purified.^x")
        .selfDelStr("^gThe song of purification fades.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("guardianrhythms")
        .display("^yGuardian Rhythms^x")
        .type("Positive")
        .selfAddStr("^yYou feel guarded by *LOW-APPLIER-SELF-POS* rhythms.^x")
        .roomAddStr("^y*ACTOR* is guarded by *LOW-APPLIER-POS* rhythms.^x")
        .selfDelStr("^yThe rhythms guarding you fade.^x"),
      effects
    );
    addToSet(
      EffectBuilder()
        .name("chainsofdissonance")
        .addBaseEffect("slow")
        .display("^bChains of Dissonance^x")
        .type("Negative"),
      effects
    );

    return true;
}


//*********************************************************************
//                      clearEffects
//*********************************************************************

void Config::clearEffects() {
    effects.clear();
}
