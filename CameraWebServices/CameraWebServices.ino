#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"


const char* SSID = "TP-Link_E22A";
const char* PASS = "danupidor1";


WiFiServer  streamSrv(81);
WiFiClient  streamCl;
WebServer   http(80);


void handleCapture() {
  camera_fb_t* fb = esp_camera_fb_get();
  if(!fb){ http.send(503,"text/plain","cam_err"); return; }

  http.setContentLength(fb->len);
  http.send(200,"image/jpeg","");
  http.client().write((const char*)fb->buf, fb->len);
  esp_camera_fb_return(fb);
}


void setup() {
  Serial.begin(115200);
  WiFi.begin(SSID,PASS);
  while(WiFi.status()!=WL_CONNECTED) delay(300);

  camera_config_t c{};
  c.ledc_channel = LEDC_CHANNEL_0;
  c.ledc_timer   = LEDC_TIMER_0;
  c.pin_d0 = Y2_GPIO_NUM; c.pin_d1 = Y3_GPIO_NUM; c.pin_d2 = Y4_GPIO_NUM;
  c.pin_d3 = Y5_GPIO_NUM; c.pin_d4 = Y6_GPIO_NUM; c.pin_d5 = Y7_GPIO_NUM;
  c.pin_d6 = Y8_GPIO_NUM; c.pin_d7 = Y9_GPIO_NUM;
  c.pin_xclk = XCLK_GPIO_NUM; c.pin_pclk = PCLK_GPIO_NUM;
  c.pin_vsync = VSYNC_GPIO_NUM; c.pin_href = HREF_GPIO_NUM;
  c.pin_sscb_sda = SIOD_GPIO_NUM; c.pin_sscb_scl = SIOC_GPIO_NUM;
  c.pin_pwdn = PWDN_GPIO_NUM; c.pin_reset = RESET_GPIO_NUM;

  c.xclk_freq_hz = 20'000'000;
  c.pixel_format = PIXFORMAT_JPEG;
  c.frame_size   = FRAMESIZE_QVGA;   
  c.jpeg_quality = 18;
  c.fb_count     = 2;               
  c.grab_mode    = CAMERA_GRAB_LATEST;

  if(esp_camera_init(&c)!=ESP_OK){
    Serial.println("Camera init failed");
    while(true) delay(1000);
  }

  Serial.println("fb_count=2  grab_mode=1  init OK");
  Serial.println("CAM IP="+WiFi.localIP().toString());

  http.on("/capture",HTTP_GET,handleCapture);
  http.begin();
  streamSrv.begin();
}


void loop() {
  http.handleClient();

  
  if(!streamCl || !streamCl.connected()){
    streamCl = streamSrv.available();
    if(streamCl){
      streamCl.println("HTTP/1.0 200 OK");
      streamCl.println("Content-Type: multipart/x-mixed-replace; boundary=frame\r\n");
    }
    return;
  }
  camera_fb_t* fb = esp_camera_fb_get();
  if(!fb){ streamCl.stop(); return; }
  streamCl.printf("--frame\r\nContent-Type:image/jpeg\r\nContent-Length:%u\r\n\r\n",fb->len);
  streamCl.write(fb->buf, fb->len); streamCl.println();
  esp_camera_fb_return(fb);
  delay(60);
}
