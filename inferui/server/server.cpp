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
#include "inferui/eval/eval_app_util.h"

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

DEFINE_int32(server_port, 9005, "Port of the server.");
DEFINE_string(server_host, "", "If client, this gives url (i.e. http://host:port/ ) of the server.");

DEFINE_string(data, "uidumps.proto", "File with app data.");


/************************* Server ***************************/

class SynthesisServer : public jsonrpc::AbstractServer<SynthesisServer> {
public:
  std::vector<std::unique_ptr<Synthesizer>> synthesizers;

  SynthesisServer(jsonrpc::HttpServer* server) : jsonrpc::AbstractServer<SynthesisServer>(*server) {
    bindAndAddMethod(
        jsonrpc::Procedure("layout", jsonrpc::PARAMS_BY_NAME, jsonrpc::JSON_ARRAY,
            // Parameters:
                           "id", jsonrpc::JSON_INTEGER,
                           "data", jsonrpc::JSON_ARRAY,
                           NULL),
        &SynthesisServer::layout);

    bindAndAddMethod(
        jsonrpc::Procedure("screenshots", jsonrpc::PARAMS_BY_NAME, jsonrpc::JSON_ARRAY,
            // Parameters:
                           NULL),
        &SynthesisServer::screenshots);

    bindAndAddMethod(
        jsonrpc::Procedure("apps", jsonrpc::PARAMS_BY_NAME, jsonrpc::JSON_ARRAY,
            // Parameters:
                           "id", jsonrpc::JSON_INTEGER,
                           NULL),
        &SynthesisServer::apps);

    bindAndAddMethod(
        jsonrpc::Procedure("dataset", jsonrpc::PARAMS_BY_NAME, jsonrpc::JSON_ARRAY,
            // Parameters:
                           "devices", jsonrpc::JSON_ARRAY,
                           NULL),
        &SynthesisServer::dataset);

    bindAndAddMethod(
        jsonrpc::Procedure("analyze_app", jsonrpc::PARAMS_BY_NAME, jsonrpc::JSON_ARRAY,
            // Parameters:
                           "id", jsonrpc::JSON_INTEGER,
                           "devices", jsonrpc::JSON_ARRAY,
                           NULL),
        &SynthesisServer::analyze_app);

    synthesizers.emplace_back(std::unique_ptr<GenUserConstraints>(new GenUserConstraints()));
//    synthesizers.emplace_back(std::unique_ptr<GenSmtSingleDeviceProbOpt>(new GenSmtSingleDeviceProbOpt(true)));
//    synthesizers.emplace_back(std::unique_ptr<GenSmtMultiDeviceProbOpt>(new GenSmtMultiDeviceProbOpt(true)));
//    synthesizers.emplace_back(std::unique_ptr<GenProbSynthesis>(new GenProbSynthesis()));

    only_constraint_views = true;

    Solver solver;
    LOG(INFO) << "Loading apps to serve...";
    ForEachValidApp(FLAGS_data.c_str(), [&](const ProtoApp& app) {
      const ProtoScreen& screen = app.screens(0);

      App syn_app(screen, only_constraint_views);
      if (syn_app.GetViews().size() <= 2) return;
//      if (syn_app.GetViews().size() > 20) return;

//      App screen_app(screen, true);
//      syn_app.InitializeAttributes(screen);
//      Json::Value layout = solver.sendPost(syn_app.ToJSON());
//      App rendered_app = JsonToApp(layout);
//
//      if (AppMatch(syn_app, rendered_app)) {
//        return;
//      }
      apps_.push_back(app);
      return;
    });
    LOG(INFO) << "Done. Loaded " << apps_.size() << " apps.";
  }

  Json::Value analyzeApp(const ProtoApp& app, const std::vector<Device>& devices) {
    Json::Value res = Json::Value(Json::objectValue);

    const ProtoScreen& screen = app.screens(0);
    App syn_app(screen, true);
    syn_app.InitializeAttributes(screen);

    res["name"] = app.package_name();
    {
      fs::path img_path = fs::path(screen.window_path());
      res["file"] = StringPrintf("%s.xml", GetBaseFilenameFromImage(img_path.filename().string()).c_str());
    }
    res["size"] = static_cast<int>(syn_app.GetViews().size());

    Json::Value properties = Json::Value(Json::objectValue);
    for (const auto& it : CheckProperties(syn_app, Device(720, 1280), devices)) {
      properties[it.first] = it.second;
    }
    res["properties"] = properties;

    return res;
  }

  void analyze_app(const Json::Value& request, Json::Value& response) {
    std::vector<Device> devices;
    for (const Json::Value &device : request["devices"]) {
      devices.push_back(Device(device[0].asInt(), device[1].asInt()));
    }

    response = analyzeApp(apps_[request["id"].asInt()], devices);
  }

  void dataset(const Json::Value& request, Json::Value& response) {
    std::vector<Device> devices;
    for (const Json::Value& device : request["devices"]) {
      devices.push_back(Device(device[0].asInt(), device[1].asInt()));
    }

    for (const ProtoApp& app : apps_) {
      response.append(analyzeApp(app, devices));
//      if (response.size() > 50) break;
    }
  }

