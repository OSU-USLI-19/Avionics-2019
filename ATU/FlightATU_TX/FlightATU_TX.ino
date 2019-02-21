/* 
 * Written by Chris Snyder, 2017/2018
 * Refactor work by Trey Elkins, 2018
 * 
 * OSU USLI Avionics & Telemetry Code
 *
 * PLEASE CHECK PREVIOUS COMMITS TO SEE DEBUGGING ATTEMPTS AND PROBLEM SPOTS.
 * SIGNIFICANT AMOUNTS OF CODE HAVE BEEN REMOVED FOR CLARITY.
 *
 */

//#include <SoftwareSerial.h>		...fairly certain this can be removed

// Libraries for SD card and Serial Peripheral Interface (SPI)
#include <SD.h>
#include <SPI.h>

// Predefine necessary packet structures
uint8_t *payloadPacket, *bufferPacket;
uint8_t *packet = new uint8_t[220];

// SD Card Setup
const int chipSelect = BUILTIN_SDCARD;
File dataFile;

const int gpsSentenceLen = 80;
char sentence[gpsSentenceLen], data[100];
int writeCounter = 0, writeIdx = 0;	// <--- If everything breaks, it's because of this

// Initial setup
void setup()
{
   Serial1.begin(9600);	// Sends to ground station, this needs to be raised
   Serial2.begin(9600);	// Receives from GPS unit

   // This isn't dynamic because memory leaks will goof the entire thing
   bufferPacket = new uint8_t[300];

   pinMode(13, OUTPUT);
   SD.begin(chipSelect);
   dataFile = SD.open("datalog.txt", FILE_WRITE);

   /* Demo of how to write to the bufferPacket: "broooo" [followed by terminator?]
      bufferPacket[0] = 0x62;
      bufferPacket[1] = 0x72;
      bufferPacket[2] = 0x6f;
      bufferPacket[3] = 0x6f;
      bufferPacket[4] = 0x6f;
      bufferPacket[5] = 0x6f;
      bufferPacket[6] = 0xEE; <-- This is our null terminator for the protocol */
}

// Main loop, will continue to run as long as microcontroller is active

// Could we write to sentence and the sending packet simultaneously? Could we write the
// packet to the SD card instead?
void loop()
{
   digitalWrite(13, HIGH);

   //MAKE SURE THIS IS STATIC, OTHERWISE IT WILL NOT FUNCTION PROPERLY.....
   static int count = 0; 

   int countPrev = 0, commaCounter = 0, comma3Idx = 0, comma6Idx = 0;
   char index1array[7];

   /*************************************************
    *        GPS Receiver Code Begins Here
    *************************************************/
   if(Serial2.available())
   {
      char gpsBuffer = Serial2.read();

      // If we get a non newline GPS read from the buffer and aren't at the end
      if((gpsBuffer != '\n') && (count < gpsSentenceLen))
      {
	 // Start reading serial and increment until we reach the length 
	 // of a GPS (NMEA) byte sentence
	 sentence[count] = gpsBuffer;
	 count++;    
      }
      else
      {
	 // If we hit a newline or the end, set a null and restart our counter
	 sentence[count] = '\0';
	 countPrev = count;
	 count = 0;

	 // If SD card file is open
	 if(dataFile)
	 {
	    // If we're at the end of a GPS sentence, write to file
	    // and increment the write (line?) index
	    if(writeCounter == 100)
	    {
	       dataFile.println(sentence);
	       writeIdx++;
	       writeCounter = 0;

	       dataFile.close();
	       dataFile = SD.open("datalog.txt", FILE_WRITE);
	    }
	    else // Otherwise increment the counter and write to file
	    {
	       writeCounter++;;
	       dataFile.println(sentence);
	    }
	 }

	 //parsing and sending GPS data, can't we just use the getData function that already exists?
	 for(int i=0; i<5; i++)
	    index1array[i] = sentence[i];

	 if( (index1array[0] == '$') && (index1array[3] == 'R'))
	 {
	    for(int k=0; k < countPrev; k++)
	    {
	       // Delineate sentence based on commas - that's how the packets are segmented
	       if(sentence[k] == ',')
			   commaCounter++;

	       if(commaCounter == 2)
			   comma3Idx = k;

	       if(commaCounter == 6)
			   comma6Idx = k; 
	    }


	    /*
	       Serial.print("Comma 3 index is: ");
	       Serial.println(comma3Idx);
	       Serial.print("Comma 6 index is: ");
	       Serial.println(comma6Idx);
	       */

	    // Parsing into new buffer
	    for(int q = 0; q < 50; q++)
	       data[q] = sentence[q];

	    int i = 0;
	    //was myData, modified to test the gps throughput
	    for(i = 0; i < (strlen(data) + 1); i++)  
	       bufferPacket[i] = (uint8_t)data[i];

	    bufferPacket[i - 1] = 0xEE;	// Set the terminator

	    // packet-up ;D 
	    payloadPacket = txRequestPacketGenerator(0x0013A200, 0x4155D78B, bufferPacket);

	    // and send it over the transceiver
	    if(Serial1)
	       Serial1.write(payloadPacket, sizeofPacketArray(payloadPacket));
	 }
      }
   }

   /***********************************************
    *        GPS Receiver Code Ends Here
    ************************************************/


   /***********************************************
    *        RF Transceiver Code Begins Here
    ************************************************/

   // Nothing yet - 2019 team let's gooooo

   /***********************************************
    *        RF Transceiver Code Ends Here
    ************************************************/
}

