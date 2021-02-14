/*
   Copyright 2016 DeepCode GmbH

   Author: Veselin Raychev
 */

#ifndef UTIL_THREAD_WORK_QUEUE_H_
#define UTIL_THREAD_WORK_QUEUE_H_

#include "glog/logging.h"

#include <condition_variable>
#include <cmath>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

template<class El>
class ProducerConsumerQueue {
public:
  ProducerConsumerQueue(size_t max_size = 1024) : max_size_(max_size) {
  }

  void Push(const El& el) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      while (queue_.size() >= max_size_) {
        cond_var_full_.wait(lock);
      }
      queue_.push(el);
    }
    cond_var_empty_.notify_one();
  }

  template<typename... _Args>
  void Emplace(_Args&&... __args) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      while (queue_.size() >= max_size_) {
        cond_var_full_.wait(lock);
      }
      queue_.emplace(std::forward<_Args>(__args)...);
    }
    cond_var_empty_.notify_one();
  }

  El PopSwap() {
    El result;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      while (queue_.empty()) {
        cond_var_empty_.wait(lock);
      }
      std::swap(queue_.front(), result);
      queue_.pop();
    }
    cond_var_full_.notify_one();
    return result;
  }

  El PopCopy() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (queue_.empty()) {
      cond_var_empty_.wait(lock);
    }
    El result(queue_.front());
    queue_.pop();
    lock.unlock();
    cond_var_full_.notify_one();
    return result;
  }

private:
  size_t max_size_;
  std::mutex mutex_;
  std::condition_variable cond_var_empty_;
  std::condition_variable cond_var_full_;
  std::queue<El> queue_;
};



class SimpleThreadPool {
public:
  SimpleThreadPool(size_t num_threads = 16, size_t max_queue_size = 1024)
      : num_threads_(num_threads), max_queue_size_(max_queue_size), stopped_(false), joined_(false) {
  }

  // Delete the move constructor, because the child threads capture a pointer to this.
  SimpleThreadPool(SimpleThreadPool&& x) = delete;
  SimpleThreadPool(SimpleThreadPool& x) = delete;

  void AddTask(const std::function<void()>& f) {
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK(!joined_);
    if (threads_.size() < num_threads_) {
      lock.unlock();
      std::thread t(StartThread());
      lock.lock();
      CHECK(!joined_);
      threads_.push_back(std::move(t));
    }
    while (queue_.size() >= max_queue_size_) {
      if (stopped_) return;
      cond_var_full_.wait(lock);
    }
    queue_.push(f);
    lock.unlock();
    cond_var_empty_.notify_all();
  }

  void StopAll() {
    std::unique_lock<std::mutex> lock(mutex_);
    stopped_ = true;
    lock.unlock();
    cond_var_full_.notify_all();
    cond_var_empty_.notify_all();
  }

  void JoinAll() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (joined_) return;
    joined_ = true;
    lock.unlock();
    cond_var_empty_.notify_all();

    for (size_t i = 0; i < threads_.size(); ++i) {
      threads_[i].join();
    }
  }
private:
  std::thread StartThread() {
    return std::thread([this](){
      for (;;) {
        std::unique_lock<std::mutex> lock(mutex_);
        for (;;) {
          if (!queue_.empty()) break;
          if (queue_.empty() && (joined_ || stopped_)) {
            return;
          }
          cond_var_empty_.wait(lock);
        }
        if (stopped_) return;
        CHECK(!queue_.empty());
        std::function<void()> fn = queue_.front();
        queue_.pop();
        lock.unlock();
        cond_var_full_.notify_one();
        fn();
      }
    });
  }

  std::queue<std::function<void()>> queue_;
  std::vector<std::thread> threads_;

  std::mutex mutex_;
  std::condition_variable cond_var_empty_;
  std::condition_variable cond_var_full_;

  size_t num_threads_;
  size_t max_queue_size_;
  bool stopped_, joined_;
};


#endif /* UTIL_THREAD_WORK_QUEUE_H_ */
