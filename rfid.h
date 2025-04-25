#define isTrailerBlock(n) ((n + 1) % 4) == 0
#define SDA_PIN 5

#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>

MFRC522DriverPinSimple ss_pin(SDA_PIN);
MFRC522DriverSPI driver{ ss_pin };  // Create SPI driver
MFRC522 mfrc522{ driver };          // Create MFRC522 instance
MFRC522::MIFARE_Key key;

byte buffer[18];  //data transfer buffer (16+2 bytes data+CRC)
byte size = sizeof(buffer);

uint8_t pageAddr = 0x06;  //In this example we will write/read 16 bytes (page 6,7,8 and 9).
                          //Ultraligth mem = 16 pages. 4 bytes per page.
                          //Pages 0 to 4 are for special functions.