/*
 Copyright 2017 DeepCode GmbH

 Author: Veselin Raychev
 */

#include "parse_queue.h"

#include <algorithm>

#include "glog/logging.h"
#include "json/json.h"

#include "base/strutil.h"
#include "base/stringprintf.h"

DEFINE_int64(parser_memory_limit, 1024*1024*1024, "Memory limit in bytes (default 1GB)");

ParseWorker::ParseWorker(
    const std::vector<std::string>& cmd_and_params)
    : subprocess_(new Subprocess(cmd_and_params)) {
  LOG(INFO) << "worker: " << JoinStrings(cmd_and_params, " ");
}

bool ParseWorker::SendParseTask(ParseTask&& task) {
  Json::Value arr(Json::arrayValue);
  Json::Value& obj = arr[0];
  for (auto attr : task.attributes) {
    obj[attr.first] = attr.second;
  }
  obj["code"] = task.code;
  Json::FastWriter writer;
  std::string line = writer.write(arr);
  VLOG(2) << "Sending task id " << task.id << " to " << subprocess_->pid();
  VLOG(3) << "Sending " << line;

  return subprocess_->Write(line.data(), line.size());
}

void ParseWorker::Flush() {
  VLOG(3) << "Flush";
  subprocess_->Flush();
}

void ParseWorker::Start(
    const std::function<void(ParseResult&&)>& parse_result_cb,
    const std::function<void()>& done_cb) {
  subprocess_->SetMemoryLimit(FLAGS_parser_memory_limit);
  subprocess_->Start([this,parse_result_cb,done_cb](int stdout_fd) {
    // capture the callback by copy as it may be executed after the method exits.
    {
      Json::CharReaderBuilder builder;
      std::unique_ptr<Json::CharReader> jsonreader(builder.newCharReader());
      FDLineReader r(stdout_fd);
      std::string s;
      VLOG(2) << "Reading from " << stdout_fd << ".";
      while (r.ReadLine(&s)) {
        ParseResult result;
        std::string errors;
        if (!jsonreader->parse(s.c_str(), s.c_str() + s.size(), &result.json_response, &errors)) {
          LOG(ERROR) << "Invalid JSON response from parser " << errors;
          break;
        }

        if (result.json_response.isMember("parse_error")) {
          result.parse_error = result.json_response["parse_error"].asString();
        }

        parse_result_cb(std::move(result));
        VLOG(2) << "Reading from " << stdout_fd;
      }
    }
    VLOG(2) << "Done receiving from " << stdout_fd << ".";
    done_cb();
  }, [](int stderr_fd) {
    FDLineReader r(stderr_fd);
    std::string s;
    while (r.ReadLine(&s)) {
      LOG(ERROR) << "Error: " << s;
    }
    VLOG(2) << "Process finished.";
  });
  VLOG(2) << "Started a worker.";
}


void ParseWorker::WaitAndStop() {
  subprocess_->Close();
  subprocess_.reset(nullptr);
}

ParseQueue::ParseQueue(
    int max_workers_per_language,
    const GetFileCodeCB& get_file_cb,
    const FileParsedCB& file_parsed_cb,
    const GetCommandCB& get_command_cb)
    : max_workers_per_language_(max_workers_per_language),
      get_file_cb_(get_file_cb),
      file_parsed_cb_(file_parsed_cb),
      get_command_cb_(get_command_cb),
      current_id_(0),
      joining_(false),
      recovery_running_(false) {
}

void ParseQueue::Join() {
  {
    std::lock_guard<std::mutex> guard(mutex_);
    CHECK(!joining_);
    joining_ = true;
  }
  while (JoinWorkers() || JoinRecovery()) {
  }
}

bool ParseQueue::JoinWorkers() {
  std::map<std::string, std::unique_ptr<LanguageWorkerSet>> workers;
  {
    std::lock_guard<std::mutex> guard(workers_mutex_);
    workers.swap(workers_);
  }
  for (auto it = workers.begin(); it != workers.end(); ++it) {
    LanguageWorkerSet* language_workers = it->second.get();
    VLOG(1) << "Joining " << language_workers->queues.size() << " workers.";
    for (size_t i = 0; i < language_workers->queues.size(); ++i) {
      WorkerQueue* queue = language_workers->queues[i].get();
      WaitQueueHasNoPendingSends(queue);
    }
  }
  for (auto it = workers.begin(); it != workers.end(); ++it) {
    LanguageWorkerSet* language_workers = it->second.get();
    VLOG(1) << "Closing " << language_workers->queues.size() << " workers.";
    for (size_t i = 0; i < language_workers->queues.size(); ++i) {
      WorkerQueue* queue = language_workers->queues[i].get();
      queue->worker.Close();
    }
  }
  for (auto it = workers.begin(); it != workers.end(); ++it) {
    LanguageWorkerSet* language_workers = it->second.get();
    VLOG(1) << "Joining " << language_workers->queues.size() << " workers.";
    for (size_t i = 0; i < language_workers->queues.size(); ++i) {
      WorkerQueue* queue = language_workers->queues[i].get();
      queue->worker.Stop();
    }
  }

  return !workers.empty();
}

