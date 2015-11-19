#include "httpClient.h"
BlockingQueue<std::string> HttpClient::urlQueue;
BlockingQueue<Response *> HttpClient::resQueue;
ltcp::ThreadPool HttpClient::scannerThreadPool;
BloomFilter HttpClient::bloomFilter;
ofstream HttpClient::resultFile("result.txt");
HttpClient::HttpClient(const std::string &_host, const std::string &_url, int _bevName):
	host(_host), curURL(_url), port(80), bevName(_bevName) {
}
void HttpClient::request() {
	//requestThreadPool.push(requestThread, bev, host, &curURL);
	std::cout << urlQueue.size() << " URL left" << std::endl;
	curURL = urlQueue.pop();
	std::string requestString;
	requestString =  "GET " + curURL + " HTTP/1.1\r\n"
	                 + "Host: " +  host + "\r\n"
	                 + "Connection: keep-alive\r\n"
	                 + "Cache-Control: max-age=0\r\n"
	                 //+"Accept:*/*\r\n"
	                 + "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"
	                 + "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/43.0.2357.134 Safari/537.36\r\n"
	                 + "Accept-Encoding: gzip, deflate, sdch\r\n"
	                 + "Accept-Language: zh-CN,zh;q=0.8,sq;q=0.6\r\n"
	                 + "\r\n";
	bufferevent_write(bev, requestString.c_str(), requestString.size());
};
void HttpClient::responseParser(ResNode *resNode) {
	if (resNode->bufLen > 15  && !strncmp(resNode->buf, "HTTP/1.1", 8)) {
		if (!strncmp(resNode->buf, "HTTP/1.1 200 OK", 15) || !strncmp(resNode->buf, "HTTP/1.1 301", 12)) {
			response = new Response();
			response->headRes = resNode;
			response->tailRes = resNode;
			char *conLenBegin = strstr(resNode->buf , "Content-Length");
			if (conLenBegin != NULL) {
				char tmp[10] = {0};
				conLenBegin += 16;
				for (int i = 0; * (conLenBegin + i) != '\r'; i++) {
					tmp[i] = *(conLenBegin + i);
				}
				response->conLen = atoi(tmp);
			}
			else {
				std::cout << "debug" << std::endl;
			}
			char *conBegin = strstr(resNode->buf, "\r\n\r\n");
			if (conBegin && response->conLen >= 4) {
				conBegin += 4;
				response->conRecLen = (resNode->bufLen - (conBegin - resNode->buf));
			}
		}
		else if (!strncmp(resNode->buf, "HTTP/1.1 404", 12)) {
			std::cout << "HTTP response status 404" << std::endl;
			response = new Response();
			response->status = 404;
			response->headRes = resNode;
			response->tailRes = resNode;
			response->conLen = 1375; //only for news.sohu.com
			response->conRecLen = resNode->bufLen;
		}
		else {
			return ;
		}
	}
	else if (response != NULL && response->status == 200) {
		response->conRecLen += resNode->bufLen;
		response->tailRes->next = resNode;
		response->tailRes = resNode;
	}
	else if (response != NULL && response->status == 404) {
		response->conRecLen += resNode->bufLen;
		response->tailRes->next = resNode;
		response->tailRes = resNode;
	}
	else {
		std::cout << "HTTP response status is not 200/301/400" << std::endl;
		if (resNode->bufLen < resNode->bufWindow) {
			request();
		}
		return ;
	}
	if (response && response->conRecLen >= response->conLen && response->conLen != 0) {
		if (response->status == 200) {
			static int readNum = 1;
			std::cout << readNum++ << " URL crawled" << std::endl;
			resultFile << host << curURL << " " << response->conLen << std::endl;
			//add response to response queue
			resQueue.push(response);
			response = NULL;
		}
		else if (response->status == 404) {
			std::cout << "HTTP response status 404" << std::endl;
			delete response;
			response = NULL;
		}
		request();
		return ;
	}
}
void HttpClient::scannerThread() {
	// this dfs is only for news.sohu.com
	while (1) {
		Response *response = resQueue.pop();
		ResNode *resNode = response->headRes;
		state curState = state0;
		std::string urlTmp = "/";
		while (resNode) {
			char *str = resNode->buf;
			for (size_t i = 0; i < resNode->bufLen; i++) {
				switch (curState) {
				case state0:
					if (str[i] == '<')  curState = state1;
					break;
				case state1:
					if (str[i] == 'a')  curState = state2;
					else   curState = state0;
					break;
				case state2:
					if (str[i] == 'h')  curState = state3;
					break;
				case state3:
					if (str[i] == 'r')  curState = state4;
					else if (str[i] == '>') curState = state0;
					else  curState = state2;
					break;
				case state4:
					if (str[i] == 'e')  curState = state5;
					else if (str[i] == '>') curState = state0;
					else  curState = state2;
					break;
				case state5:
					if (str[i] == 'f')  curState = state6;
					else if (str[i] == '>') curState = state0;
					else  curState = state2;
					break;
				case state6:
					if (str[i] == '=')  curState = state7;
					else if (str[i] == '>') curState = state0;
					else  curState = state2;
					break;
				case state7:
					if (str[i] == ' ')  curState = state7;
					else if (str[i] == '\"') curState = state8;
					else  curState = state0;
					break;
				case state8:
					if (str[i] == 'h') curState = state9;
					else if (str[i] == '/') curState = state29;
					else if (str[i] == ' ') break;
					else curState = state0;
					break;
				case state9:
					if (str[i] == 't') curState = state10;
					else curState = state0;
					break;
				case state10:
					if (str[i] == 't') curState = state11;
					else curState = state0;
					break;
				case state11:
					if (str[i] == 'p') curState = state12;
					else curState = state0;
					break;
				case state12:
					if (str[i] == ':') curState = state13;
					else curState = state0;
					break;
				case state13:
					if (str[i] == '/') curState = state14;
					else curState = state0;
					break;
				case state14:
					if (str[i] == '/') curState = state15;
					else curState = state0;
					break;
				case state15:
					if (str[i] == 'n') curState = state16;
					else curState = state0;
					break;
				case state16:
					if (str[i] == 'e') curState = state17;
					else curState = state0;
					break;
				case state17:
					if (str[i] == 'w') curState = state18;
					else curState = state0;
					break;
				case state18:
					if (str[i] == 's') curState = state19;
					else curState = state0;
					break;
				case state19:
					if (str[i] == '.') curState = state20;
					else curState = state0;
					break;
				case state20:
					if (str[i] == 's') curState = state21;
					else curState = state0;
					break;
				case state21:
					if (str[i] == 'o') curState = state22;
					else curState = state0;
					break;
				case state22:
					if (str[i] == 'h') curState = state23;
					else curState = state0;
					break;
				case state23:
					if (str[i] == 'u') curState = state24;
					else curState = state0;
					break;
				case state24:
					if (str[i] == '.') curState = state25;
					else curState = state0;
					break;
				case state25:
					if (str[i] == 'c') curState = state26;
					else curState = state0;
					break;
				case state26:
					if (str[i] == 'o') curState = state27;
					else curState = state0;
					break;
				case state27:
					if (str[i] == 'm') curState = state28;
					else curState = state0;
					break;
				case state28:
					if (str[i] == '/') curState = state29;
					else curState = state0;
					break;
				case state29:
					if (str[i] == ' ') break;
					else if (str[i] == '>') curState = state0;
					else if (str[i] == '\"') curState = state30;
					else if (str[i] == '\n' || str[i] == '\r') break;
					else {
						urlTmp.push_back(str[i]);
					}
					break;
				}
				if (curState == state30) {
					if (bloomFilter.bfCheck(urlTmp)) {
						urlQueue.push(urlTmp);
					}
					urlTmp = "/";
					curState = state0;
				}
			}
			resNode = resNode->next;
		}
		delete response;
		response = NULL;
	}
}
