package srl.inf.ethz.ch;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.json.simple.JSONValue;
import org.json.simple.parser.JSONParser;
import org.json.simple.parser.ParseException;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;

public class ConsoleServer {

    public static void main(String[] args) {
        if (args.length == 3 && args[1].equals("-") && args[2].equals("JSON")) {
            BufferedReader reader = new BufferedReader(new InputStreamReader(System.in, StandardCharsets.UTF_8));
            boolean errorOccured = false;
            try {
                JSONParser parser = new JSONParser();
                for (;;) {
                    String line = reader.readLine();
                    if (line == null)
                        break; // End of file
                    if (line.startsWith("---")) {
                        System.out.flush();
                        continue;
                    }
                    Object o;
                    try {
                        o = parser.parse(line);
                        if (!(o instanceof JSONArray)) {
                            System.err.println("Expected JSON array as input");
                            continue;
                        }
                        JSONArray arr = (JSONArray) o;
                        for (Object so : arr) {
                            if (so instanceof JSONObject && ((JSONObject) so).containsKey("code")
                                    && ((JSONObject) so).get("code") instanceof String) {
                                StringBuilder out = new StringBuilder();
                                out.append("{\"tree\":");
                                String parseError = "";
                                try {
                                    JSONObject layoutData = LayoutUtil.LayoutViews((JSONObject) ((JSONObject) so).get("code"));
                                    out.append(layoutData.toJSONString());
                                } catch(ParseException e) {
                                    parseError = e.getMessage();
                                    errorOccured = true;
                                    // append empty tree
                                    out.append("[]");
                                }
                                JSONObject json = ((JSONObject) so);
                                for (Object key : json.keySet()) {
                                    if (!key.equals("code")) {
                                        out.append(",");
                                        out.append(JSONValue.toJSONString(key));
                                        out.append(":");
                                        out.append(JSONValue.toJSONString(json.get(key)));
                                    }
                                }
                                out.append(",");
                                out.append(JSONValue.toJSONString("parse_error"));
                                out.append(":");
                                out.append(JSONValue.toJSONString(parseError));

                                out.append("}");
                                System.out.println(out);
                                System.out.flush();
                            } else {
                                System.err.println("Expected string in input.");
                            }
                        }
                    } catch (org.json.simple.parser.ParseException e) {
                        e.printStackTrace();
                        errorOccured = true;
                    }
                }
                reader.close();
            } catch (IOException e) {
                e.printStackTrace();
                errorOccured = true;
            }
            if (errorOccured) {
                System.exit(1);
            }
        } else {
            System.out.println("Usage:");
            System.out.println("  java -jar javaparser.jar -- <file>");
            System.out.println("or");
            System.out.println("  java -jar javaparser.jar -- - JSON");
        }
    }

}
