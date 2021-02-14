#include "base/github.h"
#include "base/stringprintf.h"

#include <openssl/md5.h>
#include <sstream>
#include <iomanip>

std::pair<std::string, std::string> GitHubCompareUrl(
    const std::string& repo_name,
    const std::string& parent_sha,
    const std::string& sha,
    const std::string& file_name,
    int line_number) {

  unsigned char digest[MD5_DIGEST_LENGTH];
  MD5((unsigned char*)file_name.data(), file_name.size(), (unsigned char*)&digest);

  std::ostringstream sout;
  sout << std::hex << std::setfill('0');
  for(long long c: digest) {
    sout << std::setw(2) << (long long) c;
  }

  std::string base_url = StringPrintf("https://github.com/%s/compare/%s...%s",
                      repo_name.c_str(),
                      sha.c_str(),
                      parent_sha.c_str());

  return std::make_pair(
      StringPrintf("%s#files_bucket", base_url.c_str()),
      StringPrintf("%s#diff-%s%s", base_url.c_str(), sout.str().c_str(), ((line_number == -1) ? "" : StringPrintf("L%d", line_number)).c_str())
  );
}