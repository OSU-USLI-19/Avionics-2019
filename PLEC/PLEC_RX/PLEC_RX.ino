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

// Current Sensor - this currently doesn't work?
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
int readings[numReadings]; // the readings from the analog input
int readIndex = 0;		   // the index of the current reading
int total = 0;			   // the running total
int average = 0;		   // the average
int inputPin = A2;

unsigned long currentTime = 0;
unsigned long lastTime = 0;
int arming_delay = 0;

// end allocation
volatile double analogpin[14]{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void disarmAll()
{
	digitalWrite(PIN_1D, LOW);
	digitalWrite(PIN_2D, LOW);
	digitalWrite(PIN_3D, LOW);
	digitalWrite(PIN_4D, LOW);
	digitalWrite(PIN_5D, LOW);
	digitalWrite(PIN_12S, LOW);
	digitalWrite(PIN_3S, LOW);
	digitalWrite(PIN_45S, LOW);
}

int packetIndex = 0;
int packetLength = 0;
int packetLengthIndex = 0;
int lengthCounter = 0;
int counter = 0;
int txPacketCount = 0;

//explicitly written debuggling lines are very helpful for finding out the content of the packet
uint8_t *testPacket;
uint8_t *packetPayload;
uint8_t *packet = new uint8_t[220];

bool readingPacket = false;
bool isArmed = false;

char triggerWord[8] = "covfefe";
char payloadWord[8];

void setup()
{
	Serial.begin(9600);
	Serial1.begin(9600);

	packetPayload = new uint8_t[300];

	packetPayload[0] = 0x63; //c
	packetPayload[1] = 0x6F; //o
	packetPayload[2] = 0x76; //v
	packetPayload[3] = 0x66; //f
	packetPayload[4] = 0x65; //e
	packetPayload[5] = 0x66; //f
	packetPayload[6] = 0x65; //e
	packetPayload[7] = 0xEE;

	pinMode(PIN_1D, OUTPUT);
	pinMode(PIN_2D, OUTPUT);
	pinMode(PIN_3D, OUTPUT);
	pinMode(PIN_4D, OUTPUT);
	pinMode(PIN_5D, OUTPUT);
	pinMode(PIN_12S, OUTPUT);
	pinMode(PIN_3S, OUTPUT);
	pinMode(PIN_45S, OUTPUT);

	//something in this commented out block causes the thing not to run properly
	//      pinMode(DAC0, OUTPUT);
	//      pinMode(PIN_LEDGATE, OUTPUT);
	//      pinMode(PIN_ROV, OUTPUT);
	//
	//      pinMode(PIN_1P, INPUT);
	//      pinMode(PIN_1N, INPUT);
	//      pinMode(PIN_2P, INPUT);
	//      pinMode(PIN_2N, INPUT);
	//      pinMode(PIN_3P, INPUT);
	//      pinMode(PIN_3N, INPUT);
	//      pinMode(PIN_4P, INPUT);
	//      pinMode(PIN_4N, INPUT);
	//      pinMode(PIN_5P, INPUT);
	//      pinMode(PIN_5N, INPUT);
	//      pinMode(PIN_SW, INPUT);
	//      pinMode(PIN_CUR, INPUT);
	//
	//      pinMode(PIN_PP, INPUT);
	//      pinMode(PIN_PN, INPUT);

	disarmAll();
}

void loop()
{
	//0013A2004178E2EC This is the address of Jerry the PLEC transceiver
	digitalWrite(13, HIGH);

	//RX Code
	//if data to be read from serial port
	uint8_t string[200];
	uint8_t lengthBytes[2];
	uint8_t currentByte = 0xEE;
	int counter = 0;
	bool alreadyRead = false;
	uint8_t inbyte = 0x00;
	uint8_t payload[100];
	uint8_t ATUaddress_1[8] = {0x00, 0x13, 0xA2, 0x00, 0x41, 0x64, 0x5B};
	char ATU_1_Name[5] = "rick";
	uint8_t ATUaddress_2[8] = {0x00, 0x13, 0xA2, 0x00, 0x41, 0x78, 0xE2}; //013A204178E2

	char ATU_2_Name[7] = "summer";
	bool ATU_1_identifier = true;
	bool ATU_2_identifier = true;
	bool ejectionTrigger = false;
	bool completePacket = false;

	//Serial.println("before receiving");
	if (Serial1.available())
	{
		Serial.println("serial 1 available");

		inbyte = readPacketByte();
		//Serial.println(inbyte, HEX);
		if (inbyte == 0x7E)
		{
			packetIndex = 1;
			string[0] = inbyte;
			alreadyRead = true;
			readingPacket = true;
			//Serial.println("found a packet");
		}
		if (packetIndex == 1 && !alreadyRead)
		{
			string[1] = inbyte;
			packetIndex = 2;
			packetLength = string[1];
			alreadyRead = true;
		}
		if (packetIndex == 2 && !alreadyRead)
		{

			string[2] = inbyte;
			packetLength += string[2];
			packetIndex = 3;
			alreadyRead = true;
			//          Serial.print("length of packet is: ");
			//          Serial.println(packetLength);
		}

		if ((packetIndex > 2) && (packetLengthIndex <= packetLength + 3) && !alreadyRead)
		{

			string[packetIndex] = inbyte;
			//Serial.print("current payload packet index: ");
			//Serial.println(packetIndex);
			packetLengthIndex += 1;
			packetIndex += 1;
			alreadyRead = true;
		}
		if ((packetIndex == (packetLength + 3)) && packetIndex > 2)
		{
			//end of packet
			//Serial.print("Packet: ");
			for (int l = 0; l < packetLength + 3; l++)
			{
				if (l > 14)
				{
					payload[l - 15] = string[l];
					if (l == packetLength + 2)
					{
						payload[l - 14] = 0xEE;
					}
				}
				//Serial.print((char)string[l]);  //commented out to make the ground station work
				readingPacket = false;
			}
			//Serial.println(); //commented out to make the ground station
			packetLengthIndex = 0;
			packetIndex = 0;

			uint8_t currentWord = 0x00;
			int countycount = 0;
			Serial.print("payload: ");
			while (currentWord != 0xEE)
			{
				currentWord = payload[countycount];
				Serial.print((char)currentWord);
				if (countycount < 7)
				{
					payloadWord[countycount] = (char)currentWord;
					if (countycount == 6)
					{
						completePacket = true;
					}
				}
				countycount++;
			}

			Serial.print(',');
			//Serial.println(); //for debugging, remove for actual thing

			//Serial.print("MAC address of current transmitter: ");

			for (int y = 4; y < 11; y++)
			{
				//Serial.print(string[y], HEX);
				if (ATUaddress_1[y - 4] != string[y])
				{
					ATU_1_identifier = false;
				}

				if (ATUaddress_2[y - 4] != string[y])
				{
					ATU_2_identifier = false;
				}
			}
			if (ATU_1_identifier)
			{
				//Serial.print(ATU_1_Name); //prints the atu name
			}
			if (ATU_2_identifier)
			{
				//Serial.print(ATU_2_Name); //prints the atu name
			}
			//Serial.print('@');
			Serial.println(); //for debugging
		}
		alreadyRead = false;
	}

	if (completePacket)
	{
		Serial.print("payload word:");
		Serial.println(payloadWord);
		Serial.print("trigger word:");
		Serial.println(triggerWord);
		if (strcmp(payloadWord, triggerWord) == 0)
		{
			//eject payload
			ejectionTrigger = true;
			Serial.println("ejection trigger enabled");
			//Serial.println("PAYLOAD EJECTED");
		}
	}

	if (ejectionTrigger && !isArmed)
	{
		isArmed = true;
		arming_delay = millis();
	}
	/*
        testPacket = txRequestPacketGenerator(0x0013A200, 0x4155D78B, packetPayload);

        if(Serial1 && !readingPacket)
        {
          //Serial.print("packet: ");
          for(int a = 0; a < 40; a++)
          {
            //Serial.print(testPacket[a], HEX);
          }
          //Serial.println();
          if (txPacketCount < 1)
          //if(true)
          {
            Serial1.write(testPacket, sizeofPacketArray(testPacket));
            //Serial.println(txPacketCount);
            Serial.println("sent packet");
          }
          txPacketCount++;
        }
  */

	//Serial.println("made it to the plec stuff");

	//PLEC stuff

	//int arming_delay = 2000;
	int sequence_delay = 1000;
	int rest_time = 500;
	static int igntion_seq = 0;

	//Serial.print("current ignition_seq value: ");
	//Serial.println(igntion_seq);

	currentTime = millis();

	switch (igntion_seq)
	{
	case (1):
		// Ignite ematch 1
		Serial.println("ematch 1 ignited");
		disarmAll();
		digitalWrite(PIN_1D, HIGH);
		digitalWrite(PIN_12S, HIGH);
		break;
	case (2):
		// Ignite ematch 2
		Serial.println("ematch 2 ignited");
		disarmAll();
		digitalWrite(PIN_2D, HIGH);
		digitalWrite(PIN_12S, HIGH);
		break;
	case (3):
		// Ignite ematch 3
		Serial.println("ematch 3 ignited");
		disarmAll();
		digitalWrite(PIN_3D, HIGH);
		digitalWrite(PIN_3S, HIGH);
		break;
	case (4):
		// Ignite ematch 4
		Serial.println("ematch 4 ignited");
		disarmAll();
		digitalWrite(PIN_4D, HIGH);
		digitalWrite(PIN_45S, HIGH);
		break;
	case (5):
		// Ignite ematch 5 - this is currently not working, potentially
		// due to an electrical problem
		Serial.println("ematch 5 ignited");
		disarmAll();
		digitalWrite(PIN_5D, HIGH);
		digitalWrite(PIN_45S, HIGH);
		//isArmed = false;
		igntion_seq = 0;
		break;
	default:
		disarmAll();
	}

	if (isArmed)
	{
		//Serial.print("current time: ");
		Serial.println(currentTime);

		// Charge delay configuration
		if (currentTime > arming_delay + 7 * sequence_delay)
		{
			igntion_seq = 0;
			isArmed = false;
		}
		else if (currentTime > arming_delay + 6 * sequence_delay)
		{
			igntion_seq = 5;
		}
		else if (currentTime > arming_delay + 5 * sequence_delay + rest_time)
		{
			igntion_seq = 0;
		}
		else if (currentTime > arming_delay + 5 * sequence_delay)
		{
			igntion_seq = 4;
		}
		else if (currentTime > arming_delay + 3 * sequence_delay + rest_time)
		{
			igntion_seq = 0;
		}
		else if (currentTime > arming_delay + 3 * sequence_delay)
		{
			igntion_seq = 3;
		}
		else if (currentTime > arming_delay + 2 * sequence_delay + rest_time)
		{
			igntion_seq = 0;
		}
		else if (currentTime > arming_delay + 2 * sequence_delay)
		{
			igntion_seq = 2;
		}
		else if (currentTime > arming_delay + 1 * sequence_delay + rest_time)
		{
			igntion_seq = 0;
		}
		else if (currentTime > arming_delay + 1 * sequence_delay)
		{
			igntion_seq = 1;
		}
		else
		{
			igntion_seq = 0;
		}
		Serial.print("current ignition seq value: ");
		Serial.println(igntion_seq);
	}
}

uint8_t *txRequestPacketGenerator(uint32_t SH_Address, uint32_t SL_Address, uint8_t *payload)
{
	uint16_t checksum = 0;
	//best way to do this is just use one array with the maximum number of bytes that could be put into it and making sure that
	// the 0xEE char is placed at the end of the desired packet

	//moved up to top allocate once
	//uint8_t *packet = new uint8_t[220];

	for (int xyz = 0; xyz < 220; xyz++)
	{
		packet[xyz] = 0x00;
	}
	//Serial.print("The size of the packet is: ");
	//Serial.println(sizeof(*packet)/sizeof(uint8_t));

	//DEFAULT PACKET NEEDS TO BE BASICALLY EMPTY AND CONSTRUCTED ON THE FLY IN ORDER FOR THE ESCAPE CHARACTER DEALIO TO WORK CORRECTLY

	packet[0] = 0x7E;							   //start delimeter
	packet[1] = 0x00;							   //length MSB
	packet[2] = 0x10;							   //length LSB
	packet[3] = 0x10;							   //frame type (tx request)
	packet[4] = 0x01;							   //Frame ID
	packet[5] = ((0xFF000000 & SH_Address) >> 24); //64 bit address begin
	//    Serial.print("1st byte of SH address: ");
	//    Serial.println(packet[5], HEX);
	packet[6] = ((0x00FF0000 & SH_Address) >> 16);
	//    uint32_t asdf = ((0x00FF0000 & SH_Address));
	//    uint32_t qwer = ((0x00FF0000 & SH_Address) >> 16);
	//    Serial.print("qwer: ");
	//    Serial.println(qwer, HEX);
	//    Serial.print("2nd byte of SH address: ");
	//    Serial.println(packet[6], HEX);
	packet[7] = ((0x0000FF00 & SH_Address) >> 8);
	//    Serial.print("3rd byte of SH address: ");
	//    Serial.println(packet[7], HEX);
	packet[8] = 0x000000FF & SH_Address;
	//    Serial.print("4th byte of SH address: ");
	//    Serial.println(packet[8], HEX);
	packet[9] = ((0xFF000000 & SL_Address) >> 24);
	//    Serial.print("1st byte of SL address: ");
	//    Serial.println(packet[9], HEX);
	packet[10] = ((0x00FF0000 & SL_Address) >> 16);
	//    Serial.print("2nd byte of SL address: ");
	//    Serial.println(packet[10], HEX);
	packet[11] = ((0x0000FF00 & SL_Address) >> 8);
	//    Serial.print("3rd byte of SL address: ");
	//    Serial.println(packet[11], HEX);
	packet[12] = 0x000000FF & SL_Address;
	//    Serial.print("4th byte of SL address: ");
	//    Serial.println(packet[12], HEX);
	/*
    packet[5] = 0xFF000000 & SH_Address; //64 bit address begin
    packet[6] = 0x00FF0000 & SH_Address;
    packet[7] = 0x0000FF00 & SH_Address;
    packet[8] = 0x000000FF & SH_Address;
    packet[9] = 0xFF000000 & SL_Address;
    packet[10] = 0x00FF0000 & SL_Address;
    packet[11] = 0x0000FF00 & SL_Address;
    packet[12] = 0x000000FF & SL_Address;
    packet[13] = 0x8B;
  */

	packet[13] = 0xFF; //Reseved byte 1
	packet[14] = 0xFE; //Reserved byte 2
	packet[15] = 0x00; //broadcast radius
	packet[16] = 0x00; //transmit options
	//    packet[17] = 0x79;  //RF data - y
	////    packet[18] = 0x6F;  //RF data - o
	//    packet[19] = 0x5C;  //checksum
	//    packet[20] = 0xEE;  //This is how we are going to be able to tell how many elements are in the array

	int newArraySize = 18 + sizeofPacketArray(payload); //This may change depending on what the address of the receiver is

	/*debugging lines
    Serial.print("Size of the new array is: ");
    Serial.println(newArraySize);
  */
	//int payloadSizeDiff = sizeofPacketArray(payload) - 2;
	if (sizeofPacketArray(payload) > 1)
	{
		//
		//uint8_t *tempPacket = new uint8_t[tempArraySize];
		//packet[2] += (uint8_t)payloadSizeDiff;  //not sure if this is how the arithmetic works to add uint8_t old calculation for packet length
		/* not necessary with fixed size array
      for(int q = 0; q < 17; q++)
      {
        tempPacket[q] = packet[q];
      }
    */

		/*debugging lines
      Serial.print("The size of the payload is: ");
      Serial.println(sizeofPacketArray(payload));
    */
		uint16_t packetLength = 0xE + (uint8_t)sizeofPacketArray(payload); //calculate packet length

		/*moar debugging lines
      Serial.print("The calculated packet length is: ");
      Serial.println(packetLength, HEX);
    */

		packet[1] = 0xFF00 & packetLength; //setting MSB packet length
		packet[2] = 0x00FF & packetLength; //setting LSB packet length

		//place payload in array
		for (int w = 0; w < (newArraySize); w++)
		{
			packet[w + 17] = payload[w];
		}

		/*all of the debugging lines
      for(int i = 0; i < newArraySize; i++)
      {
        Serial.print(packet[i], HEX);
        delay(10);
      }
      Serial.println();
    */

		//calculate new checksum
		for (int e = 3; e < (newArraySize - 1); e++)
		{
			/*y u no debug
        Serial.print("checksum val: ");
        Serial.println(packet[e], HEX);
      */
			checksum += packet[e];
		}

		/* i am the debug
      Serial.print("Checksum after the summed checksum is: ");
      Serial.println(checksum, HEX);
    */
		uint8_t finalChecksum = checksum & 0xFF;

		/* live love debug
      Serial.print("Checksum after the summed checksum is anded with 0xFF is: ");
      Serial.println(finalChecksum, HEX);
    */

		finalChecksum = 0xFF - finalChecksum;

		/*so many debug
      Serial.print("The calculated checksum is: ");
      Serial.println(finalChecksum, HEX);
    */

		//frame length is also fixed based on the initial estimate

		//checksum is calculated by adding all bytes exept start delimeter and length bytes
		//  That means sum from the 4th byte to the end of the rf data
		//only keep the lowest 8 bits
		//subtract quantity from 0xFF
		//checksum is not affected by the escaped data change
		//ORDER OF ASSEMBLY
		// put the bytes in order (no escaped bits have been substituted yet)
		// calculate the checksum of these
		// swap out bits for checksum
		// SHIP IT

		/*all of the debug
      Serial.print("Size of array prior to reorganizing: ");
      Serial.println(newArraySize);
    */
		packet[newArraySize - 1] = finalChecksum;
		for (int y = 1; y < newArraySize; y++)
		{

			//double check to see what happens if the checksum contains an excape character
			if (packet[y] == 0x7E || packet[y] == 0x7D || packet[y] == 0x11 || packet[y] == 0x13)
			{
				newArraySize++;
				for (int r = (newArraySize); r > (y); r--)
				{
					//Serial.println("inf loop?");
					packet[r] = packet[r - 1];

					/*debugging the progression of the packet
            for(int i = 0; i < newArraySize; i++)
            {
              Serial.print(packet[i], HEX);
             delay(10);
            }
            Serial.println();
          */
				}
				//              Serial.print("packet at the open spot before xor: ");
				//              Serial.println(packet[y+1], HEX);
				packet[y + 1] = packet[y] ^ 0x20;
				//              Serial.print("packet at the open spot before xor: ");
				//              Serial.println(packet[y+1], HEX);
				//packet
				packet[y] = 0x7D;
			}
		}

		/*debug almost done
      //this loop prints the current value of the packet
      for(int i = 0; i < newArraySize; i++)
      {
        Serial.print(packet[i], HEX);
        delay(10);
      }
      Serial.println();

      Serial.println(newArraySize);
      Serial.println(packet[newArraySize - 1], HEX);
      Serial.println(packet[newArraySize], HEX);

    */

		//while(!zeroEscChars)
		//for each value in array check to make sure they are not an escape character
		//if they are an escape character, resize the array increasing the size by one, inserting the escape character and
		//  performing the correct math on the byte in question
		//recalculate the size of the array
		//recalculate checksum according to new algorithm found in the API mode
		//The device calculates the checksum (on all non-escaped bytes) as [0xFF - (sum of all bytes from API frame type through data payload)].

		//once that is all said and done, add the 0xEE to the last index of the array

		packet[newArraySize] = 0xEE; //final value to indicate end of array
	}

	/* debugg all done
    Serial.print("checksum location before being returned: ");
    Serial.println(packet[newArraySize -1], HEX);
    Serial.print("checksum location according to sizeofPacketArray: ");
    Serial.println(sizeofPacketArray(packet));
  */
	return packet;
}

int sizeofPacketArray(uint8_t *packett)
{
	int packetCounter = 0;
	while (packett[packetCounter] != 0xEE)
	{
		packetCounter++;
	}
	if (packett[packetCounter] == 0xEE)
	{
		return packetCounter;
	}
	else
	{
		return 9000;
	}
}

uint8_t readPacketByte()
{
	uint8_t byteUnderTest = Serial1.read();
	if (byteUnderTest == 0x7D)
	{
		while (!Serial1.available())
		{
		}
		uint8_t returnByte = Serial1.read();
		returnByte = returnByte ^ 0x20;
		return returnByte;
	}

	return byteUnderTest;
}