/*
 * fluentBuilder.h
 *   Generic macros used for the Fluent API style builders
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


#define GENERIC_BUILDER(builderType, builderTarget, name, varType)      \
    builderType& name(varType name) {                                   \
        builderTarget.name = name;                                      \
        return *this;                                                   \
    }                                                                   \

#define STRING_BUILDER(name) BUILDER(name, const std::string&)
#define BOOL_BUILDER(name)   BUILDER(name, const bool&)
#define INT_BUILDER(name)    BUILDER(name, const int&)
#define LONG_BUILDER(name)   BUILDER(name, const long&)
#define SHORT_BUILDER(name)  BUILDER(name, const short&)
#define FLOAT_BUILDER(name)  BUILDER(name, const float&)


