/*
 *  © 2013. https://github.com/0xcb All rights reserved.
 */

#pragma once

#include <vector>
#include <stdio.h>
#include <string.h>
#define __forceinline __attribute__((always_inline))

using std::vector;


/*
 *  A simple container for a byte pattern. The container is passed
 *  to the Aho-Corasick Engine (ACE) and used to locate instances
 *  of the pattern in a buffer. If the pattern is matched, the offset
 *  of the starting position of the match within the buffer is added
 *  to the hits vector.
 */
class ACEPattern
{
    typedef unsigned char byte;
    typedef unsigned int  uint;
    typedef unsigned long ulong;

    private:
    
        bool _localCopy;

    public:
    
        byte*          pattern;
        uint           length;
        vector<ulong>* hits;

        /*
         *  Pattern locality is left up to the user, but defaults to a cached copy.
         */
        ACEPattern(byte* pat, uint len, bool localCopy = true)
        {
            hits       = NULL;
            length     = len;
            _localCopy = localCopy;
            if(localCopy)
            {
                pattern = new byte[len];
                memcpy(pattern, pat, len);
            }
            else
                pattern = pat;
        }

        void FreeLocalData()
        {
            if(_localCopy && pattern)
                delete[] pattern;
            pattern = NULL;
        }

        ~ACEPattern()
        {
            if(_localCopy && pattern)
                delete[] pattern;
            if(hits)
                delete hits;
        }
};
