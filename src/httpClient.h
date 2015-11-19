#ifndef _HTTP_CLIENT_H_H
#define _HTTP_CLIENT_H_H
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/dns.h>
#include "blockingQueue.h"
#include "bloomFilter.h"
#include "threadPool.h"
#include <condition_variable>
struct ResNode { //response node
	char buf[1024];
	size_t bufWindow = 1024; //buf memory window
	int bufLen = 0; //used
	ResNode *next = NULL; //next node of this response
};
struct Response {
	~Response() {
		ResNode *tmp = headRes;
		while (tmp) {
			headRes = tmp;
			tmp = tmp->next;
			delete headRes;
		}
	}
	ResNode *headRes = NULL;
	ResNode *tailRes = NULL;
	size_t conLen = 0; // html total len
	size_t conRecLen = 0; //received html len
};
class HttpClient {
public:
	HttpClient(const std::string &_host, const std::string &_url, int _bevName);
	~HttpClient() {
	}
	size_t bevName;
	const std::string host;
	const int port;
	std::string curURL;
	event_base *base;
	evdns_base *dns_base;
	bufferevent *bev;
	BloomFilter bf;
	void request();
	void responseParser(ResNode *resNode);
	static void scannerThread();
	static void requestThread(bufferevent *bev, const std::string &host, std::string *URL);
	static BlockingQueue<std::string> urlQueue;
	static BlockingQueue<Response *> resQueue;
	static ltcp::ThreadPool scannerThreadPool;
	static ltcp::ThreadPool requestThreadPool;
	static BloomFilter bloomFilter;
	static ofstream resultFile;
private:
	Response *response = NULL;
	enum state {
		state0, state1, state2, state3, state4, state5, state6, state7, state8,
		state9, state10, state11, state12, state13, state14, state15, state16, state17,
		state18, state19, state20, state21, state22, state23, state24, state25, state26,
		state27, state28, state29, state30
	};
};
#endif