  #include <math.h>

// Voltage Dividers
const int PIN_1P = A14;
const int PIN_1N = A15;
const int PIN_2P = A16;
const int PIN_2N = A17;
const int PIN_3P = A18;
const int PIN_3N = A19;
const int PIN_4P = A20;
const int PIN_4N = A21;
const int PIN_5P = A13;
const int PIN_5N = A12;
const int PIN_SW = A2;
const int PIN_CUR = A3;

// Pressure Transducer
const int PIN_PP = A0;
const int PIN_PN = A1;

// Current Sensor
int DAC0 = 0;

// LED Control 
const int PIN_LEDGATE = 3;

// Rover Relay
const int PIN_ROV = 8;

// Relays
const int PIN_1D = 18; 
const int PIN_2D = 19;
const int PIN_3D = 20;
const int PIN_4D = 23;
const int PIN_5D = 22;
const int PIN_12S = 5;
const int PIN_3S = 6;
const int PIN_45S = 7;


const int numReadings = 10;
int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average
int inputPin = A2;

unsigned long currentTime = 0;
unsigned long lastTime = 0;

// end allocation
volatile double analogpin[14] {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


void disarmAll(){
  digitalWrite(PIN_1D, LOW);
  digitalWrite(PIN_2D, LOW);
  digitalWrite(PIN_3D, LOW);
  digitalWrite(PIN_4D, LOW);
  digitalWrite(PIN_5D, LOW);
  digitalWrite(PIN_12S, LOW);
  digitalWrite(PIN_3S, LOW);
  digitalWrite(PIN_45S, LOW);
}


void setup() {
  // put your setup code here, to run once:
  pinMode(PIN_1D, OUTPUT);
  pinMode(PIN_2D, OUTPUT);
  pinMode(PIN_3D, OUTPUT);
  pinMode(PIN_4D, OUTPUT);
  pinMode(PIN_5D, OUTPUT);
  pinMode(PIN_12S, OUTPUT);
  pinMode(PIN_3S, OUTPUT);
  pinMode(PIN_45S, OUTPUT);

  pinMode(DAC0, OUTPUT);
  pinMode(PIN_LEDGATE, OUTPUT);
  pinMode(PIN_ROV, OUTPUT);

  pinMode(PIN_1P, INPUT);
  pinMode(PIN_1N, INPUT);
  pinMode(PIN_2P, INPUT);
  pinMode(PIN_2N, INPUT);
  pinMode(PIN_3P, INPUT);
  pinMode(PIN_3N, INPUT);
  pinMode(PIN_4P, INPUT);
  pinMode(PIN_4N, INPUT);
  pinMode(PIN_5P, INPUT);
  pinMode(PIN_5N, INPUT);
  pinMode(PIN_SW, INPUT);
  pinMode(PIN_CUR, INPUT);

  pinMode(PIN_PP, INPUT);
  pinMode(PIN_PN, INPUT);

  disarmAll();
  
  Serial.begin(57600);
}


void loop() {

  int arming_delay = 30000;
  int sequence_delay = 2000;
  int rest_time = 500;
  static int igntion_seq = 0;
  
  // LED GATE
  digitalWrite(PIN_LEDGATE, HIGH);
  
  // Rover Relay
  digitalWrite(PIN_ROV, HIGH);
  
  //static String incomingByte = 0;
  float weight = 0.9;

  //if (Serial.available() > 0) {
  //  incomingByte = Serial.readString();
  //}
  
  //switch(incomingByte.toInt()){
  switch(igntion_seq){
    case (1):
      // Ignite ematch 1
      disarmAll();
      digitalWrite(PIN_1D, HIGH);
      digitalWrite(PIN_12S, HIGH);
      break;
    case (2):
      // Ignite ematch 2
      disarmAll();
      digitalWrite(PIN_2D, HIGH);
      digitalWrite(PIN_12S, HIGH);
      break;
    case (3):
      // Ignite ematch 3
      disarmAll();
      digitalWrite(PIN_3D, HIGH);
      digitalWrite(PIN_3S, HIGH);
      break;
    case (4):
      // Ignite ematch 4
      disarmAll();
      digitalWrite(PIN_4D, HIGH);
      digitalWrite(PIN_45S, HIGH);
      break;
    case (5):
      // Ignite ematch 5
      disarmAll();
      digitalWrite(PIN_5D, HIGH);
      digitalWrite(PIN_45S, HIGH);
      break;
    default:
      disarmAll();
    }
//  end

  // Differential Voltages
  analogpin[1]  = analogRead(PIN_1P);///1024.0*13.06)*weight + (1-weight)*analogpin[1];
  analogpin[2]  = analogRead(PIN_1N);///1024.0*12.25)*weight + (1-weight)*analogpin[2];
  analogpin[3]  = analogRead(PIN_2P);///1024.0*13.94)*weight + (1-weight)*analogpin[3];
  analogpin[4]  = analogRead(PIN_2N);///1024.0*10.72)*weight + (1-weight)*analogpin[4];
  analogpin[5]  = analogRead(PIN_3P);///1024.0*13.72)*weight + (1-weight)*analogpin[5];
  analogpin[6]  = analogRead(PIN_3N);///1024.0*10.94)*weight + (1-weight)*analogpin[6];
  analogpin[7]  = analogRead(PIN_4P);///1024.0*13.55)*weight + (1-weight)*analogpin[7];
  analogpin[8]  = analogRead(PIN_4N);///1024.0*12.48)*weight + (1-weight)*analogpin[8];
  analogpin[9]  = analogRead(PIN_5P);///1024.0*13.47)*weight + (1-weight)*analogpin[9];
  analogpin[10] = analogRead(PIN_5N);///1024.0*11.33)*weight + (1-weight)*analogpin[10];

  // Armed Voltage
  //analogpin[11] = (analogRead(PIN_SW)/1024.0*1.0);//*weight + (1-weight)*analogpin[10];
  // Current Sensor
  //analogpin[12] = (analogRead(PIN_CUR) );//*weight + (1-weight)*analogpin[11];
  // Pressure Sensor
  //analogpin[13] = (analogRead(PIN_PP)/8192.0*1.0);//*weight + (1-weight)*analogpin[12];;
  //analogpin[14] = (analogRead(PIN_PN)/8192.0*1.0);//*weight + (1-weight)*analogpin[13];;
  

  currentTime = millis();

  if (currentTime - lastTime > 10) {
    lastTime = currentTime;
  Serial.print(currentTime);
  Serial.print(", ");
  Serial.print(analogpin[1]-analogpin[2],3);
  Serial.print(", ");
  Serial.print(analogpin[3]-analogpin[4],3);
  Serial.print(", ");
  Serial.print(analogpin[5]-analogpin[6],3);
  Serial.print(", ");
  Serial.print(analogpin[7]-analogpin[8],3);
  Serial.print(", ");
  Serial.println(analogpin[9]-analogpin[10],3);
  //Serial.print(", ");
  //Serial.print(analogpin[11],1);
  //Serial.print(", ");
  //Serial.println(analogpin[12],1);
  //Serial.print(", ");
  //Serial.println((analogpin[13]-analogpin[14])/0.026,5);
  }

  
  if (currentTime > (arming_delay)){
    if (currentTime > arming_delay+6*sequence_delay) {
      igntion_seq = 0;
    } else if (currentTime > arming_delay+5*sequence_delay) {
      igntion_seq = 5;
    } else if (currentTime > arming_delay+4*sequence_delay+rest_time) {
      igntion_seq = 0;
    } else if (currentTime > arming_delay+4*sequence_delay) {
      igntion_seq = 4;
    } else if (currentTime > arming_delay+3*sequence_delay+rest_time) {
      igntion_seq = 0;
    } else if (currentTime > arming_delay+3*sequence_delay) {
      igntion_seq = 3;
    } else if (currentTime > arming_delay+2*sequence_delay+rest_time) {
      igntion_seq = 0;
    } else if (currentTime > arming_delay+2*sequence_delay) {
      igntion_seq = 2;
    } else if (currentTime > arming_delay+1*sequence_delay+rest_time) {
      igntion_seq = 0;
    } else if (currentTime > arming_delay+1*sequence_delay) {
      igntion_seq = 1;
    } else {
      igntion_seq = 0;
    }
    
  }
  
}
