import mud
import mudLib
import stats
from mudObject import MudObject
from effects import EffectInfo
from typing import Optional


porphyriaMinutes = 90

def sayHi(actor):
	actor.send("Hi Baby!\n")

def computeBeneficial(actor: MudObject, effect: EffectInfo, applier: Optional[MudObject]) -> bool:
	duration = 0

	if applier is None or applier.getObject():
		duration = 1200
	else:
		duration = 1200
		bonus = max(60, min(3000, ((applier.intelligence.getCur()-140) * 18)))

		if applier.getClass() == mud.crtClasses.CLERIC or applier.getClass() == mud.crtClasses.PALADIN:
			bonus += 60 * applier.getLevel()

		duration += bonus

		if applier.getRoom().hasMagicBonus():
			duration += 800

	if (effect.getName() == "warmth" and actor.isEffected("alwayswarm")) or (effect.getName() == "heat-protection" and actor.isEffected("alwayscold")):
		duration /= 4

	effect.setDuration(int(duration))
	return True


def computeGravity(actor: MudObject, effect: EffectInfo, applier: Optional[MudObject]) -> bool:
	duration = 0
	
	if applier is None or applier.getObject():
		duration = 800
	else:
		duration = 2400
		bonus = max(60, min(3000, ((applier.intelligence.getCur()-140) * 18)))
		duration += bonus
	
		if applier.getRoom().hasMagicBonus():
			duration += 800
	

	effect.setDuration(int(duration))
	return True

def computeVisibility(actor: MudObject, effect: EffectInfo, applier: Optional[MudObject]) -> bool:
	duration = 0

	if applier is None or applier.getObject():
		duration = 1200
	else:
		if effect.getName() == "invisibility":
			duration = 1200
			duration += (applier.intelligence.getCur()-140) * 18
		elif effect.getName() == "pass-without-trace":
			duration = 1600
			duration += (applier.intelligence.getCur()-140) * 25
		elif effect.getName() == "blur":
			duration = 600
			duration += (applier.intelligence.getCur()-140) * 5
		else:
			duration = 600
			duration += (applier.intelligence.getCur()-140) * 9

		if applier.getClass() == mud.crtClasses.MAGE or applier.isCt():
			duration += 60 * applier.getLevel()

		if applier.getRoom().hasMagicBonus():
			duration += 600


		if applier == actor:
			duration = max(600, duration)
		else:
			duration = max(60, duration)

	effect.setDuration(int(duration))
	return True


def computeDetect(actor: MudObject, effect: EffectInfo, applier: Optional[MudObject]) -> bool:
	duration = 0
	if applier is None or applier.getObject():
		if effect.getName()  == "true-sight" or effect.getName() == "farsight":
			duration = 800
		else:
			duration = 1200
	else:
		if effect.getName() == "true-sight":
			duration = 300
			duration += ((applier.intelligence.getCur() - 140) * 18)
			if applier.getClass() == mud.crtClasses.CLERIC:
				duration += applier.getLevel() * 60
			if applier.getRoom().hasMagicBonus():
				duration += 300
		elif effect.getName() == "farsight":
			duration = 300
			duration += (applier.intelligence.getCur()-140) * 9
			if applier.getRoom().hasMagicBonus():
				duration += 400
		else:
			duration = 1200
			duration += ((applier.intelligence.getCur() - 140) * 18)
			if applier.getClass() == mud.crtClasses.MAGE:
				duration += 60 * applier.getLevel()
			if applier.getRoom().hasMagicBonus():
				duration += 600
		if applier == actor:
			duration = max(600, duration)
		else:
			duration = max(60, duration)

	effect.setDuration(int(duration))
	return True

def computeDarkInfra(actor: MudObject, effect: EffectInfo, applier: Optional[MudObject]) -> bool:
	duration = 0
	if applier is None or applier.getObject():
		applier = actor

	duration = 300 + applier.getLevel() * 300
	if applier.getRoom().hasMagicBonus():
		duration += 600

	if effect.getName() == "darkness":
		if not applier.isStaff():
			if applier.getDeity() == mud.religions.ENOCH:
				duration = duration * 2 / 3
			elif applier.getDeity() == mud.religions.ARACHNUS:
				duration = duration * 3 / 2

	if applier == actor:
		duration = max(600, duration)
	else:
		duration = max(60, duration)

	effect.setDuration(int(duration))
	return True

