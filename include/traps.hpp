/*
 * traps.h
 *   Trap routines.
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef _TRAPS_H
#define _TRAPS_H

//Trap types
#define TRAP_PIT        1   // Pit trap
#define TRAP_DART       2   // Poison dart trap
#define TRAP_BLOCK      3   // Falling block
#define TRAP_MPDAM      4   // Mp damaging trap
#define TRAP_RMSPL      5   // Spell loss trap
#define TRAP_NAKED      6   // player loses all items
#define TRAP_ALARM      7   // monster alarm trap
#define TRAP_TPORT      8   // Teleport trap
#define TRAP_ARROW      9   // Arrow trap
#define TRAP_SPIKED_PIT     10  // Spiked pit trap
#define TRAP_WORD       11  // Word of recall trap
#define TRAP_FIRE       12  // Fire trap
#define TRAP_FROST      13  // Frost trap
#define TRAP_ELEC       14  // Electricity Trap
#define TRAP_ACID       15  // Acid Trap
#define TRAP_ROCKS      16  // Rockslide trap
#define TRAP_ICE        17  // Falling icicle trap
#define TRAP_SPEAR      18  // Spear trap
#define TRAP_CROSSBOW       19  // Crossbow trap
#define TRAP_GASP       20  // Poison gas trap
#define TRAP_GASB       21  // Blinding gas trap
#define TRAP_GASS       22  // Stun gas trap
#define TRAP_DISP       23  // Displacement trap
#define TRAP_FALL       24  // Deadly fall trap
#define TRAP_CHUTE      25  // Chute trap
#define TRAP_MUD        26  // Stuck in mud trap
#define TRAP_BONEAV     27  // Boneslide trap
#define TRAP_PIERCER        28  // Piercer trap
#define TRAP_ETHEREAL_TRAVEL    29  // Ethereal travel trap
#define TRAP_WEB        30  // sticky spider web trap

#endif  /* _TRAPS_H */

