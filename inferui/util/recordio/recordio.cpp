/*
   Copyright 2017 DeepCode GmbH

   Author: Veselin Raychev
   Author: Pavol Bielik
 */

#include "recordio.h"

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "util/zstream/zstream.h"


#include "glog/logging.h"

RecordCompressedWriter::RecordCompressedWriter(const std::string& filename) {
  out_file_.reset(new zstr::ofstream(filename));
  out_stream_.reset(new google::protobuf::io::OstreamOutputStream(out_file_.get()));
  coded_stream_.reset(new google::protobuf::io::CodedOutputStream(out_stream_.get()));
}

RecordCompressedWriter::~RecordCompressedWriter() {
  if (out_file_) {
    Close();
  }
}

void RecordCompressedWriter::Write(const google::protobuf::MessageLite& message) {
  coded_stream_->WriteVarint32(message.ByteSize());
  CHECK(message.SerializeToCodedStream(coded_stream_.get()));
}

void RecordCompressedWriter::Close() {
  coded_stream_->WriteVarint32(-1);
  coded_stream_.reset(nullptr);
  out_stream_.reset(nullptr);
  out_file_.reset(nullptr);
}

RecordWriter::RecordWriter(const std::string& filename) : out_file_(filename) {
  CHECK(out_file_.is_open()) << "Could not create " << filename;
  out_stream_.reset(new google::protobuf::io::OstreamOutputStream(&out_file_));
  coded_stream_.reset(new google::protobuf::io::CodedOutputStream(out_stream_.get()));
}
RecordWriter::RecordWriter(const std::string& filename, bool append) : out_file_(filename, std::ios_base::app) {
  CHECK(out_file_.is_open()) << "Could not append to " << filename;
}
RecordWriter::~RecordWriter() {
  if (out_file_.is_open()) {
    Close();
  }
}

void RecordWriter::Write(const google::protobuf::MessageLite& message) {
  coded_stream_->WriteVarint32(message.ByteSize());
  CHECK(message.SerializeToCodedStream(coded_stream_.get()));
}

void RecordWriter::Flush() {
  out_file_.flush();
}
void RecordWriter::Close() {
  CHECK(out_file_.is_open());
  coded_stream_->WriteVarint32(-1);
  coded_stream_.reset(nullptr);
  out_stream_.reset(nullptr);
  out_file_.close();
}


RecordCompressedReader::RecordCompressedReader(const std::string& filename) {
  in_file_.reset(new zstr::ifstream(filename));
  in_stream_.reset(new google::protobuf::io::IstreamInputStream(in_file_.get()));
}

RecordCompressedReader::~RecordCompressedReader() {
  if (in_file_) {
    Close();
  }
}

void RecordCompressedReader::Close() {
  in_stream_.reset(nullptr);
  in_file_.reset(nullptr);
}

bool RecordCompressedReader::Read(google::protobuf::MessageLite* message) {
  google::protobuf::io::CodedInputStream in(in_stream_.get());
  unsigned int size;
  CHECK(in.ReadVarint32(&size));
  if (size == static_cast<unsigned int>(-1)) return false;
  CHECK_GE(static_cast<int>(size), 0);  // No integer overflow.

  google::protobuf::io::CodedInputStream::Limit limit = in.PushLimit(size);
  CHECK(message->ParseFromCodedStream(&in));
  in.PopLimit(limit);
  return true;
}

RecordReader::RecordReader(const std::string& filename) : in_file_(filename) {
  CHECK(in_file_.is_open()) << "Could not open " << filename;
  in_stream_.reset(new google::protobuf::io::IstreamInputStream(&in_file_));
}

RecordReader::~RecordReader() {
  if (in_file_.is_open()) {
    Close();
  }
}

void RecordReader::Close() {
  in_file_.close();
}

bool RecordReader::Read(google::protobuf::MessageLite* message) {
  google::protobuf::io::CodedInputStream in(in_stream_.get());
  unsigned int size;
  CHECK(in.ReadVarint32(&size));
  if (size == static_cast<unsigned int>(-1)) return false;
  CHECK_GE(static_cast<int>(size), 0);  // No integer overflow.

  google::protobuf::io::CodedInputStream::Limit limit = in.PushLimit(size);
  CHECK(message->ParseFromCodedStream(&in));
  in.PopLimit(limit);
  return true;
}
