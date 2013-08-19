/*
 * Telnet class
 * Copyright (C) 2013  Nicolai Tufar ntufar@gmail.com
 * 
 */

#ifndef TELNET_H
#define TELNET_H

using boost::asio::ip::tcp;
using namespace std;

class telnet_client{
public:
	enum { max_read_length = 512 };

public:	
	bool is_read_que_empty(void);
	char read_char_from_que(void);
	
	telnet_client(boost::asio::io_service& io_service, tcp::resolver::iterator endpoint_iterator);
	void write(const char msg);
	void close();
	deque<char> readque;
	boost::mutex mtx_;
};


#endif // TELNET_H
