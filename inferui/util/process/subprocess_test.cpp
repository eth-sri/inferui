/*
 Copyright 2017 DeepCode GmbH

 Author: Veselin Raychev
 */

#include "subprocess.h"

#include "glog/logging.h"
#include "external/gtest/googletest/include/gtest/gtest.h"

#include "base/stringprintf.h"

TEST(SubprocessTest, GrepTest) {
  std::string stdoutput;
  {
    Subprocess p(std::vector<std::string>{"/bin/grep", "-P", "2|4|5"});
    p.Start([&stdoutput](int fd){
      FDLineReader r(fd);
      std::string s;
      while (r.ReadLine(&s)) {
        stdoutput += s + ";";
      }
    }, [](int fd){
      FDLineReader r(fd);
      std::string s;
      while (r.ReadLine(&s)) {
        LOG(FATAL) << "Got line from stderr: " << s;
      }
    });
    p.WriteLine("Line 1");
    p.WriteLine("Line 2");
    p.WriteLine("Line 3");
    p.WriteLine("Line 4");
    p.WriteLine("Line 5");
    p.Close();
  }
  EXPECT_EQ("Line 2;Line 4;Line 5;", stdoutput);
}

TEST(SubprocessTest, GrepTestFlush) {
  std::string stdoutput;
  {
    Subprocess p(std::vector<std::string>{"/bin/grep", "-P", "22|48|9"});
    p.Start([&stdoutput](int fd){
      FDLineReader r(fd);
      std::string s;
      while (r.ReadLine(&s)) {
        stdoutput += s + ";";
      }
    }, [](int fd){
      FDLineReader r(fd);
      std::string s;
      while (r.ReadLine(&s)) {
        LOG(FATAL) << "Got line from stderr: " << s;
      }
    });
    for (int i = 0; i < 80; ++i) {
      p.WriteLine(StringPrintf("Line %d\n", i).c_str());
      if (i % 10 == 0) p.Flush();
    }
    p.Close();
  }
  EXPECT_EQ("Line 9;Line 19;Line 22;Line 29;Line 39;Line 48;Line 49;Line 59;Line 69;Line 79;",
      stdoutput);
}


int main(int argc, char **argv) {
  google::InstallFailureSignalHandler();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
