//
// Created by Lu Zhang on 10/4/16.
//

#include <iostream>
#include <stdatomic.h>
#include <thread>
#include "mutex_set.h"



template <typename T>
class LockFreeCircularArray {
 public:
  LockFreeCircularArray(uint32_t cap) {
    capacity_ = cap;
    LockFreeCircularArray();
  }
  LockFreeCircularArray() : write_idx_(0), read_idx_(0), read_guard_idx_(0)  {
    array_ = new T[capacity_];
  }
  ~LockFreeCircularArray() {
    delete array_;
  }

  uint32_t GetCapacity() {
    return capacity_;
  }

  uint32_t size() {
    return write_idx_ >= read_idx_ ? write_idx_ - read_idx_ : write_idx_ + capacity_ - read_idx_;
  }
  bool push(const T& e) {
    uint32_t write_idx_snapshot;
    uint32_t read_idx_snapshot;
    do {
      read_idx_snapshot = read_idx_;
      write_idx_snapshot = write_idx_;
      if (ModCap(write_idx_snapshot + 1) == ModCap(read_idx_snapshot)) // One space is wasted
        return false;
      if (!atomic_compare_exchange_strong(&write_idx_, &write_idx_snapshot, write_idx_snapshot+1)) {
        continue;
      }
      array_[ModCap(write_idx_snapshot)] = e;
      break;
    } while (true);

    while(!atomic_compare_exchange_strong(&read_guard_idx_, &write_idx_snapshot, write_idx_snapshot+1)) {
      sched_yield();
    }
    return true;
  }

  T pop() {
    uint32_t read_idx_snapshot;
    T e;
    do {
      read_idx_snapshot = read_idx_;
      // if (MODCAP(read_idx_snapshot) == MODCAP(read_guard_idx_))
      if (read_idx_snapshot == read_guard_idx_)
        return false;
      // e = array_[MODCAP(read_idx_snapshot)];
      if (!atomic_compare_exchange_strong(&read_idx_, &read_idx_snapshot, read_idx_snapshot+1))
        continue;
      return array_[ModCap(read_idx_snapshot)];
    } while (true);
  }


 private:
  uint32_t kDefaultCapacity = 1 << 10;
  uint32_t capacity_ = kDefaultCapacity;
  T* array_;
  _Atomic(uint32_t) write_idx_;
  _Atomic(uint32_t) read_idx_;
  _Atomic(uint32_t) read_guard_idx_;


  inline uint32_t ModCap(uint32_t idx) {
    return  idx % capacity_;
  }

};

void producer(LockFreeCircularArray<uint32_t >* queue, uint32_t start, uint32_t end) {
  bool ret;
  for (uint32_t i = start; i < end; ++i) {
    do {
      ret = queue->push(i);
      if (ret) {
        std::cout << "pushing " << i << " size:" << queue->size() << std::endl;
      } else {
        std::cout << "[FAIL] pushing " << i << " queue full." << std::endl;
      }
    } while (!ret);
  }
}

void consumer(LockFreeCircularArray<uint32_t >* queue) {
  while (queue->size() != 0) {
    uint32_t val = queue->pop();
    std::cout << "popping " << val << " size:" << queue->size() << std::endl;
  }
}

int main() {
  std::cout << "Hello, LockFree!" << std::endl;
  LockFreeCircularArray<uint32_t > queue;
  std::thread prod1(producer, &queue, 0, 10);
  //std::thread consumer1(consumer, &queue);
  std::thread prod2(producer, &queue, 10, 20);
  //std::thread consumer2(consumer, &queue);
  std::thread prod3(producer, &queue, 20, 30);
  //std::thread consumer3(consumer, &queue);
  prod1.join();
  prod2.join();
  prod3.join();
  //consumer1.join();
  //consumer2.join();
  //consumer3.join();
  return 0;
  /*
  uint32_t cnt = 1 << 3;
  for (uint32_t i = 0; i < cnt ; ++i) {
    bool ret = queue.push(i);
    if (ret) {
      std::cout << "after pushing " << i << " queue size:" << queue.size() << std::endl;
    } else {
      std::cout << "[FAIL] pushing " << i << " queue full." << std::endl;
    }
  }
  for (uint32_t i = 0; i < cnt-2; ++i) {
    uint32_t val = queue.pop();
    std::cout << "after popping " << val << " queue size:" << queue.size() << std::endl;
  }
  bool ret;
  uint32_t i = 0;
  while ((ret = queue.push(i++))) {
    if (ret) {
      std::cout << "after pushing " << i << " queue size:" << queue.size() << std::endl;
    } else {
      std::cout << "[FAIL] pushing " << i << " queue full." << std::endl;
    }
  }
  while (queue.size() != 0) {
    uint32_t val = queue.pop();
    std::cout << "after popping " << val << " queue size:" << queue.size() << std::endl;
  }
   */

}