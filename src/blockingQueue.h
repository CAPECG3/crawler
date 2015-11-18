#ifndef _BLOCKINGQUEUE_H_
#define _BLOCKINGQUEUE_H_
#include <condition_variable>
#include <list>
#include <assert.h>
template<typename T>
class BlockingQueue  {
public:
	BlockingQueue () : _mutex (), _condvar (), _queue ()  {

	}

	void push (const T& task)  {
		std::lock_guard<std::mutex> lock (_mutex);
		_queue.push_back (task);
		_condvar.notify_all ();
	}

	T pop ()  {
		std::unique_lock<std::mutex> lock (_mutex);
		_condvar.wait (lock, [this] {return !_queue.empty ();});
		assert (!_queue.empty ());
		T front (_queue.front ());
		_queue.pop_front ();

		return front;
	}

	size_t size() const {
		std::lock_guard<std::mutex> lock (_mutex);
		return _queue.size();
	}

private:
	BlockingQueue (const BlockingQueue& rhs);
	BlockingQueue& operator = (const BlockingQueue& rhs);

	mutable std::mutex _mutex;
	std::condition_variable _condvar;
	std::list<T> _queue;
};
#endif