/*
 The Alphanum Algorithm is an improved sorting algorithm for strings
 containing numbers.  Instead of sorting numbers in ASCII order like a
 standard sort, this algorithm sorts numbers in numeric order.

 The Alphanum Algorithm is discussed at http://www.DaveKoelle.com

 This implementation is Copyright (c) 2008 Dirk Jagdmann <doj@cubic.org>.
 It is a cleanroom implementation of the algorithm and not derived by
 other's works. In contrast to the versions written by Dave Koelle this
 source code is distributed with the libpng/zlib license.

 This software is provided 'as-is', without any express or implied
 warranty. In no event will the authors be held liable for any damages
 arising from the use of this software.

 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it
 freely, subject to the following restrictions:

 1. The origin of this software must not be misrepresented; you
 must not claim that you wrote the original software. If you use
 this software in a product, an acknowledgment in the product
 documentation would be appreciated but is not required.

 2. Altered source versions must be plainly marked as such, and
 must not be misrepresented as being the original software.

 3. This notice may not be removed or altered from any source
 distribution. */

/* $Header: /code/doj/alphanum.hpp,v 1.3 2008/01/28 23:06:47 doj Exp $ */

#include <cassert>
#include <functional>
#include <string>
#include <sstream>

#include <cctype>
#include "alphanum.hpp"

/** wrapper function for ::isdigit() */
bool alphanum_isdigit(int c) {
    return isdigit(c);
}
/**
 compare l and r with strcmp() semantics, but using
 the "Alphanum Algorithm". This function is designed to read
 through the l and r strings only one time, for
 maximum performance. It does not allocate memory for
 substrings.

 @param l NULL-terminated C-style string
 @param r NULL-terminated C-style string
 @return negative if l<r, 0 if l equals r, positive if l>r
 */
int alphanum_impl(const char *l, const char *r) {
    enum mode_t {
        STRING, NUMBER
    } mode = STRING;

    while (*l && *r) {
        if (mode == STRING) {
            char l_char, r_char;
            while ((l_char = *l) && (r_char = *r)) {
                // check if this are digit characters
                const bool l_digit = alphanum_isdigit(l_char), r_digit = alphanum_isdigit(
                        r_char);
                // if both characters are digits, we continue in NUMBER mode
                if (l_digit && r_digit) {
                    mode = NUMBER;
                    break;
                }
                // if only the left character is a digit, we have a result
                if (l_digit)
                    return -1;
                // if only the right character is a digit, we have a result
                if (r_digit)
                    return +1;
                // compute the difference of both characters
                const int diff = l_char - r_char;
                // if they differ we have a result
                if (diff != 0)
                    return diff;
                // otherwise process the next characters
                ++l;
                ++r;
            }
        } else // mode==NUMBER
        {
            // get the left number
            char *end;
            unsigned long l_int=strtoul(l, &end, 0);
            l=end;

            // get the right number
            unsigned long r_int=strtoul(r, &end, 0);
            r=end;

            // if the difference is not equal to zero, we have a comparison result
            const long diff = l_int - r_int;
            if (diff != 0)
                return diff;

            // otherwise we process the next substring in STRING mode
            mode = STRING;
        }
    }

    if (*r)
        return -1;
    if (*l)
        return +1;
    return 0;
}

////////////////////////////////////////////////////////////////////////////

// now follow a lot of overloaded alphanum_comp() functions to get a
// direct call to alphanum_impl() upon the various combinations of c
// and c++ strings.

/**
 Compare l and r with the same semantics as strcmp(), but with
 the "Alphanum Algorithm" which produces more human-friendly
 results.

 @return negative if l<r, 0 if l==r, positive if l>r.
 */
int alphanum_comp(char* l, char* r) {
    assert(l);
    assert(r);
    return alphanum_impl(l, r);
}

int alphanum_comp(const char* l, const char* r) {
    assert(l);
    assert(r);
    return alphanum_impl(l, r);
}

int alphanum_comp(char* l, const char* r) {
    assert(l);
    assert(r);
    return alphanum_impl(l, r);
}

int alphanum_comp(const char* l, char* r) {
    assert(l);
    assert(r);
    return alphanum_impl(l, r);
}

int alphanum_comp(const std::string& l, char* r) {
    assert(r);
    return alphanum_impl(l.c_str(), r);
}

int alphanum_comp(char* l, const std::string& r) {
    assert(l);
    return alphanum_impl(l, r.c_str());
}

int alphanum_comp(const std::string& l, const char* r) {
    assert(r);
    return alphanum_impl(l.c_str(), r);
}

int alphanum_comp(const char* l, const std::string& r) {
    assert(l);
    return alphanum_impl(l, r.c_str());
}

template<>
int alphanum_comp<std::string>(const std::string& l, const std::string& r) {
    return alphanum_impl(l.c_str(), r.c_str());
}

////////////////////////////////////////////////////////////////////////////

