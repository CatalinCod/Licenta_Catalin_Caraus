#include <Arduino.h>
#line 1 "C:\\Users\\Catalin\\Desktop\\Licenta\\ESPWROOM\\ESPWROOM.ino"
/*
   CamBot_Gateway  –  ESP32 Dev Module
   • serveşte fişiere LittleFS /index.html, /style.css, /app.js
   • rute robot: /F /B /L /R /S, /P‹angle›
   • /capture: cere un JPEG de la ESP32-CAM şi îl salvează în /captures/
   • /db  (GET)    → list.json   (catalog capturi)
   • /db  (POST)   → update etichetă GPT pentru o poză
*/

#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

HardwareSerial Uno(1);                         // RX=GPIO16, TX=GPIO17

const char* SSID   = "TP-Link_E22A";
const char* PASS   = "danupidor1";
const char* CAM_IP = "192.168.0.105";          // ← IP ESP32-CAM

WebServer srv(80);

/*──── helper: trimite fişier static ────*/
#line 25 "C:\\Users\\Catalin\\Desktop\\Licenta\\ESPWROOM\\ESPWROOM.ino"
void sendFile(const String& p);
#line 36 "C:\\Users\\Catalin\\Desktop\\Licenta\\ESPWROOM\\ESPWROOM.ino"
void addCmd(const char* u,char c);
#line 41 "C:\\Users\\Catalin\\Desktop\\Licenta\\ESPWROOM\\ESPWROOM.ino"
void handleServo();
#line 47 "C:\\Users\\Catalin\\Desktop\\Licenta\\ESPWROOM\\ESPWROOM.ino"
void handleCapture();
#line 70 "C:\\Users\\Catalin\\Desktop\\Licenta\\ESPWROOM\\ESPWROOM.ino"
void handleDB();
#line 92 "C:\\Users\\Catalin\\Desktop\\Licenta\\ESPWROOM\\ESPWROOM.ino"
void setup();
#line 115 "C:\\Users\\Catalin\\Desktop\\Licenta\\ESPWROOM\\ESPWROOM.ino"
void loop();
#line 25 "C:\\Users\\Catalin\\Desktop\\Licenta\\ESPWROOM\\ESPWROOM.ino"
void sendFile(const String& p){
  if(!LittleFS.exists(p)){ srv.send(404,"text/plain","NF"); return; }
  String ct="text/plain";
  if(p.endsWith(".html"))ct="text/html";
  else if(p.endsWith(".css"))ct="text/css";
  else if(p.endsWith(".js")) ct="application/javascript";
  else if(p.endsWith(".json"))ct="application/json";
  File f = LittleFS.open(p,"r");
  srv.streamFile(f, ct); f.close();
}
/*──── F/B/L/R/S rute ────*/
void addCmd(const char* u,char c){
  srv.on(u,[=](){ Uno.write(c); srv.send(200,"text/plain","OK"); });
}

/*──── /P<număr> ────*/
void handleServo(){
  Uno.write('P'); Uno.print(srv.uri().substring(2)); Uno.write('\n');
  srv.send(200,"text/plain","OK");
}

/*──── /capture ────*/
void handleCapture(){
  HTTPClient cli;
  String url = String("http://") + CAM_IP + "/capture";
  if(!cli.begin(url) || cli.GET()!=200){
    srv.send(500,"text/plain","cam_err"); cli.end(); return;
  }

  // nume fişier: yyyyMMdd_HHmmss.jpg
  struct tm t; getLocalTime(&t);         // Wi-Fi → NTP implicit
  char name[32]; strftime(name,sizeof(name),"/captures/%Y%m%d_%H%M%S.jpg",&t);

  File f = LittleFS.open(name,"w");
  cli.writeToStream(&f); f.close(); cli.end();

  // adaugă în list.json
  File db = LittleFS.open("/captures/list.json","a");
  db.printf("{\"file\":\"%s\",\"ts\":%ld}\n", name+10, time(nullptr));
  db.close();

  srv.send(200,"text/plain",name+10);    // răspunde cu numele fişierului
}

/*──── /db GET / POST ────*/
void handleDB(){
  if(srv.method()==HTTP_GET){
    sendFile("/captures/list.json");
  }else if(srv.method()==HTTP_POST){
    StaticJsonDocument<256> doc;
    deserializeJson(doc, srv.arg("plain"));
    String file = doc["file"];
    // rescrie linia în list.json cu etichetă
    File in = LittleFS.open("/captures/list.json","r");
    File out = LittleFS.open("/captures/tmp.json","w");
    for(String l; (l=in.readStringUntil('\n')).length(); ){
      StaticJsonDocument<256> row; deserializeJson(row,l);
      if(row["file"]==file) row["label"]=doc["label"];
      serializeJson(row,out); out.write('\n');
    }
    in.close(); out.close();
    LittleFS.remove("/captures/list.json");
    LittleFS.rename("/captures/tmp.json","/captures/list.json");
    srv.send(200,"text/plain","upd");
  }
}

void setup(){
  Serial.begin(115200);
  WiFi.begin(SSID,PASS); while(WiFi.status()!=WL_CONNECTED) delay(300);
  Uno.begin(9600,SERIAL_8N1,16,17);
  LittleFS.begin(true);

  /* statice */
  srv.on("/",[](){ sendFile("/index.html"); });
  srv.on("/style.css", [](){ sendFile("/style.css"); });
  srv.on("/app.js",   [](){ sendFile("/app.js"); });
  srv.serveStatic("/captures/", LittleFS, "/captures/");

  /* comenzi */
  addCmd("/F",'F'); addCmd("/B",'B'); addCmd("/L",'L');
  addCmd("/R",'R'); addCmd("/S",'S');
  srv.on("^\\/P[0-9]+$", HTTP_ANY, handleServo);
  srv.on("/capture", HTTP_GET, handleCapture);
  srv.on("/db", HTTP_ANY, handleDB);

  srv.begin();
  Serial.println("Gateway IP="+WiFi.localIP().toString());
}

void loop(){ srv.handleClient(); }