  void screenshots(const Json::Value& request, Json::Value& response) {
    LOG(INFO) << "screenshots";
    for (const ProtoApp& app : apps_) {
      Json::Value entry = Json::Value(Json::objectValue);
      const ProtoScreen& screen = app.screens(0);
      entry["path"] = StringPrintf("%s", screen.window_path().substr(std::string("/home/pavol/ETH/data/").size()).c_str());
//      LOG(INFO) << entry["path"];
      entry["name"] = app.package_name();
      response.append(entry);
    }
  }

  void layout(const Json::Value& request, Json::Value& response) {
    LOG(INFO) << "layout";
    LOG(INFO) << request["id"].asInt();
    LOG(INFO) << "data size: " << request["data"].size();
    for (const Json::Value& entry : request["data"]) {
      LOG(INFO) << "value";
      LOG(INFO) << entry.toStyledString();
    }
  }

  Json::Value ViewToJson(const View& view) {
    Json::Value entry = Json::Value(Json::objectValue);
    entry["name"] = view.name;
    Json::Value location = Json::Value(Json::arrayValue);
    location.append(view.xleft);
    location.append(view.ytop);
    location.append(view.width());
    location.append(view.height());
    entry["location"] = location;
    return entry;
  }

  std::string GetBaseFilenameFromImage(const std::string& name) const {
    std::vector<std::string> parts;
    SplitStringUsing(name, '.', &parts);
    return parts[0];
  }

  void apps(const Json::Value& request, Json::Value& response) {
    LOG(INFO) << "apps";
    LOG(INFO) << request["id"].asInt();
    int id = request["id"].asInt();
    if (id < 0 || id >= static_cast<int>(apps_.size())) {
      response["error"] = "Requested ID does is invalid!";
      return;
    }

    const ProtoApp& app = apps_[id];
    const ProtoScreen& screen = app.screens(0);
    App syn_app(screen, true);
    syn_app.InitializeAttributes(screen);
    adjustViewsByUserConstraints(&syn_app);

    LOG(INFO) << "Original";
    PrintApp(syn_app, true);

    std::vector<Device> devices;
    for (const Json::Value& device : request["devices"]) {
      devices.push_back(Device(device[0].asInt(), device[1].asInt()));
    }

    {
      Json::Value root_sizes = Json::Value(Json::arrayValue);
      Device ref_device = Device(720, 1280);
      const View& root = syn_app.GetViews()[0];
      for (const auto& device : devices) {
        View content_frame(root.xleft, root.ytop, root.xright, root.ybottom, root.name, root.id);

        TryResizeView(syn_app, content_frame, ref_device, device);
        LOG(INFO) << "ScreenResized: " << device;
        LOG(INFO) << '\t' << root;
        LOG(INFO) << '\t' << content_frame;

        Json::Value entry = Json::Value(Json::objectValue);
        entry["xleft"] = content_frame.xleft;
        entry["xright"] = content_frame.xright;
        entry["ytop"] = content_frame.ytop;
        entry["ybottom"] = content_frame.ybottom;
        root_sizes.append(entry);
      }

      response["view_sizes"] = root_sizes;
    }



//    Json::Value entry = Json::Value(Json::objectValue);
    response["name"] = app.package_name();

    response["screenshot"] = StringPrintf("%s", screen.window_path().substr(std::string("/home/pavol/ETH/data/").size()).c_str());
    response["content_frame"] = ViewToJson(syn_app.GetViews()[0]);
    Json::Value components = Json::Value(Json::arrayValue);
    {
      App full_app(screen, only_constraint_views);
      for (size_t i = 1; i < full_app.GetViews().size(); i++) {
        components.append(ViewToJson(full_app.GetViews()[i]));
      }
    }
    response["components"] = components;




    {
      fs::path img_path = fs::path(screen.window_path());
      LOG(INFO) << img_path.filename();
      LOG(INFO) << img_path.parent_path();
      std::string xml_data;
      LOG(INFO) << StringPrintf("../web/%s/%s.xml", img_path.parent_path().string().substr(std::string("/home/pavol/ETH/data/").size()).c_str(), GetBaseFilenameFromImage(img_path.filename().string()).c_str());
      ReadFileToString(StringPrintf("../web/%s/%s.xml", img_path.parent_path().string().substr(std::string("/home/pavol/ETH/data/").size()).c_str(), GetBaseFilenameFromImage(img_path.filename().string()).c_str()).c_str(), &xml_data);
      LOG(INFO) << xml_data;
      response["xml"] = xml_data;
    }

    {
      LOG(INFO) << "Start Synthesis";
      Json::Value json_layouts = Json::Value(Json::objectValue);
      for (const auto &syn : synthesizers) {
        SynResult res = syn->Synthesize(screen, only_constraint_views);
        if (res.status == Status::SUCCESS) {
          LOG(INFO) << syn->name;
          PrintApp(res.app);
          json_layouts[syn->name] = res.app.ToJSON();
          LOG(INFO) << res.app.ToXml();
        }
      }
      LOG(INFO) << "Done..";
      response["layouts"] = json_layouts;
    }

  }

private:
  std::vector<ProtoApp> apps_;
  bool only_constraint_views;
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

