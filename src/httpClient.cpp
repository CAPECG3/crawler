#include "httpClient.h"
BlockingQueue<std::string> HttpClient::urlQueue;
BlockingQueue<Response *> HttpClient::resQueue;
ltcp::ThreadPool HttpClient::scannerThreadPool;
BloomFilter HttpClient::bloomFilter;
std::string HttpClient::initPath;
int HttpClient::readNum = 1;
ofstream HttpClient::resultFile("result.txt");
HttpClient::HttpClient(const std::string &_host, const std::string &_url, int _bevName):
	host(_host), curURL(_url), port(80), bevName(_bevName) {
}
void HttpClient::request() {
	//requestThreadPool.push(requestThread, bev, host, &curURL);
	curURL = urlQueue.pop();
	size_t tmp = curURL.find_last_of("/");
	if (tmp != std::string::npos) {
		curPath = curURL.substr(0, tmp + 1);
	}
	else {
		curPath = "/";
	}
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
bool HttpClient::responseParser(ResNode *resNode) {
	if (resNode->bufLen > 15  && !strncmp(resNode->buf, "HTTP/1.1", 8) ) {
		if (!strncmp(resNode->buf, "HTTP/1.1 200 OK", 15) || !strncmp(resNode->buf, "HTTP/1.1 301", 12)  ) {
			response = new Response();
			response->headRes = resNode;
			response->tailRes = resNode;
			response->status = 200;
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
				char *chunk = strstr(resNode->buf , "chunked");
				if (chunk != NULL)response->encoding = "chunked";
			}
			char *conBegin = strstr(resNode->buf, "\r\n\r\n");
			if (conBegin && response->conLen >= 4) {
				conBegin += 4;
				response->conRecLen = (resNode->bufLen - (conBegin - resNode->buf));
			}
		}
		else if (!strncmp(resNode->buf, "HTTP/1.1 404", 12)) {
			//std::cout << "HTTP response status 404" << std::endl;
			response = new Response();
			response->status = 404;
			response->headRes = resNode;
			response->tailRes = resNode;
			response->conLen = 1375; //only for news.sohu.com
			response->conRecLen = resNode->bufLen;
		}
		else if (!strncmp(resNode->buf, "HTTP/1.1 400", 12)) {
			response = new Response();
			response->status = 400;
			response->headRes = resNode;
			response->tailRes = resNode;
			response->conLen = 1142; //only for news.sohu.com
			response->conRecLen = resNode->bufLen;
		}
		else {
			return false;
		}
	}
	else if (response != NULL && (response->status == 200)) {
		if (response->encoding == "text") {
			response->conRecLen += resNode->bufLen;
			response->tailRes->next = resNode;
			response->tailRes = resNode;
		}
		else if (response->encoding == "chunked") {
			response->conRecLen += resNode->bufLen;
			response->tailRes->next = resNode;
			response->tailRes = resNode;
			char *ifEnd = strstr(resNode->buf , "\r\n0\r\n\r\n");
			if (ifEnd != NULL) {
				std::cout << readNum++ << " URL crawled" << std::endl;
				std::cout << urlQueue.size() << " URL left" << std::endl;
				resultFile << host << curURL << " " << response->conLen << std::endl;
				response->curPath = curPath;
				//add response to response queue
				resQueue.push(response);
				response = NULL;
				return true;
			}
			return false;
		}
	}
	else if (response != NULL && response->status == 404) {
		response->conRecLen += resNode->bufLen;
		response->tailRes->next = resNode;
		response->tailRes = resNode;
	}
	else if (response != NULL && response->status == 400) {
		response->conRecLen += resNode->bufLen;
		response->tailRes->next = resNode;
		response->tailRes = resNode;
	}
	else {
		std::cout << "HTTP response status is not 200/301/400" << std::endl;
		return false;
	}
	if (response && response->conRecLen >= response->conLen && response->encoding == "text") {
		if (response->status == 200) {
			std::cout << readNum++ << " URL crawled" << std::endl;
			std::cout << urlQueue.size() << " URL left" << std::endl;
			resultFile << host << curURL << " " << response->conLen << std::endl;
			response->curPath = curPath;
			//add response to response queue
			resQueue.push(response);
			response = NULL;
		}
		else if (response->status == 404 || response->status == 400) {
			//std::cout << "HTTP response status 404" << std::endl;
			delete response;
			response = NULL;
		}
		return true;
	}
	return false;
}
void HttpClient::scannerThread() {
	// this dfs is only for news.sohu.com
	while (1) {
		Response *response = resQueue.pop();
		ResNode *resNode = response->headRes;
		state curState = state0;
		std::string urlTmp;
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
					else if (str[i] == '\\') break;
					else  curState = state2;
					break;
				case state7:
					if (str[i] == ' ')  curState = state7;
					else if (str[i] == '\"'  || str[i] == '\'') curState = state8;
					else  curState = state0;
					break;
				case state8:
					if (str[i] == ' ') break;
					else if (str[i] == '>') curState = state0;
					else if (str[i] == '\"'  || str[i] == '\'') curState = state9;
					else if (str[i] == '\\') break;
					else if (str[i] == '\n' || str[i] == '\r') break;
					else {
						urlTmp.push_back(str[i]);
					}
					break;
				}
				if (curState == state9) {
					//Get the relative path
					/*
					size_t tmp = urlTmp.find("http://");
					if (tmp != std::string::npos) {
						urlTmp = urlTmp.substr(tmp + 7);
					}
					*/
					size_t tmp1 = urlTmp.find("http://news.sohu.com");
					size_t tmp2 = urlTmp.find("127.0.0.1");
					if (tmp1 != std::string::npos) {
						urlTmp = urlTmp.substr(tmp1 + 20);
					}
					else if (tmp2 != std::string::npos) {
						urlTmp = urlTmp.substr(tmp2 + 9);
					}
					else if (urlTmp.find(".com") != std::string::npos || urlTmp.find(".net") != std::string::npos
					         || urlTmp.find(".cn") != std::string::npos || urlTmp.find("www.") != std::string::npos
					         || urlTmp.find(".org") != std::string::npos || urlTmp.find("javascript") != std::string::npos) {
						urlTmp = "";
						curState = state0;
						break;
					}
					else if (urlTmp[0] != '/' && urlTmp[0] != '#') {
						urlTmp = response->curPath + urlTmp;

					}
					else if (urlTmp[0] == '#') {
						urlTmp = urlTmp.substr(1);
					}
					size_t tmp = urlTmp.find_last_of("?");
					if (tmp != std::string::npos) {
						urlTmp = urlTmp.substr(0, tmp);
					}
					if (initPath != "/") urlTmp = initPath + urlTmp;
					if (urlTmp.size() != 0 && bloomFilter.bfCheck(urlTmp)) {
						urlQueue.push(urlTmp);
					}
					urlTmp = "";
					curState = state0;
				}
			}
			resNode = resNode->next;
		}
		delete response;
		response = NULL;
	}
}
