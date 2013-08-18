CC=clang++
CXX=clang++
CFLAGS=`pkg-config gtkmm-3.0 --cflags --libs`

all: Server ScannerGUI

Server: AhoCorasick.o Server.o

ScannerGUI: ScannerGUI.cpp
	$(CXX) -o ScannerGUI ScannerGUI.cpp $(CFLAGS)

clean:
	rm -f AhoCorasick.o Server.o Server ScannerGUI
