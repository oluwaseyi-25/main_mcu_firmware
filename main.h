#ifdef TEST_MODE
#define LOG(msg)         \
    screenSerial.println();    \
    screenSerial.println(msg); \
    screenSerial.flush();
#define LOG_CAM(msg)         \
    screenSerial.println();    \
    screenSerial.print("[CAM] ");    \
    screenSerial.println(msg); \
    screenSerial.flush();
#define LOGF(...) \
    screenSerial.printf(__VA_ARGS__);
#else
#define LOG(msg)
#define LOG_CAM(msg)
#define LOGF(...)
#endif

#ifdef ERROR_LOGGING
#define LOG_ERR(msg)     \
    screenSerial.println();    \
    screenSerial.println(msg); \
    screenSerial.flush();
#else
#define LOG_ERR(msg)
#endif

#define WAIT 1000

#define CAMERA_RX 32
#define CAMERA_TX 33

#define FPRINT_RX 16
#define FPRINT_TX 17

#define SCREEN_RX 3
#define SCREEN_TX 1

#include <Arduino.h>
#include <HardwareSerial.h>
#include "SPIFFS.h"
#include <WiFi.h>
#include <Arduino_JSON.h>
#include <WebSocketsClient.h>
#include <Adafruit_Fingerprint.h>


WiFiClient client;




// Replace with your network credentials
String WiFiSSID = "MTN_4G_48437C";
String WiFiPass = "Pelumi0209";

// #define cameraSerial Serial
HardwareSerial screenSerial(0);
HardwareSerial fingerPrintSerial(1);
HardwareSerial cameraSerial(2);
Adafruit_Fingerprint finger(&fingerPrintSerial);

uint8_t id;  // id number for fingerprint

// fingeprint attempt counter
int mismatch_count = 0;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 3000;


//========================COMMANDS===============================
#define CMD_RET(status, body) \
  ret.status = status; \
  ret.body = body; \
  return ret;

#define MAX_USERS 5
#define MAX_ARGS 5
#define MAX_CONTACTS 5
#define NAME_SIZE 32

typedef struct {
  String status;
  String body;
} CMD_RESPONSE;

typedef struct {
  JSONVar args;
} CMD_INPUT;

enum SCREEN {
  HOME_SCREEN,
  SETTINGS_SCREEN,
  CLASS_DETAILS_SCREEN,
  CURRENT_USER_SCREEN,
  CAPTURE_SCREEN
};

enum AUTH_MODE {
  NONE,
  FPRINT,
  FACE,
  HYBRID
};

enum COMMANDS {
  CHANGE_NETWORK,
  CHANGE_SCREEN,
  FLASH_CARD,
  CAPTURE_FPRINT,
  START_CLASS,
  TAKE_PHOTO,
  DIAGNOSTICS,
  SAVE_NEW_USER
};

typedef struct {
  uint32_t matric_no;
  uint8_t level;
  char dept[4];
  uint8_t fingerprintId = 0;
} user;

typedef CMD_RESPONSE (*OpPtr)(CMD_INPUT);

OpPtr opcodeToFunc(int opcode);
bool verifyPassword(String pwd);
CMD_RESPONSE exec_cmd(JSONVar cmd);
JSONVar cmdResponseToJSON(CMD_RESPONSE);

WebSocketsClient webSocket;

user* new_user;
uint8_t new_user_fprint_id = 0;
String new_user_card_uid;
user* current_user;
JSONVar cmd, response;
JSONVar current_user_json;
bool enroll_flag = false;
bool flash_card_flag = false;
bool card_scanned = false;
bool face_scanned = false;
bool wifi_status;

uint8_t n_users = 0;
String screen_command_str, screen_response_str, ssid, password;
JSONVar screen_command, screen_response;

enum SCREEN current_state = HOME_SCREEN;
enum AUTH_MODE current_auth = NONE;