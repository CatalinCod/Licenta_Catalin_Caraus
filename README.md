Linkul către repositoryul cu codul: https://github.com/CatalinCod/Licenta_Catalin_Caraus 

Pasul 1: În CameraWebServices.ino se modifică liniile:
const char* SSID = "Numele_tău_WiFi";
const char* PASS = "Parola_WiFi";
După se face upload și se noteaza IP-ul din serial monitor

Pasul 2: În ESPWROOM.ino se modifică linia 
const char* CAM_IP = "IP din SerialMonitor de mai devreme";

Pasul 3: În app.js modificăm liniile:              
const proxy = "http://192.168.0.10:8000";   
const CAM   = "http://192.168.0.109:81/stream";
cu Ip-urile noastre.

Pasul 4:
Pe Arduino 1.8.9 deschidem ESPWROOM.ino și TOOLS alegem Sketch Data Upload pentru a încarca
websiteul pe LittleFS

Pasul 5: Rulăm serverul:
uvicorn vision_proxy:app --host 0.0.0.0 --port 8000 --reload

Pasul 6: Alimetăm robotul 

Pasul 7: Mergem pe adresa gateway-ului.
