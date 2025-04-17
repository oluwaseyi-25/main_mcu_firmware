// Mapping from instructions to functions
OpPtr opcodeToFunc(String opcode) {
  OpPtr ret;
  // if (strcmp(opcode.c_str(), "echo") == 0)
  //   ret = echo;
  // else {
  //   ret = [](CMD_INPUT cmd_input) -> CMD_RESPONSE {
  //     CMD_RESPONSE ret = { "ERR", "Operation not supported yet" };
  //     return ret;
  //   };
  // }
  return ret;
}

// CMD_RESPONSE exec_cmd(JsonDocument cmd) {
//   const char* opcode = cmd["cmd"];
//   CMD_INPUT cmd_input = {
//     cmd.remove();
//   };
//   return opcodeToFunc(cmd["cmd"])(cmd_input);
// }

JsonDocument cmdResponseToJSON(CMD_RESPONSE response) {
  JsonDocument ret;
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