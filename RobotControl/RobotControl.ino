#include <SoftwareSerial.h>
#include <Servo.h>


const uint8_t M1I1 = 4,  M1I2 = 7,  M1E = 5;   
const uint8_t M2I1 = 8,  M2I2 = 12, M2E = 6;   
const uint8_t M3I1 = 13, M3I2 = A0, M3E = 10;  
const uint8_t M4I1 = A1, M4I2 = A2, M4E = 11;  
const uint8_t SERVO = A5;


SoftwareSerial esp(2, 3);
Servo cam;

const uint8_t SPEED = 255;   


void drv(uint8_t in1, uint8_t in2, uint8_t en, int8_t dir) {
  if (dir == 0) {            
    analogWrite(en, 0);
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
  } else {
    digitalWrite(in1, dir > 0);
    digitalWrite(in2, dir < 0);
    analogWrite(en, SPEED);
  }
}

void all(int8_t dir) {       
  drv(M1I1,M1I2,M1E,dir);
  drv(M2I1,M2I2,M2E,dir);
  drv(M3I1,M3I2,M3E,dir);
  drv(M4I1,M4I2,M4E,dir);
}

void stopAll() { all(0); }


void left()  {
  drv(M1I1,M1I2,M1E,0);
  drv(M3I1,M3I2,M3E,1);
  drv(M2I1,M2I2,M2E, 0);
  drv(M4I1,M4I2,M4E, 1);
}


void right() {
  drv(M1I1,M1I2,M1E, 1);
  drv(M3I1,M3I2,M3E, 0);
  drv(M2I1,M2I2,M2E,1);
  drv(M4I1,M4I2,M4E,0);
}


void exec(char c) {
  if (c == '\n' || c == '\r') return;        
  if (c == 'P') {                            
    cam.write(constrain(esp.parseInt(), 0, 180));
    return;
  }
  switch (c) {
    case 'F': all( 1); break;
    case 'B': all(-1); break;
    case 'L': left(); break;
    case 'R': right(); break;
    case 'S': stopAll(); break;
  }
}

void setup() {
  Serial.begin(9600);
  esp.begin(9600);

  cam.attach(SERVO);
  cam.write(90);                             

  uint8_t outs[] = {M1I1,M1I2,M2I1,M2I2,M3I1,M3I2,M4I1,M4I2};
  uint8_t pwns[] = {M1E,M2E,M3E,M4E};
  for (uint8_t p: outs) pinMode(p, OUTPUT);
  for (uint8_t p: pwns) pinMode(p, OUTPUT);

  stopAll();
}

void loop() {
  if (esp.available())    exec(esp.read());
  if (Serial.available()) exec(Serial.read());   
} 