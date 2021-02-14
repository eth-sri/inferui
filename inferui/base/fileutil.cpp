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

#include "base/fileutil.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

void ReadFileToStringOrDie(const char* filename, std::string* r) {
  r->clear();
  FILE* f = fopen(filename, "rt");
  CHECK(f != NULL) << "Could not open " << filename << " for reading.";
  char buf[4096];
  while (!feof(f)) {
    if (fgets(buf, 4096, f) == buf) {
      r->append(buf);
    } else {
      if (!feof(f))
        LOG(FATAL) << "Read error from " << filename;
    }
  }
  fclose(f);
}

const std::string ReadFileToStringOrDie(const char* filename) {
  std::string result;
  ReadFileToStringOrDie(filename, &result);
  return result;
}

bool ReadFileToString(const char* filename, std::string* r) {
  r->clear();
  FILE* f = fopen(filename, "rt");
  if (f == NULL) return false;
  char buf[4096];
  while (!feof(f)) {
    if (fgets(buf, 4096, f) == buf) {
      r->append(buf);
    } else {
      if (!feof(f)) {
        return false;
      }
    }
  }
  fclose(f);
  return true;
}

void WriteStringToFileOrDie(const char* filename, const std::string& s) {
  FILE* f = fopen(filename, "wt");
  CHECK(f != NULL) << "Could not open " << filename << " for writing.";
  fprintf(f, "%s", s.c_str());
  fclose(f);
}

bool WriteStringToFile(const char* filename, const std::string& s) {
  FILE* f = fopen(filename, "wt");
  if (f == NULL) {
    return false;
  }
  fputs(s.c_str(), f);
  int ret = fclose(f);
  return (ret != EOF);
}

bool FileExists(const char* filename) {
  struct stat stat_info;
  return stat(filename, &stat_info) == 0 && !S_ISDIR(stat_info.st_mode);
}

bool DirectoryExists(const char* dirname) {
  struct stat stat_info;
  return stat(dirname, &stat_info) == 0 && S_ISDIR(stat_info.st_mode);
}

bool CreateDirectoryRecursive(const char* dir_name) {
  std::string path(dir_name);
  for (size_t i = 1; i < path.size(); ++i) {
    if (path[i] == '/') {
      path[i] = 0;
      if (mkdir(path.c_str(), S_IRWXU) != 0) {
        if (errno != EEXIST) return false;
      }
      path[i] = '/';
    }
  }

  if (mkdir(path.c_str(), S_IRWXU) != 0) {
    if (errno != EEXIST) return false;
  }
  return true;
}


bool DeleteFile(const char* filename) {
  return unlink(filename) == 0;
}

std::vector<std::string> FindFiles(const char* dirname, const std::string extension) {
  std::vector<std::string> files;
  for(const auto& p : fs::recursive_directory_iterator(dirname)) {
    if (!fs::is_regular_file(p)) continue;
    if (!extension.empty() && p.path().extension().compare(extension) != 0) continue;

    files.push_back(p.path().c_str());
  }
  return files;
}

std::string BaseName(const std::string& path) {
  return path.substr(path.find_last_of("/\\") + 1);
}