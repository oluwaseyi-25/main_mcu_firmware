CMD_RESPONSE template_(CMD_INPUT cmd_input) {
  CMD_RESPONSE ret;
  return ret;

}

void log_attendance() {
  JSONVar attendance_data = current_user_json;
  attendance_data["cmd"] = "log_attendance";
  webSocket.sendTXT(JSON.stringify(attendance_data).c_str());
  return;
}

CMD_RESPONSE enroll_user(CMD_INPUT cmd_input) {
  CMD_RESPONSE ret = {"OK", "User enrolled"};
  
  JSONVar new_user_data = cmd_input.args;
  new_user_data["fprint_id"] = new_user_fprint_id;
  new_user_data["card_uid"] = new_user_card_uid;
  new_user_data["cmd"] = String("enroll_user");
  webSocket.sendTXT(JSON.stringify(new_user_data).c_str());
  return ret;
}

CMD_RESPONSE take_photo(CMD_INPUT cmd_input) {
  CMD_RESPONSE ret = {"OK", "Photo taken successfully"};

  JSONVar res;
  res["args"] = cmd_input.args;
  res["cmd"] = "take_photo";
  
  face_scan_timer = millis();
  received_face_scan_response = false;
  // Send command to ESP32-CAM
  cameraSerial.println(JSON.stringify(res).c_str());
  return ret;
}

/**
 * @brief Changes the WiFi credentials and reconnects to the network.
 *
 * This function updates the WiFi SSID and password based on the provided command input.
 * It writes the new credentials to a configuration file and attempts to reconnect to the network.
 *
 * @param cmd_input Command input containing the new SSID and password.
 * @return CMD_RESPONSE A response object indicating the status and result of the operation.
 */
CMD_RESPONSE change_wifi(CMD_INPUT cmd_input){
  CMD_RESPONSE ret = {"OK", "WiFi changed successfully"};
  JSONVar cam_cmd;
  cam_cmd["args"] = cmd_input.args;
  cam_cmd["cmd"] = "change_wifi";
  if (cmd_input.args.hasOwnProperty("ssid") && cmd_input.args.hasOwnProperty("pwd"))
  {
    ssid = (const char *)cmd_input.args["ssid"];
    password = (const char *)cmd_input.args["pwd"];
    if (!writeFile(SPIFFS, "/config.json", JSON.stringify(cmd_input.args).c_str()))
    {
      ret.status = "ERR";
      ret.body = "Failed to write to config file";
      return ret;
    }
    WiFi.disconnect();
    if (!connect_to_network())
    {
      ret.status = "ERR";
      ret.body = "Failed to connect to new WiFi network";
      return ret;
    }
    cameraSerial.println(JSON.stringify(cam_cmd).c_str());
  }
  else
  {
    ret.status = "ERR";
    ret.body = "SSID and password are required";
  }
  return ret;
}

CMD_RESPONSE start_class(CMD_INPUT cmd_input) {
  CMD_RESPONSE ret = {"OK", "Started class"};
  ret.status = "OK";
  String auth_mode = (String) cmd_input.args["auth_mode"];
  if (auth_mode == "NONE") {
    current_auth = NONE;
  } else if (auth_mode == "FINGERPRINT") {
    current_auth = FPRINT;
  } else if (auth_mode == "FACE") {
    current_auth = FACE;
  } else if (auth_mode == "HYBRID") {
    current_auth = HYBRID;
  }
  JSONVar class_details = cmd_input.args;
  class_details["cmd"] = "start_class";
  webSocket.sendTXT(JSON.stringify(class_details).c_str());
  return ret;
}

CMD_RESPONSE capture_fprint(CMD_INPUT cmd_input) {
  CMD_RESPONSE ret;
  enroll_flag = true;
  ret.status = "OK";
  return ret;
}

CMD_RESPONSE flash_card(CMD_INPUT cmd_input) {
  CMD_RESPONSE ret;
  user new_user;
  String matric_no = String((const char *)cmd_input.args["matric_no"]);
  matric_no.remove(4, 1);  // remove the "/"
  new_user.matric_no = (uint32_t)atoll(matric_no.c_str());
  new_user.level = (uint8_t)(atol((const char *)cmd_input.args["level"]) / 100);
  String((const char *)cmd_input.args["dept"]).toCharArray(new_user.dept, 4);
  new_user.fingerprintId = new_user_fprint_id;
  ret.status = "OK";

  const uint8_t *p = (const uint8_t *)&new_user;

  screenSerial.println("Bring card close!!!");
  while (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    yield();
  }

  // Determine card type
  if (mfrc522.PICC_GetType(mfrc522.uid.sak) == MFRC522::PICC_Type::PICC_TYPE_MIFARE_UL) {
    LOG("Ultralight Card detected\n");
    writeToUltralight(6, (uint8_t *)p, sizeof(user));
  } else if (mfrc522.PICC_GetType(mfrc522.uid.sak) == MFRC522::PICC_Type::PICC_TYPE_MIFARE_1K) {
    LOG("1K Card detected\n");
    writeToBlock(8, (uint8_t *)p, sizeof(user));
  }

  // Save the UID on a String variable
  new_user_card_uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) {
      new_user_card_uid += "0"; 
    }
    new_user_card_uid += String(mfrc522.uid.uidByte[i], HEX);
  }

  mfrc522.PICC_HaltA();
  ret.body = "Card flashed!!";
  return ret;
}

CMD_RESPONSE change_screen(CMD_INPUT cmd_input) {
  CMD_RESPONSE ret;
  const char *new_screen = (const char *)cmd_input.args["screen_name"];
  if (strcmp(new_screen, "home_screen") == 0) {
    current_state = HOME_SCREEN;
    ret.status = "OK";
    ret.body = new_screen;
  } else if (strcmp(new_screen, "settings_screen") == 0) {
    current_state = SETTINGS_SCREEN;
    ret.status = "OK";
    ret.body = new_screen;
  } else if (strcmp(new_screen, "class_details_screen") == 0) {
    current_state = CLASS_DETAILS_SCREEN;
    ret.status = "OK";
    ret.body = new_screen;
  } else if (strcmp(new_screen, "capture_screen") == 0) {
    current_state = CAPTURE_SCREEN;
    ret.status = "OK";
    ret.body = new_screen;
  } else if (strcmp(new_screen, "current_user_screen") == 0) {
    current_state = CURRENT_USER_SCREEN;
    ret.status = "OK";
    ret.body = new_screen;
  } else {
    ret.status = "ERR";
    ret.body = "Invalid screen";
  }
  return ret;
}
