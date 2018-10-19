/*
	Based on script by Søren Thing Andersen (access.thing.dk),
	Dr.Leong and Miguel Balboa for RFID MODULE KIT 13.56 MHZ
	Module: MF-RC522

	Code was integrated and converted to work on ESP-8266 Hardware
	with an ST7735 LCD display
*/

/*
	arduino hardware library for ESP based boards
	https://github.com/esp8266/Arduino
*/
#include <ESP8266WiFi.h>

/*
	necessary for display and rfid module
*/
#include <SPI.h>

/*
	Core graphics library
	https://github.com/adafruit/Adafruit-GFX-Library
*/
#include <Adafruit_GFX.h>

/*
	Hardware-specific library
	https://github.com/norm8332/ST7735_ESP8266
*/
#include <ESP_ST7735.h>

/*
	rfid module lbrary
	https://github.com/miguelbalboa/rfid
*/
#include <MFRC522.h>

/*
	12, 13, 14 are used for SPI,
	TFT_RST could be freed by connecting it to the ESP reset pin
*/
#define BTN_UP       0
#define BTN_DN       3
#define RFID_SS      5
#define RFID_RST     4
#define TFT_CS       2
#define TFT_DC      16
#define TFT_RST     15

MFRC522 mfrc522(RFID_SS, RFID_RST);
Adafruit_ST7735 tft_display = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

uint32_t id;
byte block;
byte len;
byte status;

int displayIndex = 0; // scroll index for lines
int displaySteps = 5; // scroll amount for lines
byte rfidData[4096]; // data buffer for card

void setup()
{
	// Init serial connection
	Serial.begin(115200);
	
	// Init SPI bus
	SPI.begin();
	SPI.setFrequency(3000000);
	
	// Init display
	tft_display.initR(INITR_BLACKTAB);

	tft_display.fillScreen(ST7735_BLACK);
	tft_display.setCursor(0, 0);
	tft_display.setRotation(3);  
	tft_display.println("davedarko -ESP RFID- beta");

	mfrc522.PCD_Init(); 
	mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);

	analyzeReader();
}

void loop()
{

	readButtonsAndUpdateDisplayOnChange();

	// Look for new cards
	if (!mfrc522.PICC_IsNewCardPresent())
	{
		return;
	}

	// Select one of the cards
	if (!mfrc522.PICC_ReadCardSerial())
	{
		return;
	}

	MFRC522::MIFARE_Key key;
	for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
	
	id = 0;
	// Dump debug info about the card. PICC_HaltA() is automatically called.

	for (int i = 3; i >= 0; i--)
	{
		id = id + (int) mfrc522.uid.uidByte[i];
		if (i != 0) id = id << 8;
	}

	tft_display.setCursor(0, 0);
	tft_display.fillScreen(ST7735_BLACK);
	tft_display.println(id);

	dumpToDisplay(&(mfrc522.uid));
	char textToWrite[ 16 ];
	sprintf(textToWrite, "%lu", id);
}

void readButtonsAndUpdateDisplayOnChange()
{
	pinMode(BTN_UP, INPUT_PULLUP);
	pinMode(BTN_DN, INPUT_PULLUP);

	if (digitalRead(BTN_DN) == LOW)
	{
		displayIndex -= displaySteps;
		if (displayIndex<0) displayIndex = 0;
		displayStoredRfidInfos();
	}

	if (digitalRead(BTN_UP) == LOW)
	{
		displayIndex += displaySteps;
		displayStoredRfidInfos();
	}
}

void analyzeReader()
{
	// Get the MFRC522 software version
	byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
	tft_display.print(F("MFRC522 Software Version: 0x"));
	tft_display.print(v, HEX);
	if (v == 0x91)
	{
		tft_display.print(F(" = v1.0"));
	}
	else if (v == 0x92)
	{
		tft_display.print(F(" = v2.0"));
	}
	else
	{
		tft_display.print(F(" (unknown)"));
	}
	tft_display.println("");
	
	// When 0x00 or 0xFF is returned, communication probably failed
	if ((v == 0x00) || (v == 0xFF))
	{
		tft_display.println(F("WARNING: Communication failure, is the MFRC522 properly connected?"));
	}
}

void displayStoredRfidInfos()
{
	tft_display.fillScreen(ST7735_BLACK);
	tft_display.setCursor(0, 0);

	for (int i = displayIndex; i < displayIndex + 19; i++)
	{
		tft_display.setTextColor(ST7735_YELLOW);
		int j = i * 5;
		if (j < 1000) tft_display.print("0");
		if (j < 100) tft_display.print("0");
		if (j < 10) tft_display.print("0");
		tft_display.print(j);
		tft_display.setTextColor(ST7735_WHITE);
		tft_display.print("|");
		tft_display.setTextColor(ST7735_GREEN);

		tft_display.print(char(rfidData[j+0]));
		tft_display.print(char(rfidData[j+1]));
		tft_display.print(char(rfidData[j+2]));
		tft_display.print(char(rfidData[j+3]));
		tft_display.print(char(rfidData[j+4]));

		tft_display.setTextColor(ST7735_WHITE);

		macro_tft_print_hex(rfidData[j+0]);
		macro_tft_print_hex(rfidData[j+1]);
		macro_tft_print_hex(rfidData[j+2]);
		macro_tft_print_hex(rfidData[j+3]);
		macro_tft_print_hex(rfidData[j+4]);
		tft_display.print("|");
		tft_display.println();
	}
}

