/*
 Copyright 2017 DeepCode GmbH

 Author: Veselin Raychev
 */

#ifndef UTIL_PROCESS_SUBPROCESS_H_
#define UTIL_PROCESS_SUBPROCESS_H_

#include <functional>
#include <string>
#include <vector>
#include <thread>

#include <stdio.h>

// Implements a line-by-line reader from a file descriptor.
class FDLineReader {
public:
  explicit FDLineReader(int fd);
  bool ReadLine(std::string* s);
  ~FDLineReader();

private:
  FILE* f_;
};

// Implements a subprocess with redirected stdin, stdout and stderr.
class Subprocess {
public:
  explicit Subprocess(const std::vector<std::string>& cmd_and_params);
  explicit Subprocess(std::vector<std::string>&& cmd_and_params);
  ~Subprocess();

  void Start(
      const std::function<void(int fd)>& stdout_reader,
      const std::function<void(int fd)>& stderr_reader);

  // Writer to stdin.
  bool WriteLine(const char* data);
  bool Write(const char* data, size_t size);
  void Flush();
  void Close();

  // Sets a memory limit for the child process. Must be called before Start.
  void SetMemoryLimit(int64_t memory_limit) {
    memory_limit_ = memory_limit;
  }

  int pid() const { return pid_; }

private:
  std::vector<std::string> cmd_and_params_;
  FILE* stdin_file_;
  std::vector<std::thread> reader_threads_;
  int pid_;
  int64_t memory_limit_;
};



#endif /* UTIL_PROCESS_SUBPROCESS_H_ */
