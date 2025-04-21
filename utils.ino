// Mapping from instructions to functions
OpPtr opcodeToFunc(String opcode) {
  OpPtr ret;
  if (strcmp(opcode.c_str(), "change_screen") == 0)
    ret = change_screen;
  else if (strcmp(opcode.c_str(), "flash_card") == 0)
    ret = flash_card;
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