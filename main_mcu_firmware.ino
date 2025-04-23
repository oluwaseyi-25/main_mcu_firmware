#define TEST_MODE
#define ERROR_LOGGING
#include "main.h"
#include "rfid.h"


void setup() {
  // Boot delay
  for (int i = 0; i < 5; i++) {
    LOGF("[BOOT] %u...\n", i);
    delay(1000);
  }

  Serial.begin(921600);
  while (!Serial)
    ;
  fingerPrintSerial.begin(57600, SERIAL_8N1, 15, 16);
  screenSerial.begin(921600, SERIAL_8N1, 19, 20);
  while (!fingerPrintSerial)
    ;
  checkFingerprintModule();

  // Initialize SPIFFS
  SPIFFS.begin(true);
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully.");

  // Load configuration from SPIFFS
  if (!loadConfig()) {
    Serial.println("Failed to load configuration. Using default values.");
    ssid = "MTN_4G_48437C";
    password = "Pelumi0209";
  }

  if (connect_to_network())
    Serial.printf("Connected to WiFi. IP address: %s\n", WiFi.localIP().toString().c_str());

  webSocket.begin("192.168.0.200", 5000, "/command");
  Serial.println("Connecting to WebSocket server...");
  while (!webSocket.isConnected()) {
    Serial.print(".");
    webSocket.loop();
    delay(500);
  }
  Serial.println("\nConnected to WebSocket server.");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
  webSocket.enableHeartbeat(15000, 3000, 2);  // Enable heartbeat with 15s ping interval, 3s pong timeout, and 2 disconnect attempts

  mfrc522.PCD_Init();
  memset(&(key.keyByte), 0xFF, 6);
}

void loop() {
  // listen for incoming commands on serial port
  if (Serial.available()) {
    screen_command_str = Serial.readStringUntil('\n');
    Serial.printf("\nReceived command: %s\n", screen_command_str.c_str());
    // Confirm if the command is a valid json string
    screen_command = JSON.parse(screen_command_str);
    if (JSON.typeof(screen_command) == "undefined") {
      Serial.println("Parsing input failed!");
    } else {
      Serial.println(JSON.stringify(
                           cmdResponseToJSON(
                             exec_cmd(screen_command)))
                       .c_str());
    }
  }

  if (current_state == CURRENT_USER_SCREEN) {
    rfid_loop();
    if (current_auth == FPRINT || current_auth == HYBRID)
      fingerprint_loop();
  } else if (current_state == CAPTURE_SCREEN) {
    // Enter fingerprint enrollment mode if initiated correctly
    if (enroll_flag) {
      finger.getTemplateCount();
      new_user_fprint_id = finger.templateCount + 1;
      switch (FingerprintEnroll(new_user_fprint_id)) {
        case 1:
          screenSerial.println("User enrolled successfully");
          break;

        case -2:
          screenSerial.println("Timeout");
          break;

        default:
          screenSerial.println("Couldn't enroll user");
          break;
      }
      enroll_flag = false;  // Leave enrollment mode immediately after.
    }
  }
  webSocket.loop();
}