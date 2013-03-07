/*
 * utils.h
 *	 Header file for small useful utilities
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

#ifndef UTILS_H_
#define UTILS_H_



// -----------------------------------
// MIN and MAX.
// -----------------------------------
template<class Type>
inline const Type& tMIN(const Type& arg1, const Type& arg2)
{
    return arg2 < arg1 ? arg2 : arg1;
}

template<class Type>
inline const Type& tMAX(const Type& arg1, const Type& arg2)
{
    return arg2 > arg1 ? arg2 : arg1;
}



#endif /* UTILS_H_ */
