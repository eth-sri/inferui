/*
   Copyright 2018 Software Reliability Lab, ETH Zurich

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


#ifndef CC_SYNTHESIS_SOLVER_H
#define CC_SYNTHESIS_SOLVER_H

#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <glog/logging.h>
#include "json/json.h"


class Solver {

public:
  Solver() : json_reader(Json::CharReaderBuilder().newCharReader()) {
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
  }

  ~Solver() {
    /* we're done with libcurl, so clean it up */
    curl_easy_cleanup(curl);
    curl_global_cleanup();
  }

  Json::Value parseJson(const std::string s) const {
    Json::Value json_response;
    std::string errors;
    if (!json_reader->parse(s.c_str(), s.c_str() + s.size(), &json_response, &errors)) {
      LOG(FATAL) << "Invalid JSON response from parser " << errors;
    }
    return json_response;
  }

  Json::Value sendPost(const Json::Value& data) {
    Json::FastWriter fastWriter;
    return sendPost(fastWriter.write(data), "localhost:9100/layout", false);
  }

  Json::Value sendPostToOracle(const Json::Value& data) {
  	  Json::FastWriter fastWriter;
  	  return sendPost(fastWriter.write(data), "localhost:4446/predict", true);
  }

  Json::Value sendPostToTransformator(const Json::Value& data) {
	  LOG(INFO) << "sendPostToTransformator";
  	  Json::FastWriter fastWriter;
  	  return sendPost(fastWriter.write(data), "localhost:4242/predict", true);
  }

  Json::Value sendPostToVisualizer(const Json::Value& data) {
  	  Json::FastWriter fastWriter;
  	  return sendPost(fastWriter.write(data), "localhost:4446/visualize", true);
  }

  // Adapted code from https://curl.haxx.se/libcurl/c/postinmemory.html
  Json::Value sendPost(const std::string& data, const std::string& server, bool json_header) {
    CHECK(curl);
    CURLcode res;
    struct MemoryStruct chunk;

    chunk.memory = (char *) malloc(1);  /* will be grown as needed by realloc above */
    chunk.size = 0;    /* no data at this point */

	struct curl_slist *hs=NULL;
	hs = curl_slist_append(hs, "Content-Type: application/json");
	if(json_header){
    	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
    }
	curl_easy_setopt(curl, CURLOPT_URL, server.c_str());

    /* send all data to this function  */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

    /* we pass our 'chunk' struct to the callback function */
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    /* some servers don't like requests that are made without a user-agent
       field, so we provide one */
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());

    /* if we don't provide POSTFIELDSIZE, libcurl will strlen() by
       itself */
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size());

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);
    /* Check for errors */
    std::string response;
    if(res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
      response = "";
    }
    else {
      /*
       * Now, our chunk.memory points to a memory block that is chunk.size
       * bytes big and contains the response.
       */
      response = std::string(chunk.memory);
    }

    free(chunk.memory);
    return parseJson(response);
  }

private:
  struct MemoryStruct {
    char *memory;
    size_t size;
  };

  CURL *curl;

  std::unique_ptr<Json::CharReader> json_reader;

  static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = (char *) realloc(mem->memory, mem->size + realsize + 1);
    if(mem->memory == NULL) {
      /* out of memory! */
      printf("not enough memory (realloc returned NULL)\n");
      return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
  }
};


#endif //CC_SYNTHESIS_SOLVER_H
