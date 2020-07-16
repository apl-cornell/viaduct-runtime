#ifndef __SYNCQUEUE_H__
#define __SYNCQUEUE_H__

#include <queue>
#include <mutex>
#include <condition_variable>

// Thread-safe queue.
template <class T>
class SyncQueue
{
private:
  std::queue<T> queue;
  mutable std::mutex mutex;
  std::condition_variable cond_var;

public:
  SyncQueue(): queue(), mutex(), cond_var() {}

  ~SyncQueue() {}

  // Add an element to the queue.
  void enqueue(T elem) {
    std::lock_guard<std::mutex> lock(this->mutex);
    queue.push(elem);
    cond_var.notify_one();
  }

  // Get the "front"-element.
  // If the queue is empty, wait until a element is avaiable.
  T dequeue(void) {
    std::unique_lock<std::mutex> lock(this->mutex);

    // release lock as long as the wait and reaquire it afterwards.
    while (queue.empty()) {
      cond_var.wait(lock);
    }

    T val = queue.front();
    queue.pop();
    return val;
  }
};

#endif
