import mud
import mudLib
import stats
from mudObject import MudObject
from effects import EffectInfo
from typing import Optional

def potionHeal(actor):
    heal = mud.dice(2, 6, 0)
    actor.send("You feel a little better (" + str(heal) + ")\n" )
    actor.hp.increase(heal)

def potionHarm(actor):
    harm = mud.dice(2, 6, 0)
    actor.send(f"That hurt! ({str(harm)})\n")

    # What about checking for death here?
    actor.hp.decrease(harm)