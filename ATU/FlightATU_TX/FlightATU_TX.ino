/* 
 * Written by Chris Snyder, 2017/2018
 * Refactor work by Trey Elkins, 2018/2019
 * 
 * OSU USLI Avionics & Telemetry Code
*/

#include <SD.h>

// Predefine packet structures
uint8_t *bufferPacket = new uint8_t[300], *packet = new uint8_t[220];

// SD Card Setup
const int chipSelect = BUILTIN_SDCARD;
File dataFile;

const int gpsSentenceLen = 80;
char gpsSentence[gpsSentenceLen];
int writeCounter = 0;

void setup()
{
    Serial1.begin(9600); // XBee to ground station
    Serial2.begin(9600); // Receives from GPS unit

    pinMode(13, OUTPUT);
    digitalWrite(13, HIGH);

    SD.begin(chipSelect);
    dataFile = SD.open("datalog.txt", FILE_WRITE);
}

void loop()
{
    static int count = 0; // This must be a static data type?

    if (Serial2.available()) // Receive from GPS 
    {
        char gpsBuffer = Serial2.read();    // Read byte from serial input

        // If we get a non newline GPS read from the buffer and aren't at the end
        if ((gpsBuffer != '\n') && (count < gpsSentenceLen))
        {
            // Start reading serial and increment until we reach the length
            // of a GPS (NMEA) byte sentence
            gpsSentence[count] = gpsBuffer;
            count++;
        }
        else    // If we hit a newline or the end, set a null and restart our counter
        {
            gpsSentence[count] = '\0';
            count = 0;

            if (dataFile)   // If SD card file is open write data to SD card
            {
                if (writeCounter == 100)
                {
                    dataFile.println(gpsSentence);
                    writeCounter = 0;
                    dataFile.close();
                    dataFile = SD.open("datalog.txt", FILE_WRITE);
                }
                else // Otherwise increment the counter and write to file
                {
                    writeCounter++;
                    dataFile.println(gpsSentence);
                }
            }

            // Ensure that data is $GPRMC or $GNRMC
            if ((gpsSentence[0] == '$') && (gpsSentence[3] == 'R'))
            {
                for (int i = 0; i < 50; i++)
                    bufferPacket[i] = (uint8_t)gpsSentence[i];

                bufferPacket[50] = 0xEE; // Set terminator
                bufferPacket = txRequestPacketGenerator(0x0013A200, 0x4155D78B, bufferPacket);

                if (Serial1.available())    // Send over transceiver
                    Serial1.write(bufferPacket, sizeofPacketArray(bufferPacket));
            }
        }
    }
}

// Transmission Packet Generator, returns packed data... addresses are hard-coded currently
uint8_t *txRequestPacketGenerator(uint32_t SH_Address, uint32_t SL_Address, uint8_t *payload)
{
    // Best way to do this is just use one array with the maximum number of bytes that could be put into it and making sure that
    // the 0xEE char is placed at the end of the desired packet
    uint16_t checksum = 0;

    // Global initialization
    for (int x = 0; x < 220; x++)
        packet[x] = 0x00;

    // DEFAULT PACKET NEEDS TO BE BASICALLY EMPTY AND CONSTRUCTED
    // ON THE FLY IN ORDER FOR THE ESCAPE CHARACTER DEALIO TO WORK CORRECTLY

    // Can we scrunch this up into standard start bits?
    packet[0] = 0x7E; // start delimeter
    packet[1] = 0x00; // length MSB
    packet[2] = 0x10; // length LSB
    packet[3] = 0x10; // frame type (tx request)
    packet[4] = 0x01; // Frame ID

    // ECE Magic
    packet[5] = ((0xFF000000 & SH_Address) >> 24); // 64 bit address begin
    packet[6] = ((0x00FF0000 & SH_Address) >> 16);
    packet[7] = ((0x0000FF00 & SH_Address) >> 8);
    packet[8] = (0x000000FF & SH_Address);

    packet[9] = ((0xFF000000 & SL_Address) >> 24);
    packet[10] = ((0x00FF0000 & SL_Address) >> 16);
    packet[11] = ((0x0000FF00 & SL_Address) >> 8);
    packet[12] = 0x000000FF & SL_Address;

    packet[13] = 0xFF; // Reseved byte 1
    packet[14] = 0xFE; // Reserved byte 2
    packet[15] = 0x00; // broadcast radius
    packet[16] = 0x00; // transmit options
    // Other reserved bits can be injected here

    int newArraySize = 18 + sizeofPacketArray(payload); // This may change depending on what the address of the receiver is

    if (sizeofPacketArray(payload) > 1)
    {
        uint16_t packetLength = 0xE + (uint8_t)sizeofPacketArray(payload); // calculate packet length

        packet[1] = 0xFF00 & packetLength; // setting MSB packet length
        packet[2] = 0x00FF & packetLength; // setting LSB packet length

        // place payload in array
        for (int w = 0; w < (newArraySize); w++)
            packet[w + 17] = payload[w];

        // calculate new checksum
        for (int e = 3; e < (newArraySize - 1); e++)
            checksum += packet[e];

        uint8_t finalChecksum = checksum & 0xFF;
        finalChecksum = 0xFF - finalChecksum;

        //frame length is also fixed based on the initial estimate

        // checksum is calculated by adding all bytes exept start delimeter and length bytes
        // That means sum from the 4th byte to the end of the rf data
        // only keep the lowest 8 bits
        // subtract quantity from 0xFF
        // checksum is not affected by the escaped data change
        // ORDER OF ASSEMBLY
        // put the bytes in order (no escaped bits have been substituted yet)
        // calculate the checksum of these
        // swap out bits for checksum
        // SHIP IT

        packet[newArraySize - 1] = finalChecksum;

        for (int y = 1; y < newArraySize; y++)
        {
            //double check to see what happens if the checksum contains an escape character
            if (packet[y] == 0x7E || packet[y] == 0x7D || packet[y] == 0x11 || packet[y] == 0x13)
            {
                newArraySize++;

                for (int r = (newArraySize); r > (y); r--)
                    packet[r] = packet[r - 1];

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

        packet[newArraySize] = 0xEE; // add terminator to array
    }

    return packet;
}

// Simple sizeof
int sizeofPacketArray(uint8_t *packett)
{
    int packetCounter = 0;

    while (packett[packetCounter] != 0xEE)
        packetCounter++;

    if (packett[packetCounter] == 0xEE)
        return packetCounter;
    else
        return 9000; // This better not be a MEME
}

/* Final Notes:
 * 1. Can we combine the header creation somehow? Instead of like 8 lines have one that
 * feeds a complex string in?
 * 
 * 2. Can we do some data structure magic to create a class or struct for the packets 
 * rather than just using character pushes, or is that going to cause problems with
 * transmission? Might take some time to look at.
*/
