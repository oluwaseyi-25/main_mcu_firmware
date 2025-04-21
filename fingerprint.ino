#define TIMEOUT 5000

// Makes sure the fingerprint sensor is installed and functional
void checkFingerprintModule() {
  while (1) {
    if (finger.verifyPassword()) {
      LOG("Found fingerprint sensor!\n");
      delay(WAIT);
      finger.getTemplateCount();
      LOG(String("Sensor contains " + String(finger.templateCount) + " templates").c_str());
      delay(WAIT);
      break;
    } else {
      LOG("Did not find fingerprint_out Sensor :(\nReset the ESP - check wiring");
      delay(WAIT);
    }
  }
}

// Checks the code returned by from the Fingerprint module
// and sends the error message to correct the MQTT topic / Websocket.
int8_t checkCode(uint8_t code) {
  switch (code) {
    case FINGERPRINT_NOFINGER:
      return 0;

    case FINGERPRINT_OK:
    case FINGERPRINT_PACKETRECIEVEERR:
    case FINGERPRINT_IMAGEFAIL:
    case FINGERPRINT_ENROLLMISMATCH:
    case FINGERPRINT_BADLOCATION:
    case FINGERPRINT_FLASHERR:
    default:
      return -1;
  }
}

// Enroll a new user fingerprint model into the Fingerprint module's memory
int8_t FingerprintEnroll(user* new_user) {
  int8_t p = -1;
  uint32_t timer;
  timer = millis();
  while (1) {
    if (millis() - timer > TIMEOUT) return -2;
    p = checkCode(finger.getImage());
    if (p > 0)
      break;
    else if (p < 0)
      continue;
    yield();
  }

  // OK success!
  p = checkCode(finger.image2Tz(1));
  if (p < 0) {
    return p;
  }

  p = -1;
  //checks to make sure your finger has been removed
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
    yield();
  }

  p = -1;
  timer = millis();
  while (1) {
    if (millis() - timer > TIMEOUT) return -2;
    p = checkCode(finger.getImage());
    if (p > 0) break;
    else if (p < 0) continue;
    yield();
  }

  // OK success!
  p = checkCode(finger.image2Tz(2));
  if (p < 0) return p;
  else {
  }

  // OK converted!
  p = checkCode(finger.createModel());
  if (p < 0) return p;
  else {
  }

  p = checkCode(finger.storeModel(new_user->fingerprintId));
  if (p < 0) return p;
  else {
    return 1;
  }
}

// Hnadles Fingerprint Authentication logic
// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p == FINGERPRINT_NOFINGER)
    return -3;

  else if (p != FINGERPRINT_OK)
    return -1;  // ERROR

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)
    return -1;  // ERROR

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)
    return -2;  // We don't know you
  return finger.fingerID;
}

// Handles Fingerprint operations in loop()
void fingerprint_loop() {
  static int tries_left = 3;
  static volatile int finger_status;

  finger_status = getFingerprintIDez();
  switch (finger_status) {
    case (-3):  // No finger
    case (-1):  // Any other error
      break;    // Fail safely

    case (-2):  // We don't know you
      {
        tries_left -= 1;
        while (finger.getImage() != FINGERPRINT_NOFINGER) {
          yield();
        }
        LOG("We don't know you!");
        if (!tries_left) {
          LOG("\nMaximum trials exceeded!");
        }
        break;
      }

    default:  // We know you
      {
        tries_left = 3;
        // current_user = *getUser(finger_status);
        Serial.printf("We know you! ID: %d\n", finger_status);
        while (finger.getImage() != FINGERPRINT_NOFINGER)
          yield();
        break;
      }
  }

  // Enter fingerprint enrollment mode if initiated correctly
  if (enroll_flag) {
    CMD_RESPONSE res;
    switch (FingerprintEnroll(new_user)) {
      case 1:
        // new_user->next = users_list;
        // users_list = new_user;
        // n_users++;
        res.status = "OK";
        res.body = "User enrolled successfully";
        break;
      case -2:
        res.status = "ERR";
        res.body = "Operation Timed out";
        break;
      default:
        res.status = "ERR";
        res.body = "Couldn't enroll user";
        break;
    }
    enroll_flag = false;  // Leave enrollment mode immediately after.
  }
}
