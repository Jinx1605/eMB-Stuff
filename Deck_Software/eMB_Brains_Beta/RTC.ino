
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
