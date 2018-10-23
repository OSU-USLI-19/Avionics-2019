//#include <i2c_t3.h>
//#include <i2c_t3.cpp>

#include <Wire.h>
#include <Adafruit_MPL3115A2.h>
//#include <Adafruit_MPL3115A2.cpp>

//TWI Addresses
int iBaro_Addr = 0xC0;
//int iIMU_Addr = 0x;
//int iPres_Addr = 0x;

//Hardware pins:
int iSDA_Pin = 18;
int iSCL_Pin = 19;
int pLED = 13;
bool bLED = false;


//time variables
unsigned long timeCurMS;
unsigned long timePrintMS = 0;
unsigned long timeBlinkMS = 0;
unsigned long timeReadMS = 0;
const unsigned long PRINT_TIME = 100; // ms
const unsigned long BLINK_TIME = 1000; //ms

//sensor read variables
float PressureRead = 0;
float AltRead = 0;

//instance of MPL3115 (low pres sense) class
Adafruit_MPL3115A2 Alt_Sensor = Adafruit_MPL3115A2();

void setup() {
  //Serial Comms for debugging
  Serial.begin(9600);

  Serial.println("Start");
  //TWI setup
  //Wire.setSDA(iSDA_Pin);  //set SDA Pin
  //Wire.setSCL(iSCL_Pin);  //set SCL Pin
  //Wire.begin();           //start TWI comm

  Alt_Sensor.begin();       //initialize altimeter
  
  pinMode(pLED, OUTPUT); 
}

void loop() {
  timeCurMS = millis();

  PressureRead = Alt_Sensor.getPressure();
  //AltRead = Alt_Sensor.getAltitude();

  //serial output for debug
  
  if((timeCurMS - timePrintMS) >= PRINT_TIME){
    Serial.print("Time (s): ");
    Serial.print((float)timeCurMS/1000);
    //Serial.print("   Altitude (ft): ");
    //Serial.print(AltRead*0.3048);
    Serial.print("   Pressure (kPa) ");
    Serial.print(PressureRead/1000.0);
    Serial.println();
    timePrintMS = timeCurMS; 
  }
  
  //Serial.println("Time: ");
  //delay(1000);
  //heartbeat blink:
  /*
  if((timeCurMS - timeBlinkMS) >= BLINK_TIME){
    if(bLED){
      bLED = false;
      digitalWrite(pLED,LOW); }
    else{
      bLED = true;
      digitalWrite(pLED,HIGH); } 
    timeBlinkMS = timeCurMS;
  }
  */
}