def computeStatLower(actor: MudObject, effect: EffectInfo, applier: Optional[MudObject]) -> bool:
	if actor.removeOppositeEffect(effect):
		return False
	if actor.isUndead() and effect.getName() == "weakness":
		return False

	duration = 0
	strength = -30

	if applier is None or applier.getObject():
		duration = 60
	else:
		duration = 180
		if applier.getRoom().hasMagicBonus():
			duration += 90


	effect.setStrength(strength)
	effect.setDuration(int(duration))

	return True

def computeStatRaise(actor: MudObject, effect: EffectInfo, applier: Optional[MudObject]) -> bool:
	if actor.removeOppositeEffect(effect):
		return False
	if actor.isUndead() and effect.getName() == "fortitude":
		return False

	duration = 0
	strength = 30

	if applier is None or applier.getObject():
		duration = 60
	else:
		duration = 180
		if applier.getRoom().hasMagicBonus():
			duration += 90


	effect.setStrength(strength)
	effect.setDuration(int(duration))

	return True

def computeLanguages(actor: MudObject, effect: EffectInfo, applier: Optional[MudObject]) -> bool:
	duration = 0

	if applier is None or applier.getObject():
		duration = 300
	else:
		duration = max(300, 480 + ((applier.intelligence.getCur() - 140 ) * 2))
		if applier.getRoom().hasMagicBonus():
			duration += 180
	
	effect.setDuration(int(duration))
	return True

def computeResist(actor: MudObject, effect: EffectInfo, applier: Optional[MudObject]) -> bool:
	actor.removeOppositeEffect(effect)
	duration = 0
	
	if applier is None or applier.getObject():
		if effect.getName() == "resist-magic":
			duration = 1200
		else:
			duration = 300
	else:
		duration = max(300, 1200 + ((applier.intelligence.getCur() - 140) * 18) )

		if applier.getRoom().hasMagicBonus():
			duration += 800

	effect.setDuration(int(duration))
	return True

def computeDisable(actor: MudObject, effect: EffectInfo, applier: Optional[MudObject]) -> bool:
	duration = 0

	if effect.getName() == "petrification":
		duration = -1
	elif effect.getName() == "hold-person":
		if applier is None or applier.getObject():
			duration = mud.rand(9, 12)
		else:
			duration = mud.rand(12,18) + ((applier.intelligence.getCur() * 2) - ((actor.intelligence.getCur() + actor.piety.getCur())/2) - 140)/20
	elif effect.getName() == "confusion":
		if applier is None or applier.getObject():
			duration = 60
		else:
			duration = max(60, applier.intelligence.getCur())
	elif effect.getName() == "blindness":
		duration = 300 - (actor.constitution.getCur()/10)
	elif effect.getName() == "silence":
		duration = 600

	effect.setDuration(int(duration))
	return True

def computeNatural(actor: MudObject, effect: EffectInfo, applier: Optional[MudObject]) -> bool:
	if effect.getName() == "enlarge" or effect.getName() == "reduce":
		# TODO: Modify removeOpposite Effect to not remove perm effects
		if actor.removeOppositeEffect(effect):
			return False

	if applier is None or applier.getObject():
		duration = 800
		strength = 1
		effect.setStrength(strength)
		effect.setDuration(int(duration))
		enlarge = False
		if effect.getName() == "enlarge":
			enlarge = True
		if not actor.changeSize(0, strength, enlarge):
			return False
#	// FROM_CREATURE is handled in the spell because it's strength can be set
#	// manually by the caster
	return True

def computeDeathSickness(actor: MudObject, effect: EffectInfo, applier: Optional[MudObject]) -> bool:
	duration = 0
	strength = 1

	# Two minutes per level
	duration = actor.getLevel() * 30
	# We'll start the strength out at 100 and reduce it each pulse
	strength = 100

	effect.setStrength(strength)
	effect.setDuration(int(duration))
	return True
	
