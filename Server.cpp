#include <iostream>
#include <string>
#include <map>
#include <ostream>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <istream>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <boost/algorithm/string.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include <iostream>
#include "ACEPattern.h"
#include "ACE.cpp"
using namespace std;

vector<string> keywords;
    typedef unsigned char byte;
    typedef unsigned int  uint;
    typedef unsigned long ulong;
    

class Pattern {
public:
	string 	HexString;
	string 	BinaryString;
	string 	GUID;
};
map<string, Pattern> patterns;
//AhoCorasick ahocorasick;
ACE *ace = new ACE();

std::string char_to_hex(unsigned char input){
   static const char* const lut = "0123456789abcdef";
   std::string output;
   
   output.push_back(lut[input >> 4]);
   output.push_back(lut[input & 15]);
   return output;
}

std::string string_to_hex(const std::string& input)
{
    static const char* const lut = "0123456789abcdef";
    size_t len = input.length();

    std::string output;
    output.reserve(2 * len);
    for (size_t i = 0; i < len; ++i)
    {
        const unsigned char c = input[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
	output += " ";
    }
    return output;
}

#include <algorithm>
#include <stdexcept>
std::string hex_to_string(const std::string& input)
{
  string s = input;
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    static const char* const lut = "0123456789ABCDEF";
    size_t len = s.length();
    if (len & 1) throw std::invalid_argument("odd length");

    std::string output;
    output.reserve(len / 2);
    for (size_t i = 0; i < len; i += 2)
    {
        char a = s[i];
        const char* p = std::lower_bound(lut, lut + 16, a);
        if (*p != a) throw std::invalid_argument("not a hex digit");

        char b = s[i + 1];
        const char* q = std::lower_bound(lut, lut + 16, b);
        if (*q != b) throw std::invalid_argument("not a hex digit");

        output.push_back(((p - lut) << 4) | (q - lut));
    }
    return output;
}

#include <fstream>
#include <string>
#include <cerrno>
std::string get_file_contents(const char *filename)
{
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  if (in)
  {
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();
    return(contents);
  }
  throw(errno);
}


bool addPattern(string line){
	Pattern p;
	size_t dotPosition = line.find('.');
	if( dotPosition == -1 )
		return false;

	p.HexString = line.substr( 0, dotPosition);
	p.GUID = line.substr( line.find('{') + 1, line.find('}') - line.find('{') - 1 );

	if( p.HexString.length() < 10 )
		return false;
	
	string s = hex_to_string(p.HexString);
	
	patterns[s] = p;
	ACEPattern *aceP = new ACEPattern((byte*)s.c_str(), s.size(), true);
	ace->AddPattern(aceP);
	//keywords.push_back( s );

	return true;
}

string loadPaternsFile(string patternFilePath){
	if( ace != NULL )
		delete ace;
	ace = new ACE();
	
    patterns.clear();
    keywords.clear();
    ifstream patternsFile;
    string line;
    string result = "Pattern file "+patternFilePath+" loaded successfully.";

    patternsFile.open( patternFilePath.c_str() );
    if( patternsFile.is_open() ){
	    int counter = 0;
	    while( patternsFile.good() ){
		    std::getline( patternsFile, line );
		    if( line.size() < 5 ){
		      continue;
		    }
		    if( addPattern(line) == false ){
		      continue;
		    }
		    counter++;
	    }
	    cout << "Read ";
	    cout << counter;
	    cout << " patterns from file" <<endl;
	    patternsFile.close();
    }else{
	    return "Failed to open pattern file: "+patternFilePath;
    }

	ace->Compile();
    //ahocorasick.initializeMachine(keywords);
    
    return result;
}

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}


int main(int argc, char **argv) {
    string patternsFilePath = "/home/ntufar/projects/Scanner/doc/patterns100";
    string loadPatternsResult = loadPaternsFile(patternsFilePath);
    cout << loadPatternsResult << endl;
    
    
    std::string userInput;
    
    while(1){
      cout << "OVER" << endl;
      
      getline (cin, userInput);
      vector<string> tokens;
      //boost::split(tokens, userInput, boost::is_any_of("\t \r\n"));
      //boost::tokenizer<boost::escaped_list_separator<char> > t(userInput, boost::escaped_list_separator<char>("\\", ",", "\""));
      boost::tokenizer<boost::escaped_list_separator<char> > t(userInput, boost::escaped_list_separator<char>("\\", " ", "\""));
	  BOOST_FOREACH(string s, t)
		tokens.push_back(s);
      
      if( tokens.size() < 1 )
	continue;
      
      if(tokens[0] == "quit"){
		cout << "Godbye" <<endl;
		exit(0);
      }
      
      if(tokens[0] == "loadpatterns"){
		if( tokens.size() < 2 ){
		  cout << "Please provide file name"<<endl;
		  continue;
		}
		string loadpatternsresult = loadPaternsFile(tokens[1]);
		cout << loadpatternsresult << endl;
		continue;
      }
      
      if(tokens[0] == "scanfile"){
		void *filecontents;
		string scanFileName = tokens[1];
		

		{
			boost::interprocess::file_mapping m_file(scanFileName.c_str(), boost::interprocess::read_only);
			boost::interprocess::mapped_region region(m_file, boost::interprocess::read_only); 
			
			ace->Search((byte*)region.get_address(), region.get_size());
			//results = ahocorasick.query((unsigned char*)region.get_address(), region.get_size());
		}
		
		for(std::vector<string>::iterator it = ace->results.begin(); it != ace->results.end(); ++it) {
		  string s = *it;
		  Pattern p = patterns[s];
		  cout << p.GUID << endl;
		}
		continue;
      }
      
      if(tokens[0] == "scanarray"){
		string charArray = tokens[1]; 
		//cout <<tokens[1] <<endl;
		string binArray = hex_to_string(charArray);
		//cout << string_to_hex(binArray)<<endl;
		
		ace->Search((byte*)binArray.c_str(), binArray.size());
		
		//vector<string> results = ahocorasick.query(binArray);
		
		
		for(std::vector<string>::iterator it = ace->results.begin(); it != ace->results.end(); ++it) {
		  string s = *it;
		  Pattern p = patterns[s];
		  cout << p.GUID << endl;
		}
		continue;
	  }
      
      cout << "Unrecognized command."<<endl;
    }
    
    exit(0);
    
    
    string text = keywords[9];
    
    
   
    text = "hello wworld this will be a viruu"+text+"hello world again";
    
    cout << "text:          " << string_to_hex(text) << endl;
    
    
    //vector<string> results = ahocorasick.query((unsigned char*)filecontents, filesize);
    //vector<string> results = ahocorasick.query(text);
    //vector<string> results = ahocorasick.query(whole_file);
    //vector<string> results = ahocorasick.query(x);
    
    
    
    return 0;
}

