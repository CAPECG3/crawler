#include"bloomFilter.h"
#include<cmath>
#include<iostream>
#include<cstring>
void BloomFilter::bfAdd(unsigned int(*hashFamily[])(const char *str, unsigned int len), const char *x) {
	std::lock_guard<std::mutex> lock (_mutex);
	for (int i = 0; i < K; i++) {
		int len = strlen(x);
		unsigned int hash = (*hashFamily[i])(x, len);
		unsigned int index = hash % m;
		binary[index] = true;
	}
}
bool BloomFilter::bfSearch(unsigned int(*hashFamily[])(const char *str, unsigned int len), const char *x) {
	std::lock_guard<std::mutex> lock (_mutex);
	for (int i = 0; i < K; i++) {
		int len = strlen(x);
		unsigned int hash = (*hashFamily[i])(x, len);
		unsigned int index = hash % m;
		if (binary[index] == false) {
			return false;
		}
	}
	return true;
}
bool BloomFilter::bfCheck(const string &url) {
	const char *url_c = url.c_str();
	if (bfSearch(h.hashFamily, url_c)) {
		return false;
	}
	else {
		bfAdd(h.hashFamily, url_c);
		return true;
	}
}