 /*
 * vprint.cpp
 *   Functions related to vprint (printf replacement for realms)
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
#include <cstdarg>

#include "creatures.hpp"
#include "mud.hpp"
#include "login.hpp"
#include "server.hpp"
#include "socket.hpp"
#include "vprint.hpp"

// Function Prototypes
bstring delimit(const char *str, int wrap);

void Creature::bPrint(const bstring& toPrint) const {
    printColor("%s", toPrint.c_str());
}

void Creature::print(const char *fmt,...) const {
    // Mad hack, but it'll stop some stupid errors
    if(!this)
        return;

    Socket* printTo = getSock();

    if(isPet())
        printTo = getConstMaster()->getSock();

    if(!printTo)
        return;

    va_list ap;

    va_start(ap, fmt);
    if(isPet()) {
        printTo->print("Pet> ");
    }
    printTo->vprint(fmt, ap);
    va_end(ap);
}

void Creature::printColor(const char *fmt,...) const {
    Socket* printTo = getSock();

    if(isPet())
        printTo = getConstMaster()->getSock();

    if(!this || !printTo)
        return;

    va_list ap;

    va_start(ap, fmt);
    if(isPet()) {
        printTo->print("Pet> ");
    }
    printTo->vprint(fmt, ap);
    va_end(ap);
}
void Player::vprint(const char *fmt, va_list ap) const {
    if(this && mySock)
        mySock->vprint(fmt, ap);
}


static int VPRINT_flags = 0;

void Socket::vprint(const char *fmt, va_list ap) {
    char    *msg;
    va_list aq;

    if(!this) {
        std::clog << "vprint(): called with null this! :(\n";
        return;
    }

    VPRINT_flags = 0;
    if(myPlayer)
        VPRINT_flags = myPlayer->displayFlags();
    // Incase vprint is called multiple times with the same ap
    // (in which case ap would be undefined, so make a copy of it
    va_copy(aq, ap);
    int n = vasprintf(&msg, fmt, aq);
    va_end(aq);

    if(n == -1) {
        std::clog << "Problem with vasprintf in vprint!" << std::endl;
        return;
    }
    bstring toPrint;

    if(!myPlayer || (myPlayer && myPlayer->getWrap() == -1))
        toPrint = delimit( msg, getTermCols() - 4);
    else if(myPlayer && myPlayer->getWrap() > 0)
        toPrint = delimit( msg, myPlayer->getWrap());
    else
        toPrint = msg;
    toPrint += "^x";
    bprint(toPrint);

    free(msg);
}

int print_objcrt(FILE *stream, const struct printf_info *info, const void *const *args) {
    char *buffer;
    int len;

    if(info->spec == 'B') {
        const bstring *tmp = *((const bstring **) (args[0]));
        len = asprintf(&buffer, "%s", tmp->c_str());
    }
    else if(info->spec == 'T') {
        const std::ostringstream *tmp = *((const std::ostringstream **) (args[0]));
        len = asprintf(&buffer, "%s", tmp->str().c_str());
    }
    // M = Capital Monster N = small monster
    else if(info->spec == 'M' || info->spec == 'N') {
        const Creature *crt = *((const Creature **) (args[0]));
        if(info->spec == 'M') {
            bstring tmp = crt->getCrtStr(nullptr, VPRINT_flags | CAP, info->width);
            len = asprintf(&buffer, "%s", tmp.c_str());
        }
        else {
            bstring tmp = crt->getCrtStr(nullptr, VPRINT_flags, info->width);
            len = asprintf(&buffer, "%s", tmp.c_str());
        }
        if(len == -1)
            return(-1);
    }
    else if(info->spec == 'R') {
        const Creature *crt = *((const Creature **) (args[0]));
        len = asprintf(&buffer, "%s", crt->getCName());
        if(len == -1)
            return(-1);
    }
    // O = Capital Object P = small object
    else if(info->spec == 'O' || info->spec == 'P') {
        const Object *obj = *((const Object **) (args[0]));
        if(info->spec == 'O') {
            bstring tmp = obj->getObjStr(nullptr, VPRINT_flags | CAP, info->width);
            len = asprintf(&buffer, "%s", tmp.c_str());
        }
        else {
            bstring tmp = obj->getObjStr(nullptr, VPRINT_flags, info->width);
            len = asprintf(&buffer, "%s", tmp.c_str());
        }

        if(len == -1)
            return(-1);
    }
    // Unhandled type
    else {
        return(-1);
    }

    // Pad to the minimum field width and print to the stream.
    //len = fprintf (stream, "%*s", (info->left ? -info->width : info->width), buffer);
    len = fprintf(stream, "%s", buffer);

    // Clean up and return.
    free(buffer);
    return(len);
}

int print_arginfo (const struct printf_info *info, size_t n, int *argtypes) {
    // We always take exactly one argument and this is a pointer to the structure..
    if(n > 0)
        argtypes[0] = PA_POINTER;
    return(1);
}

int Server::installPrintfHandlers() {
    int r = 0;
    // bstring
    r |= register_printf_function('B', print_objcrt, print_arginfo);
    // std::ostringstream
    r |= register_printf_function('T', print_objcrt, print_arginfo);
    // creature
    r |= register_printf_function('N', print_objcrt, print_arginfo);
    // capital creature
    r |= register_printf_function('M', print_objcrt, print_arginfo);
    // object
    r |= register_printf_function('P', print_objcrt, print_arginfo);
    // capital object
    r |= register_printf_function('O', print_objcrt, print_arginfo);
    // creature's real name
    r |= register_printf_function('R', print_objcrt, print_arginfo);
    return(r);
}
