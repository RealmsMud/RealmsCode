/*
 * effectsAttr.cpp
 *   Effect attributes
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
#include "effects.h"

//*********************************************************************
//						Effect
//*********************************************************************

Effect::Effect(xmlNodePtr rootNode) {
	xmlNodePtr curNode = rootNode->children;

    pulsed = isSpellEffect = usesStr = false;
    pulseDelay = 5;

	while(curNode) {
			 if(NODE_NAME(curNode, "Name")) xml::copyToBString(name, curNode);
		else if(NODE_NAME(curNode, "BaseEffect")) baseEffects.push_back(xml::getBString(curNode));
		else if(NODE_NAME(curNode, "Display")) xml::copyToBString(display, curNode);
		else if(NODE_NAME(curNode, "OppositeEffect")) xml::copyToBString(oppositeEffect, curNode);
		else if(NODE_NAME(curNode, "SelfAddStr")) xml::copyToBString(selfAddStr, curNode);
		else if(NODE_NAME(curNode, "SelfDelStr")) xml::copyToBString(selfDelStr, curNode);
		else if(NODE_NAME(curNode, "RoomAddStr")) xml::copyToBString(roomAddStr, curNode);
		else if(NODE_NAME(curNode, "RoomDelStr")) xml::copyToBString(roomDelStr, curNode);
		else if(NODE_NAME(curNode, "Pulsed")) xml::copyToBool(pulsed, curNode);
		else if(NODE_NAME(curNode, "PulseDelay")) xml::copyToNum(pulseDelay, curNode);
		else if(NODE_NAME(curNode, "Type")) xml::copyToBString(type, curNode);
		else if(NODE_NAME(curNode, "ComputeScript")) xml::copyToBString(computeScript, curNode);
		else if(NODE_NAME(curNode, "ApplyScript")) xml::copyToBString(applyScript, curNode);
		else if(NODE_NAME(curNode, "PreApplyScript")) xml::copyToBString(preApplyScript, curNode);
		else if(NODE_NAME(curNode, "PostApplyScript")) xml::copyToBString(postApplyScript, curNode);
		else if(NODE_NAME(curNode, "UnApplyScript")) xml::copyToBString(unApplyScript, curNode);
		else if(NODE_NAME(curNode, "PulseScript")) xml::copyToBString(pulseScript, curNode);
		else if(NODE_NAME(curNode, "Spell")) xml::copyToBool(isSpellEffect, curNode);
		else if(NODE_NAME(curNode, "UsesStrength")) xml::copyToBool(usesStr, curNode);

		curNode = curNode->next;
	}
}

//*********************************************************************
//						getPulseScript
//*********************************************************************

bstring Effect::getPulseScript() const {
    return(pulseScript);
}

//*********************************************************************
//						getUnApplyScript
//*********************************************************************

bstring Effect::getUnApplyScript() const {
    return(unApplyScript);
}

//*********************************************************************
//						getApplyScript
//*********************************************************************

bstring Effect::getApplyScript() const {
    return(applyScript);
}

//*********************************************************************
//						getPreApplyScript
//*********************************************************************

bstring Effect::getPreApplyScript() const {
    return(preApplyScript);
}

//*********************************************************************
//						getPostApplyScript
//*********************************************************************

bstring Effect::getPostApplyScript() const {
    return(postApplyScript);
}

//*********************************************************************
//						getComputeScript
//*********************************************************************

bstring Effect::getComputeScript() const {
    return(computeScript);
}

//*********************************************************************
//						getType
//*********************************************************************

bstring Effect::getType() const {
    return(type);
}

//*********************************************************************
//						isPulsed
//*********************************************************************

bool Effect::isPulsed() const {
    return(pulsed);
}

//*********************************************************************
//						isSpell
//*********************************************************************

bool Effect::isSpell() const {
    return(isSpellEffect);
}

//*********************************************************************
//						isSpell
//*********************************************************************

bool Effect::usesStrength() const {
    return(usesStr);
}

//*********************************************************************
//						getRoomDelStr
//*********************************************************************

bstring Effect::getRoomDelStr() const {
    return(roomDelStr);
}

//*********************************************************************
//						getRoomAddStr
//*********************************************************************

bstring Effect::getRoomAddStr() const {
    return(roomAddStr);
}

//*********************************************************************
//						getSelfDelStr
//*********************************************************************

bstring Effect::getSelfDelStr() const {
    return(selfDelStr);
}

//*********************************************************************
//						getSelfAddStr
//*********************************************************************

bstring Effect::getSelfAddStr() const {
    return(selfAddStr);
}

//*********************************************************************
//						getOppositeEffect
//*********************************************************************

bstring Effect::getOppositeEffect() const {
    return(oppositeEffect);
}

//*********************************************************************
//						getDisplay
//*********************************************************************

bstring Effect::getDisplay() const {
    return(display);
}

//*********************************************************************
//						hasBaseEffect
//*********************************************************************

bool EffectInfo::hasBaseEffect(const bstring& effect) const {
	return(myEffect->hasBaseEffect(effect));
}

//*********************************************************************
//						hasBaseEffect
//*********************************************************************

bool Effect::hasBaseEffect(const bstring& effect) const {
	for(const bstring& be : baseEffects) {
		if(be == effect)
			return(true);
	}
	return(false);
}

//*********************************************************************
//						getName
//*********************************************************************

bstring Effect::getName() const {
    return(name);
}

//*********************************************************************
//						EffectInfo
//*********************************************************************
// TODO: Add applier here

EffectInfo::EffectInfo(bstring pName, time_t pLastMod, long pDuration, int pStrength, MudObject* pParent, const Creature* owner) {
	clear();
	myEffect = gConfig->getEffect(pName);
	if(!myEffect)
		throw bstring("Can't find effect " + pName);
	name = pName;
	setOwner(owner);
	lastPulse = lastMod = pLastMod;
	duration = pDuration;
	strength = pStrength;

    myParent = pParent;
}

//*********************************************************************
//						EffectInfo
//*********************************************************************

EffectInfo::EffectInfo() {
	clear();
}

//*********************************************************************
//						EffectInfo
//*********************************************************************

EffectInfo::EffectInfo(xmlNodePtr rootNode) {
	clear();
	xmlNodePtr curNode = rootNode->children;

	while(curNode) {
			if(NODE_NAME(curNode, "Name")) xml::copyToBString(name, curNode);
		else if(NODE_NAME(curNode, "Duration")) xml::copyToNum(duration, curNode);
		else if(NODE_NAME(curNode, "Strength")) xml::copyToNum(strength, curNode);
		else if(NODE_NAME(curNode, "Extra")) xml::copyToNum(extra, curNode);
		else if(NODE_NAME(curNode, "PulseModifier")) xml::copyToNum(pulseModifier, curNode);

		curNode = curNode->next;
	}

	lastPulse = lastMod = time(0);
	myEffect = gConfig->getEffect(name);

	if(!myEffect) {
		throw bstring("Can't find effect listing " + name);
	}
}

//*********************************************************************
//						clear
//*********************************************************************

void EffectInfo::clear() {
	myParent = 0;
	myEffect = 0;
	lastMod = 0;
    lastPulse = 0;
    pulseModifier = 0;
	duration = 0;
	strength = 0;
	extra = 0;
    myApplier = 0;
}

//*********************************************************************
//						EffectInfo
//*********************************************************************

EffectInfo::~EffectInfo() {
}

//*********************************************************************
//						setParent
//*********************************************************************

void EffectInfo::setParent(MudObject* pParent) {
	myParent = pParent;
}

//*********************************************************************
//						getEffect
//*********************************************************************

Effect* EffectInfo::getEffect() const {
    return(myEffect);
}

//*********************************************************************
//						getName
//*********************************************************************

const bstring EffectInfo::getName() const {
    return(name);
}

//*********************************************************************
//						getOwner
//*********************************************************************

const bstring EffectInfo::getOwner() const {
    return(pOwner);
}

//*********************************************************************
//						isOwner
//*********************************************************************

bool EffectInfo::isOwner(const Creature* owner) const {
	// currently, only players can own effects
	return(owner && owner->isPlayer() && pOwner == owner->name);
}

//*********************************************************************
//						getLastMod
//*********************************************************************

time_t EffectInfo::getLastMod() const {
    return(lastMod);
}

//*********************************************************************
//						getDuration
//*********************************************************************

long EffectInfo::getDuration() const {
    return(duration);
}

//*********************************************************************
//						getStrength
//*********************************************************************

int EffectInfo::getStrength() const {
    return(strength);
}

//*********************************************************************
//						getExtra
//*********************************************************************

int EffectInfo::getExtra() const {
    return(extra);
}

//*********************************************************************
//						isPermanent
//*********************************************************************

bool EffectInfo::isPermanent() const {
    return(duration == -1);
}

//*********************************************************************
//						getParent
//*********************************************************************

MudObject* EffectInfo::getParent() const {
    return(myParent);
}

//*********************************************************************
//						getApplier
//*********************************************************************

MudObject* EffectInfo::getApplier() const {
    return(myApplier);
}

//*********************************************************************
//						setOwner
//*********************************************************************

void EffectInfo::setOwner(const Creature* owner) {
	if(owner)
		pOwner = owner->name;
	else
		pOwner = "";
}

//*********************************************************************
//						setStrength
//*********************************************************************

void EffectInfo::setStrength(int pStrength) {
	if(!this)
		return;
	strength = pStrength;
}

//*********************************************************************
//						setExtra
//*********************************************************************

void EffectInfo::setExtra(int pExtra) {
	if(!this)
		return;
	extra = pExtra;
}

//*********************************************************************
//						setDuration
//*********************************************************************

void EffectInfo::setDuration(long pDuration) {
	if(!this)
		return;
	duration = pDuration;
}
