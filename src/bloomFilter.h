#ifndef _BLOOM_FILTER_H_
#define _BLOOM_FILTER_H_
#include <condition_variable>
#include <fstream>
#include <cmath>
#include "hash.h"
using namespace std;
class BloomFilter {
public:
	bool bfCheck(const string &url);
	//BloomFilter() :n(6), K(11), p(0.01) {
	BloomFilter() : n(150000), p(0.01) {
		mCount();
		binary = new bool[m];
		fill(binary, binary + m, false);
	}
	~BloomFilter() {
		delete [] binary;
		binary = NULL;
	}
private:
	const int n;//字符串个数
	int m;//二进制位长度
	const double p;//错误率
	bool *binary;
	void mCount() {
		m = n * 1.44 * log2(1.0 / p);
	}
	void bfAdd(unsigned int(*hashFamily[])(const char *str, unsigned int len), const char *x);
	bool bfSearch(unsigned int(*hashFamily[])(const char *str, unsigned int len), const char *x);
	mutable std::mutex _mutex;
	Hash h;
};
#endif