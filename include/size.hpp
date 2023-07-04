/*
 * size.h
 *   Header file for size
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

#pragma once

#include <string>

enum Size {
    NO_SIZE         =0,
    SIZE_FINE       =1,
    SIZE_DIMINUTIVE =2,
    SIZE_TINY       =3,
    SIZE_SMALL      =4,
    SIZE_MEDIUM     =5,
    SIZE_LARGE      =6,
    SIZE_HUGE       =7,
    SIZE_GARGANTUAN =8,
    SIZE_COLOSSAL   =9,

    MAX_SIZE        =SIZE_COLOSSAL
};

Size getSize(const std::string& str);
const std::string & getSizeName(Size size);
int searchMod(Size size);
Size whatSize(int i);

