#include "crawler.h"
size_t Crawler::endNum = 0;
size_t Crawler::socketNum = 1;
size_t Crawler::socketNumLimit = 200;
Crawler::Crawler(const std::string &_host, const std::string &_url):
	host(_host), initURL(_url), port(80), sock(socketNumLimit),
	bev(socketNumLimit, NULL), httpClient(socketNumLimit, NULL) {
	HttpClient::urlQueue.push(initURL);
	HttpClient::scannerThreadPool.run();
	for (size_t i = 0; i < HttpClient::scannerThreadPool._size; i++) {
		HttpClient::scannerThreadPool.push(HttpClient::scannerThread);
	}
	init_event();
	connect();
}
void Crawler::init_event() {
	//init event_base

	base = event_base_new();
	dns_base = evdns_base_new(base, 1);
	endNum = 0;
	for (size_t i = 0; i < socketNum; i++) {
		httpClient[i] = new HttpClient(host, initURL, i);
		httpClient[i]->dns_base = dns_base;
		//socket new
		bev[i] = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
		//bufferevent_setcb(bev[i], cb_read, cb_write , cb_connect, httpClient[i]);
		bufferevent_setcb(bev[i], cb_read, NULL , cb_connect, httpClient[i]);
		bufferevent_enable(bev[i], EV_READ | EV_WRITE | EV_PERSIST);
		//bufferevent_set_timeouts(bev, &read_time, &write_time);
		httpClient[i]->bev = bev[i];
		httpClient[i]->base = base;
	}
}
void Crawler::connect() {
	for (size_t i = 0; i < socketNum; i++) {
		//connect all socket
		bufferevent_socket_connect_hostname(bev[i], dns_base, AF_UNSPEC, host.c_str(), port);
	}
	//event loop
	event_base_dispatch(base);
	init_event();
	connect();
}
void Crawler::cb_connect(struct bufferevent *bev, short events, void *ptr) {
	HttpClient *httpClient = (HttpClient *)ptr;
	if (events & BEV_EVENT_CONNECTED) {
		std::cout << "bev" << httpClient->bevName << " Connect okay." << std::endl;
		//send request
		httpClient->request();
	}
	else if (events & BEV_EVENT_ERROR) {
		int err = bufferevent_socket_get_dns_error(bev);
		if (err)
			std::cout << "bev:" << httpClient->bevName << " DNS error: " << evutil_gai_strerror(err) << std::endl;
		std::cout << "bev" << httpClient->bevName << " Closing" << std::endl;
	}
	else if (events & BEV_EVENT_EOF) {
		event_base *base = httpClient->base;
		evdns_base *dns_base = httpClient->dns_base;
		std::cout << "bev" << httpClient->bevName << " Connect closed" << std::endl;
		bufferevent_free(bev);
		delete httpClient;
		endNum++;
		if (endNum >= socketNum) {
			std::cout << "base loop break" << std::endl;
			event_base_loopbreak(base);
			evdns_base_free(dns_base, true);
			event_base_free(base);
			socketNum = socketNumLimit;
		}
	}
}
void Crawler::cb_read(struct bufferevent * bev, void *ptr) {
	HttpClient *httpClient = (HttpClient *)ptr;
	//std::cout << "bev" << httpClient->bevName << " Read okay." << std::endl;
	struct evbuffer *input = bufferevent_get_input(bev);
	size_t len = evbuffer_get_length(input);
	int n;
	while (1) {
		ResNode *resNode = new ResNode();
		n = evbuffer_remove(input, resNode->buf, resNode->bufWindow);
		//fwrite(resNode->buf, 1, n, stdout);
		if (n <= 0) {
			delete resNode;
			break;
		}
		resNode->bufLen = n;
		if (httpClient->responseParser(resNode) == true) {
			httpClient->request();
		}
	}
}
void Crawler::cb_write(struct bufferevent * bev, void *ptr) {
	HttpClient *httpClient = (HttpClient *)ptr;
	//std::cout << "bev" << httpClient->bevName << " Write okay." << std::endl;
}
