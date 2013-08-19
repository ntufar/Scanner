CC=clang++
CXX=clang++
CFLAGS=`pkg-config gtkmm-3.0 --cflags --libs` -ggdb
LDFLAGS=-lboost_system -lboost_thread-mt

all: Server ScannerGUI

Server: AhoCorasick.o Server.o

ScannerGUI: ScannerGUI.cpp telnet.cpp
	$(CXX) -o ScannerGUI ScannerGUI.cpp telnet.cpp $(CFLAGS) $(LDFLAGS)

clean:
	rm -f AhoCorasick.o Server.o Server ScannerGUI