def pulseDeathSickness(actor: MudObject, effect: EffectInfo) -> bool:
	strength = effect.getStrength()
	duration = effect.getDuration()

	if not actor.isEffected("petrification") and mud.rand(1,100) < (strength/2):
		actor.wake("^DA strong urge to vomit wakes you!")
		actor.send("^GYour death-sickness causes you to vomit. EWWW.\n")
		actor.unhide()

		# Stun then half of the time for 0-2 seconds
		if(mud.rand(1,100) < 50):
			actor.stun(mud.rand(0,2))
			actor.send("^DYou become disoriented.\n")


		if actor.getRoom():
			mudLib.broadcastRoom(actor.getRoom(), "^G*ACTOR* vomits all over the ground.", actor=actor, ignore=actor)

	newStrength = 0
	if strength != 0:
		newStrength = float(strength) - ((float(strength)/float(duration))*20.0)

	if strength > 75 and newStrength <= 75:
		actor.send("^cYou feel a little better.\n")
		if actor.getRoom():
			mudLib.broadcastRoom(actor.getRoom(), "^c*ACTOR* looks a little better.", actor=actor, ignore=actor)
	elif strength > 50 and newStrength <= 50:
		actor.send("^cYou feel better.\n")
		if actor.getRoom():
			mudLib.broadcastRoom(actor.getRoom(), "^c*ACTOR* looks better.", actor=actor, ignore=actor)
	elif strength > 25 and newStrength <= 25:
		actor.send("^cYou are nearly recovered.\n")
		if actor.getRoom():
			mudLib.broadcastRoom(actor.getRoom(), "^c*ACTOR* looks nearly recovered.", actor=actor, ignore=actor)

	strength = min(max(newStrength,0), 100)
	#actor.send("DEATH SICKNESS: New Strength " + str(strength) + ", Duration " + str(duration) + "\n")
	effect.setStrength(int(strength))


# Needs: 

#		Creature::poisonedByPlayer
#		Creature::poisonedByMonster
def pulsePoison(actor: MudObject, effect: EffectInfo) -> bool:
	if effect.getName() == "poison":
		actor.wake("Terrible nightmares disturb your sleep!")
		actor.send("^r^#Poison courses through your veins.\n")
		mudLib.broadcastRoom(actor.getRoom(), "^cPoison courses through *LOW-ACTOR*'s veins.", actor=actor, ignore=actor)

		dmg = effect.getStrength() + mud.rand(1,3)
		if actor.constitution.getCur() > 120:
			percent = 1.0 - (actor.constitution.getCur() - 120.0) / (680.0 - 120.0)
			percent *= dmg
			dmg = int(percent)
		dmg = max(1, dmg)
		actor.hp.decrease(dmg);

		if actor.isMonster() and actor.getPoisonedBy() != "":
			player = mud.gServer.findPlayer(actor.getPoisonedBy())
			if not player is None:
				exp = dmg
				if player.getDeity() != mud.religions.ARACHNUS:
					exp /= 2
				actor.getMonster().addEnmDmg(player, exp)

		if actor.hp.getCur() < 1:
			mudLib.broadcastRoom(actor.getRoom(), "*ACTOR* drops dead from poison.", actor=actor, ignore=actor)
			actor.setDeathType(mud.DeathType.POISON_GENERAL)
			if actor.isPlayer():
				if actor.poisonedByPlayer():
					actor.setDeathType(mud.DeathType.POISON_PLAYER)
				elif actor.poisonedByMonster():
					actor.setDeathType(mud.DeathType.POISON_MONSTER)
			return False
	return True


def pulseDisease(actor: MudObject, effect: EffectInfo) -> bool:
	if effect.getName() == "disease":
		actor.wake("Terrible nightmares disturb your sleep!")
		actor.send("^bYou feel nauseous.\n^r^#Fever grips your mind.\n")
		mudLib.broadcastRoom(actor.getRoom(), "^cFever grips *LOW-ACTOR*.", actor=actor, ignore=actor)

		dmg = effect.getStrength() + mud.rand(1,3)
		if actor.constitution.getCur() > 120:
			# a spread between 400 (50%) and 120 (0%) resistance
			percent = 1.0 - (actor.constitution.getCur() - 120.0) / (680.0 - 120.0)
			percent *= dmg
			dmg = int(percent)
		dmg = max(1, dmg)
		actor.hp.decrease(dmg);

		if actor.hp.getCur() < 1:
			mudLib.broadcastRoom(actor.getRoom(), "*ACTOR* dies from disease.", actor=actor, ignore=actor)
			actor.setDeathType(mud.DeathType.DISEASE)
			return False
	return True


