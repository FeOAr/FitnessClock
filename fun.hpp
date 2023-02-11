#include <Arduino.h>

extern String myDate;
extern String mytime;
extern Ds1302 rtc;
extern Ds1302::DateTime now;
extern const char *WeekDays[];
void refreshtime() {
  rtc.getDateTime(&now);
  static uint8_t last_second = 0;
  if (last_second != now.second) {
    last_second = now.second;
    // init
    myDate = "20";
    mytime = "";

    // year
    myDate += now.year;
    // month
    if (now.month < 10) {
      myDate += "-0";
      myDate += now.month;
    } else {
      myDate += "-";
      myDate += now.month;
    }
    // day
    if (now.day < 10) {
      myDate += "-0";
      myDate += now.day;
    } else {
      myDate += "-";
      myDate += now.day;
    }

    myDate += "-";
    myDate += WeekDays[now.dow - 1];

    // hour
    if (now.hour < 10) {
      mytime += " 0";
      mytime += now.hour;
    } else {
      mytime += " ";
      mytime += now.hour;
    }
    // minute
    if (now.minute < 10) {
      mytime += ":0";
      mytime += now.minute;
    } else {
      mytime += ":";
      mytime += now.minute;
    }
    // second
    if (now.second < 10) {
      mytime += ":0";
      mytime += now.second;
    } else {
      mytime += ":";
      mytime += now.second;
    }
  }
}
