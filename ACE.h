/*
 *  © 2013. https://github.com/0xcb All rights reserved.
 *
 *
 *  Usage example:
 *
 *  // Create the ACE object:
 *  ACE* Ace = new ACE();
 *
 *  // Add hundreds or thousands of patterns from byte arrays:
 *  Ace->AddPattern(new ACEPattern(NEEDLE, NEEDLE_SIZE));
 *  Ace->AddPattern(new ACEPattern(ANOTHER_NEEDLE, NEEDLE_SIZE));
 *
 *  // Compile the trie:
 *  Ace->Compile();
 *
 *  // Search a buffer for instances of all patterns in the trie:
 *  Ace->Search(HAYSTACK, HAYSTACK_SIZE);
 *
 *  // Iterate patterns in the trie to find matches:
 *  for(int i = 0; i < Ace->patterns.size(); i++)
 *      if(Ace->patterns[i]->hits)
 *          ; // A pattern was matched! Do something here.
 *
 */

#ifndef _ACE_H_
#define _ACE_H_

#pragma intrinsic(memset, memcpy)

#include <vector>
#include <queue>
#include <tuple>
#include "ACEPattern.h"

#define BOOL bool

using std::vector;
using std::queue;
using std::tuple;
using std::make_tuple;
using std::get;

class ACE
{
    typedef unsigned char       byte;
    typedef unsigned int        uint;
    typedef unsigned short      ushort;
    typedef unsigned long       ulong;
    typedef vector<ACEPattern*> PatternList;

    public:
        PatternList patterns;
		vector<std::string> results;
		
        ACE();
        void AddPattern(ACEPattern* pattern);
        void Compile();

        /*
         *  The abstracted INode methods leave us with a very compact search loop.
         */
        void Search(byte* buffer, uint length);
        ~ACE();
};


#endif //_ACE_H_
