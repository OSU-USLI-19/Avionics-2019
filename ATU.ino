#include <SoftwareSerial.h>

#include <SD.h>
#include <SPI.h>

SoftwareSerial gpsSerial(7,8);

//explicitly written debuggling lines are very helpful for finding out the content of the packet
uint8_t *testPacket;
uint8_t *packetPayload;
int counter = 0;
char *myData = "4348.0022,N,12038.9769,W this is a test";
char *myDataTest = new char[200]; 
uint8_t *packet = new uint8_t[220];
const int chipSelect = BUILTIN_SDCARD;
File dataFile;

const int gpsSentenceS = 80;
char sentence[gpsSentenceS];
int writeCounter;
int writeIdx;
char data[100];

void setup()
{
		//Serial.begin(9600);
    Serial1.begin(9600);
		Serial2.begin(38400);
//		while(!Serial)
//		{
//      ;
//    }
    
    packetPayload = new uint8_t[300];

    pinMode(13, OUTPUT);
    SD.begin(chipSelect);
    dataFile = SD.open("datalog.txt", FILE_WRITE);
    
    //yo
//    packetPayload[0] = 0x79;
//    packetPayload[1] = 0x6f;
//    packetPayload[2] = 0xEE;

    //lol
//    packetPayload[0] = 0x6c;
//    packetPayload[1] = 0x6F;
//    packetPayload[2] = 0x6c;
//    packetPayload[3] = 0xEE;

    //broooo
    packetPayload[0] = 0x62;
    packetPayload[1] = 0x72;
    packetPayload[2] = 0x6f;
    packetPayload[3] = 0x6f;
    packetPayload[4] = 0x6f;
    packetPayload[5] = 0x6f;
    packetPayload[6] = 0xEE;
}

