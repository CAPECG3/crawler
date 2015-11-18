#include "httpClient.h"
BlockingQueue<std::string> HttpClient::urlQueue;
ltcp::ThreadPool HttpClient::threadPool(4);
BloomFilter HttpClient::bloomFilter;
ofstream HttpClient::resultFile("result.txt");
void HttpClient::request() {
	//threadPool.push(requestThread, bev, host, &curURL);
	requestThread(bev, host, &curURL);
};
void HttpClient::requestThread(bufferevent *bev, const std::string &host, string *URL) {
	std::cout << urlQueue.size() << " URL left" << std::endl;
	*URL = urlQueue.pop();
	std::string requestString;
	requestString =  "GET " + *URL + " HTTP/1.1\r\n"
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
}
void HttpClient::headerParser(char *buf, size_t len) {
	if (!strncmp(buf, "HTTP/1.1", 8)) {
		if (!strncmp(buf, "HTTP/1.1 200 OK", 15)) {
			if (responseHeader)
				delete responseHeader;
			responseHeader = new ResponseHeader();
			responseHeader->curLength = len;
			char *header = buf + 17;
			//find content-length
			char *contentBegin = strstr(header , "Content-Length");
			if (contentBegin != NULL) {
				char tmp[10] = {0};
				contentBegin += 16;
				for (int i = 0; * (contentBegin + i) != '\r'; i++) {
					tmp[i] = *(contentBegin + i);
				}
				responseHeader->contentLength = atoi(tmp);
			}
			//html data point
			if (responseHeader->contentLength != 0) {
				responseHeader->content = strstr(contentBegin, "\r\n\r\n");
			}
			if (responseHeader->content != NULL) {
				responseHeader->content += 4;
				responseHeader->curLength = (len - (responseHeader->content - buf));
				responseHeader->receivedLength = responseHeader->curLength;
			}
		}
		else if (!strncmp(buf, "HTTP/1.1 404", 12)) {
			request();
			return ;
		}
		else if (!strncmp(buf, "HTTP/1.1 301", 12)) {
			request();
			return ;
		}
		else {
			return ;
		}
	}
	else if (responseHeader != NULL) {
		responseHeader->receivedLength += len;
		responseHeader->curLength = len;
		responseHeader->content = buf;
		scannerThread(buf, len);
	}
	if (responseHeader && responseHeader->receivedLength >= responseHeader->contentLength && responseHeader->contentLength != 0) {
		//std::cout << "bev" << bevName << " Read done." << std::endl;
		static int readNum = 1;
		std::cout << readNum++ << " URL crawled" << std::endl;
		responseHeader->readDone = true;
		resultFile << host << curURL << " " << responseHeader->contentLength << std::endl;
		if (responseHeader) {
			delete responseHeader;
			responseHeader = NULL;
		}
		request();
		return ;
	}
}
void HttpClient::scanner(char *str, size_t len) {
	threadPool.push(scannerThread, str, len);
}
void HttpClient::scannerThread(char *str, size_t len) {
	//fwrite(str, 1, len, stdout);
	state curState = state0;
	std::string urlTmp = "/";
	for (size_t i = 0; i < len; i++) {
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
			else if (str[i] == '\"') curState = state10;
			else  curState = state0;
			break;
		case state10:
			if (str[i] == '/') {   curState = state8; break;  }
			if ((len - i) >= 7) {
				std::string tmp(str + i, 7);
				if (tmp == "http://") {
					i += 7;
				}
			}
			//if ((len - i) >= this->host.size()) {
			//std::string tmp(str + i, this->host.size());
			//if (tmp == this->host) {
			//i += this->host.size();
			//break;
			//}
			if ((len - i) >= 13) {
				std::string tmp(str + i, 13);
				if (tmp == "news.sohu.com") {
					i += 12;
					/*
					count
					static int hehe=0;
					std::cout << ++hehe << std::endl;
					*/
					break;
				}
				else {
					curState = state0;
					break;
				}
			}
			else if (str[i] == ' ')    break;
			else curState = state0;
			break;
		case state8:
			if (str[i] == ' ') break;
			else if (str[i] == '>') curState = state0;
			else if (str[i] == '\"') curState = state9;
			else if (str[i] == '\n' || str[i] == '\r') break;
			else {
				urlTmp.push_back(str[i]);
			}
			break;
		}
		if (curState == state9) {
			if (bloomFilter.bfCheck(urlTmp)) {
				urlQueue.push(urlTmp);
			}
			urlTmp = "/";
			curState = state0;
		}
	}
	//delete str;
}