// Transmission Packet Generator, returns packed data... addresses are hard-coded currently
uint8_t *txRequestPacketGenerator(uint32_t SH_Address, uint32_t SL_Address, uint8_t *payload)
{
   // Best way to do this is just use one array with the maximum number of bytes that could be put into it and making sure that
   // the 0xEE char is placed at the end of the desired packet
   uint16_t checksum = 0;

   // Initialization was done globally - is that alright?
   for(int x = 0; x < 220; x++)
      packet[x] = 0x00;

   // Serial.print("The size of the packet is: ");
   // Serial.println(sizeof(*packet)/sizeof(uint8_t));

   // DEFAULT PACKET NEEDS TO BE BASICALLY EMPTY AND CONSTRUCTED 
   // ON THE FLY IN ORDER FOR THE ESCAPE CHARACTER DEALIO TO WORK CORRECTLY

   // Can we scrunch this up into standard start bits?
   packet[0] = 0x7E; //start delimeter
   packet[1] = 0x00; //length MSB
   packet[2] = 0x10; //length LSB
   packet[3] = 0x10; //frame type (tx request)
   packet[4] = 0x01; //Frame ID

   // ECE Magic
   packet[5] = ((0xFF000000 & SH_Address) >> 24); //64 bit address begin
   packet[6] = ((0x00FF0000 & SH_Address) >> 16);
   packet[7] = ((0x0000FF00 & SH_Address) >> 8);
   packet[8] = (0x000000FF & SH_Address);

   packet[9] = ((0xFF000000 & SL_Address) >> 24);
   packet[10] = ((0x00FF0000 & SL_Address) >> 16);
   packet[11] = ((0x0000FF00 & SL_Address) >> 8);
   packet[12] = 0x000000FF & SL_Address;

   packet[13] = 0xFF;  //Reseved byte 1
   packet[14] = 0xFE;  //Reserved byte 2
   packet[15] = 0x00;  //broadcast radius
   packet[16] = 0x00;  //transmit options
   // Other reserved bits could be injected here

   int newArraySize = 18 + sizeofPacketArray(payload); //This may change depending on what the address of the receiver is

   /*debugging lines
     Serial.print("Size of the new array is: ");
     Serial.println(newArraySize);
     */

   if(sizeofPacketArray(payload) > 1)
   {
      uint16_t packetLength = 0xE + (uint8_t)sizeofPacketArray(payload);  //calculate packet length

      packet[1] = 0xFF00 & packetLength;  //setting MSB packet length
      packet[2] = 0x00FF & packetLength;  //setting LSB packet length

      // place payload in array
      for(int w = 0; w < (newArraySize); w++)
	 packet[w + 17] = payload[w]; 

      //calculate new checksum
      for(int e = 3; e < (newArraySize - 1); e++)
	 checksum += packet[e];

      uint8_t finalChecksum = checksum & 0xFF;
      finalChecksum = 0xFF - finalChecksum;

      //frame length is also fixed based on the initial estimate

      // checksum is calculated by adding all bytes exept start delimeter and length bytes
      //  That means sum from the 4th byte to the end of the rf data
      // only keep the lowest 8 bits
      // subtract quantity from 0xFF
      // checksum is not affected by the escaped data change
      // ORDER OF ASSEMBLY
      // put the bytes in order (no escaped bits have been substituted yet)
      // calculate the checksum of these
      // swap out bits for checksum
      // SHIP IT

      packet[newArraySize - 1] = finalChecksum;

      for(int y = 1; y < newArraySize; y++)
      {
	 //double check to see what happens if the checksum contains an excape character
	 if(packet[y] == 0x7E || packet[y] == 0x7D || packet[y] == 0x11 || packet[y] == 0x13)
	 {
	    newArraySize++;

	    for(int r = (newArraySize); r > (y); r--)
	       packet[r] = packet[r-1];

	    packet[y + 1] = packet[y] ^ 0x20;
	    packet[y] = 0x7D;
	 }
      }

      // while(!zeroEscChars)
      // for each value in array check to make sure they are not an escape character
      // if they are an escape character, resize the array increasing the size by one, inserting the escape character and
      // performing the correct math on the byte in question
      // recalculate the size of the array
      // recalculate checksum according to new algorithm found in the API mode
      // The device calculates the checksum (on all non-escaped bytes) as [0xFF - (sum of all bytes from API frame type through data payload)].

      //once that is all said and done, add the 0xEE to the last index of the array
      packet[newArraySize] = 0xEE;//final value to indicate end of array
   }

   // Do we need some kind of 'else' failure mechanism here?
   return packet;
}

