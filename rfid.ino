byte store[] = { "The warning missage WARNING: Communication failure, is the MFRC522 properly connected? indicates that your Raspberry Pi or Arduino is unable to communicate with the MFRC522 RFID module. Here are some troubleshooting steps to fix the issueThe warning message WARNING: Communication failure, is the MFRC522 properly connected? indicates that your Raspberry Pi or Arduino is unable to communicate with the MFRC522 RFID module. Here are some troubleshooting steps to fix the issueThe warning message WARNING: Compile" };


// MIFARE 1K card functions
void writeToBlock(byte block, byte* data, uint16_t size) {
  LOG("\n---------------------------------");
  LOGF("Writing to block %u\n", block);
  LOG("---------------------------------");

  uint16_t offset = 0;
  int sectorid = (byte)block / 4;
  for (int blockid = block; blockid < 64; blockid++) {
    if ((blockid % 4 == 3 && blockid != 63) || blockid == 0 || blockid == block) {
      LOGF("-------------\nSector %d\n-------------\n", sectorid);
      LOGF("Requesting permission for block %u \n-------------\n", blockid + 1);

      // Authenticate the specified block using KEY_A = 0x60
      if (mfrc522.PCD_Authenticate(0x60, blockid + 1, &key, &(mfrc522.uid)) != 0) {
        LOG("\nAuthentication failed.");
        return;
      }

      sectorid++;
      if (blockid != block)
        continue;
    }

    if (blockid == 0 || blockid == 63) {
      continue;
    }

    byte write_data[16] = { 0 };
    if (offset < size) {
      LOGF("\nWrite to block %d (in sector %u): ", blockid, (byte)blockid / 4);
      for (int k = 0; k < 16; k++) {
        if (offset + k < size) {
          write_data[k] = *(data + (offset + k));
        } else {
          write_data[k] = 0;
        }
        Serial.print((char)write_data[k]);
      }

      // write to block
      if (mfrc522.MIFARE_Write(blockid, write_data, 16) != 0) {
        LOG("\nWrite failed.");
      } else {
        Serial.print("\nData written successfully in block: ");
        LOG(blockid);
      }
      offset += 16;
      LOG();
    } else break;
  }
}

void readFromBlock(byte block, byte* buffer, uint16_t size) {
  LOG("\n---------------------------------");
  LOGF("Reading from block %u\n", block);
  LOG("---------------------------------");

  uint16_t offset = 0;
  int sectorid = (byte)block / 4;
  for (int blockid = block; blockid < 64; blockid++) {
    if ((blockid % 4 == 3 && blockid != 63) || blockid == 0 || blockid == block) {
      LOGF("-------------\nSector %d\n-------------\n", sectorid);
      LOGF("Requesting permission for block %u \n-------------\n", blockid + 1);

      // Authenticate the specified block using KEY_A = 0x60
      if (mfrc522.PCD_Authenticate(0x60, blockid + 1, &key, &(mfrc522.uid)) != 0) {
        LOG("\nAuthentication failed.");
        return;
      }
      sectorid++;
      if (blockid != block)
        continue;
    }

    if (blockid == 0 || blockid == 63) {
      continue;
    }

    byte read_data[18] = { 0 };
    byte read_size = 18;

    // write to block
    if (mfrc522.MIFARE_Read(blockid, read_data, &read_size) != 0) {
      LOG("\nRead failed.");
    } else {
      LOGF("\nData read successfully from block: %d\n", blockid);
    }

    if (offset < size) {
      LOGF("\nRead from block %d (in sector %u): ", blockid, (byte)blockid / 4);
      for (int k = 0; k < 16; k++) {
        if (offset + k < size) {
          *(buffer + (offset + k)) = read_data[k];
        } else break;
        Serial.print((char)read_data[k]);
      }
      offset += 16;
      LOG();
    } else break;
  }
}

// MIFARE Ultralight card functions
void writeToUltralight(byte pageAddr, byte* buffer, byte size) {
  if (mfrc522.PICC_GetType(mfrc522.uid.sak) != MFRC522::PICC_Type::PICC_TYPE_MIFARE_UL) {
    return;
  }

  if (size > 40) {
    LOG_ERR("Space constraint");
    return;
  }

  // Write data ***********************************************
  for (int i = 0; i < size / 4; i++) {
    //data is writen in blocks of 4 bytes (4 bytes per page)
    if (mfrc522.MIFARE_Ultralight_Write(pageAddr + i, &buffer[i * 4], 4) != 0) {
      LOG_ERR(F("MIFARE_Write() failed: "));
      return;
    } else {
      LOGF("MIFARE_Ultralight_Write() OK to page %u: %*.s \n", pageAddr + i, 4, (char *)&buffer[i * 4]);
    }
  }
  LOG();
}

void readFromUltralight(byte pageAddr, byte* buffer, byte size) {
  if (mfrc522.PICC_GetType(mfrc522.uid.sak) != MFRC522::PICC_Type::PICC_TYPE_MIFARE_UL)
    return;

  if (size > 40) {
    LOG("Space constraint");
    return;
  }

  // Read data ***************************************************
  byte read_data[18] = { 0 };
  byte read_size = 18;
  LOG(F("Reading data ... "));

  // data in 4 pages (16 bytes) are read at once.
  for (uint16_t offset = 0; offset < size; offset += 16) {
    memset(read_data, 0, 18);
    if (mfrc522.MIFARE_Read(pageAddr + (offset / 4), read_data, &read_size) != 0) {
      Serial.print(F("MIFARE_Read() failed: "));
    } else {
      Serial.print("\nData read successfully from page: ");
      LOG(pageAddr + (offset / 4));
      for (int k = 0; k < 16; k++) {
        if (offset + k < size) {
          *(buffer + (offset + k)) = read_data[k];
        } else break;
        LOGF(" %02X ", read_data[k]);
      }
      LOG();
    }
  }
}

void rfid_loop() {
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  static byte readStuff[sizeof(user)];
  // Determine card type
  if (mfrc522.PICC_GetType(mfrc522.uid.sak) == MFRC522::PICC_Type::PICC_TYPE_MIFARE_UL) {
    LOG("Ultralight Card detected\n");
    readFromUltralight(6, readStuff, sizeof(user));
  } else if (mfrc522.PICC_GetType(mfrc522.uid.sak) == MFRC522::PICC_Type::PICC_TYPE_MIFARE_1K) {
    LOG("1K Card detected\n");
    readFromBlock(8, readStuff, sizeof(user));
  }

  // TODO: Error check
  current_user = (user*)&readStuff;
  current_user_json["matric_no"] = String(current_user->matric_no);
  current_user_json["level"] = String(current_user->level * 100);
  current_user_json["dept"] = String(current_user->dept);
  current_user_json["fingerprintId"] = current_user->fingerprintId;
  if (current_auth == NONE) {
    log_attendance();
    card_scanned = false;
    current_user_json["verified"] = true;
  }
  else {
    current_user_json["verified"] = false;
    card_scanned = true;
    face_scanned = false;
  }

  screenSerial.println(JSON.stringify(current_user_json).c_str());
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}
