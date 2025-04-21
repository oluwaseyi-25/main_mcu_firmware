CMD_RESPONSE template_(CMD_INPUT cmd_input) {
  CMD_RESPONSE ret;
  return ret;
}

CMD_RESPONSE start_class(CMD_INPUT cmd_input) {
  CMD_RESPONSE ret;
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

  Serial.println("Bring card close!!!");
  while (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    yield();
  }

  writeToBlock(8, (uint8_t *)p, sizeof(user));

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
