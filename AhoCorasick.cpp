/*
 * Implementatin of Aho-Corasick multiple string search algorithm
 * Copyright (C) 2013  Nicolai Tufar ntufar@gmail.com
 * 
 */


#include "AhoCorasick.h"


using namespace std;

 
 
/////////////////////////////////////////////////////////////////////////////////////////
// Aho-Corasick's algorithm, as explained in  http://dx.doi.org/10.1145/360825.360855  //
/////////////////////////////////////////////////////////////////////////////////////////


//const int MAXS = 22000; // Max number of states in the matching machine.
                              // Should be equal to the sum of the length of all keywords.

                              
                              
const int MAXC = 0xff; // Number of characters in the alphabet.
 
 
map<int, map<int,int> > out; // Output for each state, as a bitwise mask.
               // Bit i in this mask is on if the keyword with index i appears when the
               // machine enters this state.
// Used internally in the algorithm.

map<int,int> f; // Failure function

map< int, map<int,int> > g; // Goto function, or -1 if fail.


// Builds the string matching machine.
// 
// words - Vector of keywords. The index of each keyword is important:
//         "out[state] & (1 << i)" is > 0 if we just found word[i] in the text.
// lowestChar - The lowest char in the alphabet. Defaults to 'a'.
// highestChar - The highest char in the alphabet. Defaults to 'z'.
//               "highestChar - lowestChar" must be <= MAXC, otherwise we will
//               access the g matrix outside its bounds and things will go wrong.
//
// Returns the number of states that the new machine has. 
// States are numbered 0 up to the return value - 1, inclusive.
int buildMatchingMachine(const vector<string> &words, char lowestChar = 0, char highestChar = 0xff) {
  out.clear();
  f.clear();
  g.clear();
    
    int states = 1; // Initially, we just have the 0 state
        
    for (int i = 0; i < words.size(); ++i) {
        const string &keyword = words[i];
        int currentState = 0;
        for (int j = 0; j < keyword.size(); ++j) {
            int c = keyword[j] - lowestChar;
            if (g[currentState].find(c) == g[currentState].end()) { // Allocate a new node
                g[currentState][c] = states++;
            }
            currentState = g[currentState][c];
        }
        //out[currentState] |= (1 << i); // There's a match of keywords[i] at node currentState.
        out[currentState][i] |= 1; // There's a match of keywords[i] at node currentState.
    }
    
    // State 0 should have an outgoing edge for all characters.
    for (int c = 0; c < MAXC; ++c) {
        if (g[0].find(c) == g[0].end()) {
            g[0][c] = 0;
        }
    }
 
    // Now, let's build the failure function
    queue<int> q;
    for (int c = 0; c <= highestChar - lowestChar; ++c) {  // Iterate over every possible input
        // All nodes s of depth 1 have f[s] = 0
        if (g[0].find(c) != g[0].end() and g[0][c] != 0) {
            f[g[0][c]] = 0;  
            q.push(g[0][c]);
        }
    }
    while (q.size()) {
        int state = q.front();
        q.pop();
        for (int c = 0; c <= highestChar - lowestChar; ++c) {
            if (g[state].find(c) != g[state].end()) {
                int failure = f[state];
                while (g[failure].find(c) == g[failure].end()) {
                    failure = f[failure];
                }
                failure = g[failure][c];
                f[g[state][c]] = failure;
                //out[g[state][c]] |= out[failure]; // Merge out values
		/*
				for( int x = 0; x < out[failure].size(); x++ ){
				  if(out[g[state][c]][x]==0)
					out[g[state][c]][x] = out[failure][x];
				}
		*/
                q.push(g[state][c]);
            }
        }
    }
 
    return states;
}


// Finds the next state the machine will transition to.
//
// currentState - The current state of the machine. Must be between
//                0 and the number of states - 1, inclusive.
// nextInput - The next character that enters into the machine. Should be between lowestChar 
//             and highestChar, inclusive.
// lowestChar - Should be the same lowestChar that was passed to "buildMatchingMachine".
 
// Returns the next state the machine will transition to. This is an integer between
// 0 and the number of states - 1, inclusive.
int findNextState(int currentState, char nextInput, char lowestChar = 0) {
    int answer = currentState;
    int c = nextInput - lowestChar;
    while (g[answer].find(c) == g[answer].end()) answer = f[answer];
    return g[answer][c];
}


/////////////////////////////////////////////////////////////////////////////////////////
//                          End of Aho-Corasick's algorithm.                           //
/////////////////////////////////////////////////////////////////////////////////////////



AhoCorasick::AhoCorasick(){
}

void AhoCorasick::initializeMachine(const vector< string >& patterns)
{
  this->patterns.clear();
  this->patterns = patterns;
  buildMatchingMachine(this->patterns, 0, 0xff);
}



std::string string_to_hex(const std::string& input);
std::string hex_to_string(const std::string& input);
std::string char_to_hex(unsigned char input);

vector< string > AhoCorasick::query(string text)
{
   vector<string> result;
    int currentState = 0;
    for (int i = 0; i < text.size(); ++i) {
       currentState = findNextState(currentState, text[i], 0);
       if (out.find(currentState) == out.end()) continue; // Nothing new, let's move on to the next character. 
       for (int j = 0; j < this->patterns.size(); ++j) {
           if (out[currentState][j] != 0) { // Matched keywords[j]
	     result.push_back(this->patterns[j]);
           }
       }
   }
   return result;
}




vector< string > AhoCorasick::query(unsigned char *text, size_t length)
{
   vector<string> result;
   unsigned char *x = (unsigned char*)text;
   string y;
   
   int currentState = 0;
    for (int i = 0; i < length; ++i) {
       currentState = findNextState(currentState, text[i], 0);
       if (out.find(currentState) == out.end()) continue; // Nothing new, let's move on to the next character. 
       for (int j = 0; j < this->patterns.size(); ++j) {
           if (out[currentState][j] != 0) { // Matched keywords[j]
	     result.push_back(this->patterns[j]);
           }
       }
   }
   return result;
}

