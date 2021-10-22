/*
 *  This is the C++ implementation of the MD5 Message-Digest
 *  Algorithm desrcipted in RFC 1321.
 *  I translated the C code from this RFC to C++.
 *  There is now warranty.
 *
 *  Feb. 12. 2005
 *  Benjamin Gr�delbach
 */

/*
 * Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
 * rights reserved.
 * 
 * License to copy and use this software is granted provided that it
 * is identified as the "RSA Data Security, Inc. MD5 Message-Digest
 * Algorithm" in all material mentioning or referencing this software
 * or this function.
 * 
 * License is also granted to make and use derivative works provided
 * that such works are identified as "derived from the RSA Data
 * Security, Inc. MD5 Message-Digest Algorithm" in all material
 * mentioning or referencing the derived work.
 * 
 * RSA Data Security, Inc. makes no representations concerning either
 * the merchantability of this software or the suitability of this
 * software for any particular purpose. It is provided "as is"
 * without express or implied warranty of any kind.
 * 
 * These notices must be retained in any copies of any part of this
 * documentation and/or software.
 */

//---------------------------------------------------------------------- 
//include protection
#ifndef MD5_H
#define MD5_H

#include <string>

//---------------------------------------------------------------------- 
//STL includes

//---------------------------------------------------------------------- 
//typedefs
typedef unsigned char *POINTER;

/*
 * MD5 context.
 */
typedef struct 
{
    unsigned long int state[4];           /* state (ABCD) */
    unsigned long int count[2];           /* number of bits, modulo 2^64 (lsb first) */
    unsigned char buffer[64];         /* input buffer */
} MD5_CTX;

/*
 * MD5 class
 */
class MD5
{

    private:

        void MD5Transform (unsigned long int state[4], unsigned char block[64]);
        void Encode (unsigned char*, unsigned long int*, unsigned int);
        void Decode (unsigned long int*, unsigned char*, unsigned int);
        void MD5_memcpy (POINTER, POINTER, unsigned int);
        void MD5_memset (POINTER, int, unsigned int);

    public:
    
        void MD5Init (MD5_CTX*);
        void MD5Update (MD5_CTX*, unsigned char*, unsigned int);
        void MD5Final (unsigned char [16], MD5_CTX*);

    MD5(){};
};


class md5wrapper
{
    private:
        MD5 *md5;
    
        /*
         * internal hash function, calling
         * the basic methods from md5.h
         */ 
        std::string hashit(std::string text);

        /*
         * converts the numeric giets to
         * a valid std::string
         */
        std::string convToString(unsigned char *bytes);
    public:
        //constructor
        md5wrapper();

        //destructor
        ~md5wrapper();
        
        /*
         * creates a MD5 hash from
         * "text" and returns it as
         * string
         */ 
        std::string getHashFromString(std::string text);

        /*
         * creates a MD5 hash from
         * a file specified in "filename" and 
         * returns it as string
         */ 
        std::string getHashFromFile(std::string filename);
};

//---------------------------------------------------------------------- 
//End of include protection
#endif

/*
 * EOF
 */
