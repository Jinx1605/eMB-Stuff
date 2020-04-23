/*

    eMB Brain Beta

    - Adafruit M0 Metro Express / M4 Metro Airlift Lite
    - RFM69HCW Radio - CS:8, INT:3, RST:4
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

#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel pixl(1, 40, NEO_GRB + NEO_KHZ800);

// ** Global Use Data ** //

String logName = "";
String dateNow = "";
String timeNow = "";

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

const long interval = 1000;           // interval at which to blink (milliseconds)
const int ledPin =  LED_BUILTIN;
int ledState = LOW;
unsigned long previousMillis = 0;
unsigned long lastHeardFrom = 0;

boolean all_good = false;
short loopCount = 0;

void setup() {
  // put your setup code here, to run once:

  pinMode(ledPin, OUTPUT);

  
  Serial.begin(115200);
  //while(!Serial);

  RTC_Init();

  if(!MLX90614_Init()){
    Serial.println("MLX90614 Temp Module Missing!");
  }

  Radio_Init();

  // Init onboard NeoPixel
  pixl.begin();
  
  pixl.clear();
  pixl.setPixelColor(0, pixl.Color(0, 150, 0)); // GREEN
  pixl.show();
  all_good = true;
  
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

/*
   
   MLX90614 Temp Sensor Stuff
   
*/
boolean MLX90614_Init() {
  Wire.beginTransmission(0x5A);
  if (Wire.endTransmission() == 0) {
    return true;
  } else {
    return false;
  }
}

double MLX90614_ObjectTempF(void) {
  return (MLX90614_ReadTemp(0x07) * 9 / 5) + 32;
}

double MLX90614_AmbientTempF(void) {
  return (MLX90614_ReadTemp(0x06) * 9 / 5) + 32;
}

float MLX90614_ReadTemp(uint8_t reg) {
  float temp;
  temp = MLX90614_Read16(reg);
  temp *= .02;
  temp -= 273.15;
  return temp;
}

uint16_t MLX90614_Read16(uint8_t a) {
  uint16_t ret;
  Wire.beginTransmission(0x5A); //MLX90614
  Wire.write(a);
  Wire.endTransmission(false);

  Wire.requestFrom(0x5A, (uint8_t)3);
  ret = Wire.read();
  ret |= Wire.read() << 8;
  
  //uint8_t pec = Wire.read();
  
  return ret;
}


void loop() {
  // put your main code here, to run repeatedly:
  rtc_data = RTC_Now();
  timeNow = format_time(rtc_data.hour,rtc_data.minute,rtc_data.second, 0);
  dateNow = format_date(rtc_data.month, rtc_data.day, rtc_data.year);
  // RTC_Sprint(rtc_data);

  /*
  if(!MLX90614_Init()){
    Serial.println("MLX90614 Temp Module Missing!");
  } else {
    Serial.print("Motor : ");
    Serial.print(MLX90614_ObjectTempF(),3);
    Serial.print(" F, Ambient : ");
    Serial.print(MLX90614_AmbientTempF(),2);
    Serial.println("F");
  }
  */
  
  Radio_Packetize();
  Radio_Read();

  loopCount++;

  if (loopCount == 5 || loopCount == 15) {
    //update oled every half second
    // Serial.println("update oled");
    //Serial.print("LoopCount : ");
    //Serial.println(loopCount);
    unsigned long currentMillis = millis();
    if (currentMillis - lastHeardFrom >= 500) {
      //previousMillis = currentMillis;
      joystick_connected = false;
      all_good = false;                
      //Serial.println(timeNow + " : alls well!");
    }
    
  } else if (loopCount == 10 || loopCount == 20) {
    // Serial.println("oled update and log time!");
    // log every secondish :P
    //if () { }
    //Serial.print("JoyStiic : ");
    //Serial.print(joystick_connected);
    //Serial.print(" - LoopCount : ");
    //Serial.println(loopCount);
    loopCount = 0;
  }

  if(!all_good){
    pixl.setPixelColor(0, pixl.Color(150, 0, 0)); // RED
    pixl.show();
  } else {
    pixl.setPixelColor(0, pixl.Color(0, 150, 0)); // GREEN
    pixl.show();
  }
  
}
