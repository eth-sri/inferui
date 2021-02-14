/*
   Copyright 2017 DeepCode GmbH

   Author: Veselin Raychev
   Author: Pavol Bielik
 */

#ifndef UTIL_RECORDIO_RECORDIO_H_
#define UTIL_RECORDIO_RECORDIO_H_

#include <fstream>
#include <memory>

#include <google/protobuf/message_lite.h>
#include <mutex>

namespace google { namespace protobuf { namespace io {
class OstreamOutputStream;
class CodedOutputStream;
class IstreamInputStream;
}}}

class RecordCompressedWriter {
public:
  RecordCompressedWriter(const std::string& filename);
  ~RecordCompressedWriter();

  void Write(const google::protobuf::MessageLite& message);

  void Close();

private:
  std::unique_ptr<std::ostream> out_file_;
  std::unique_ptr<google::protobuf::io::OstreamOutputStream> out_stream_;
  std::unique_ptr<google::protobuf::io::CodedOutputStream> coded_stream_;
};

class RecordWriter {
public:
  RecordWriter(const std::string& filename);
  RecordWriter(const std::string& filename, bool append);
  ~RecordWriter();

  void Write(const google::protobuf::MessageLite& message);

  void Flush();
  void Close();
private:
  std::ofstream out_file_;
  std::unique_ptr<google::protobuf::io::OstreamOutputStream> out_stream_;
  std::unique_ptr<google::protobuf::io::CodedOutputStream> coded_stream_;
};


class RecordCompressedReader {
public:
  RecordCompressedReader(const std::string& filename);
  ~RecordCompressedReader();

  bool Read(google::protobuf::MessageLite* message);

  void Close();

private:
  std::unique_ptr<std::istream> in_file_;
  std::unique_ptr<google::protobuf::io::IstreamInputStream> in_stream_;
};

class RecordReader {
public:
  RecordReader(const std::string& filename);
  ~RecordReader();

  bool Read(google::protobuf::MessageLite* message);

  void Close();

private:
  std::ifstream in_file_;
  std::unique_ptr<google::protobuf::io::IstreamInputStream> in_stream_;
};

template <class Record>
class BufferedRecordWriter {
public:
  BufferedRecordWriter(const std::string& file_name) : file_name_(file_name) {
  }

  void Write(const Record& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    records_.push_back(value);
  }

  void Close() {
    RecordCompressedWriter writer(file_name_);
    for (const Record& record : records_) {
      writer.Write(record);
    }
    writer.Close();
  }

private:
  std::mutex mutex_;
  std::vector<Record> records_;
  const std::string file_name_;
};

template <class Record>
std::vector<Record> ReadIntoVector(const char* file_name) {
  std::vector<Record> results;
  RecordCompressedReader reader(file_name);
  Record report;
  while(reader.Read(&report)) {
    results.push_back(report);
  }
  reader.Close();
  return results;
}

template <class Record, class Callback>
void ForEachRecord(const char* file_name, const Callback& cb) {
  RecordCompressedReader reader(file_name);
  Record report;
  while(reader.Read(&report)) {
    if (!cb(report)) break;
  }
  reader.Close();
}


#endif /* UTIL_RECORDIO_RECORDIO_H_ */
