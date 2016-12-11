/*
 * bstring.h
 *   Extension to basic_string<char>
 *   ____            _               
 *  |  _ \ ___  __ _| |_ __ ___  ___ 
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *  
 *  Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
// Based on CStdStr by Joe O'Leary (http://www.joeo.net/code/StdString.zip)

#ifndef BSTRING_H_
#define BSTRING_H_

#include <iostream>
#include <algorithm>

#ifndef ASSERT
#include <assert.h>
#define ASSERT(f) assert((f))
#endif

#ifndef VERIFY
#ifdef _DEBUG
#define VERIFY(x) ASSERT((x))
#else
#define VERIFY(x) x
#endif
#endif


#include "utils.h"

#include <stdarg.h>
#include <stdio.h>

#include <string>       // basic_string
#include <sstream>  // for stringstream

// ------------------------------------------------------------
// strLen: strlen
// ------------------------------------------------------------
inline int strLen(const char* s) {
    return s == 0 ? 0 : std::basic_string<char>::traits_type::length(s);
}
inline int strLen(const std::string& s) {
    return s.length();
}

// ------------------------------------------------------------
// String assignment functions -- assign str to dst
// ------------------------------------------------------------
inline void assignStr(std::string& dst, const std::string& src) {
    if(dst.c_str() != src.c_str()) {
        dst.erase();
        dst.assign(src);
    }
}
inline void assignStr(std::string& dst, const char* src) {
    // Watch out for nulls
    if(src == nullptr)
    {
        dst.erase();
    }
    // If src actually points to part of dst, we must NOT erase(), but
    // rather take a substring
    else if(src >= dst.c_str() && src <= dst.c_str()+dst.size())
    {
        dst = dst.substr(static_cast<std::string::size_type>(src-dst.c_str()));
    }
    // Otherwise (most cases) do the assignment
    else
    {
        dst.assign(src);
    }
}
inline void assignStr(std::string& dst, const int n) {
    ASSERT(n==0);
    dst.assign("");
}   

// ------------------------------------------------------------
// String concatenation functions -- add src to first
// ------------------------------------------------------------

inline void strAdd(std::string& dst, const std::string& src) {
    if(&src == &dst)
        dst.reserve(2*dst.size());

    dst.append(src.c_str());
}

inline void strAdd(std::string& dst, const char* src) {
    if(src) {
        // If the string being added is our internal string or a part of our
        // internal string, then we must NOT do any reallocation without
        // first copying that string to another object (since we're using a
        // direct pointer)

        if( src >= dst.c_str() && src <= dst.c_str()+dst.length()) {
            if( dst.capacity() <= dst.size()+strLen(src) )
                dst.append(std::string(src));
            else
                dst.append(src);
        }
        else {
            dst.append(src); 
        }
    }
}

inline void strAdd(std::string& dst, int val) {
    std::stringstream ss;
    ss << val;
    strAdd(dst, ss.str());
}

// -----------------------------------------------------------------
// strToUpper/strToLower: Uppercase/Lowercase conversion functions
// -----------------------------------------------------------------

inline void strToLower(char* str, size_t len)
{
    std::use_facet< std::ctype<char> >(std::locale()).tolower(str, str+len);
}

inline void strToUpper(char* str, size_t len)
{
    std::use_facet< std::ctype<char> >(std::locale()).toupper(str, str+len);
}
// -----------------------------------------------------------------------------
// sscoll/ssicoll: Collation wrappers
// -----------------------------------------------------------------------------

inline int sscoll(const char* sz1, int nLen1, const char* sz2, int nLen2)
{
    const std::collate<char>& coll = std::use_facet< std::collate<char> >(std::locale());

    return coll.compare(sz2, sz2+nLen2, sz1, sz1+nLen1);
}

inline int ssicoll(const char* sz1, int nLen1, const char* sz2, int nLen2)
{
    const std::locale loc;
    const std::collate<char>& coll = std::use_facet< std::collate<char> >(std::locale());

    // Some implementations seem to have trouble using the collate<>
    // facet typedefs so we'll just default to basic_string and hope
    // that's what the collate facet uses (which it generally should)

    //  std::collate<char>::string_type s1(sz1);
    //  std::collate<char>::string_type s2(sz2);
    std::basic_string<char> s1(sz1 ? sz1 : "");
    std::basic_string<char> s2(sz2 ? sz2 : "");

    strToLower(const_cast<char*>(s1.c_str()), nLen1);
    strToLower(const_cast<char*>(s2.c_str()), nLen2);
    return coll.compare(s2.c_str(), s2.c_str()+nLen2,
        s1.c_str(), s1.c_str()+nLen1);
}

// -----------------------------------------------------------------------------
// strCaseCmp: comparison (case insensitive )
// -----------------------------------------------------------------------------
inline int strCaseCmp(const char* s1, const char* s2)
{
    std::locale loc;
    const std::ctype<char>& ct = std::use_facet< std::ctype<char> >(std::locale());
    char f;
    char l;

    do 
    {
        f = ct.tolower(*(s1++));
        l = ct.tolower(*(s2++));
    } while( (f) && (f == l) );

    return (int)(f - l);
}

// This struct is used for TrimRight() and TrimLeft() function implementations.
struct NotSpace : public std::unary_function<char, bool>
{
    const std::locale loc;
    NotSpace(const std::locale& locArg=std::locale()) : loc(locArg) {}
    bool operator() (char ch) const { return !std::isspace(ch, loc); }
};

// -----------------------------------------------------------------
//  class bstring - The class itself!
// -----------------------------------------------------------------
class bstring : public std::basic_string<char>
{

    // Typedefs for shorter names.  Using these names also appears to help
    // us avoid some ambiguities that otherwise arise on some platforms
public:
    typedef std::basic_string<char>     my_base;           // my base class
    typedef bstring                     my_type;           // myself
    typedef my_base::const_pointer      my_const_pointer;  // const char*
    typedef my_base::pointer            my_pointer;        // char*
    typedef my_base::iterator           my_iterator;       // my iterator type
    typedef my_base::const_iterator     my_const_iterator; // you get the idea...
    typedef my_base::reverse_iterator   my_reverse_iterator;
    typedef my_base::size_type          my_size_type;   
    typedef my_base::value_type         my_value_type; 
    typedef my_base::allocator_type     my_allocator_type;

    // Constructors!
    bstring()  { }
    
    bstring(const bstring& str): my_base(str)  { }

    bstring(const std::string& str) {
        assignStr(*this, str);
    }

    bstring(const char* str) {
        *this = str;
    }
    bstring(const unsigned char* uStr) {
        *this = reinterpret_cast<const char*>(uStr);
    }
    
    bstring(int val) {
        std::stringstream ss;
        ss << val;
        *this = ss.str();
    }

    bstring(my_const_pointer str, my_size_type n) : my_base(str, n) { }

    bstring(my_const_iterator first, my_const_iterator last) : my_base(first, last) {  }

    bstring(my_size_type size, my_value_type ch, const my_allocator_type& al = my_allocator_type()) : my_base(size, ch, al) { }
    
    int toInt() {
        if(empty()) return(0);
        return(atoi(getBuf()));
    }
    
    // -----------------------------------------------
    // Case changing functions
    // -----------------------------------------------

    bstring& toUpper() {
        if( !empty() )
            strToUpper(getBuf(), this->size());

        return *this;
    }

    bstring& toLower() {
        if( !empty() )
            strToLower(getBuf(), this->size());

        return *this;
    }

    bstring& normalize() {
        return trim().toLower();
    }

    // --------------------------------------------------
    // Direct buffer access
    // --------------------------------------------------

    char* getBuf(int minLen = -1) {
        if( static_cast<int>(size()) < minLen )
            this->resize(static_cast<my_size_type>(minLen));

        return this->empty() ? const_cast<char*>(this->data()) : &(this->at(0));
    }

    char* setBuf(int len) {
        len = ( len > 0 ? len : 0 );
        if( this->capacity() < 1 && len == 0 )
            this->resize(1);

        this->resize(static_cast<my_size_type>(len));
        return const_cast<char*>(this->data());
    }

    void relBuf(int newLen=-1) {
        this->resize(static_cast<my_size_type>(newLen > -1 ? newLen : strLen(this->c_str())));
    }

    bool equals(const char* str, bool useCase=false) const {   // get copy, THEN compare (thread safe)
        return  useCase ? this->compare(str) == 0 : strCaseCmp(bstring(*this).c_str(), str) == 0;
    } 

    char getAt(int idx) const {
        return this->at(static_cast<my_size_type>(idx));
    }

    char* getBuffer(int minLen=-1) {
        return getBuf(minLen);
    }
    char* getBufferSetLength(int len) {
        return setBuf(len);
    }

    int Insert(int idx, char ch) {
        if( static_cast<my_size_type>(idx) > this->size() -1 )
            this->append(1, ch);
        else
            this->insert(static_cast<my_size_type>(idx), 1, ch);

        return getLength();
    }

    int Insert(unsigned int idx, my_const_pointer sz) {
        if( idx >= this->size() )
            this->append(sz, strLen(sz));
        else
            this->insert(static_cast<my_size_type>(idx), sz);

        return getLength();
    }

    bool isEmpty() const {
        return this->empty();
    }

    int getLength() const {
        return static_cast<int>(this->length());
    }


    bstring left(int count) const {
        // Range check the count.

        count = tMAX(0, tMIN(count, static_cast<int>(this->size())));
        return this->substr(0, static_cast<my_size_type>(count)); 
    }
    bstring right(int count) const {
        // Range check the count.

        count = tMAX(0, tMIN(count, static_cast<int>(this->size())));
        return this->substr(this->size()-static_cast<my_size_type>(count));
    }
    bstring mid(int first ) const {
        return mid(first, size()-first);
    }

    bstring mid(int first, int count) const {

        if( first < 0 )
            first = 0;
        if( count < 0 )
            count = 0;

        if( first + count > (signed)size() )
            count = size() - first;

        if( first > (signed)size() )
            return bstring();

        ASSERT(first >= 0);
        ASSERT(first + count <= (signed)size());

        return this->substr(static_cast<my_size_type>(first), static_cast<my_size_type>(count));
    }

    int Remove(char ch) {
        my_size_type idx = 0;
        int nRemoved = 0;
        while( (idx=this->find_first_of(ch)) != my_base::npos ) {
            this->erase(idx, 1);
            nRemoved++;
        }
        return nRemoved;
    }

    int Replace(char oldCh, char newCh) {
        int numReplaced = 0;
        for( my_iterator iter=this->begin(); iter != this->end(); iter++ ) {
            if( *iter == oldCh ) {
                *iter = newCh;
                numReplaced++;
            }
        }
        return numReplaced;
    } 
    int Replace(my_const_pointer szOld, my_const_pointer szNew) {
        int nReplaced               = 0;
        my_size_type nIdx           = 0;
        my_size_type nOldLen        = strLen(szOld);

        if( nOldLen != 0 ) {
            // If the replacement string is longer than the one it replaces, this
            // string is going to have to grow in size,  Figure out how much
            // and grow it all the way now, rather than incrementally

            my_size_type nNewLen = strLen(szNew);
            if( nNewLen > nOldLen ) {
                int nFound= 0;
                while( nIdx < this->length() && (nIdx=this->find(szOld, nIdx)) != my_base::npos ) {
                    nFound++;
                    nIdx += nOldLen;
                }
                this->reserve(this->size() + nFound * (nNewLen - nOldLen));
            }
            static const char ch = char(0);
            my_const_pointer szRealNew = szNew == 0 ? &ch : szNew;
            nIdx = 0;
            while( nIdx < this->length() && (nIdx=this->find(szOld, nIdx)) != my_base::npos ) {
                this->replace(this->begin()+nIdx, this->begin()+nIdx+nOldLen, szRealNew);

                nReplaced++;
                nIdx += nNewLen;
            }
        }

        return nReplaced;
    }
    
    int Find(char ch) const
    {
        return(find_first_of(ch));
    }

    int Find(my_const_pointer szSub) const
    {
        return(find(szSub));
    }

    int Find(char ch, int nStart) const
    {
        return(find_first_of(ch, static_cast<my_base::size_type>(nStart)));
    }

    int Find(my_const_pointer szSub, int nStart) const
    {
        return(find(szSub, static_cast<my_base::size_type>(nStart)));
    }

    int FindOneOf(my_const_pointer szCharSet) const
    {
        return(find_first_of(szCharSet));
    }
    
    int ReverseFind(char ch) const {
        my_size_type idx = this->find_last_of(ch);
        return(idx);
    }

    int ReverseFind(my_const_pointer szFind, my_size_type pos=my_base::npos) const {
        return(rfind(0 == szFind ? bstring() : szFind, pos));
    }

    void setAt(int nIndex, char ch) {
        ASSERT(this->size() > static_cast<my_size_type>(nIndex));
        this->at(static_cast<my_size_type>(nIndex))     = ch;
    }


    // -------------------------------------------------------------------------
    // trim and its variants
    // -------------------------------------------------------------------------
    bstring& trim() {
        return trimLeft().trimRight();
    }

    bstring& trimLeft() {
        this->erase(this->begin(),
            std::find_if(this->begin(), this->end(), NotSpace()));

        return *this;
    }

    bstring&  trimLeft(char trimChar) {
        this->erase(0, this->find_first_not_of(trimChar));
        return *this;
    }

    bstring&  trimLeft(my_const_pointer trimChars) {
        this->erase(0, this->find_first_not_of(trimChars));
        return *this;
    }

    bstring& trimRight() {
        my_reverse_iterator it = std::find_if(this->rbegin(), this->rend(), NotSpace());
        if( !(this->rend() == it) )
            this->erase(this->rend() - it);

        this->erase(!(it == this->rend()) ? this->find_last_of(*it) + 1 : 0);
        return *this;
    }

    bstring&  trimRight(char trimChar) {
        my_size_type idx    = this->find_last_not_of(trimChar);
        this->erase(my_base::npos == idx ? 0 : ++idx);
        return *this;
    }

    bstring&  trimRight(my_const_pointer trimChars) {
        my_size_type idx    = this->find_last_not_of(trimChars);
        this->erase(my_base::npos == idx ? 0 : ++idx);
        return *this;
    }

    void freeExtra() {
        bstring mt;
        this->swap(mt);
        if( !mt.empty() )
            this->assign(mt.c_str(), mt.size());
    }

    void Format(const char* fmt, ...)
    {
        va_list argList;
        va_start(argList, fmt);
        FormatV(fmt, argList);
        va_end(argList);
    }

    void AppendFormat(const char* fmt, ...)
    {
        va_list argList;
        va_start(argList, fmt);
        AppendFormatV(fmt, argList);
        va_end(argList);
    }

#define STD_BUF_SIZE        1024

    // an efficient way to add formatted characters to the string.  You may only
    // add up to STD_BUF_SIZE characters at a time, though
    void AppendFormatV(const char* fmt, va_list argList)
    {
        char buf[STD_BUF_SIZE];
        int nLen = vsprintf(buf, fmt, argList);
        if( nLen > 0  )
            this->append(buf, nLen);
    }

    void FormatV(const char* szFormat, va_list argList)
    {
        int nLen = strLen(szFormat) + STD_BUF_SIZE;
        char *buf;
        buf = new char[nLen];
        vsprintf(buf, szFormat, argList);
        assignStr(*this, buf);
        delete[] buf;

    }

    int Collate(my_const_pointer that) const
    {
        return sscoll(this->c_str(), this->length(), that, strLen(that));
    }

    int CollateNoCase(my_const_pointer that) const
    {
        return ssicoll(this->c_str(), this->length(), that, strLen(that));
    }

    int Compare(my_const_pointer that) const
    {
        return this->compare(that);   
    }

    int CompareNoCase(my_const_pointer that)  const
    {
        return strCaseCmp(this->c_str(), that);
    }

    int Delete(int idx, int count=1)
    {
        if( idx < 0 )
            idx = 0;

        if( idx < getLength() )
            this->erase(static_cast<my_size_type>(idx), static_cast<my_size_type>(count));

        return getLength();
    }

    void Empty()
    {
        this->erase();
    }

/*  bool operator==(const bstring& str) {
        return(this->equals(str.c_str(),false));
    }*/
    // -----------------------------------------------
    // Assignment operators
    // -----------------------------------------------
    bstring& operator=(const bstring& str) {
        assignStr(*this, str);
        return *this;
    }

    bstring& operator=(const std::string& str) {
        assignStr(*this, str);
        return *this;
    }

    bstring& operator=(const char* str) {
        assignStr(*this, str);
        return *this;
    }

    bstring& operator=(const char ch) {
        this->assign(1, ch);
        return *this;
    }
    // -----------------------------------------------
    // Array-indexing operators.
    // -----------------------------------------------
    char& operator[](size_type idx) {
        return my_base::operator[](static_cast<my_size_type>(idx));
    }

    const char& operator[](size_type idx) const {
        return my_base::operator[](static_cast<my_size_type>(idx));
    }

    char& operator[](int idx) {
        return my_base::operator[](static_cast<my_size_type>(idx));
    }

    const char& operator[](int idx) const {
        return my_base::operator[](static_cast<my_size_type>(idx));
    }

