/*
 * size.h
 *	 Header file for size
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
#ifndef SIZE_H_
#define SIZE_H_

enum Size {
	NO_SIZE			=0,
	SIZE_FINE		=1,
	SIZE_DIMINUTIVE	=2,
	SIZE_TINY		=3,
	SIZE_SMALL		=4,
	SIZE_MEDIUM		=5,
	SIZE_LARGE		=6,
	SIZE_HUGE		=7,
	SIZE_GARGANTUAN	=8,
	SIZE_COLOSSAL	=9,

	MAX_SIZE		=SIZE_COLOSSAL
};


#endif /*SIZE_H_*/
