#define TEST_MODE
#define ERROR_LOGGING
#include "main.h"
#include "rfid.h"

void camera_loop() {
  static uint8_t tries_left = 3;
  if (!cameraSerial.available()) return;

  camera_response = cameraSerial.readStringUntil('\n');
  LOG_CAM(camera_response.c_str());
  camera_response_json = JSON.parse(camera_response);

  if (JSON.typeof(current_user_json) != "null" && camera_response_json.hasOwnProperty("verified")) {
    current_user_json["verified"] = (bool)camera_response_json["verified"];

    if ((bool)current_user_json["verified"]) {
      LOG("We know you!");
      screenSerial.println(JSON.stringify(current_user_json).c_str());
      log_attendance();
      tries_left = 3;
      current_user_json = JSONVar(nullptr);  // Wipe current user from memory
      card_scanned = false;
    } else {
      LOG("We don't know you!");
      current_user_json["tries_left"] = tries_left;
      tries_left -= 1;

      if (!tries_left) {
        LOG("\nMaximum trials exceeded!");
        log_attendance();
        current_user_json = JSONVar(nullptr);  // Wipe current user from memory
        tries_left = 3;  // Reset counter
        card_scanned = false;
      }
      screenSerial.println(JSON.stringify(current_user_json).c_str());
    }
  }
}

void setup() {
  screenSerial.begin(921600, SERIAL_8N1, SCREEN_RX, SCREEN_TX);
  while (!screenSerial)
    ;

  cameraSerial.begin(921600, SERIAL_8N1, CAMERA_RX, CAMERA_TX);
  while (!cameraSerial)
    ;

  fingerPrintSerial.begin(57600, SERIAL_8N1, FPRINT_RX, FPRINT_TX);
  while (!fingerPrintSerial)
    ;

  // Boot delay
  for (int i = 0; i < 5; i++) {
    LOGF("[BOOT] %u...\n", i);
    delay(1000);
  }

  checkFingerprintModule();

  // Initialize SPIFFS
  SPIFFS.begin(true);
  if (!SPIFFS.begin(true)) {
    LOG_ERR("An Error has occurred while mounting SPIFFS");
  }
  LOG("SPIFFS mounted successfully.");

  // Load configuration from SPIFFS
  if (!loadConfig()) {
    Serial.println("Failed to load configuration. Using default values.");
    ssid = "MTN_4G_48437C";
    password = "Pelumi0209";
    ws_ip = "192.168.0.200";
    ws_port = 5000;
    ws_route = "/command";
  }

  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_11dBm);
  if (!connect_to_network()) {
    LOG("WiFi connection timed out...\nCheck your credentials...");
  } else {
    LOGF("Connected to WiFi. IP address: %s\n", WiFi.localIP().toString().c_str());
  }

  // TODO: Make this modifiable
  webSocket.begin(ws_ip.c_str(), ws_port, ws_route.c_str());
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
  webSocket.enableHeartbeat(15000, 3000, 2);  // Enable heartbeat with 15s ping interval, 3s pong timeout, and 2 disconnect attempts

  mfrc522.PCD_Init();
  memset(&(key.keyByte), 0xFF, 6);

  // Time keeping
  if (WiFi.isConnected()) {
    esp_sntp_servermode_dhcp(1);  // (optional)
    sntp_set_time_sync_notification_cb(timeavailable);
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
  }
}

void loop() {
  if (!WiFi.isConnected()) {
    LOG_ERR("WiFi has been disconnected\nReconnecting...\n");
    if (!connect_to_network()) return;
  }

  static uint32_t home_screen_timer = millis();
  if (millis() - home_screen_timer > 500) {
    getTime();
    home_screen_timer = millis();
  }

  // listen for incoming commands on serial port
  if (screenSerial.available()) {
    screen_command_str = screenSerial.readStringUntil('\n');
    LOGF("\nReceived command: %s\n", screen_command_str.c_str());
    screen_command = JSON.parse(screen_command_str);
    // Confirm if the command is a valid json string
    if (JSON.typeof(screen_command) == "undefined") {
      LOG_ERR("Parsing input failed!");
    } else {
      LOG(JSON.stringify(cmdResponseToJSON(exec_cmd(screen_command))).c_str());
    }
  }

  if (current_state == CURRENT_USER_SCREEN) {
    rfid_loop();
    camera_loop();
    if (current_auth == FPRINT || current_auth == HYBRID)
      fingerprint_loop();
    if (!face_scanned && card_scanned && (current_auth == FACE || current_auth == HYBRID)) {
      // Tell the camera to take a new photo

      CMD_INPUT face_details;
      face_details.args["cmd"] = "verify_face";
      face_details.args["matric_no"] = current_user->matric_no;
      face_details.args["level"] = current_user->level;
      face_details.args["dept"] = current_user->dept;

      LOG_CAM(JSON.stringify(cmdResponseToJSON(take_photo(face_details))).c_str());
      face_scanned = true;
    }

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