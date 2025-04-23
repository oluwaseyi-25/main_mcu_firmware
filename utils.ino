// Mapping from instructions to functions
OpPtr opcodeToFunc(String opcode) {
  OpPtr ret;
  if (opcode == "change_screen")
    ret = change_screen;
  else if (opcode == "flash_card")
    ret = flash_card;
  else if (opcode == "capture_fprint")
    ret = capture_fprint;
  else if (opcode == "start_class")
    ret = start_class;
  else if (opcode == "change_wifi")
    ret = change_wifi;
  else if (opcode == "diagnostics")
    ret = [](CMD_INPUT cmd_input) -> CMD_RESPONSE {
      CMD_RESPONSE ret = { "OK", "Diagnostics command executed successfully" };
      // Return a report of the current state of the system
      String wifiStatus = (WiFi.status() == WL_CONNECTED) ? "Connected" : "Disconnected";
      String ipAddress = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "N/A";

      // Get memory stats
      uint32_t freeHeap = ESP.getFreeHeap();
      uint32_t totalHeap = ESP.getHeapSize();
      uint32_t usedHeap = totalHeap - freeHeap;

      // Check for PSRAM availability
      String psramStatus = psramFound() ? "Available" : "Not Available";
      uint32_t psramSize = psramFound() ? ESP.getPsramSize() : 0;
      uint32_t freePsram = psramFound() ? ESP.getFreePsram() : 0;
      uint32_t usedPsram = psramSize - freePsram;

      ret.body = "Camera Diagnostics Report:\n";
      ret.body += "WiFi Status: " + wifiStatus + "\n";
      ret.body += "IP Address: " + ipAddress + "\n";
      ret.body += "Memory Stats:\n";
      ret.body += "  Total Heap: " + String(totalHeap) + " bytes\n";
      ret.body += "  Used Heap: " + String(usedHeap) + " bytes\n";
      ret.body += "  Free Heap: " + String(freeHeap) + " bytes\n";
      ret.body += "PSRAM Stats:\n";
      ret.body += "  PSRAM Status: " + psramStatus + "\n";
      if (psramFound()) {
        ret.body += "  Total PSRAM: " + String(psramSize) + " bytes\n";
        ret.body += "  Used PSRAM: " + String(usedPsram) + " bytes\n";
        ret.body += "  Free PSRAM: " + String(freePsram) + " bytes\n";
      }
      return ret;
    };
  else {
    ret = [](CMD_INPUT cmd_input) -> CMD_RESPONSE {
      CMD_RESPONSE ret = { "ERR", "Operation not supported yet" };
      return ret;
    };
  }
  return ret;
}

CMD_RESPONSE exec_cmd(JSONVar cmd) {
  CMD_INPUT cmd_input = {
    (JSONVar)cmd["args"]
  };
  return opcodeToFunc(cmd["cmd"])(cmd_input);
}

JSONVar cmdResponseToJSON(CMD_RESPONSE response) {
  JSONVar ret;
  ret["cmd_response_status"] = response.status;
  ret["cmd_response_body"] = response.body;
  return ret;
}

/**
 * @brief Reads a file from the SPIFFS file system.
 *
 * This function opens a file for reading and reads its content into a string.
 * It returns the content of the file as a string.
 *
 * @param fs The file system object (SPIFFS).
 * @param path The path of the file to read.
 * @return String The content of the file as a string.
 */
String readFile(fs::FS &fs, const char *path) {
  static String file_content;
  LOGF("Reading file: %s\r\n", path);

  fs::File file = fs.open(path);
  if (!file || file.isDirectory()) {
    LOG_ERR("- failed to open file for reading");
    return String("");
  }

  LOG("- read from file:");
  char read_char;
  while (file.available()) {
    read_char = file.read();
    file_content += read_char;
  }
  file.close();
  return file_content;
}

/**
 * @brief Writes a message to a file in the SPIFFS file system.
 *
 * This function opens a file for writing and writes the provided message to it.
 * It creates the file if it does not exist.
 *
 * @param fs The file system object (SPIFFS).
 * @param path The path of the file to write to.
 * @param message The message to write to the file.
 * @return true if the write operation was successful, false otherwise.
 */
bool writeFile(fs::FS &fs, const char *path, const char *message) {
  LOGF("Writing file: %s\r\n", path);

  fs::File file = fs.open(path, FILE_WRITE, true);
  if (!file) {
    LOG_ERR("- failed to open file for writing");
    return false;
  }
  if (!file.print(message)) {
    LOG_ERR("- write failed");
    file.close();
    return false;
  }
  LOG("- file written\n");
  file.close();
  return true;
}

/**
 * @brief Loads the configuration from the SPIFFS file system.
 *
 * This function reads the configuration file and parses it as JSON.
 * It extracts the SSID and password from the JSON object.
 *
 * @return true if the configuration was loaded successfully, false otherwise.
 */
bool loadConfig() {
  String fileContent = readFile(SPIFFS, "/config.json");

  if (fileContent.isEmpty())
    return false;

  Serial.println(fileContent.c_str());
  JSONVar json = JSON.parse(fileContent);
  if (JSON.typeof(json) == "undefined") {
    Serial.println("Parsing input failed!");
    return false;
  }

  Serial.println("JSON parsed successfully.");
  ssid = (const char *)json["ssid"];
  password = (const char *)json["password"];
  Serial.printf("SSID: %s", ssid.c_str());
  Serial.printf("\t Password: %s\n", password.c_str());
  return true;
}

/**
 * @brief Connects to the specified WiFi network.
 *
 * This function attempts to connect to the WiFi network using the provided SSID and password.
 * It waits for a maximum of 10 seconds for the connection to be established.
 *
 * @return true if the connection was successful, false otherwise.
 */
bool connect_to_network() {
  // Connect to WiFi network
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.println("Connecting to WiFi...");
  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startAttemptTime > 10000)  // 10 seconds timeout
    {
      Serial.println("WiFi connection timed out...\nCheck your credentials...");
      return false;
    }
    Serial.print(".");
    delay(500);
    yield();  // Allow other tasks to run
  }
  Serial.printf("\nConnected to WiFi network. IP address: %s\n", WiFi.localIP().toString().c_str());
  return true;
}

/**
 * @brief Handles WebSocket events.
 *
 * This function processes various WebSocket events such as connection, disconnection,
 * receiving text messages, and errors. It also handles ping and pong events.
 *
 * @param type The type of WebSocket event.
 * @param payload The payload data associated with the event.
 * @param length The length of the payload data.
 */
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED:
    Serial.printf("[WSc] Disconnected!\n");
    break;
  case WStype_CONNECTED:
    Serial.printf("[WSc] Connected to url: %s\n", payload);
    webSocket.sendTXT("Connected");
    break;
  case WStype_TEXT:
    Serial.printf("[WSc] get text: %s\n", payload);
    break;
  case WStype_ERROR:
    Serial.printf("[WSc] Error occurred!\n");
    break;
  case WStype_PING:
    Serial.printf("[WSc] Ping received!\n");
    break;
  case WStype_PONG:
    Serial.printf("[WSc] Pong received!\n");
    break;
  default:
    Serial.printf("[WSc] Unhandled event type: %d\n", type);
    break;
  }
}