bool ParseQueue::JoinRecovery() {
  std::vector<std::thread> recovery_threads;
  {
    std::lock_guard<std::mutex> guard(mutex_);
    recovery_threads.swap(recovery_threads_);
  }
  if (!recovery_threads.empty()) {
    recovery_tasks_ready_.notify_all();
    for (size_t i = 0; i < recovery_threads.size(); ++i) {
      recovery_threads[i].join();
    }
    return true;
  }
  return false;
}

int64_t ParseQueue::AllocateTaskId() {
  std::lock_guard<std::mutex> lock(workers_mutex_);
  int64_t id = current_id_;
  current_id_++;
  return id;
}

void ParseQueue::SendTask(const std::string& language, int64_t task_id) {
  ParseTask task;
  task.id = task_id;
  get_file_cb_(task_id, &task);
  WorkerQueue* queue;
  {
    std::lock_guard<std::mutex> guard(workers_mutex_);
    ParseQueue::LanguageWorkerSet* workers = GetWorkers(language);
    queue = GetQueue(language, workers);
    queue->sent_tasks.push(task_id);
    queue->num_pending_sends++;
  }
  {
    std::lock_guard<std::mutex> guard(queue->send_mutex);
    queue->worker.SendParseTask(std::move(task));
    if (task.attributes.count("flush")) {
      queue->worker.Flush();
    }
  }
  {
    std::lock_guard<std::mutex> guard(workers_mutex_);
    queue->num_pending_sends--;
  }
  queue->pending_sends.notify_all();
}

void ParseQueue::WaitQueueHasNoPendingSends(WorkerQueue* queue) {
  std::unique_lock<std::mutex> guard(workers_mutex_);
  queue->pending_sends.wait(guard, [queue]()->bool{ return queue->num_pending_sends == 0; });
}

ParseQueue::WorkerQueue* ParseQueue::GetQueue(
    const std::string& language,
    LanguageWorkerSet* workers) {
  DCHECK(!workers_mutex_.try_lock());  // Check that workers_mutex_ is held.
  int first_valid = -1;
  for (size_t i = 0; i < workers->queues.size(); ++i) {
    if (workers->queues[i]->is_valid &&
        workers->queues[i]->sent_tasks.empty()) {
      return workers->queues[i].get();
    }
    if (workers->queues[i]->is_valid) {
      first_valid = i;
    }
  }
  if (first_valid == -1 ||
      static_cast<int>(workers->queues.size()) < max_workers_per_language_) {
    WorkerQueue* queue = new WorkerQueue(get_command_cb_(language));
    workers->queues.push_back(std::unique_ptr<WorkerQueue>(queue));
// This form is within a workers_mutex_, because otherwise another thread may perform join before we are started.
    queue->worker.Start([this,workers,queue](ParseResult&& result){
      workers_mutex_.lock();
      queue->num_received++;
      queue->sent_tasks.pop();
      workers_mutex_.unlock();
      file_parsed_cb_(std::move(result));
    }, [this,workers,queue,language](){
      std::queue<int64_t> pending_tasks;
      workers_mutex_.lock();
      WorkerQueue* queue_to_be_deleted = GetWorkerItemsForRecovery(language, queue, &pending_tasks);
      queue->is_valid = false;
      workers_mutex_.unlock();
      // From now on, nobody can change the remaining tasks in the queue.

      if (!pending_tasks.empty() || queue_to_be_deleted != nullptr) {
        LOG(INFO) << "There are " << pending_tasks.size() << " unfinished tasks after subprocessed died.";
        LeaveWorkerItemsForRecovery(language, std::move(pending_tasks), queue_to_be_deleted);
        // Start the thread after submitting the task, because if we are finishing
        // then the recovery thread should never be idle (elsewhere it may stop).
        StartErrorRecoveryThreadIfNotRunning();
      }
    });
    VLOG(2) << "Started task worker " << queue->worker.pid();
    return queue;
  }

  size_t min_size = workers->queues[first_valid]->sent_tasks.size();
  int min_i = first_valid;
  for (size_t i = 0; i < workers->queues.size(); ++i) {
    if (workers->queues[i]->is_valid &&
        workers->queues[i]->sent_tasks.size() < min_size) {
      min_size = workers->queues[i]->sent_tasks.size();
      min_i = i;
    }
  }
  return workers->queues[min_i].get();
}

ParseQueue::WorkerQueue* ParseQueue::GetWorkerItemsForRecovery(
    const std::string& language,
    WorkerQueue* queue,
    std::queue<int64_t>* pending_tasks) {
  DCHECK(!workers_mutex_.try_lock());
  pending_tasks->swap(queue->sent_tasks);

  auto workers_it = workers_.find(language);
  if (workers_it != workers_.end()) {
    for (size_t i = 0; i < workers_it->second->queues.size(); ++i) {
      if (workers_it->second->queues[i].get() == queue) {
        WorkerQueue* result = workers_it->second->queues[i].release();  // Now we own the queue.
        workers_it->second->queues.erase(workers_it->second->queues.begin() + i);
        return result;
      }
    }
  }
  return nullptr;
}

