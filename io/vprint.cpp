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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef __APPLE__
#include <printf.h>                  // for register_printf_specifier, printf_info
#endif
#include <cctype>                    // for isdigit
#include <cstdarg>                   // for va_list, va_end, va_start, va_copy
#include <cstddef>                   // for ptrdiff_t, size_t
#include <cstdint>                   // for intmax_t, uintmax_t
#include <cstdio>                    // for snprintf, fprintf, vasprintf, FILE
#include <cstdlib>                   // for free, atoi
#include <cstring>                   // for strchr, strdup
#include <optional>                  // for optional
#include <ostream>                   // for operator<<, endl
#include <sstream>                   // for ostringstream
#include <string>                    // for string, basic_string
#include <string_view>               // for string_view
#include <vector>                    // for vector

#include "creatureStreams.hpp"       // for Streamable, ColorOff, ColorOn
#include "global.hpp"                // for CAP
#include "mudObjects/creatures.hpp"  // for Creature
#include "mudObjects/objects.hpp"    // for Object
#include "mudObjects/players.hpp"    // for Player
#include "server.hpp"                // for Server
#include "socket.hpp"                // for Socket

// Function Prototypes
std::string delimit(const char *str, int wrap);
static int realmsVasprintf(char **out, const char *fmt, va_list ap);

void Creature::printPaged(std::string_view toPrint) const {
    if(hasSock())
        getSock()->printPaged(toPrint);
}

void Creature::donePaging() const {
    if(hasSock())
        getSock()->donePaging();
}

void Creature::bPrint(std::string_view toPrint) const {
    (Streamable &) *this << ColorOn << toPrint << ColorOff;
}

void Creature::bPrintPython(const std::string& toPrint) const {
    (Streamable &) *this << ColorOn << toPrint << ColorOff;
}

