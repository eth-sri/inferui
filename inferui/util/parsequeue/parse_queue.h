/*
 Copyright 2017 DeepCode GmbH

 Author: Veselin Raychev
 */

#ifndef PARSING_PARSE_QUEUE_H_
#define PARSING_PARSE_QUEUE_H_

#include <memory>
#include <string>
#include <vector>
#include <queue>
#include <map>

#include "json/json.h"

#include "base/stringset.h"

#include "util/process/subprocess.h"
#include "util/thread/work_queue.h"


struct ParseTask {
  int64_t id;
  std::map<std::string, std::string> attributes;
  std::string code;
};

struct ParseResult {
  Json::Value json_response;
  std::string parse_error;
};

// A single parser process. A process can accept multiple inputs.
class ParseWorker {
public:
  ParseWorker(
          const std::vector<std::string>& cmd_and_params);
  ParseWorker(const ParseWorker&) = delete;
  // Move constructor is deleted, because this object is passed to async callbacks.
  ParseWorker(ParseWorker&&) = delete;

  void Start(
          const std::function<void(ParseResult&&)>& parse_result_cb,
          const std::function<void()>& done_cb);

  void WaitAndStop();

  bool SendParseTask(ParseTask&& task);

  void Flush();

  void Close() {
    subprocess_->Close();
  }
  void Stop() {
    subprocess_.reset(nullptr);
  }

  int pid() { return subprocess_->pid(); }

private:
  std::unique_ptr<Subprocess> subprocess_;
};

// A queue that includes a number of worker processes for different programming
// languages and is able to retry in case a parser process fails.
class ParseQueue {
public:
  typedef std::function<void(int64_t, ParseTask*)> GetFileCodeCB;
  typedef std::function<void(ParseResult&&)> FileParsedCB;
  typedef std::function<std::vector<std::string>(const std::string&)> GetCommandCB;

  explicit ParseQueue(
          int max_workers_per_language,
          const GetFileCodeCB&,
          const FileParsedCB&,
          const GetCommandCB&);
  ParseQueue(const ParseQueue&) = delete;
  // Move constructor is deleted, because this object is passed to async callbacks.
  ParseQueue(ParseQueue&&) = delete;

  void Join();

  int64_t AllocateTaskId();
  void SendTask(const std::string& language, int64_t task_id);

private:
  int max_workers_per_language_;
  GetFileCodeCB get_file_cb_;
  FileParsedCB file_parsed_cb_;
  GetCommandCB get_command_cb_;
  int64_t current_id_;
  bool joining_;
  bool recovery_running_;
  std::mutex mutex_;

  struct TaskStatus {
    std::string language;
    int64_t time_sent;
  };

  struct WorkerQueue {
    explicit WorkerQueue(
            const std::vector<std::string>& cmd_and_params)
            : is_valid(true), worker(cmd_and_params), num_received(0), num_pending_sends(0) {
    }

    bool is_valid;
    ParseWorker worker;
    std::queue<int64_t> sent_tasks;
    int num_received;

    int num_pending_sends;
    std::mutex send_mutex;
    std::condition_variable pending_sends;
  };

  struct LanguageWorkerSet {
    LanguageWorkerSet() : current_id(0) {}
    int current_id;
    std::vector<std::unique_ptr<WorkerQueue>> queues;
  };

  // Requires that the workers->mutex is help locked.
  WorkerQueue* GetQueue(
          const std::string& language,
          LanguageWorkerSet* workers);

  LanguageWorkerSet* GetWorkers(const std::string& language);

  void WaitQueueHasNoPendingSends(WorkerQueue* queue);

  std::map<int64_t, TaskStatus> tasks_;
  std::map<std::string, std::unique_ptr<LanguageWorkerSet>> workers_;
  std::map<WorkerQueue*, std::unique_ptr<WorkerQueue>> failed_queues_;
  std::mutex workers_mutex_;

  // Error recovery. This is thread that deletes failed workers and
  // resubmits tasks ot other workers.
  void StartErrorRecoveryThreadIfNotRunning();

  WorkerQueue* GetWorkerItemsForRecovery(
          const std::string& language,
          WorkerQueue* queue,
          std::queue<int64_t>* pending_tasks);

  void LeaveWorkerItemsForRecovery(
          const std::string& language,
          std::queue<int64_t>&& pending_tasks,
          WorkerQueue* queue_that_should_be_deleted);

  void RecoveryThread();

  struct RecoveryTasks {
    RecoveryTasks() {}
    std::vector<WorkerQueue*> failed_queues;
    std::vector<int64_t> suspected_fail_tasks;
    std::vector<int64_t> remaining_tasks;

    bool empty() const {
      return failed_queues.empty() && suspected_fail_tasks.empty() && remaining_tasks.empty();
    }
  };

  std::map<std::string, RecoveryTasks> recovery_tasks_;
  std::vector<std::thread> recovery_threads_;
  std::condition_variable recovery_tasks_ready_;

  bool JoinWorkers();
  bool JoinRecovery();
};


#endif /* PARSING_PARSE_QUEUE_H_ */
