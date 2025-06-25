#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "time.h"

HardwareSerial Uno(1);                      


const char* SSID   = "TP-Link_E22A";
const char* PASS   = "danupidor1";
const char* CAM_IP = "192.168.0.107";        


WebServer srv(80);


void listDir(const char* path) {
  File root = SPIFFS.open(path);
  while (true) {
    File f = root.openNextFile();
    if (!f) break;
    Serial.printf("%s  (%u B)\n", f.path(), f.size());
  }
}

void createIfMissing() {
  if (!SPIFFS.exists("/captures"))  SPIFFS.mkdir("/captures");
  if (!SPIFFS.exists("/captures/list.json")) {
    File f = SPIFFS.open("/captures/list.json", "w"); f.close();
  }
}

void addCaptureToDB(const char* fname) {
  DynamicJsonDocument doc(4096);
  File f = SPIFFS.open("/captures/list.json", "r");
  if (f) { deserializeJson(doc, f); f.close(); }
  JsonArray arr = doc.is<JsonArray>() ? doc.as<JsonArray>()
                                      : doc.to<JsonArray>();
  JsonObject row = arr.createNestedObject();
  row["file"] = fname;
  row["ts"]   = time(nullptr);
  f = SPIFFS.open("/captures/list.json", "w");
  serializeJson(arr, f); f.close();
}


void setupWiFi() {
  WiFi.begin(SSID, PASS);
  for (int i = 0; WiFi.status() != WL_CONNECTED && i < 10; ++i) {
    delay(1000); Serial.print('.');
  }
  if (WiFi.status() != WL_CONNECTED) ESP.restart();
  Serial.printf("\nIP: %s\n", WiFi.localIP().toString().c_str());
}


void setup() {
  Serial.begin(115200);
  setupWiFi();

  if (!SPIFFS.begin(false)) {                
    Serial.println("Primul mount eșuat → formatez");
    if (!SPIFFS.begin(true)) { while (true); }
  }
  createIfMissing();
  Serial.println("Conținut SPIFFS:"); listDir("/");

  Uno.begin(9600, SERIAL_8N1, 16, 17);


  srv.on("/", []() {
    File f = SPIFFS.open("/index.html", "r");
    srv.streamFile(f, "text/html"); f.close();
  });
  srv.on("/style.css", []() {
    File f = SPIFFS.open("/style.css", "r");
    srv.streamFile(f, "text/css"); f.close();
  });
  srv.on("/app.js", []() {
    File f = SPIFFS.open("/app.js", "r");
    srv.streamFile(f, "application/javascript"); f.close();
  });

  srv.on("/F", [](){ Uno.write('F'); srv.send(200,"text/plain","OK"); });
  srv.on("/B", [](){ Uno.write('B'); srv.send(200,"text/plain","OK"); });
  srv.on("/L", [](){ Uno.write('L'); srv.send(200,"text/plain","OK"); });
  srv.on("/R", [](){ Uno.write('R'); srv.send(200,"text/plain","OK"); });
  srv.on("/S", [](){ Uno.write('S'); srv.send(200,"text/plain","OK"); });

  
  srv.onNotFound([](){
    String u = srv.uri();
    if (u.length() > 2 && u[1] == 'P' && isDigit(u[2])) {
      Uno.write('P'); Uno.print(u.substring(2)); Uno.write('\n');
      srv.send(200,"text/plain","OK"); return;
    }
    srv.send(404,"text/plain","NF");
  });

  
  srv.on("/capture", HTTP_GET, [](){
    HTTPClient cli;
    if (!cli.begin(String("http://") + CAM_IP + "/capture") || cli.GET() != 200) {
      srv.send(500,"text/plain","cam_err"); cli.end(); return;
    }
    struct tm t; getLocalTime(&t);
    char fn[32]; strftime(fn, sizeof(fn), "/captures/%Y%m%d_%H%M%S.jpg", &t);
    File f = SPIFFS.open(fn, "w");
    cli.writeToStream(&f); f.close(); cli.end();
    addCaptureToDB(fn + 10);
    srv.send(200,"text/plain", String(fn + 10));
  });


  srv.on("/db", HTTP_GET, [](){
    File f = SPIFFS.open("/captures/list.json","r");
    srv.streamFile(f,"application/json"); f.close();
  });

  srv.begin();
  Serial.printf("Gateway ready @ http://%s/\n", WiFi.localIP().toString().c_str());
}

void loop() { srv.handleClient(); }
