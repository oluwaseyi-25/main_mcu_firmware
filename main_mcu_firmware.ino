#define TEST_MODE
#include "main.h"
#include "rfid.h"


void setup() {
  Serial.begin(921600);
  while (!Serial)
    ;
  fingerPrintSerial.begin(57600, SERIAL_8N1, 15, 16);
  screenSerial.begin(921600, SERIAL_8N1, 19, 20);
  while (!fingerPrintSerial)
    ;
  checkFingerprintModule();

  mfrc522.PCD_Init();
  memset(&(key.keyByte), 0xFF, 6);
}

void loop() {
  static char read_char;
  screen_command_str = "";
  read_char = 0;
  while (screenSerial.available() > 0) {
    read_char = screenSerial.read();
    if (read_char == '\n') {
      screenSerial.flush();
      screen_command = JSON.parse(screen_command_str);
      if (JSON.typeof_(screen_command) != "undefined")
        Serial.println(JSON.stringify(
                             cmdResponseToJSON(
                               exec_cmd(screen_command)))
                         .c_str());
      break;
    }
    screen_command_str += read_char;
  }

  if (current_state == CURRENT_USER_SCREEN) {
    rfid_loop();
    fingerprint_loop();
  }
}