//    char& operator[](unsigned int idx) {
//        return my_base::operator[](static_cast<my_size_type>(idx));
//    }
//
//    const char& operator[](unsigned int idx) const {
//        return my_base::operator[](static_cast<my_size_type>(idx));
//    }

    // -----------------------------------------------
    // inline concatenation.
    // -----------------------------------------------
    bstring& operator+=(const bstring& str) {
        strAdd(*this, str);
        return *this;
    }

    bstring& operator+=(const std::string& str) {
        strAdd(*this, str);
        return *this;
    }

    bstring& operator+=(const char* str) {
        strAdd(*this, str);
        return *this;
    }

    bstring& operator+=(const char ch) {
        append(1, ch);
        return *this;
    }
    // addition operators -- global friend functions.

    friend bstring operator+(const bstring& s1,     const bstring& s2);

    friend bstring operator+(const bstring& s1,     const char* s2);
    friend bstring operator+(const bstring& s,      char ch);
    friend bstring operator+(const bstring& s,      int val);

    friend bstring operator+(const char*s1,         const bstring& s2);
    friend bstring operator+(const char ch,         const bstring& s);
    friend bstring operator+(int val,               const bstring& s);


}; // end bstring

inline bstring operator+(const bstring& s1, const bstring& s2) {
    bstring strRet(s1);
    strRet.append(s2);
    return strRet;
}


