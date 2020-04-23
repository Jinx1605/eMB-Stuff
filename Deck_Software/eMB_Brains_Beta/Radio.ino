void Radio_Init() {
  Serial.println("RFM69 Radio Init Start.");
  // RFM69 Setup  
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);
  // manual reset
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);

  if (!rf69_manager.init()) {
    Serial.println("RFM69 Radio Init Failed");
    while (1);
  }
  
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for low power module)
  if (!rf69.setFrequency(RF69_FREQ)) {
    Serial.println("setFrequency failed");
  }

  // If you are using a high power RF69 eg RFM69HW, you *must* set a Tx power with the
  // ishighpowermodule flag set like this:
  rf69.setTxPower(20, true);
  
  // set encryption
  rf69.setEncryptionKey(crypt_key);

  Serial.println("RFM69 Radio Init Success!");
}

void Radio_Decode(char* data) {
  String the_data(data);
  int ind1   = the_data.indexOf(",");
  int ind2   = the_data.indexOf(",", ind1 + 1);
  int ind3   = the_data.indexOf(",", ind2 + 1);
  int ind4   = the_data.indexOf(",", ind3 + 1);
  int ind5   = the_data.indexOf(",", ind4 + 1);
  int ind6   = the_data.indexOf(",", ind5 + 1);

  String vert = the_data.substring(0, ind1);
  String horz = the_data.substring(ind1 + 1, ind2);
  String jbut = the_data.substring(ind2 + 1, ind3);
  String zbut = the_data.substring(ind3 + 1, ind4);
  String cbut = the_data.substring(ind4 + 1, ind5);
  String bvol = the_data.substring(ind5 + 1, ind6);

  joystick_info[0] = vert.toInt();
  joystick_info[1] = horz.toInt();
  joystick_info[2] = jbut.toInt();
  joystick_info[3] = zbut.toInt();
  joystick_info[4] = cbut.toInt();
  joystick_info[5] = bvol.toInt();

  if (!joystick_info[2]) {
    joystick_info[2] = 1;
  } else {
    joystick_info[2] = 0;
  }

  if (!joystick_info[3]) {
    joystick_info[3] = 1;
  } else {
    joystick_info[3] = 0;
  }

  if (!joystick_info[4]) {
    joystick_info[4] = 1;
  } else {
    joystick_info[4] = 0;
  }

}

void Radio_Read() {
  
  if (rf69_manager.available()) {
    // Wait for a message addressed to us from the client
    uint8_t len = sizeof(buf);
    uint8_t from;
    if (rf69_manager.recvfromAck(buf, &len, &from)) {
      joystick_connected = true;
      all_good = true;
      Serial.print("got request from : 0x");
      Serial.print(from, HEX);
      Serial.print(": ");
      Serial.println((char*)buf);
      
      unsigned long currentMillis = millis();
      lastHeardFrom = currentMillis;
      
      // Send a reply back to the originator client
      if (!rf69_manager.sendtoWait((uint8_t*)radiopacket, sizeof(radiopacket), from)){
        Serial.println("sendtoWait failed");
        all_good = false;
      }
    }
  }
  
}

void Radio_Packetize() {
  String data_to_send = "";

  data_to_send = timeNow;
  data_to_send += ",";
  data_to_send += dateNow;
  
  // Serial.print("Data to Send: "); Serial.println(data_to_send);

  data_to_send.toCharArray(radiopacket, data_to_send.length() + 1);
}
