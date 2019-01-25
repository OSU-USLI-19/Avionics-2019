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

// Potentially better to initialize inside RX Stream?
int packetIndex = 0;
int packetLength = 0;
int packetLengthIndex = 0;

uint8_t ATUaddress_1[8] = {0x00, 0x13, 0xA2, 0x00, 0x41, 0x64, 0x5B};
uint8_t ATUaddress_2[8] = {0x00, 0x13, 0xA2, 0x00, 0x41, 0x78, 0xE2}; //013A204178E2

char ATU_1_Name[5] = "rick", ATU_2_Name[7] = "summer";
char transmitFlag = '0'; // Used to determine if PLEC trigger needs to be sent

uint8_t *PLECpacket, *packetPayload = new uint8_t[10];
uint8_t *packet = new uint8_t[220];

void setup()
{
    Serial.begin(9600);  // USB to PC
    Serial1.begin(9600); // Xbee
}

void loop()
{
    if (Serial1.available())
        rxStream();

    // This will be expanded into the PLEC trigger proper
    if (transmitFlag != '0')
    {
        PLECTrigger();
        //txStream(transmitFlag);
        transmitFlag = '0';
    }
}

// This needs to be changed to query from GUI for PLEC trigger
void rxStream()
{
    // Maybe rename 'string' so that we know what it does
    uint8_t string[200], payload[100];
    uint8_t inbyte = 0x00;

    bool alreadyRead = false, ATU_1_identifier = true, ATU_2_identifier = true;

    // read from serial port
    inbyte = readPacketByte();

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
    }
}

// Transmission functionality on reception of signal from GUI
void PLECTrigger()
{
    Serial.println("Transmitting PLEC Trigger");

    // 0013A2004178E2EC This is the address of Jerry the PLEC transceiver

    PLECpacket = txRequestPacketGenerator(0x0013A200, 0x4178E2EC, packetPayload); //transmit to jerry the plec
    // PLECpacket = txRequestPacketGenerator(0x0013A200, 0x4155D78B, packetPayload); //This is for transmitting to the ground station

    if (Serial1)
    {
        Serial.print("tx packet: ");
        for (int a = 0; a < 40; a++)
        {
            Serial.print(" 0x");
            Serial.print(PLECpacket[a], HEX);
        }

        Serial.println();
        Serial1.write(PLECpacket, sizeofPacketArray(PLECpacket));
        Serial.println("Transmission complete.");
    }
}

// Input from XBee serial
uint8_t readPacketByte()
{
    uint8_t byteUnderTest = Serial1.read();

    if (byteUnderTest == 0x7D)
    {
        // Does this need to be here?
        while (!Serial1.available())
        {
        } // Stalls until the serial is availabe?

        uint8_t returnByte = Serial1.read();
        returnByte = returnByte ^ 0x20;
        return returnByte;
    }

    return byteUnderTest;
}

// Transmission Packet Generator, returns packed data... addresses are hard-coded currently
uint8_t *txRequestPacketGenerator(uint32_t SH_Address, uint32_t SL_Address, uint8_t *payload)
{
    // Best way to do this is just use one array with the maximum number of bytes that could be put into it and making sure that
    // the 0xEE char is placed at the end of the desired packet
    uint16_t checksum = 0;

    // Initialization was done globally - is that alright?
    for (int x = 0; x < 220; x++)
        packet[x] = 0x00;

    // Serial.print("The size of the packet is: ");
    // Serial.println(sizeof(*packet)/sizeof(uint8_t));

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
    // Other reserved bits could be injected here

    int newArraySize = 18 + sizeofPacketArray(payload); // This may change depending on what the address of the receiver is

    /* debugging lines
     Serial.print("Size of the new array is: ");
     Serial.println(newArraySize);
    */

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

        // frame length is also fixed based on the initial estimate

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

        for (int y = 1; y < newArraySize; y++)
        {
            //double check to see what happens if the checksum contains an excape character
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

        //once that is all said and done, add the 0xEE to the last index of the array
        packet[newArraySize] = 0xEE; //final value to indicate end of array
    }

    // Do we need some kind of 'else' failure mechanism here?
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