inline bstring operator+(const bstring& s1, const char* s2) {
    return bstring(s1) + bstring(s2);
}

inline bstring operator+(const bstring& s, char ch)
{
    bstring strRet(s);
    strRet.append(1, ch);
    return strRet;
}
inline bstring operator+(const bstring& s, int val)
{
    bstring strRet(s);
    strAdd(strRet, val);
    return(strRet);
}
inline bstring operator+(const char* s1, const bstring& s2) {
    bstring strRet(s1);
    strRet.append(s2);
    return strRet;
}

inline bstring operator+(const char ch, const bstring& s) {
    bstring strRet(ch);
    strRet.append(s);
    return strRet;
}
inline bstring operator+(int val, const bstring& s) {
    bstring strRet(val);
    strRet.append(s);
    return strRet;
}

struct bstringLessNoCase : public std::binary_function<bstring, bstring, bool>
{
    inline bool operator()(const bstring& sLeft, const bstring& sRight) const
    { return strCaseCmp(sLeft.c_str(), sRight.c_str()) < 0; }
};
struct bstringEqualsNoCase : public std::binary_function<bstring, bstring, bool>
{
    inline bool operator()(const bstring& sLeft, const bstring& sRight) const
    { return strCaseCmp(sLeft.c_str(), sRight.c_str()) == 0; }
};

namespace std
{
    inline void swap(bstring& s1, bstring& s2) throw()
    {
        s1.swap(s2);
    }
}

typedef bstring Status;

// Custom comparison operator to sort by the numeric id instead of standard string comparison
struct idComp : public std::binary_function<const bstring&, const bstring&, bool> {
    bool operator() (const bstring& lhs, const bstring& rhs) const;
};


#endif /*BSTRING_H_*/
