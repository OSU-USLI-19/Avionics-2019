// Direct continuation of Rev2
#include <ELECHOUSE_CC1101.h>

// BYTE TYPE AIN'T DEPRECATED, better to use that than char due to typing - char != byte
byte RX_buffer[61];
int size, i, packetIndex = 0, packetLength = 0, packetLengthIndex = 0;

uint8_t ATUaddress_1[8] = {0x00, 0x13, 0xA2, 0x00, 0x41, 0x64, 0x5B};
uint8_t ATUaddress_2[8] = {0x00, 0x13, 0xA2, 0x00, 0x41, 0x78, 0xE2}; //013A204178E2
char ATU_1_Name[5] = "rick", ATU_2_Name[7] = "summer";

uint8_t *packetPayload = new uint8_t[10], *packet = new uint8_t[220];

void setup()
{
  Serial.begin(9600);
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.SetReceive();
}

void loop()
{
  if(ELECHOUSE_cc1101.CheckReceiveFlag())
  {
    size=ELECHOUSE_cc1101.ReceiveData(RX_buffer);
    for(i=0;i<size;i++)
    {
      Serial.print(char(RX_buffer[i]));
    }
    Serial.print("@\n");
    ELECHOUSE_cc1101.SetReceive();
  }
  //Serial.print("Receive test\n");
}