void ParseQueue::LeaveWorkerItemsForRecovery(
    const std::string& language,
    std::queue<int64_t>&& pending_tasks,
    WorkerQueue* queue_that_should_be_deleted) {
  {
    std::lock_guard<std::mutex> guard(mutex_);
    RecoveryTasks& recovery = recovery_tasks_[language];
    recovery.suspected_fail_tasks.push_back(pending_tasks.front());
    pending_tasks.pop();
    while (!pending_tasks.empty()) {
      recovery.remaining_tasks.push_back(pending_tasks.front());
      pending_tasks.pop();
    }
    if (queue_that_should_be_deleted != nullptr) {
      // Put the queue to be deleted by the recovery thread.
      failed_queues_.emplace(queue_that_should_be_deleted, std::unique_ptr<WorkerQueue>(queue_that_should_be_deleted));
      recovery.failed_queues.push_back(queue_that_should_be_deleted);
    }
  }
  recovery_tasks_ready_.notify_all();
}


ParseQueue::LanguageWorkerSet* ParseQueue::GetWorkers(const std::string& language) {
  DCHECK(!workers_mutex_.try_lock());  // Must be locked.
  auto it = workers_.find(language);
  if (it == workers_.end()) {
    it = workers_.emplace(
        language, std::unique_ptr<LanguageWorkerSet>(new LanguageWorkerSet())).first;
  }
  return it->second.get();
}

void ParseQueue::StartErrorRecoveryThreadIfNotRunning() {
  std::lock_guard<std::mutex> guard(mutex_);
  if (recovery_running_) return;
  recovery_running_ = true;
  recovery_threads_.emplace_back([this](){
    RecoveryThread();
  });
}

void ParseQueue::RecoveryThread() {
  LOG(INFO) << "Starting recovery thread.";
  for (;;) {
    VLOG(1) << "Waiting to recover task...";

    // Wait for a task.
    RecoveryTasks task;
    std::string language;
    {
      std::unique_lock<std::mutex> guard(mutex_);
      for (auto it = recovery_tasks_.begin(); it != recovery_tasks_.end(); ++it) {
        VLOG(1) << "Pending tasks for language:" << it->first
            << " crashing-tasks:" << it->second.suspected_fail_tasks.size()
            << " remaining-tasks:" << it->second.remaining_tasks.size()
            << " subprocesses-to-join:" << it->second.failed_queues.size();
      }
      recovery_tasks_ready_.wait(guard, [this]()->bool{
        return joining_ ||
            std::find_if(recovery_tasks_.begin(), recovery_tasks_.end(),
                [](const std::pair<const std::string, RecoveryTasks>& t)->bool{
              return !t.second.empty();
            }) != recovery_tasks_.end();
      });
      for (auto it = recovery_tasks_.begin(); it != recovery_tasks_.end(); ++it) {
        if (!it->second.empty()) {
          VLOG(1) << "Recovered task for language:" << it->first
              << " crashing-tasks:" << it->second.suspected_fail_tasks.size()
              << " remaining-tasks:" << it->second.remaining_tasks.size()
              << " subprocesses-to-join:" << it->second.failed_queues.size();
          std::swap(it->second, task);
          language = it->first;
          recovery_tasks_.erase(it);
          break;
        }
      }
    }

    if (language.empty() || task.empty()) {
      // No task was there, but we are finishing.
      recovery_running_ = false;
      CHECK(joining_);
      LOG(INFO) << "Recovery thread done.";
      return;
    }


    VLOG(1) << "Recovered a task.";

    // Remove resources for the failed process.
    for (WorkerQueue* failed_queue : task.failed_queues) {
      VLOG(1) << "Waiting for process with failure to finish.";
      WaitQueueHasNoPendingSends(failed_queue);
      failed_queue->worker.WaitAndStop();
      std::lock_guard<std::mutex> guard(mutex_);
      failed_queues_.erase(failed_queue);
    }

    // Send remaining tasks as usual for parsing.
    for (int64_t task_id : task.remaining_tasks) {
      SendTask(language, task_id);
    }

    // Retry tasks that we suspect are the failure culprit (to check if they really are).
    // Run them in a separate process.
    for (int64_t task_id : task.suspected_fail_tasks) {
      VLOG(1) << "Retrying task " << task_id << "...";
      ParseWorker single_parser(get_command_cb_(language));
      bool parse_success = false;
      single_parser.Start([this,&parse_success](ParseResult&& result){
        parse_success = true;
        file_parsed_cb_(std::move(result));
      }, [](){});
      VLOG(2) << "Started suspected failed task worker " << single_parser.pid();
      ParseTask task;
      task.id = task_id;
      get_file_cb_(task_id, &task);
      single_parser.SendParseTask(std::move(task));
      single_parser.WaitAndStop();
      if (!parse_success) {
        ParseResult failure;
        failure.parse_error = "Internal parser failure.";
        file_parsed_cb_(std::move(failure));
      }
      VLOG(1) << "Retrying task " << task_id << " done.";
    }
  }
}
