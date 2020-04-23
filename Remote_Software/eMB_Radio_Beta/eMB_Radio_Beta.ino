// rf69 demo tx rx oled.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple addressed, reliable messaging client
// with the RH_RF69 class. RH_RF69 class does not provide for addressing or
// reliability, so you should only use RH_RF69  if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example rf69_server.
// Demonstrates the use of AES encryption, setting the frequency and modem 
// configuration

#include <SPI.h>
#include <Wire.h>
#include <RHReliableDatagram.h>
#include <RH_RF69.h>

#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel pixl(1, 8, NEO_GRB + NEO_KHZ800);

// Where to send packets to!
#define DEST_ADDRESS   1

// change addresses for each client board, any number :)
#define MY_ADDRESS     2
#define LED LED_BUILTIN

/************ Radio Setup ***************/

// Change to 434.0 or other frequency, must match RX's freq!
#define RF69_FREQ 915.0

#define RFM69_CS      10
#define RFM69_INT     6
#define RFM69_RST     11

byte DEV_ADDR = 0x20;
int reading = 0;

int joystiq_info[5] = {
  0, // vertical
  0, // horizontal
  0, // j-button state
  0, // z-button state
  0  // c-button state
};

int c_button_pin = 9;
int z_button_pin = 5;

// Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS, RFM69_INT);

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram rf69_manager(rf69, MY_ADDRESS);

// The encryption key has to be the same as the one in the server
uint8_t crypt_key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

// Global Use data
String dateNow = "";
String timeNow = "";

const long interval = 500;           // interval at which to blink (milliseconds)
const int ledPin =  LED_BUILTIN;
int ledState = LOW;
unsigned long previousMillis = 0;

boolean all_good = false;

void setup() {
  
  Wire.begin();// join i2c bus

  pinMode(c_button_pin, INPUT_PULLUP);
  pinMode(z_button_pin, INPUT_PULLUP);

  Serial.begin(115200);
  // while (!Serial) { delay(1); } // wait until serial console is open, remove if not tethered to computer

  checkRadio();
  
  pinMode(ledPin, OUTPUT);
  
  // Init onboard NeoPixel
  pixl.begin();
  
  pixl.clear();
  pixl.setPixelColor(0, pixl.Color(0, 150, 0)); // GREEN
  pixl.show();
  all_good = true;
}

void checkRadio() {
  
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

  // manual reset
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);
  
  if (!rf69_manager.init()) {
    Serial.println("RFM69 radio init failed");
    all_good = false;
    while (1);
  }
  Serial.println("RFM69 radio init OK!");
  
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for low power module)
  // No encryption
  if (!rf69.setFrequency(RF69_FREQ)) {
    Serial.println("setFrequency failed");
    all_good = false;
  }

  // If you are using a high power RF69 eg RFM69HW, you *must* set a Tx power with the
  // ishighpowermodule flag set like this:
  rf69.setTxPower(20, true);  // range from 14-20 for power, 2nd arg must be true for 69HCW
                    
  rf69.setEncryptionKey(crypt_key);
}

// Dont put this on the stack:
uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
char data[] = "";
char radiopacket[40] = "";

void loop() {  

  read_joystiq();
  format_joystiq_data();

  // Send a message to manager_server
  if (rf69_manager.sendtoWait((uint8_t*)radiopacket, sizeof(radiopacket), DEST_ADDRESS)) {
    // Now wait for a reply from the server
    Serial.print("Sent: "); Serial.print(radiopacket); Serial.print(" ");
    uint8_t len = sizeof(buf);
    uint8_t from;   
    if (rf69_manager.recvfromAckTimeout(buf, &len, 1000, &from)) {
      all_good = true;
      decodeRemotePacket((char*)buf);
    } else {
      Serial.println("No reply, is rf69_reliable_datagram_server running?");
    }
  } else {
    Serial.println("No Server Listening to me...");
    all_good = false;
    // Serial.println("sendtoWait failed");
  }
  

  /*
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
      
    }

    // Serial.println(timeNow + " : alls well!");
    // set the LED with the ledState of the variable:
    digitalWrite(ledPin, ledState);
  }
  */

  if(!all_good) {
    pixl.setPixelColor(0, pixl.Color(150, 0, 0)); // RED
    pixl.show();
  } else {
    pixl.setPixelColor(0, pixl.Color(0, 150, 0)); // GREEN
    pixl.show();
  }
  
  delay(125);
}

void read_joystiq() {
  
  joystiq_info[0] = getVertical();
  joystiq_info[1] = getHorizontal();
  joystiq_info[2] = getButton();
  joystiq_info[3] = digitalRead(z_button_pin);
  joystiq_info[4] = digitalRead(c_button_pin);

}

void format_joystiq_data() {
  String joystiq_string = "";
  
  joystiq_string = joystiq_info[0];
  joystiq_string += ",";
  joystiq_string += joystiq_info[1];
  joystiq_string += ",";
  joystiq_string += joystiq_info[2];
  joystiq_string += ",";
  joystiq_string += joystiq_info[3];
  joystiq_string += ",";
  joystiq_string += joystiq_info[4];
  
  joystiq_string.toCharArray(radiopacket, joystiq_string.length() + 1);
}

void decodeRemotePacket(char* data) {
    String the_data(data);
    int ind1   = the_data.indexOf(",");
    int ind2   = the_data.indexOf(",", ind1+1);

    timeNow = the_data.substring(0, ind1);
    dateNow = the_data.substring(ind1+1,ind2);

    Serial.print("Recieved: ");
    Serial.println(the_data);
    
}

// getHorizontal() returns an int between 0 and 1023 
// representing the Joystiic's horizontal position
// (axis indicated by silkscreen on the board)
// centered roughly on 512
int getHorizontal() {

  Wire.beginTransmission(DEV_ADDR);
  Wire.write(0x00);
  Wire.endTransmission(false);
  Wire.requestFrom(DEV_ADDR, 2);

  while (Wire.available()) {
    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();
    reading = (uint16_t)msb << 8 | lsb;
  }

  Wire.endTransmission();
  return reading;

}

// getVertical() returns an int between 0 and 1023 
// representing the Joystiic's vertical position
// (axis indicated by silkscreen on the board)
// centered roughly on 512
int getVertical() {

  Wire.beginTransmission(DEV_ADDR);
  Wire.write(0x02);
  Wire.endTransmission(false);
  Wire.requestFrom(DEV_ADDR, 2);

  while (Wire.available()) {
    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();
    reading = (uint16_t)msb << 8 | lsb;
  }

  Wire.endTransmission();
  return reading;

}

// getButton() returns a byte indicating the 
// position of the button where a 0b1 is not
// pressed and 0b0 is pressed
byte getButton() {

  byte btnbyte = 0b00000000;

  Wire.beginTransmission(DEV_ADDR);
  Wire.write(0x04);
  Wire.endTransmission(false);
  Wire.requestFrom(DEV_ADDR, 1);    // request 1 byte

  while (Wire.available()) {
    btnbyte = Wire.read();
  }

  Wire.endTransmission();
  return btnbyte;

}
