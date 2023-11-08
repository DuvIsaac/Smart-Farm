#include <DFRobot_DHT11.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#define DHT11_delay 1000
#define ProSM       750
#define PT          1000
#define ST          1000
#define ProCO2      1800
#define ProPC       400
#define CLOSE       31
#define OPEN        125

////////////////////////////////////////////////////////////////////////////////////////////

int SMpin[4] = {A0, A1, A2, A3};
int SMSensor[4];
double MeanSM = 0;
int THpin = 2;
DFRobot_DHT11 THSensor;
LiquidCrystal_I2C lcd(0x27,20,4);
int PCSensor = A4;
int PCval = 0;
int fan = 3;
int MQ135 = A5;
int avrPPM;
int co2PPM;
int PRelay1 = 5;
int PRelay2 = 6;
Servo servo;
int Servopin = 4;
unsigned long Ptime;
unsigned long Stime;
unsigned long now;
int LED = 7;
bool RP = 1;

////////////////////////////////////////////////////////////////////////////////////////////

int f1(){
  // 수분 많을 때
  if(MeanSM < ProSM){
    return 0;
  }

  //수분 부족
  else if(MeanSM > ProSM + 50){
    return 1;
  }
}

int f2(){
  // 어두울 때
  if(PCval > ProPC){
    return 1;
  }

  //밝을 때
  else if(PCval < ProPC - 25){
    return 0;
  }
}

int f3(){
  if(co2PPM > ProCO2){
    return 1;
  }
  else if(co2PPM < ProCO2 - 10){
    return 0;
  }
}

void SDisplay(){
  Serial.print("MeanSM: "); Serial.print(MeanSM);
  Serial.print("\t\ttemp: "); Serial.print(THSensor.temperature);
  Serial.print("\thumi: "); Serial.print(THSensor.humidity);
  Serial.print("\tphoto: "); Serial.print(PCval);
  Serial.print("\tCO2: "); Serial.println(co2PPM);

  Serial.print("Pump: "); Serial.print(f1());
  Serial.print("\tLED: "); Serial.print(f2());
  Serial.print("\tfan: "); Serial.println(f3());

  Serial.println();
}

void ServoWrite(int pos){
  servo.write(pos);
}

void FanWrite(bool a){
  digitalWrite(fan, a);
}

void PumpWrite(bool a){
  unsigned long t = now - Ptime;

  if(2*PT <= t){
    Ptime = now;
    return;
  }

  // if(RP != a){
  //   RP = a;
  //   digitalWrite(PRelay1, a);
  //   digitalWrite(PRelay2, !a);
  //   delay(1000);
  //   return;
  // }

  if(!a){
    digitalWrite(PRelay1, 0);
    digitalWrite(PRelay2, 0);
    return;
  }

  if(t < PT){
    digitalWrite(PRelay1, 1);
    digitalWrite(PRelay2, 1);
  }
  else if(PT <= t){
    digitalWrite(PRelay1, 0);
    digitalWrite(PRelay2, 0);
  }
}

void LEDWrite(int a){
  digitalWrite(LED, a);
}

int averageMQ135(){
  byte count = 10;
  float value = 0;

  for(int i=0;i<count;i++){
    value += analogRead(MQ135);
    delay(20);
  }

  value = value / count;
  return value;
}

void SMtoPump(){
  // 수분 많을 때
  if(MeanSM < ProSM){
    PumpWrite(0);
  }

  //수분 부족
  else if(MeanSM > ProSM + 50){
    PumpWrite(1);
  }
}

void MQ135toFan(){
  if(co2PPM > ProCO2){
    FanWrite(1);
    ServoWrite(OPEN);
  }
  else if(co2PPM < ProCO2 - 10){
    FanWrite(0);
    ServoWrite(CLOSE);
  }
}

void PCtoLED(){
  LEDWrite(1);
  // // 어두울 때
  // if(PCval > ProPC){
  //   LEDWrite(1);
  // }

  // //밝을 때
  // else if(PCval < ProPC - 25){
  //   LEDWrite(0);
  // }
}

void SensorInput(){
  unsigned long t = now - Stime;

  if(t < ST) return;
 
  // 토양 습도 센서
  for(int i=0;i<4;i++){
    SMSensor[i] = analogRead(SMpin[i]);
    MeanSM += SMSensor[i];

    // Serial.print(SMSensor[i]);
    // Serial.print(" ");
  }
  // Serial.println();
  MeanSM /= 4;
  
  // 온습도 센서
  THSensor.read(THpin);
  
  // 광도 센서
  PCval = analogRead(PCSensor);

  // MQ135 센서
  avrPPM = averageMQ135();
  co2PPM = map(avrPPM, 0, 1023, 400, 5000);

  SDisplay();

  Stime = now;
}

////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(9600);
  
  pinMode(fan, OUTPUT);
  pinMode(PRelay1, OUTPUT);
  pinMode(PRelay2, OUTPUT);
  pinMode(LED, OUTPUT);

  servo.attach(Servopin);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0,0);
  lcd.print("DuRuMi          ");
  lcd.setCursor(0,1);
  lcd.print("        v23.11.8");

  Ptime = millis();
  Stime = Ptime;
}

void loop() {
  now = millis();
  if(Ptime > now) Ptime -= 4294967295;
  if(Stime > now) Stime -= 4294967295;

  SensorInput();

  MQ135toFan();
  PCtoLED();
  SMtoPump();
}