// Simple sizeof
int sizeofPacketArray(uint8_t *packett)
{
   int packetCounter = 0;

   while(packett[packetCounter] != 0xEE)
      packetCounter++;

   if(packett[packetCounter] == 0xEE)
      return packetCounter;   
   else
      return 9000;	// This better not be a MEME
}

// Debugging method, displays data from GPS (NMEA) packet
char *displayGPS()
{
   char field[20];
   String stringyMcStringface;
   //Serial.print(" ");
   getField(field,0);//checks to see what data line is being received
   if(strcmp(field, "$GPRMC") == 0)
   {
      //Serial.print("Lat: ");
      stringyMcStringface += "Lat: ";
      getField(field, 3); //latitude is the third part of the data sentence, so it is stored in field
      //Serial.print(field);
      stringyMcStringface+= field;
      getField(field, 4); //N/S
      //Serial.print(field);
      stringyMcStringface+= field;

      //Serial.print(" Long: ");
      stringyMcStringface+= "Long: ";
      getField(field, 5);
      //Serial.print(field);
      stringyMcStringface+= field;
      getField(field, 6);
      //Serial.print(field);
      stringyMcStringface+= field;
      stringyMcStringface.toCharArray(field, 20);
   }
   if(strcmp(field, "$GPGGA") == 0)
   {
      Serial.print(" Alt: ");
      getField(field, 9);
      Serial.println(field);
   } 
   return field;
}

// Submethod for displayGPS()
void getField(char* buffer, int index)
{
   int sentencePos = 0;
   int fieldPos = 0;
   int commaCount = 0;
   while (sentencePos < gpsSentenceLen)
   {
      if (sentence[sentencePos] == ',')
      {
	 commaCount ++;
	 sentencePos ++;
      }

      if (commaCount == index)
      {
	 buffer[fieldPos] = sentence[sentencePos];
	 fieldPos ++;
      }

      sentencePos ++;
   }

   buffer[fieldPos] = '\0';
} 

/* Final Notes:
 * 1. Could we write to data and the output stream simultaneously? Create the packets
 * and then send and write based on a flag rather than writing them all and THEN copying
 * one in ten packets?
 *
 * 2. Can we combine the header creation somehow? Instead of like 8 lines have one that
 * feeds a complex string in?
 * 
 * 3. Can we do some data structure magic to create a class or struct for the packets 
 * rather than just using character pushes, or is that going to cause problems with
 * transmission? Might take some time to look at.
 *
 * 4. Default testing accessors? Feed it values and it spits to serial until it runs out
 * of stuff to spit? Makes it easier to debug...
 *
 */