def pulseFestering(actor: MudObject, effect: EffectInfo) -> bool:
	dmg = mud.rand(int(1+actor.hp.getMax()/30), int(1+actor.hp.getMax()/20))
	dmg=max(1, dmg)
	actor.wake("Terrible nightmares disturb your sleep!")
	actor.send("^bYour wounds fester and bleed for " + actor.customColorize("*CC:DAMAGE*") + str(dmg) + "^b damage.^x\n")
	mudLib.broadcastRoom(actor.getRoom(), "*ACTOR*'s wounds fester and bleed.", actor=actor, ignore=actor)
	actor.hp.decrease(dmg)
	if actor.hp.getCur() < 1:
		mudLib.broadcastRoom(actor.getRoom(), "*ACTOR* dies from cursed wounds.", actor=actor, ignore=actor)
		actor.setDeathType(mud.DeathType.WOUNDED)
		return False
	return True


def pulseCreepingDoom(actor: MudObject, effect: EffectInfo) -> bool:
	if actor.isMonster() and actor.getType == mud.mType.ARACHNID:
		return True
	if actor.getDeity() == mud.religions.ARACHNUS:
		return True

	actor.wake("Terrible nightmares disturb your sleep!")
	dmg=int(effect.getStrength() / 2 + mud.rand(1,3))

	actor.send("^DCursed spiders crawl all over your body and bite you for " + actor.customColorize("*CC:DAMAGE*") + str(dmg) + "^D damage.^x\n")

	mudLib.broadcastRoom(actor.getRoom(), "Cursed spiders crawl all over *ACTOR*.", actor=actor, ignore=actor)
	
	actor.hp.decrease(dmg)
	actor.send("Current HP: " + str(actor.hp.getCur()) + "\n")
	if actor.hp.getCur() < 1:
		actor.send("^DThe cursed spiders devour you!^x\n")
		mudLib.broadcastRoom(actor.getRoom(), "Cursed spiders devour *ACTOR*!", actor=actor, ignore=actor)
		actor.setDeathType(mud.DeathType.CREEPING_DOOM)
		return False
	return True


def computeLycanthropy(actor: MudObject, effect: EffectInfo, applier: Optional[MudObject]) -> bool:
	if not actor.willBecomeWerewolf():
		return False
	
	effect.setStrength(1)
	# 90 minutes until lycanthropy takes effect
	effect.setDuration(90 * 60)
	return True
	
	
def pulseLycanthropy(actor: MudObject, effect: EffectInfo) -> bool:
	# permanent lycanthropy has no pulse effects
	if effect.getDuration() == -1:
		return True
	
	# Grow in strength with every pulse (Making it harder to cure the longer they wait)
	effect.setStrength(effect.getStrength() + 1)
	if mud.rand(1,5) == 1:
		msg = mud.rand(1,3)
		if msg == 1:
			actor.send("^gThe hair on your arms seems to grow longer.\n")
		elif msg == 2:
			actor.send("^gYou feel a strong urge to howl at the moon.\n")
		elif msg == 3:
			actor.send("^gYour ears twitch.\n")
	
	# if they let their disease run too low, it'll become permanent
	if effect.getDuration() <= 20:
		actor.send("^gYour body suddenly tenses; undergoing a dramatic change.\n")
		actor.send("^gYou feel your eyes sharpen and focus, your ears flick back and your nostrils flare.\n")
		actor.send("^gYou throw your head back and howl at the top of your lungs.\n")
		mudLib.broadcastRoom(actor.getRoom(), "^g*ACTOR* suddenly tenses; *A-HISHER* body undergoes a dramatic change.", actor=actor, ignore=actor)
		mudLib.broadcastRoom(actor.getRoom(), "^g*A-UPHISHER* eyes sharpen and focus, *A-HISHER* ears flick back, mimicking an alert wolf;", actor=actor, ignore=actor)
		mudLib.broadcastRoom(actor.getRoom(), "^g*A-UPHISHER* nostrils flare and *A-HISHER* pupils dilate.", actor=actor, ignore=actor)
		mudLib.broadcastRoom(actor.getRoom(), "^g*ACTOR* radiates with the essence of the hunt.", actor=actor, ignore=actor)
		mudLib.broadcastRoom(actor.getRoom(), "\n^g*ACTOR* throws *A-HISHER* head back and howls at the top of *A-HISHER* lungs.", actor=actor, ignore=actor)
		actor.makeWerewolf()
		effect.setDuration(-1)

	return True


def computePorphyria(actor: MudObject, effect: EffectInfo, applier: Optional[MudObject]) -> bool:
	# how many minutes until porphyria takes effect
	minutes = 90
	
	effect.setStrength(1)
	if not actor.willBecomeVampire():
		return False
	effect.setStrength(1)
	effect.setDuration(porphyriaMinutes * 60);
	return True


