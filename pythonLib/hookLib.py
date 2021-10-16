import mud
import mudLib
import MudObjects

def doPurchaseCast(actor, args, target):       
	effect = actor.getEffect()
	if effect == "":
		retVal = True
	casterId = args.split(",")[0]
	if casterId != "":
		caster = target.getParent().findCreature(target, casterId)
		target.send(caster.getName() + " casts " + mudLib.indefinite_article(effect) + " " + effect + " spell on you.\n")
		mudLib.broadcastRoom(target.getRoom(), "*ACTOR* casts " + mudLib.indefinite_article(effect) + " " + effect + " spell on *TARGET*.", actor = caster, target = target, ignore = target)
	else:
		caster = actor
	target.addEffect(effect, actor.getEffectDuration(), actor.getEffectStrength(),  caster, True, target, False)
	retVal = False
