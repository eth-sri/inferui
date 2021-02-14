/*
   Copyright 2018 Software Reliability Lab, ETH Zurich
 */

#include <stdlib.h>
#include "glog/logging.h"

#include "json/json.h"
#include "json/server.h"
#include "json/server_connectors_httpserver.h"
#include "json/common_exception.h"
#include "json/client.h"
#include "json/client_connectors_httpclient.h"


#include "base/stringprintf.h"
#include "base/stringset.h"
#include "inferui/model/uidump.pb.h"
#include "util/recordio/recordio.h"
#include "inferui/model/model.h"
#include "base/fileutil.h"
#include "inferui/eval/eval_util.h"

DEFINE_int32(server_port, 9017, "Port of the server.");
DEFINE_string(server_host, "", "If client, this gives url (i.e. http://host:port/ ) of the server.");

/************************* Server ***************************/

enum ERROR_CODES {
  INPUT_ERROR = 1,
  SYNTHESIS_ERROR,

};

class SynthesisServer : public jsonrpc::AbstractServer<SynthesisServer> {
public:

  SynthesisServer(jsonrpc::HttpServer* server) : jsonrpc::AbstractServer<SynthesisServer>(*server), syn(true) {
    bindAndAddMethod(
        jsonrpc::Procedure("layout", jsonrpc::PARAMS_BY_NAME, jsonrpc::JSON_ARRAY,
            // Parameters:
                           "id", jsonrpc::JSON_INTEGER,
                           "ref_device", jsonrpc::JSON_OBJECT,
                           "devices", jsonrpc::JSON_ARRAY,
                           "layout", jsonrpc::JSON_OBJECT,
                           NULL),
        &SynthesisServer::layout);


  }

  Device parseDevice(const Json::Value& obj) {
    int width = obj["width"].asInt();
    int height = obj["height"].asInt();
    return Device(width, height);
  }

  void ASSERT(bool value, ERROR_CODES code, const std::string& message) {
    if (!value) {
      throw jsonrpc::JsonRpcException(static_cast<int>(code), message, Json::Value(Json::nullValue));
    }
  }

  void ASSERT(bool value, ERROR_CODES code, const std::string& message, const Json::Value& data) {
    if (!value) {
      throw jsonrpc::JsonRpcException(static_cast<int>(code), message, data);
    }
  }

  View JsonToView(const Json::Value& value, std::map<std::string, int>& ids) {
    if (!value.isMember("attributes")) {
      ids.insert(std::make_pair("parent", 0));
      return View(value["location"][0].asInt(), value["location"][1].asInt(),
                  value["location"][2].asInt() + value["location"][0].asInt(),
                  value["location"][3].asInt() + value["location"][1].asInt(),
                  "parent", 0);
    } else {
      const Json::Value &attributes = value["attributes"];
      ASSERT(value.isMember("type"), ERROR_CODES::INPUT_ERROR, "Expects each component to declare its type", value);
      const std::string type = value["type"].asString();
      const std::string& id_string = attributes[Constants::name(Constants::Name::ID, Constants::Type::INPUT_XML)].asString();
      if (!Contains(ids, id_string)) {
        ids.insert(std::make_pair(id_string, ids.size()));
      }
      int id_val = FindWithDefault(ids, id_string, -1);
      View view = View(value["location"][0].asInt(), value["location"][1].asInt(),
                  value["location"][2].asInt() + value["location"][0].asInt(),
                  value["location"][3].asInt() + value["location"][1].asInt(),
                  type, id_val, id_string);

      ASSERT(attributes.isMember(Constants::name(Constants::Name::layout_width, Constants::Type::INPUT_XML)), ERROR_CODES::INPUT_ERROR, "Expects each component to define its width", value);
      ASSERT(attributes.isMember(Constants::name(Constants::Name::layout_height, Constants::Type::INPUT_XML)), ERROR_CODES::INPUT_ERROR, "Expects each component to define its height", value);
      view.view_size[Orientation::HORIZONTAL] = GetViewSize(attributes[Constants::name(Constants::Name::layout_width, Constants::Type::INPUT_XML)].asString());
      view.view_size[Orientation::VERTICAL] = GetViewSize(attributes[Constants::name(Constants::Name::layout_height, Constants::Type::INPUT_XML)].asString());

      return view;
    }
  }

  App JsonToApp(const Json::Value& layout, std::map<std::string, int>& ids) {
    App app;
    app.AddView(JsonToView(layout["content_frame"], ids));
    for (auto component : layout["components"]) {
      app.AddView(JsonToView(component, ids));
    }
    return app;
  }

  void layout(const Json::Value& request, Json::Value& response) {
    LOG(INFO) << request;
    std::map<std::string, int> ids;
    App app = JsonToApp(request["layout"], ids);
    for (const View& view : app.GetViews()) {
      LOG(INFO) << view;
    }

    Device ref_device = parseDevice(request["ref_device"]);
    std::vector<Device> devices;
    for (auto device : request["devices"]) {
      devices.push_back(parseDevice(device));
    }
    syn.SetDevice(ref_device, devices);
    app.SetResizable(ref_device, devices);

    SynResult res = syn.Synthesize(std::move(app));
    LOG(INFO) << res.status;

    ASSERT(res.status == Status::SUCCESS, ERROR_CODES::SYNTHESIS_ERROR, StringPrintf("Synthesis Unsuccesfull: %s", StatusStr(res.status).c_str()));

    Json::Value layout(Json::arrayValue);
    for (size_t i = 1; i < res.app.GetViews().size(); i++) {
      const View& view = res.app.GetViews()[i];
      Json::Value view_json = view.ToJSON(Constants::INPUT_XML);

      const Json::Value &attributes = request["layout"]["components"][view.id - 1]["attributes"];
      for (const auto& name : {Constants::Name::layout_width, Constants::Name::layout_height}) {
        ASSERT(GetViewSize(view_json[Constants::name(name, Constants::Type::INPUT_XML)].asString()) == GetViewSize(attributes[Constants::name(name, Constants::Type::INPUT_XML)].asString()),
               ERROR_CODES::SYNTHESIS_ERROR, "Inconsistent view size");

        //TODO: check that the input is consistent with the supplied view size
        view_json[Constants::name(name, Constants::Type::INPUT_XML)] = attributes[Constants::name(name, Constants::Type::INPUT_XML)];
      }

      layout.append(view_json);
    }
    response["layout"] = layout;
    LOG(INFO) << response;
  }

private:
  GenSmtMultiDeviceProbOpt syn;
};

void RunServer() {
  LOG(INFO) << "Starting server on port: " << FLAGS_server_port;
  jsonrpc::HttpServer jsonhttp(FLAGS_server_port, "", "", 1);  // Single-threaded.
  SynthesisServer server(&jsonhttp);

  server.StartListening();

  for (;;) sleep(1);
  server.StopListening();
}

/************************* Client ***************************/

int main(int argc, char** argv) {
  google::InstallFailureSignalHandler();
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  RunServer();

  return 0;
}