void loop()
{
    digitalWrite(13, HIGH);
    static int count = 0; //MAKE SURE THIS IS STATIC, OTHERWISE IT WILL NOT FUNCTION PROPERLY.....
    int countPrev = 0;
    int commaCounter = 0;
    char index1array[7];
    int comma3Idx = 0;
    int comma6Idx = 0;

    /*************************************************
     *        GPS Reciever Code Begins Here
     ************************************************/
    if(Serial2.available())
     {
        char gpsS = Serial2.read();
        if(gpsS != '\n' && count<gpsSentenceS)
        {
          sentence[count] = gpsS;
          count++;    
          //Serial.print(gpsS);
          
        }
        else
        {
          /*
          Serial.println();
          Serial.print("This is the sentence: ");
          Serial.println(sentence);
          */
          sentence[count] = '\0';
          countPrev=count;
          count = 0;
          //delay(10);
          //Serial.println(sentence);
          if(dataFile)
          {
            if(writeCounter == 100)
            {
              //Serial.println("writing to file open close");
              dataFile.println(sentence);
              writeIdx++;
              writeCounter = 0;
              
            //  Serial.println(writeIdx);
              dataFile.close();
              dataFile = SD.open("datalog.txt", FILE_WRITE);
            }
            else
            {
              writeCounter++;
          //    Serial.println(sentence);
        //      Serial.println("writing to file");
              dataFile.println(sentence);
      //        dataFile.println("LOLOLOL");
    //          Serial.println(sentence);
            }
          }
  //        Serial.println(sentence);
          //Serial1.println(sentence); //printing to transceiver
          //Serial1.print("br");
          //displayGPS();
          
          //parsing and sending GPS data, can't we just use the getData function that already exists?
          for(int i=0; i<5; i++)
          {
            index1array[i] = sentence[i];
          }
          if( (index1array[0] == '$') && (index1array[3] == 'R'))
          {
  
            for(int k=0; k<countPrev; k++)
            {
              if(sentence[k] == ',')
              {
                commaCounter++;
              }
              if(commaCounter == 2)
              {
                comma3Idx = k;
                
              }
              if(commaCounter == 6)
              {
                comma6Idx = k; 
                
               }
  
            }
  /*
            Serial.print("Comma 3 index is: ");
            Serial.println(comma3Idx);
            Serial.print("Comma 6 index is: ");
            Serial.println(comma6Idx);
*/
  
//              for(int q= (comma3Idx+1); q < (comma6Idx+2); q++)
//              //for(int q= 0; q < (comma6Idx+2); q++)
//              {
//                data[q-(comma3Idx+1)] = sentence[q];
//                //data[q] = sentence[q];
//              }
              //data[comma6Idx+2] = '\0';
              for(int q = 0; q < 50; q++)
              {
                data[q] = sentence[q];
              }
/*
            Serial.print("My parsing results: ");
            Serial.println(data);
*/
            int asdf = 0;
            for(asdf = 0; asdf < (strlen(data) + 1); asdf++)  //was myData, modified to test the gps throughput
            {
              //delay(100);
              //Serial.println("test");
              //packetPayload[asdf] = (uint8_t)myData[asdf];
              packetPayload[asdf] = (uint8_t)data[asdf];
        //      delay(100);
        //      Serial.print("value at index ");
        //      Serial.print(asdf);
        //      Serial.print(": ");
        //      Serial.println(packetPayload[asdf], HEX);
            }
            packetPayload[asdf - 1] = 0xEE;

            testPacket = txRequestPacketGenerator(0x0013A200, 0x4155D78B, packetPayload);
            if(Serial1)
            {
              Serial1.write(testPacket, sizeofPacketArray(testPacket));
            }
            //myDataTest = displayGPS();
           // Serial.print("display GPS parsing results: ");
            //Serial.println(myDataTest);
            
            //Serial1.println(data);
            
  //          //this gets the latitude direction
  //          while(commaCounter < 3)
  //          {
  //            if(sentence[commaIdx] == ',')
  //            {
  //              commaCounter++;
  //              
  //            }
  //            comma3Idx++;
  //          }
  //          latDirection = sentence[commaIdx];
  //        
  //        Serial.println(latDirection);
  //        //Serial1.print(latDirection);
  //
  //
  //         //this gets the longitude direction
  //          while(commaCounter < 6)
  //          {
  //            if(sentence[commaIdx] == ',')
  //            {
  //              commaCounter++;
  //              
  //            }
  //            comma6Idx++;
  //          }
  //          longDirection = sentence[comma6Idx];
  //        
  //        Serial.println(longDirection);
  //        //Serial1.print(longDirection);
          
          }
        }
     }

     /*************************************************
     *        GPS Reciever Code Ends Here
     ************************************************/


     /*************************************************
     *        RF Transciever Code Begins Here
     ************************************************/
    
    //Serial.print("test");
    //Serial.println(counter);
    //delay(1000);
    //char *myData = "reeeeek"; //This was the first string used as a test
    //char *myData = "4348.0022,N,12038.9769,W this is a test";
//    for(int asdf = 0; asdf < 28; asdf++)
//    {
//      Serial.println(myData[asdf]);
//      Serial.println((uint8_t)myData[asdf], HEX);
//    }
    
    //packetPayload = (uint8_t*)myData;
   // Serial.println(strlen(myData));
    //Serial.println(packetPayload[strlen(myData) - 1], HEX);
    //delay(100);
    //packetPayload[7] = 0xEE;
    
    //Serial.println(packetPayload[strlen(myData) - 1], HEX);
    //delay(100);


    //This isn't necessary anymore despite being necessary in the first implementation
//    int asdf = 0;
//    for(asdf = 0; asdf < (strlen(myDataTest) + 1); asdf++)  //was myData, modified to test the gps throughput
//    {
//      //delay(100);
//      //Serial.println("test");
//      //packetPayload[asdf] = (uint8_t)myData[asdf];
//      packetPayload[asdf] = (uint8_t)myDataTest[asdf];
////      delay(100);
////      Serial.print("value at index ");
////      Serial.print(asdf);
////      Serial.print(": ");
////      Serial.println(packetPayload[asdf], HEX);
//    }
//    packetPayload[asdf - 1] = 0xEE;



    
    //Serial.println(packetPayload[asdf], HEX);
//    for(int poiu = 0; poiu < sizeofPacketArray(packetPayload)+1; poiu++)
//    {
//      Serial.println(packetPayload[poiu], HEX);
//    }
		//if(counter == 0)  //remember to set this to 0
		//{
        //Serial.println("test_1234");

        
				//testPacket = txRequestPacketGenerator(0x0013A200, 0x4155D78B, packetPayload);
        
        
        /*debugging lines
        Serial.print("how does this even work: ");
        Serial.println(testPacket[22], HEX);
        Serial.print("The size of the packet in the loop function is: ");
        Serial.println(sizeofPacketArray(testPacket));

        Serial.println(testPacket[sizeofPacketArray(testPacket)], HEX);
        */
        
//				for(int i = 0; i < sizeofPacketArray(testPacket); i++)
//				{
//            //Serial.println("test");
//            //Serial.print("The size of the packet in the loop function is: ");
//            //Serial.println(sizeofPacketArray(testPacket));
//            //Serial.println(sizeof(testPacket));
//            //Serial.print("Size of a uint8_t is: ");
//            //Serial.println(sizeof(uint8_t));
//            /*moar debugging lines
//            Serial.print("byte at index ");
//            Serial.print(i);
//            Serial. print(": ");
//						Serial.println(testPacket[i], HEX);
//           delay(10);
//           */
//           //counter++;
//				}


       //counter++;
       //Serial.println(counter);

        //Serial1.write(testPacket, sizeofPacketArray(testPacket));

        
        //7E01010107D33A204155D78BFFFE00796F5C
        //7E01010107D5DA204155D78BFFFE00796F5C

        //7E07D3110107D33A204155D78BFFFE006C6F6CFD
        //7E07D3110107D33A204155D78BFFFE006C6F6CFD

        //7E07D3110107D33A204155D78BFFFE006C6F6CEE
        //7E07D3110107D33A204155D78BFFFE006C6F6C
        //7E07D3110107D33A204155D78BFFFE006C6F6C




       


				
		//}

     /*************************************************
     *        RF Transciever Code Ends Here
     ************************************************/
}