void macro_tft_print_hex(byte b)
{
	tft_display.print('|');
	if(b < 0x10)
	{
		tft_display.print('0');
	}
	tft_display.print(b, HEX);
}

void show_error()
{
	 tft_display.fillScreen(ST7735_BLACK);
	 tft_display.setCursor(0, 0);
	 tft_display.setTextColor(ST7735_RED);
	 tft_display.print(F("Dumping memory contents not implemented for that PICC type."));
}

void PICC_DumpMifareClassicSectorToBuffer(
	MFRC522::Uid *uid,     ///< Pointer to Uid struct returned from a successful PICC_Select().
	MFRC522::MIFARE_Key *key,  ///< Key A for the sector.
	byte sector      ///< The sector to dump, 0..39.
) {
	byte status;
	byte firstBlock;    // Address of lowest address to dump actually last block dumped)
	byte no_of_blocks;    // Number of blocks in sector
	bool isSectorTrailer; // Set to true while handling the "last" (ie highest address) in the sector.

	// The access bits are stored in a peculiar fashion.
	// There are four groups:
	//    g[3]  Access bits for the sector trailer, block 3 (for sectors 0-31) or block 15 (for sectors 32-39)
	//    g[2]  Access bits for block 2 (for sectors 0-31) or blocks 10-14 (for sectors 32-39)
	//    g[1]  Access bits for block 1 (for sectors 0-31) or blocks 5-9 (for sectors 32-39)
	//    g[0]  Access bits for block 0 (for sectors 0-31) or blocks 0-4 (for sectors 32-39)
	// Each group has access bits [C1 C2 C3]. In this code C1 is MSB and C3 is LSB.
	// The four CX bits are stored together in a nible cx and an inverted nible cx_.
	byte c1, c2, c3;    // Nibbles
	byte c1_, c2_, c3_;   // Inverted nibbles
	bool invertedError;   // True if one of the inverted nibbles did not match
	byte g[4];        // Access bits for each of the four groups.
	byte group;       // 0-3 - active group for access bits

	// Determine position and size of sector.
	if (sector < 32)
	{
		// Sectors 0..31 has 4 blocks each
		no_of_blocks = 4;
		firstBlock = sector * no_of_blocks;
	}
	else if (sector < 40)
	{
		// Sectors 32-39 has 16 blocks each
		no_of_blocks = 16;
		firstBlock = 128 + (sector - 32) * no_of_blocks;
	}
	else
	{
		// Illegal input, no MIFARE Classic PICC has more than 40 sectors.
		return;
	}

	// Dump blocks, highest address first.
	byte byteCount;
	byte buffer[18];
	byte blockAddr;
	isSectorTrailer = true;
	for (int8_t blockOffset = no_of_blocks - 1; blockOffset >= 0; blockOffset--)
	{
		blockAddr = firstBlock + blockOffset;

		if (isSectorTrailer)
		{
			status = mfrc522.PCD_Authenticate(mfrc522.PICC_CMD_MF_AUTH_KEY_A, firstBlock, key, uid);

			if (status != MFRC522::STATUS_OK)
			{
				Serial.print(F("PCD_Authenticate() failed: "));
				Serial.println(mfrc522.GetStatusCodeName(status));
				return;
			}
		}
		// Read block
		byteCount = sizeof(buffer);
		status = mfrc522.MIFARE_Read(blockAddr, buffer, &byteCount);
		if (status != MFRC522::STATUS_OK)
		{
			Serial.print(F("MIFARE_Read() failed: "));
			Serial.println(mfrc522.GetStatusCodeName(status));
			continue;
		}

		// Dump data
		for (byte index = 0; index < 16; index++)
		{
			rfidData[blockAddr*16+index] = buffer[index];

			Serial.print(blockAddr);
			Serial.print("*16 + ");
			Serial.print(index);
			Serial.print(" = ");
			Serial.print(blockAddr*16+index);
			Serial.print(": ");
			Serial.print(buffer[index], HEX);
			Serial.println();
		}
		// Parse sector trailer data
		if (isSectorTrailer)
		{
			c1  = buffer[7] >> 4;
			c2  = buffer[8] & 0xF;
			c3  = buffer[8] >> 4;
			c1_ = buffer[6] & 0xF;
			c2_ = buffer[6] >> 4;
			c3_ = buffer[7] & 0xF;
			invertedError = (c1 != (~c1_ & 0xF)) || (c2 != (~c2_ & 0xF)) || (c3 != (~c3_ & 0xF));
			g[0] = ((c1 & 1) << 2) | ((c2 & 1) << 1) | ((c3 & 1) << 0);
			g[1] = ((c1 & 2) << 1) | ((c2 & 2) << 0) | ((c3 & 2) >> 1);
			g[2] = ((c1 & 4) << 0) | ((c2 & 4) >> 1) | ((c3 & 4) >> 2);
			g[3] = ((c1 & 8) >> 1) | ((c2 & 8) >> 2) | ((c3 & 8) >> 3);
			isSectorTrailer = false;
		}

		Serial.println();
	}

	return;
}

