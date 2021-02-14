/*
   Copyright 2017 Software Reliability Lab, ETH Zurich

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

#ifndef PARSERS_GITHUB_H
#define PARSERS_GITHUB_H

#include <string>

std::pair<std::string, std::string> GitHubCompareUrl(
    const std::string& repo_name,
    const std::string& parent_sha,
    const std::string& sha,
    const std::string& file_name,
    int line_number=-1
);

#endif //PARSERS_GITHUB_H
