/*
   Copyright 2015 Software Reliability Lab, ETH Zurich

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#ifndef BASE_FILEUTIL_H_
#define BASE_FILEUTIL_H_

#include <stdio.h>
#include <string>
#include <cerrno>
#include <cstring>

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#include "glog/logging.h"

void ReadFileToStringOrDie(const char* filename, std::string* r);
const std::string ReadFileToStringOrDie(const char* filename);
bool ReadFileToString(const char* filename, std::string* r);
void WriteStringToFileOrDie(const char* filename, const std::string& s);

// Writes string to a file and return true on success
bool WriteStringToFile(const char* filename, const std::string& s);
bool FileExists(const char* filename);
bool DirectoryExists(const char* dirname);
// Deletes a file and returns true on succes
bool DeleteFile(const char* filename);

// Creates a directory. Returns true if the directory exists or was successfully created.
bool CreateDirectoryRecursive(const char* dirname);

std::vector<std::string> FindFiles(const char* dirname, const std::string extension = "");

template <class Cb>
void ForEachFile(const char* dirname, const Cb& cb, const std::string extension = "") {
  for(const auto& p : fs::recursive_directory_iterator(dirname)) {
    if (!fs::is_regular_file(p)) continue;
    if (!extension.empty() && p.path().extension().compare(extension) != 0) continue;

    cb(p.path().c_str());
  }
}

std::string BaseName(const std::string& path);

// Creates a temporary file that is automatically deleted in the destructor.
class TempFile {
public:
  TempFile(const std::string& directory) {
    file_name_ = (char *) malloc(sizeof(char) * (directory.size() + 6 + 1 + 1));

    // create file name template
    strcpy(file_name_, directory.c_str());
    if (directory.empty() || directory.back() != '/') {
      strcat(file_name_, "/");
    }
    // will be replaced with the actual file name
    strcat(file_name_, "XXXXXX");

    int fd = mkstemp(file_name_);
    if (fd == -1) {
      LOG(ERROR) << "Unable to create temporary file: " << std::strerror(errno);
      free(file_name_);
      file_name_ = nullptr;
    } else {
      close(fd);
    }
  }

  ~TempFile() {
    if (file_name_ != nullptr) {
      std::remove(file_name_);
      free(file_name_);
    }
  }

  // May return nullptr if creating the temporary file has failed.
  const char* Name() const {
    return file_name_;
  }

private:
  char* file_name_;
};

#endif /* BASE_FILEUTIL_H_ */
