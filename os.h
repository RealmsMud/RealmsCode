/*
 * os.h
 *   Operating system dependent includes and defines
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
#ifndef OS_H_
#define OS_H_

/* custom assert stuff to output to the log file */
void _assertlog( const char * strExp, const char *strFile, unsigned int nLine );

#define merror(a, b)  new_merror(a, b, __FILE__, __LINE__ )

#undef  ASSERTLOG

#ifdef  NDEBUG
#define ASSERTLOG(exp)     ((void)0)
#else
#define ASSERTLOG(exp) (void)( (exp) || (_assertlog(#exp, __FILE__, __LINE__), 0) )
#endif  /* NDEBUG */

#endif /*OS_H_*/
