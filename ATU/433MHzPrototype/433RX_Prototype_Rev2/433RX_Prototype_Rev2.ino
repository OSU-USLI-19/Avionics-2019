#include <ELECHOUSE_CC1101.h>

byte RX_buffer[70];
int size,i;

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
