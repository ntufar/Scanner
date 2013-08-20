CC=clang++
CXX=clang++
CFLAGS=`pkg-config gtkmm-3.0 --cflags` -ggdb -std=c++11
LDFLAGS=-lboost_system-mt -lboost_thread-mt -lboost_filesystem-mt `pkg-config gtkmm-3.0 --libs`

all: Server ScannerGUI

Server: Server.o

%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

ScannerGUI: ScannerGUI.cpp telnet.cpp
	$(CXX) -o ScannerGUI ScannerGUI.cpp telnet.cpp $(CFLAGS) $(LDFLAGS)

clean:
	rm -f Server.o Server ScannerGUI ACE.o
