CMD_RESPONSE change_screen(CMD_INPUT cmd_input) {
  CMD_RESPONSE ret;
  const char* new_screen = (const char*)cmd_input.args["screen_name"];
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
  }  else if (strcmp(new_screen, "capture_screen") == 0) {
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