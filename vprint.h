/*
 * vprint.h
 *	 Header file for vprint
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
#ifndef VPRINT_H_
#define VPRINT_H_

#include <stdio.h>
#include <stdlib.h>

#if !defined(__CYGWIN__) && !defined(__MACOS__)

#include <printf.h>
// Function prototypes
int print_arginfo (const struct printf_info *info, size_t n, int *argtypes);
int print_object(FILE *stream, const struct printf_info *info, const void *const *args);
int print_creature(FILE *stream, const struct printf_info *info, const void *const *args);

#endif

#endif /*VPRINT_H_*/
