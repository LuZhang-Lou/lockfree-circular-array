//
// Created by Lu Zhang on 10/4/16.
//

#include <iostream>
#include <atomic>
#include <thread>
#include <vector>
#include <set>
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

  T pop(bool* res) {
    uint32_t read_idx_snapshot;
    T e;
    do {
      read_idx_snapshot = read_idx_;
      if (read_idx_snapshot == read_guard_idx_) {
        *res = false;
        return 0;
      }
      // e = array_[MODCAP(read_idx_snapshot)];
      if (!atomic_compare_exchange_strong(&read_idx_, &read_idx_snapshot, read_idx_snapshot+1))
        continue;
      *res = true;
      return array_[ModCap(read_idx_snapshot)];
    } while (true);
  }


 private:
  uint32_t kDefaultCapacity = 1 << 10;
  uint32_t capacity_ = kDefaultCapacity;
  T* array_;
  std::atomic<uint32_t> write_idx_;
  std::atomic<uint32_t> read_idx_;
  std::atomic<uint32_t> read_guard_idx_;

  inline uint32_t ModCap(uint32_t idx) {
    return  idx % capacity_;
  }

};

void producer(std::atomic<int>* guard, LockFreeCircularArray<uint32_t >* queue, uint32_t start, uint32_t end) {
  bool ret;
  for (uint32_t i = start; i < end; ++i) {
    do {
      ret = queue->push(i);
    } while (!ret);
  }
  (*guard)++;
}

void consumer(std::atomic<int>* prod_token, std::atomic<int>* con_token, LockFreeCircularArray<uint32_t >* queue, std::set<uint32_t >* set) {
  while (queue->size() != 0  || *prod_token != 4) {
    bool res;
    uint32_t val = queue->pop(&res);
    if (res) {
      set->insert(val);
    }
  }
  (*con_token)++;
}

int main() {
  std::cout << "Hello, LockFree!" << std::endl;
  LockFreeCircularArray<uint32_t > queue;
  std::atomic<int> prod_token(0);
  std::atomic<int> con_token(0);
  std::vector<std::set<uint32_t> > sets(5);
  std::thread prod1(producer, &prod_token, &queue, 0, 10000);
  std::thread prod2(producer, &prod_token, &queue, 10000, 20000);
  std::thread prod3(producer, &prod_token, &queue, 20000, 30000);
  std::thread prod4(producer, &prod_token, &queue, 30000, 40000);

  std::thread consumer1(consumer, &prod_token, &con_token, &queue, &sets[0]);
  std::thread consumer3(consumer, &prod_token, &con_token, &queue, &sets[2]);
  std::thread consumer4(consumer, &prod_token, &con_token, &queue, &sets[3]);
  std::thread consumer5(consumer, &prod_token, &con_token, &queue, &sets[4]);
  std::thread consumer2(consumer, &prod_token, &con_token, &queue, &sets[1]);

  prod1.detach();
  prod2.detach();
  prod3.detach();
  prod4.detach();

  consumer1.detach();
  consumer2.detach();
  consumer3.detach();
  consumer4.detach();
  consumer5.detach();
  while (con_token != 5) {
    ;
  }
  std::cout << "verifying..: " << std::endl;
  for (uint32_t i = 0; i < 40000u; ++i) {
    uint8_t j = 0;
    while (j < 5u) {
      if (sets[j].find(i) != sets[j].end()) {
        break;
      }
      ++j;
    }
    if (j == 5u) {
      std::cout << "missing : " << i << std::endl;
    }
  }
  return 0;
}