def pulsePorphyria(actor: MudObject, effect: EffectInfo) -> bool:
	# how many minutes until porphyria takes effect
	
	
	pActor = actor.getPlayer()
	dmg = mud.rand(1, max(2, int(effect.getStrength()/10) ) )
	actor.wake("Terrible nightmares disturn your sleep!")

	# Player inflicted porphyira does less damage
	if actor.isPlayer() is not None and pActor.getAfflictedBy() != "":
		dmg /= 2

	dmg = max(1,dmg)
	# pulse once every 20 seconds, base 50% chance to take damage
	# if a player is the cause, make it happen less often
	remove = False
	if mud.rand(0,1) == 1:
		effect.setStrength(effect.getStrength() + 1)
		msg = mud.rand(1,4)
		if msg == 1:
			actor.send("^rBlood sputters through your veins.\n")
		elif msg == 2:
			actor.send("^rBlood drains from your head.\n")
		elif msg == 3:
			actor.send("^rFever grips your mind.\n")
		elif msg == 4:
			actor.send("^WA sharp headache strikes you.\n")
		# Todo: replace with customColorize *CC:DAMAGE*
		actor.send("You take " + actor.customColorize("*CC:DAMAGE*") + str(dmg) + "^x damage.\n");
		actor.hp.decrease(dmg)
	# Pulse every 3 minutes
	if (effect.getStrength() > (porphyriaMinutes * 3) and not actor.getRoom().isSunlight()) or (actor.isPlayer() and actor.hp.getCur() < 1):
		remove = True
		if actor.willBecomeVampire():
			actor.makeVampire()
			if actor.isPlayer():
				actor.send("^rYour body shudders, your limbs go as cold as ice!\n")
				actor.send("^rFangs suddenly grow, replacing your teeth and sending blood running down your throat!\n")
				actor.send("^R^#You have been turned into a vampire.\n")
			mudLib.broadcastRoom(actor.getRoom(), "^r*ACTOR* shudders and convulses!", actor=actor, ignore=actor)
			mudLib.broadcastRoom(actor.getRoom(), "^rFangs grow from *ACTOR*'s teeth!", actor=actor, ignore=actor)
	
	if actor.hp.getCur() < 1:
		mudLib.broadcastRoom(actor.getRoom(), "*ACTOR* dies from porphyria!", actor=actor, ignore=actor)
		actor.setDeathType(mud.DeathType.DISEASE)
		return False

	if remove == True:
		return False
	elif effect.getDuration() <= 20:
		# porphyria should never actually wear off; if they are staying in
		# the sunlight to avoid becoming a vampire, just prolong it
		effect.setDuration(effect.getDuration() + 20)
	return True


def pulseWall(actor: MudObject, effect: EffectInfo) -> bool:
	if effect.getExtra() > 0:
		effect.setExtra(effect.getExtra() - 1)
		if effect.getExtra() == 0:
			ef = effect.getEffect();
			mudLib.broadcastRoom(effect.getParent().getRoom(), ef.getRoomAddStr(), actor=actor)

	return True


def pulseCamouflage(actor: MudObject, effect: EffectInfo) -> bool:
	if effect.getDuration() <= 60 and actor.getClass() == mud.crtClasses.DRUID and actor.getRoom().isForest():
		effect.setDuration(60)
	return True

def computeRegen(actor: MudObject, effect: EffectInfo, applier: Optional[MudObject]) -> bool:
	duration = 0

	# 30 + 1/2 minute per level
	duration = 1800 + actor.getLevel() * 30

	effect.setDuration(int(duration))
	return True
	
	
def computeBloodSac(actor: MudObject, effect: EffectInfo, applier: Optional[MudObject]) -> bool:
	strength = 0
	
	percentage = mud.getConBonusPercentage(actor.constitution.getCur())
	curMax = actor.hp.getMax()
	# Our target health is 1.5 the max health, so grab that and then compute
	# what our adjustment needs to be, factoring in current ConBOnus
	# In this case rounding may be off, but we ignore that
	targetMax = curMax * 1.5
	target = targetMax / (1.0+percentage)
	adjCurMax = curMax - actor.hp.getModifierAmt("ConBonus")
	adjustment = int(round(target) - adjCurMax)
	
	effect.setStrength(adjustment)
	return True
