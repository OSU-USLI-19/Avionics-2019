#include <ELECHOUSE_CC1101.h>

void setup()
{
  Serial.begin(9600);
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.SetReceive();
}

byte RX_buffer[400] = {};
byte size, flag;
int packetIndex = 0;
int packetLength = 0;
int packetLengthIndex = 0;
uint8_t ATUaddress_1[8] = {0x00, 0x13, 0xA2, 0x00, 0x41, 0x64, 0x5B};
uint8_t ATUaddress_2[8] = {0x00, 0x13, 0xA2, 0x00, 0x41, 0x78, 0xE2}; //013A204178E2

char ATU_1_Name[5] = "rick", ATU_2_Name[7] = "summer";
char transmitFlag = '0'; // Used to determine if PLEC trigger needs to be sent

uint8_t *PLECpacket, *packetPayload = new uint8_t[10];
uint8_t *packet = new uint8_t[220];

void loop()
{
  if (ELECHOUSE_cc1101.CheckReceiveFlag())
  {
    size = ELECHOUSE_cc1101.ReceiveData(RX_buffer);

    int i = 0;
    for (i = 0; i < size; i++)
    {
      uint8_t string[200], payload[100];
      uint8_t inbyte = 0x00;

      bool alreadyRead = false, ATU_1_identifier = true, ATU_2_identifier = true;

      // read from serial port
      inbyte = RX_buffer[i];

      if (inbyte == 0x7E)
      {
        packetIndex = 1;
        string[0] = inbyte;
        alreadyRead = true;
      }

      // Read a fresh packet, case 1
      if (packetIndex == 1 && !alreadyRead)
      {
        string[1] = inbyte;
        packetIndex = 2;
        packetLength = string[1];
        alreadyRead = true;
      }

      // Read a fresh packet, case 2
      if (packetIndex == 2 && !alreadyRead)
      {
        string[2] = inbyte;
        packetLength += string[2];
        packetIndex = 3;
        alreadyRead = true;
      }

      // Read a fresh packet, case 3
      if ((packetIndex > 2) && (packetLengthIndex <= packetLength + 3) && !alreadyRead)
      {
        string[packetIndex] = inbyte;
        packetLengthIndex += 1;
        packetIndex += 1;
        alreadyRead = true;
      }

      if ((packetIndex == (packetLength + 3)) && packetIndex > 2)
      {
        // End of packet
        // Serial.print("Packet: ");
        for (int l = 0; l < packetLength + 3; l++)
        {
          if (l > 14)
          {
            payload[l - 15] = string[l];

            if (l == (packetLength + 2))
              payload[l - 14] = 0xEE;
          }

          // Serial.print((char)string[l]);  // commented out to make the ground station work

          packetLengthIndex = 0;
          packetIndex = 0;
        }

        uint8_t currentWord = 0x00;
        int idx = 0;

        // Used to print to the PC serial for display/debug
        while (currentWord != 0xEE)
        {
          currentWord = payload[idx];
          Serial.print((char)currentWord);
          idx++;
        }

        Serial.print(',');
        // Serial.println(); //for debugging, remove for actual thing

        // Determine which flight ATU we're receiving from
        for (int y = 4; y < 11; y++)
        {
          //Serial.print(string[y], HEX);
          if (ATUaddress_1[y - 4] != string[y])
            ATU_1_identifier = false;

          if (ATUaddress_2[y - 4] != string[y])
            ATU_2_identifier = false;
        }

        if (ATU_1_identifier)
          Serial.print(ATU_1_Name); // Prints the atu name

        if (ATU_2_identifier)
          Serial.print(ATU_2_Name); // Prints the atu name

        alreadyRead = false;
        Serial.print('@');
        Serial.print('\n');
      }
    }
    ELECHOUSE_cc1101.SetReceive();
  }
  //Serial.print("Receive test\n");
}
