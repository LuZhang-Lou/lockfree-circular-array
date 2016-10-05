//
// Created by Lu Zhang on 10/4/16.
//

#ifndef LOCKFREEQUEUE_MUTEX_SET_H
#define LOCKFREEQUEUE_MUTEX_SET_H
#include <unordered_set>
#include <mutex>

template <typename T, typename Comp = std::less<T>>
class MutexSet {
 public:
  typedef typename std::unordered_set<T, Comp>::iterator iterator;
  std::pair<iterator, bool> insert(const T& val) {
    std::unique_lock<std::mutex> loc(mutex_);
    return set_.insert(val);
  };
 private:
  std::mutex mutex_;
  std::unordered_set<T> set_;
};

#endif //LOCKFREEQUEUE_MUTEX_SET_H
