//
// Created by lifang on 15/10/27.
//


#ifndef _TLCP_THREADS_H_
#define _TLCP_THREADS_H_

#include <thread>
#include <vector>
#include <sstream>
#include <string>
#include <stdexcept>
#include <chrono>
#include "task.h"

namespace ltcp {
class ThreadPool {
	typedef std::function<void()> function;
private:
	bool kill = false;
	size_t _size;

	std::vector<std::thread> threads;
	bool used = false;
	std::condition_variable cv;
	Task bq;
	std::mutex _mutex;


public:
	ThreadPool(size_t size = 4) : _size(size), bq(cv, _mutex) {
	}
	//push tasks into threadPool
	void push(ThreadPool::function closure) {
		bq.push(std::move(closure));

	}

	template<class _Fp, class... _Args>
	void push(_Fp func, _Args ..._args) {
		function fun = std::bind(func, _args...);
		bq.push(std::move(fun));
	};
	//start thread run
	void run() {
		for (size_t i = 0; i < _size; ++i) {
			threads.push_back(std::thread(
			[&]() {
				while (true) {
					function task;
					{
						task = bq.pop();

						if (kill) return;
					}
					task();
				}
			}
			                  ));
		}
	}
	//cancel remain tasks
	void cancel() {
		kill = true;
		for (size_t i = 0; i < threads.size(); ++i) {
			bq.push(function([]() { }));
		}
		for (auto &t : threads) {
			t.join();
		}
		kill = false;
		threads.clear();
	}
	//wait for tasks to be done
	void wait() {
		if (threads.size() == 0)
			return;
		{
			std::unique_lock<std::mutex> lock(_mutex);
			//cv.wait_for(lock,std::chrono::milliseconds(10),[&](){return bq.size()==0;});
			cv.wait(lock, [&]() { return bq.size() == 0; });
		}
		//cancel();
	}

	~ThreadPool() {
		cancel();
	}
};

}
#endif //TLCP_THREADS_HPP