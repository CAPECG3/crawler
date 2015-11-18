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
struct ResponseHeader {
	char *content;
	size_t contentLength = 0;
	size_t curLength = 0;
	size_t receivedLength = 0;
	bool readDone = false;
};
class HttpClient {
public:
	HttpClient(const std::string &_host, const std::string &_url, int _bevName):
		host(_host), curURL(_url), port(80), bevName(_bevName) {
	}
	~HttpClient() {
		if (responseHeader) {
			delete responseHeader;
			responseHeader = NULL;
		}
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
	void headerParser(char *buf, size_t len);
	static BlockingQueue<std::string> urlQueue;
	static ltcp::ThreadPool threadPool;
	static BloomFilter bloomFilter;
	static ofstream resultFile;
private:
	static void requestThread(bufferevent *bev, const std::string &host, string *URL);
	static void scannerThread(char *str, size_t len);
	static void scanner(char *str, size_t len);
	ResponseHeader *responseHeader = NULL;
	enum state {
		state0, state1, state2, state3, state4, state5, state6, state7, state8, state9, state10
	};
};
#endif