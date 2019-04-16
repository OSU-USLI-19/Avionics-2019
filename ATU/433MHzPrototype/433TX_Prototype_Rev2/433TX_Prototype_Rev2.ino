#include <ELECHOUSE_CC1101.h>

// Maximum packet size defined in library
#define size 61

byte TX_buffer[size]="$GPRMC,225446,A,4916.45,N,12311.12,W,000.5,054.7,191194,020.3,E*68";
int i;

byte stringTemp[size] = "$GPRMC,225446,A,4916.45,N,12311.12,W,000.5";

uint8_t *payloadPacket, *bufferPacket;
uint8_t *packet = new uint8_t[220];

const int gpsSentenceLen = 80;
char sentence[gpsSentenceLen], data[100];

void setup()
{
  Serial.begin(9600);
  ELECHOUSE_cc1101.Init();
}

void loop()
{
  //Serial.print("test tx\n");
  ELECHOUSE_cc1101.SendData(stringTemp,size);
  delay(1);
}
