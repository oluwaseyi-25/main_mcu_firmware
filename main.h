
#ifdef TEST_MODE
#define LOG(msg) \
  Serial.println(); \
  Serial.println(msg);
#else
#define LOG(msg)
#endif

#include <stdint.h>
#include <Arduino.h>
#include <HardwareSerial.h>
#include "SPIFFS.h"
#include <WiFi.h>

#include <Arduino_JSON.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebSrv.h>

#define WAIT 1000

WiFiClient client;



// Fingerprint Related stuff
#include <Adafruit_Fingerprint.h>

// Replace with your network credentials
String WiFiSSID = "MTN_4G_48437C";
String WiFiPass = "Pelumi0209";

HardwareSerial screenSerial(1);
HardwareSerial fingerPrintSerial(2);
Adafruit_Fingerprint finger(&fingerPrintSerial);

uint8_t id;  // id number for fingerprint
int getFingerprintIDez();
// End Fingerprint Declarations


// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create a WebSocket object
AsyncWebSocket ws("/ws");

// fingeprint attempt counter
int mismatch_count = 0;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 3000;



// WEBSOCKET DECLARATIONS

// void initWebSocket();
// // WebSocket transmission to dashboard
// void notifyClients(String sensorReadings);

// // WebSocket event handler
// void onEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);
// void handleWebSocketMessage(void* arg, uint8_t* data, size_t len);

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

typedef struct {
  uint32_t matric_no;
  uint8_t level;
  char dept[4];
  uint8_t fingerprintId = 0;
} user;

typedef CMD_RESPONSE (*OpPtr)(CMD_INPUT);

OpPtr opcodeToFunc(String opcode);
bool verifyPassword(String pwd);
CMD_RESPONSE exec_cmd(JSONVar cmd);
JSONVar cmdResponseToJSON(CMD_RESPONSE);

user* new_user;
uint8_t new_user_fprint_id = 0;
user* current_user;
JSONVar cmd, response;
String PASSWORD = "123456";
bool enroll_flag = false;
bool flash_card_flag = false;

uint8_t n_users = 0;
String screen_command_str, screen_response_str;
JSONVar screen_command, screen_response;

enum SCREEN current_state = HOME_SCREEN;