uint8_t *txRequestPacketGenerator(uint32_t SH_Address, uint32_t SL_Address, uint8_t *payload)
{
    uint16_t checksum = 0;
    //best way to do this is just use one array with the maximum number of bytes that could be put into it and making sure that
    // the 0xEE char is placed at the end of the desired packet

    //moved up to top allocate once
    //uint8_t *packet = new uint8_t[220];
    
    for(int xyz = 0; xyz < 220; xyz++)
    {
        packet[xyz] = 0x00;
    }
    //Serial.print("The size of the packet is: ");
    //Serial.println(sizeof(*packet)/sizeof(uint8_t));

    //DEFAULT PACKET NEEDS TO BE BASICALLY EMPTY AND CONSTRUCTED ON THE FLY IN ORDER FOR THE ESCAPE CHARACTER DEALIO TO WORK CORRECTLY

    packet[0] = 0x7E; //start delimeter
    packet[1] = 0x00; //length MSB
    packet[2] = 0x10; //length LSB
    packet[3] = 0x10; //frame type (tx request)
    packet[4] = 0x01; //Frame ID
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
    
    packet[13] = 0xFF;  //Reseved byte 1
    packet[14] = 0xFE;  //Reserved byte 2
    packet[15] = 0x00;  //broadcast radius
    packet[16] = 0x00;  //transmit options
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
    if(sizeofPacketArray(payload) > 1)
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
      uint16_t packetLength = 0xE + (uint8_t)sizeofPacketArray(payload);  //calculate packet length
      
      /*moar debugging lines
      Serial.print("The calculated packet length is: ");
      Serial.println(packetLength, HEX);
      */
      
      packet[1] = 0xFF00 & packetLength;  //setting MSB packet length
      packet[2] = 0x00FF & packetLength;  //setting LSB packet length
      
      //place payload in array
      for(int w = 0; w < (newArraySize); w++)
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
      for(int e = 3; e < (newArraySize - 1); e++)
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
      for(int y = 1; y < newArraySize; y++)
      {
        
          //double check to see what happens if the checksum contains an excape character
          if(packet[y] == 0x7E || packet[y] == 0x7D || packet[y] == 0x11 || packet[y] == 0x13)
          {
              newArraySize++;
              for(int r = (newArraySize); r > (y); r--)
              {
                  //Serial.println("inf loop?");
                  packet[r] = packet[r-1];

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
      
      packet[newArraySize] = 0xEE;//final value to indicate end of array
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
    while(packett[packetCounter] != 0xEE)
    {
        packetCounter++;
    }
    if(packett[packetCounter] == 0xEE)
    {
        return packetCounter;   
    }
    else
    {
        return 9000;
    }
    
}

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

void getField(char* buffer, int index)
{
  int sentencePos = 0;
  int fieldPos = 0;
  int commaCount = 0;
  while (sentencePos < gpsSentenceS)
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