void Creature::print(const char *fmt,...) const {
    // Mad hack, but it'll stop some stupid errors
    if(!this)
        return;

    std::shared_ptr<Socket> printTo = getSock();

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
    std::shared_ptr<Socket> printTo = getSock();

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
    if(this) {
        if (auto sock = mySock.lock()) {
            sock->vprint(fmt, ap);
        }
    }
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
    int n = realmsVasprintf(&msg, fmt, aq);
    va_end(aq);

    if(n == -1) {
        std::clog << "Problem with vasprintf in vprint!" << std::endl;
        return;
    }
    std::string toPrint;

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

static std::optional<std::string> renderSpecifier(char spec, int width, const void *ptr) {
    switch(spec) {
        case 'B': {
            const std::string *s = static_cast<const std::string *>(ptr);
            return s ? *s : std::string();
        }
        case 'T': {
            const std::ostringstream *s = static_cast<const std::ostringstream *>(ptr);
            return s ? s->str() : std::string();
        }
        // M = Capital Monster; N = small monster
        case 'M': case 'N': {
            const Creature *crt = static_cast<const Creature *>(ptr);
            if(!crt) return std::string();
            return crt->getCrtStr(nullptr, VPRINT_flags | (spec == 'M' ? CAP : 0), width);
        }
        case 'R': {
            const Creature *crt = static_cast<const Creature *>(ptr);
            return crt ? std::string(crt->getCName()) : std::string();
        }
        // O = Capital Object; P = small object
        case 'O': case 'P': {
            const Object *obj = static_cast<const Object *>(ptr);
            if(!obj) return std::string();
            return obj->getObjStr(nullptr, VPRINT_flags | (spec == 'O' ? CAP : 0), width);
        }
        default:
            return std::nullopt;
    }
}

#ifndef __APPLE__
int print_objcrt(FILE *stream, const struct printf_info *info, const void *const *args) {
    auto rendered = renderSpecifier((char)info->spec, info->width, *((const void *const *) args[0]));
    if(!rendered)
        return(-1);
    return(fprintf(stream, "%s", rendered->c_str()));
}

int print_arginfo (const struct printf_info *info, size_t n, int *argtypes, int* size) {
    // We always take exactly one argument and this is a pointer to the structure
    if(n > 0) {
        argtypes[0] = PA_POINTER;
        if(info->spec == 'O' || info->spec == 'P') size[0] = sizeof(Object *);
        else if(info->spec == 'R' || info->spec == 'M' || info->spec == 'N') size[0] = sizeof(Creature *);
        else if(info->spec == 'b') size[0] = sizeof(std::string *);
        else if(info->spec == 'T') size[0] = sizeof(std::ostringstream *);
    }
    return(1);
}
#endif

static const char CUSTOM_SPECS[] = "bTNMPOR";

static void appendStandardSpec(std::string &out, const std::string &spec, const std::string &length, char conv, va_list &ap) {
    char buf[256];
    auto emit = [&](auto value) {
        int need = snprintf(buf, sizeof(buf), spec.c_str(), value);
        if(need < 0) return;
        if((size_t) need < sizeof(buf)) {
            out.append(buf, need);
        } else {
            std::vector<char> big(need + 1);
            snprintf(big.data(), big.size(), spec.c_str(), value);
            out.append(big.data(), need);
        }
    };
    switch(conv) {
        case 'd': case 'i':
            if(length == "l") emit(va_arg(ap, long));
            else if(length == "ll" || length == "q") emit(va_arg(ap, long long));
            else if(length == "j") emit(va_arg(ap, intmax_t));
            else if(length == "z") emit(va_arg(ap, long));
            else if(length == "t") emit(va_arg(ap, ptrdiff_t));
            else emit(va_arg(ap, int));
            break;
        case 'o': case 'u': case 'x': case 'X':
            if(length == "l") emit(va_arg(ap, unsigned long));
            else if(length == "ll" || length == "q") emit(va_arg(ap, unsigned long long));
            else if(length == "j") emit(va_arg(ap, uintmax_t));
            else if(length == "z") emit(va_arg(ap, size_t));
            else emit(va_arg(ap, unsigned int));
            break;
        case 'e': case 'E': case 'f': case 'F': case 'g': case 'G': case 'a': case 'A':
            if(length == "L") emit(va_arg(ap, long double));
            else emit(va_arg(ap, double));
            break;
        case 'c':
            emit(va_arg(ap, int));
            break;
        case 's':
            emit(va_arg(ap, const char *));
            break;
        case 'p':
            emit(va_arg(ap, void *));
            break;
        default:
            out.append(spec);
            break;
    }
}

// Portable replacement for glibc's register_printf_specifier + vasprintf: walk the format, rendering our custom
// specifiers and delegating standard ones to snprintf, consuming va_args in lockstep.
std::string realmsFormat(const char *fmt, va_list ap) {
    std::string out;
    for(const char *p = fmt; *p; ) {
        if(*p != '%') { out.push_back(*p++); continue; }
        const char *start = p++;
        if(*p == '%') { out.push_back('%'); ++p; continue; }

        std::string flags;
        while(*p && strchr("-+ #0'", *p)) flags.push_back(*p++);

        bool starWidth = false;
        std::string width;
        if(*p == '*') { starWidth = true; ++p; }
        else while(isdigit((unsigned char) *p)) width.push_back(*p++);

        bool hasPrec = false, starPrec = false;
        std::string prec;
        if(*p == '.') {
            hasPrec = true; ++p;
            if(*p == '*') { starPrec = true; ++p; }
            else while(isdigit((unsigned char) *p)) prec.push_back(*p++);
        }

        std::string length;
        while(*p && strchr("hlLqjzt", *p)) length.push_back(*p++);

        if(!*p) { out.append(start, p - start); break; }
        char conv = *p++;

        int dynWidth = starWidth ? va_arg(ap, int) : 0;
        int dynPrec = starPrec ? va_arg(ap, int) : 0;

        if(strchr(CUSTOM_SPECS, conv)) {
            int w = starWidth ? dynWidth : (width.empty() ? 0 : atoi(width.c_str()));
            const void *ptr = va_arg(ap, void *);
            if(auto rendered = renderSpecifier(conv, w, ptr))
                out.append(*rendered);
            continue;
        }

        std::string spec = "%" + flags;
        spec += starWidth ? std::to_string(dynWidth) : width;
        if(hasPrec) spec += "." + (starPrec ? std::to_string(dynPrec) : prec);
        spec += length;
        spec += conv;
        appendStandardSpec(out, spec, length, conv, ap);
    }
    return out;
}

static int realmsVasprintf(char **out, const char *fmt, va_list ap) {
#ifdef __APPLE__
    std::string s = realmsFormat(fmt, ap);
    *out = strdup(s.c_str());
    return *out ? (int) s.size() : -1;
#else
    return vasprintf(out, fmt, ap);
#endif
}

int Server::installPrintfHandlers() {
#ifdef __APPLE__
    // Custom specifiers are expanded by realmsFormat(); macOS has no register_printf_specifier.
    return 1;
#else
    int r = 1;
    // std::string
    r &= register_printf_specifier('b', print_objcrt, print_arginfo);
    // std::ostringstream
    r &= register_printf_specifier('T', print_objcrt, print_arginfo);
    // creature
    r &= register_printf_specifier('N', print_objcrt, print_arginfo);
    // capital creature
    r &= register_printf_specifier('M', print_objcrt, print_arginfo);
    // object
    r &= register_printf_specifier('P', print_objcrt, print_arginfo);
    // capital object
    r &= register_printf_specifier('O', print_objcrt, print_arginfo);
    // creature's real name
    r &= register_printf_specifier('R', print_objcrt, print_arginfo);
    return(r);
#endif
}
