//
// Created by lifang on 15/10/28.
//

#ifndef _TLCP_TASK_H_
#define _TLCP_TASK_H_
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
namespace ltcp {

class Task {
	typedef std::function<void()> function;
private:
	function _task;
	std::mutex &_mutex;
	std::queue<function> tasks;
	std::condition_variable cv;
	std::condition_variable &notify;
public:
	Task(std::condition_variable &notifyMe,
				 std::mutex &mut) : notify(notifyMe), _mutex(mut) {

	}

	void push(function &&func) {
		std::lock_guard<std::mutex> guard(_mutex);
		tasks.push(func);
		cv.notify_one();
	}

	function pop() {
		function task;
		std::unique_lock<std::mutex> lock(_mutex);
		cv.wait(lock, [this] { return !tasks.empty(); });
		if (tasks.size()) {
			task = tasks.front();
			tasks.pop();
		}
		if (tasks.size() == 0)
			notify.notify_one();
		return task;
	}

	void clear() {
		std::lock_guard<std::mutex> guard(_mutex);
		while (!tasks.empty()) {
			tasks.pop();
		}
	}

	size_t size() {
		return tasks.size();
	}
};
}
#endif //TLCP_TASK_HPP
