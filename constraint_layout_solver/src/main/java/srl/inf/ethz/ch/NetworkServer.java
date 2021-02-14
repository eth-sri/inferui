package srl.inf.ethz.ch;


import com.sun.net.httpserver.Headers;
import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpHandler;
import com.sun.net.httpserver.HttpServer;
import org.apache.commons.cli.*;
import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;
import org.json.simple.parser.ParseException;

import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;

import java.net.HttpURLConnection;
import java.net.InetSocketAddress;

import static srl.inf.ethz.ch.LayoutUtil.getField;


public class NetworkServer {

    private static void SetResponse(HttpExchange t, String data) throws IOException {
        t.sendResponseHeaders(200, data.length());
        OutputStream os = t.getResponseBody();
        os.write(data.getBytes());
        os.close();
    }

    public static void main(String[] args) throws IOException {
        // create the command line parser
        CommandLineParser parser = new DefaultParser();

        // create the Options
        Options options = new Options();
        options.addOption("help", "print this message");
        options.addOption("origin", true, "value of expected Access-Control-Allow-Origin field (e.g. 'http://inferui.com'). default: *");
        options.addOption("port", true, "server port. default: 9000");
        CommandLine cmd = new CommandLine.Builder().build();
        try {
            cmd = parser.parse(options, args);
        } catch (Exception exp ) {
            System.out.println( "Unexpected exception:" + exp.getMessage() );
            System.exit(1);
        }

        if (cmd.hasOption("help")){
            HelpFormatter formatter = new HelpFormatter();
            formatter.printHelp( "java -jar layout.jar [OPTION]", options );
            System.exit(0);
        }

        final String ORIGIN = cmd.getOptionValue("origin", "*");
        final int port = Integer.parseInt(cmd.getOptionValue("port", "9100"));

        HttpServer server = HttpServer.create(new InetSocketAddress(port), 0);
        System.out.println("server started at " + port);
        server.createContext("/layout", new HttpHandler() {
            @Override
            public void handle(HttpExchange t) throws IOException {
                System.out.println(Thread.currentThread().getId());
                if (t.getRequestMethod().equals("OPTIONS")) {
                    System.out.println("request OPTIONS");
                    Headers headers = t.getResponseHeaders();
                    headers.add("Access-Control-Allow-Origin", ORIGIN);
                    headers.add("Access-Control-Allow-Methods", "POST, OPTIONS");
                    headers.add("Access-Control-Allow-Headers", "X-Requested-With");
                    headers.add("Access-Control-Allow-Headers", "Content-Type");
                    t.sendResponseHeaders(HttpURLConnection.HTTP_OK, -1);
                    return;
                } else if (t.getRequestMethod().equals("POST")) {
                    System.out.println("request POST");
                    Headers headers = t.getResponseHeaders();
                    headers.add("Access-Control-Allow-Origin", ORIGIN);
                    headers.add("Content-Type", "application/json");

                    JSONParser parser = new JSONParser();
                    try {
                        JSONObject data = (JSONObject) parser.parse(new InputStreamReader(t.getRequestBody(), "UTF-8"));
                        System.out.println("Parsed Data:");
                        System.out.println(data.toJSONString());
                        JSONObject responseData = LayoutUtil.LayoutViews(data);
                        System.out.println("Writing Response:");
                        System.out.println(responseData.toJSONString());
                        SetResponse(t, responseData.toJSONString());
                    } catch (ParseException e) {
                        System.out.println("Parse Error!");
                        SetResponse(t, "{\"error\": \"ParseException\"}");
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                    return;
                }
            }
        });


        server.setExecutor(null);
        server.start();
    }



}
