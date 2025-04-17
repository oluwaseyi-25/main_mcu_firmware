// Mapping from instructions to functions
OpPtr opcodeToFunc(String opcode) {
  OpPtr ret;
  if (strcmp(opcode.c_str(), "change_screen") == 0)
    ret = change_screen;
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

// Search for registered user by their fingerprint id
struct USER* getUser(uint8_t id) {
  static struct USER* user;
  LOG("Searching database for User with id " + String(id));
  for (user = users_list; user != NULL; user = user->next) {
    if (user->fingerprintId == id) {
      LOG("\nFound User!!!\nUser info: " + String(user->name) + " " + String(user->fingerprintId));
      return user;
    }
    LOG("\nNot User " + String(user->name) + " " + String(user->fingerprintId));
  }
  LOG("\nUser not found!!!");
  return 0;
}