/**
* Dumps memory contents of a MIFARE Classic PICC.
* On success the PICC is halted after dumping the data.
*/
void PICC_DumpMifareClassicToDisplay(  
	MFRC522::Uid *uid,   ///< Pointer to Uid struct returned from a successful PICC_Select().
	byte piccType  ///< One of the PICC_Type enums.
) {

	MFRC522::MIFARE_Key key;
	// All keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
	for (byte i = 0; i < 6; i++) {
		key.keyByte[i] = 0xFF;
	}

	byte no_of_sectors = 0;
	switch (piccType)
	{
		case MFRC522::PICC_TYPE_MIFARE_MINI:
			// Has 5 sectors * 4 blocks/sector * 16 bytes/block = 320 bytes.
			no_of_sectors = 5;
			break;

		case MFRC522::PICC_TYPE_MIFARE_1K:
			// Has 16 sectors * 4 blocks/sector * 16 bytes/block = 1024 bytes.
			no_of_sectors = 16;
			break;

		case MFRC522::PICC_TYPE_MIFARE_4K:
			// Has (32 sectors * 4 blocks/sector + 8 sectors * 16 blocks/sector) * 16 bytes/block = 4096 bytes.
			no_of_sectors = 40;
			break;

		default: // Should not happen. Ignore.
			break;
	}

	// Dump sectors, highest address first.
	if (no_of_sectors)
	{
		// Serial.println(F("Sector Block   0  1  2  3   4  5  6  7   8  9 10 11  12 13 14 15  AccessBits"));
		for (int8_t i = 0; i < no_of_sectors; i++)
		{
			PICC_DumpMifareClassicSectorToBuffer(uid, &key, i);
		}
	}
	mfrc522.PICC_HaltA(); // Halt the PICC before stopping the encrypted session.
	mfrc522.PCD_StopCrypto1();
} // End PICC_DumpMifareClassicToSerial()



void dumpToDisplay(MFRC522::Uid *uid)
{
	byte piccType = mfrc522.PICC_GetType(uid->sak);
	
	tft_display.print(F("PICC type: "));
	tft_display.println(mfrc522.PICC_GetTypeName(piccType));

	// Dump contents
	switch (piccType)
	{
		case MFRC522::PICC_TYPE_MIFARE_MINI:
		case MFRC522::PICC_TYPE_MIFARE_1K:
		case MFRC522::PICC_TYPE_MIFARE_4K:
			PICC_DumpMifareClassicToDisplay(uid, piccType);
			delay(2000);
			displayStoredRfidInfos();
			break;
			
		case MFRC522::PICC_TYPE_MIFARE_UL:
			dumpMifareUltralightToBigBuffer();
			delay(2000);
			displayStoredRfidInfos();
			break;
			
		case MFRC522::PICC_TYPE_ISO_14443_4:
			Serial.println("PICC_TYPE_ISO_14443_4");
		case MFRC522::PICC_TYPE_ISO_18092:
			Serial.println("PICC_TYPE_ISO_18092");
		case MFRC522::PICC_TYPE_MIFARE_PLUS:
			Serial.println("PICC_TYPE_MIFARE_PLUS");
		case MFRC522::PICC_TYPE_TNP3XXX:
			Serial.println("PICC_TYPE_TNP3XXX");
			delay(2000);
			show_error();
			break;
			
		case MFRC522::PICC_TYPE_UNKNOWN:
			Serial.println("PICC_TYPE_UNKNOWN");
		case MFRC522::PICC_TYPE_NOT_COMPLETE:
			Serial.println("PICC_TYPE_NOT_COMPLETE");
		default:
			delay(2000);
			show_error();
			break; // No memory dump here
	}
	mfrc522.PICC_HaltA(); // Already done if it was a MIFARE Classic PICC.
}

void dumpMifareUltralightToBigBuffer() 
{
	byte status;
	byte byteCount;
	byte buffer[18];
	byte i;
	int position_overall = 0;

	for (byte page = 0; page < 16; page += 4) 
	{ // Read returns data for 4 pages at a time.
		// Read pages
		byteCount = sizeof(buffer);

		status = mfrc522.MIFARE_Read(page, buffer, &byteCount);    
		if (status != MFRC522::STATUS_OK) 
		{
			break;
		}
		for (byte offset = 0; offset < 4; offset++) 
		{
			for (byte index = 0; index < 4; index++) 
			{
				i = 4 * offset + index;
				position_overall = page * 4 + i;
				rfidData[position_overall] = buffer[i];
			}
		}
	}
} // End PICC_DumpMifareUltralightToSerial()
