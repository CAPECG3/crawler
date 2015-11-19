#ifndef _CRAWLER_H_
#define _CRAWLER_H_
#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/dns.h>
#include <string.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include "httpClient.h"
class Crawler {
public:
	Crawler(const std::string &_host, const std::string &_url);
private:
	const  std::string host;  //host being crawled
	const std::string initURL; //the first url 
	const int port; //host port
	event_base *base; //libevent event_base
	std::vector<bufferevent *> bev; //bufferevent with a socket 
	std::vector<HttpClient *> httpClient;  //
	std::vector<evutil_socket_t> sock;
	evdns_base *dns_base;
	void init_event();
	void connect();
	static size_t socketNum;
	static size_t socketNumLimit;
	static size_t endNum;
	static void cb_connect(struct bufferevent *bev, short events, void *ptr);
	static void cb_read(struct bufferevent *bev, void *ptr);
	static void cb_write(struct bufferevent *bev, void *ptr);
};
#endif
