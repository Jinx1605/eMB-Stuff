/*

    eMB Brain Beta

    - Adafruit M4 Metro Airlift Lite
    - RFM69HCW Radio
    - DS3231 RTC
    - DimensionEngineering Sabertooth 2x32 ESC

*/

#include <SPI.h>
#include <Wire.h>

#include <USBSabertooth.h>
#define ST_ESC_BAUDRATE 19200

USBSabertoothSerial C;
USBSabertooth ST(C, 128);

#include "RTClib.h"
RTC_DS3231 rtc;

#define DS3231_I2C_ADDR 0x68
#define DS3231_TEMP_MSB 0x11

char daysOfTheWeek[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

struct RTC_Time {
  int month;
  int day;
  int year;
  int hour;
  int minute;
  int second;
  String temp;
};

RTC_Time rtc_data;


#include <RHReliableDatagram.h>
#include <RH_RF69.h>


#define RFM69_CS      8
#define RFM69_INT     3
#define RFM69_RST     4
#define RF69_FREQ     915.0

// Where to send packets to!
#define DEST_ADDRESS  2
// change addresses for each client board, any number :)
#define MY_ADDRESS    1

// Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS, RFM69_INT);
// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram rf69_manager(rf69, MY_ADDRESS);

// The encryption key has to be the same as the one in the server
uint8_t crypt_key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};


uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
uint8_t data[] = "";
char radiopacket[RH_RF69_MAX_MESSAGE_LEN] = "";

// ** Global Use Data ** //

String logName = "";
String dateNow = "";
String timeNow = "";

String format_time(int hr, int mn, int sx, int tzd){
  String theTime = "";
  int hour = hr + tzd;
  String ampm = (hour >= 12 ) ? "pm" : "am";
  hour = (hour > 12) ? (hour - 12) : hour;
  hour = (hour == 0) ? 12 : hour;
  if (hour < 0) { ampm = "pm"; };
  hour  = (hour <= 0) ? hour + 12 : hour;
  String hur  = String(hour);
  String min  = (mn < 10) ? "0" + String(mn) : String(mn);
  String sec  = (sx < 10) ? "0" + String(sx) : String(sx);
  theTime += hur + ":";
  theTime += min + ":";
  theTime += sec;
  theTime += " " + ampm;
  return theTime;
}

String format_date(int mn, int dy, int yr) {
  String theDate = "";
  String month = (mn < 10) ? "0" + String(mn) : String(mn);
  String day   = (dy < 10) ? "0" + String(dy) : String(dy);
  String year  = String((yr - 2000));
  theDate += month + "-" + day + "-" + year;
  return theDate;
}

float throttle = 0;
float throttlePercentage = 0;
boolean killSwitch = true;

boolean joystick_connected = false;
int joystick_info[6] = {
  0, // vertical
  0, // horizontal
  0, // j-button state
  0, // z-button state
  0, // c-button state
  0  // battery voltage
};


void setup() {
  // put your setup code here, to run once:
  Wire.begin();
  Serial.begin(115200);
  // while(!Serial);

  RTC_Init();

  Radio_Init();

}

/*
   RTC_Init Function
   checks RTC presence and
   intitalizes it.
*/
void RTC_Init() {
  Serial.println("Init the RTC");

  if (!RTC_Check()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, Setting time.");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  rtc_data = RTC_Now();
  RTC_Sprint(rtc_data);

  timeNow = format_time(rtc_data.hour,rtc_data.minute,rtc_data.second, 0);
  dateNow = format_date(rtc_data.month, rtc_data.day, rtc_data.year);
  
}

boolean RTC_Check() {
  Wire.begin();
  Wire.beginTransmission(DS3231_I2C_ADDR);
  if (Wire.endTransmission() == 0) {
    Serial.println("RTC present");
  } else {
    Serial.println("RTC not found");
    return false;
  }
  return true;
}

/*
   RTC_Now Function
   Returns RTC Data in RTC_Time Structure.
*/
RTC_Time RTC_Now() {
  DateTime now = rtc.now();
  RTC_Time data;
  data.month  = now.month();
  data.day    = now.day();
  data.year   = now.year();
  data.hour   = now.hour();
  data.minute = now.minute();
  data.second = now.second();
  data.temp   = RTC_Temp() + "F";
  return data;
}

/*
    Function RTC_Temp
    returns approximate temperature
    from DS3231 RTC Module in F.
*/
String RTC_Temp() {
  short temp_msb;
  String temp;

  Wire.beginTransmission(DS3231_I2C_ADDR);
  Wire.write(DS3231_TEMP_MSB);
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDR, 1);
  temp_msb = Wire.read();

  // for readings in C just use temp_msb
  double tmp = (temp_msb * 1.8) + 32;
  temp = String(tmp);
  return temp;
}

void RTC_Sprint(RTC_Time data) {
  Serial.print("RTC Data: ");
  Serial.print(data.month);
  Serial.print("-");
  Serial.print(data.day);
  Serial.print("-");
  Serial.print(data.year);
  Serial.print(" , ");
  Serial.print(data.hour);
  Serial.print(":");
  Serial.print(data.minute);
  Serial.print(":");
  Serial.print(data.second);
  Serial.print(" , Chip Temp: ");
  Serial.print(data.temp);
  Serial.println();
}

void ESC_Init(int FreeWheel) {
  // Setup Sabertooth ESC
  SabertoothTXPinSerial.begin(ST_ESC_BAUDRATE);
  // ST.autobaud();
  if (!FreeWheel) {
    ST.freewheel(1, false);
    ST.freewheel(2, false);
  } else {
    ST.freewheel(1, true);
    ST.freewheel(2, true);
  }
}

void ESC_Send(float pwr) {
  ST.motor(1, pwr);
  ST.motor(2, pwr);
}

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
  rf69.setTxPower(14, true);
  
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
  if (rf69_manager.sendtoWait((uint8_t *)radiopacket, strlen(radiopacket), DEST_ADDRESS)) {
    // Now wait for a reply from the server
    uint8_t len = sizeof(buf);
    uint8_t from;
    if (rf69_manager.recvfromAckTimeout(buf, &len, 500, &from)) {
      //killswitch(false);
      buf[len] = 0;
      //      Serial.print("Remote #");
      //      Serial.print(from); Serial.print(": ");
      //      Serial.println((char*)buf);

      Radio_Decode((char*)buf);
    }
  } else {
    if (joystick_connected) {
      //killswitch(true);
    }
  }
}

void Radio_Packetize() {
  String data_to_send = "";

  data_to_send = timeNow;
  data_to_send += ",";
  data_to_send += dateNow;

  data_to_send.toCharArray(radiopacket, data_to_send.length() + 1);
}

void loop() {
  // put your main code here, to run repeatedly:
  rtc_data = RTC_Now();
  timeNow = format_time(rtc_data.hour,rtc_data.minute,rtc_data.second, 0);
  dateNow = format_date(rtc_data.month, rtc_data.day, rtc_data.year);
  RTC_Sprint(rtc_data);
  
  Radio_Packetize();
  //Radio_Read();
}
