/*
 Copyright 2017 DeepCode GmbH

 Author: Veselin Raychev
 */

#include "subprocess.h"

#include <unistd.h>
#include <errno.h>
#include <wait.h>
#include <sys/resource.h>

#include <fcntl.h>

#include "glog/logging.h"


FDLineReader::FDLineReader(int fd) {
  f_ = fdopen(fd, "rt");
}

FDLineReader::~FDLineReader() {
  fclose(f_);
}

bool FDLineReader::ReadLine(std::string* s) {
  s->clear();
  for (;;) {
    char buf[4096];
    if (fgets(buf, 4096, f_) == nullptr) return false;
    s->append(buf);
    if (s->size() == 0) return false;
    if ((*s)[s->size() - 1] == '\n') break;
  }
  s->pop_back();
  return true;
}


class LinuxPipe {
public:
  LinuxPipe() : fd_{-1, -1} {
    if (pipe(fd_) < 0) {
      LOG(FATAL) << "Could not create pipe. " << strerror(errno);
    }
  }
  ~LinuxPipe() {
    Close();
  }
  int reader() const { return fd_[0]; }
  int writer() const { return fd_[1]; }
  void CloseReader() {
    if (fd_[0] != -1) {
      close(fd_[0]);
      fd_[0] = -1;
    }
  }
  void CloseWriter()  {
    if (fd_[1] != -1) {
      close(fd_[1]);
      fd_[1] = -1;
    }
  }
  int DisownReader() {
    int result = fd_[0];
    fd_[0] = -1;
    return result;
  }
  int DisownWriter() {
    int result = fd_[1];
    fd_[1] = -1;
    return result;
  }
  void Close() { CloseReader(); CloseWriter(); }
private:
  int fd_[2];
};

Subprocess::Subprocess(const std::vector<std::string>& cmd_and_params)
    : cmd_and_params_(cmd_and_params), stdin_file_(nullptr),
      pid_(-1), memory_limit_(-1) {
}
Subprocess::Subprocess(std::vector<std::string>&& cmd_and_params)
    : cmd_and_params_(std::move(cmd_and_params)), stdin_file_(nullptr),
      pid_(-1), memory_limit_(-1) {
}

Subprocess::~Subprocess() {
  int status;
  VLOG(2) << "Waiting for " << pid_ << " to finish.";
  if (waitpid(pid_, &status, 0) < 0) {
    VLOG(2) << "Error in waitpid. " << strerror(errno);
  }
  VLOG(2) << "Process " << pid_ << " done.";

  for (size_t i = 0; i < reader_threads_.size(); ++i) {
    reader_threads_[i].join();
  }
}

namespace {
  void Redirect(int pipe_fd, int fd) {
    if (dup2(pipe_fd, fd) == -1) {
      LOG(FATAL) << "Could not redirect. " << strerror(errno);
    }
  }

  void MakeFDNotInheritedByChildren(int fd) {
    fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
  }
}

void Subprocess::Start(
    const std::function<void(int)>& stdout_reader,
    const std::function<void(int)>& stderr_reader) {
  signal(SIGPIPE, SIG_IGN);
  std::vector<char*> params(cmd_and_params_.size() + 2, nullptr);
  for (size_t i = 0; i < cmd_and_params_.size(); ++i) {
    params[i] = const_cast<char*>(cmd_and_params_[i].c_str());
  }
  LinuxPipe stdin_pipe, stdout_pipe, stderr_pipe;
  int child_id = fork();
  if (child_id < 0) {
    LOG(FATAL) << "Could not fork. " << strerror(errno);
  }
  if (child_id == 0) {
    // Child process.
    Redirect(stdin_pipe.reader(), STDIN_FILENO);
    Redirect(stdout_pipe.writer(), STDOUT_FILENO);
//    Redirect(stderr_pipe.writer(), STDERR_FILENO);
    stdin_pipe.Close();
    stdout_pipe.Close();
//    stderr_pipe.Close();
    if (memory_limit_ != -1) {
      struct rlimit64 limits;
      limits.rlim_cur = memory_limit_;
      limits.rlim_max = memory_limit_;
      setrlimit64(RLIMIT_DATA, &limits);
    }
    int rv = execv(cmd_and_params_[0].c_str(), params.data());
    if (write(STDERR_FILENO, "Error in exec\n", 15) == 15) {
      sleep(1);
    }
    exit(rv);
  }
  // Parent process.
  pid_ = child_id;
  stdin_pipe.CloseReader();
  stdout_pipe.CloseWriter();
//  stderr_pipe.CloseWriter();

  MakeFDNotInheritedByChildren(stdin_pipe.writer());  // This ensures that once we close the stdin, the child will see it as eof.
  MakeFDNotInheritedByChildren(stdout_pipe.reader());
//  MakeFDNotInheritedByChildren(stderr_pipe.reader());

  stdin_file_ = fdopen(stdin_pipe.DisownWriter(), "wt");
  int stdout_fd = stdout_pipe.DisownReader();
  reader_threads_.emplace_back([child_id,stdout_fd,stdout_reader]{
    // capture the callback by copy as it may be executed after the method exits.
    stdout_reader(stdout_fd);
    VLOG(2) << "Reading stdout done for pid " << child_id;
  });
//  int stderr_fd = stderr_pipe.DisownReader();
//  reader_threads_.emplace_back([child_id,stderr_fd,stderr_reader]{
// //     capture the callback by copy as it may be executed after the method exits.
//    stderr_reader(stderr_fd);
//    VLOG(2) << "Reading stderr done for pid " << child_id;
//  });

}

bool Subprocess::WriteLine(const char* data) {
  if (stdin_file_ == nullptr) return false;
  return fprintf(stdin_file_, "%s\n", data) > 0;
}
bool Subprocess::Write(const char* data, size_t size) {
  if (stdin_file_ == nullptr) return false;
  return fwrite(data, 1, size, stdin_file_) == size;
}
void Subprocess::Flush() {
  fflush(stdin_file_);
}
void Subprocess::Close() {
  fclose(stdin_file_);
  stdin_file_ = nullptr;
  VLOG(2) << "Closed stdin for process " << pid_ << " expecting the process will end.";
}

