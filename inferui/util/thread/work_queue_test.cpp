/*
   Copyright 2016 DeepCode GmbH

   Author: Veselin Raychev
 */

#include "util/thread/work_queue.h"

#include "base/stringprintf.h"
#include "glog/logging.h"
#include <string>
#include <thread>
#include <mutex>

#include "external/gtest/googletest/include/gtest/gtest.h"


void OneConsumerTest(int queue_capacity, int num_threads, int num_items_per_thread) {
  ProducerConsumerQueue<std::string> q(queue_capacity);
  std::vector<std::thread> threads;
  for (int thread_id = 0; thread_id < num_threads; ++thread_id) {
    threads.emplace_back([&q,thread_id,num_items_per_thread](){
      for (int i = 0; i < num_items_per_thread; ++i) {
        q.Push(StringPrintf("%d-%d", thread_id, i));
      }
      q.Push("");
    });
  }
  std::set<std::string> popped;
  int num_empty = 0;
  for (;;) {
    std::string s(q.PopSwap());
    if (s.empty()) {
      ++num_empty;
      if (num_empty == num_threads) break;
    } else {
      EXPECT_TRUE(popped.insert(s).second);
    }
  }
  EXPECT_EQ(num_threads * num_items_per_thread, static_cast<int>(popped.size()));
  for (int thread_id = 0; thread_id < num_threads; ++thread_id) {
    threads[thread_id].join();
  }
}

TEST(ProducerConsumerQueueTest, OneConsumer128_16_128) {
  OneConsumerTest(128, 16, 128);
}
TEST(ProducerConsumerQueueTest, OneConsumer8_32_128) {
  OneConsumerTest(8, 32, 128);
}
TEST(ProducerConsumerQueueTest, OneConsumer1024_32_1024) {
  OneConsumerTest(1024, 32, 1024);
}
TEST(ProducerConsumerQueueTest, OneConsumer1_8_128) {
  OneConsumerTest(1, 8 ,128);
}
TEST(ProducerConsumerQueueTest, OneConsumer8_1_1024) {
  OneConsumerTest(8, 1, 1024);
}


void RunTasks(size_t num_threads, size_t max_queue_size, size_t num_tasks) {
  SimpleThreadPool pool(num_threads, max_queue_size);
  int64_t parallel_computed_sum = 0;
  std::mutex m;
  for (size_t i = 0; i < num_tasks; ++i) {
    pool.AddTask([i,&m,&parallel_computed_sum](){
      int64_t sum = 0;
      for (size_t k = 0; k < i; ++k) sum += sqrt(k);

      std::unique_lock<std::mutex> lock(m);
      parallel_computed_sum += sum;
    });
  }
  pool.JoinAll();

  int64_t seqentially_computed_sum = 0;
  int64_t tmp_sum = 0;
  for (size_t i = 0; i < num_tasks; ++i) {
    seqentially_computed_sum += tmp_sum;
    tmp_sum += sqrt(i);
  }

  EXPECT_EQ(parallel_computed_sum, seqentially_computed_sum);
}


TEST(SimpleThreadPoolTest, Pool16_128_4096) {
  RunTasks(16, 128, 4096);
}
TEST(SimpleThreadPoolTest, Pool8_1024_16394) {
  RunTasks(8, 1024, 16384);
}
TEST(SimpleThreadPoolTest, Pool2_8_16384) {
  RunTasks(2, 8, 16384);
}
TEST(SimpleThreadPoolTest, Pool16_128_16384) {
  RunTasks(16, 128, 16384);
}


int main(int argc, char **argv) {
  google::InstallFailureSignalHandler();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

