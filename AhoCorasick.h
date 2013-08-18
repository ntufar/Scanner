/*
 * Implementatin of Aho-Corasick multiple string search algorithm
 * Copyright (C) 2013  Nicolai Tufar ntufar@gmail.com
 * 
 */

#ifndef AHOCORASICK_H
#define AHOCORASICK_H

#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>
#include <sstream>
#include <fstream>
#include <cassert>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cstdio>
#include <vector>
#include <cmath>
#include <queue>
#include <deque>
#include <stack>
#include <list>
#include <map>
#include <set>

using namespace std;

class AhoCorasick
{
private:
  vector<string> patterns;
public:
  AhoCorasick( const vector<string> &patterns ); // Constructor
  AhoCorasick(); // Constructor
  void initializeMachine(const vector< string > &patterns);
  
  vector<string> query(string text);
  vector<string> query(unsigned char *text, size_t length);

};

#endif // AHOCORASICK_H
