#include "WString.h"
#include <Arduino.h>
#include <Ds1302.h>
#include <U8g2lib.h>

extern U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2;
extern Ds1302::DateTime now;
Ds1302::DateTime wcTimeStamp;
Ds1302::DateTime calculateTemp;
static uint8_t oldSecond = 0;
int wcHour, wcMin, wcSec, wcday;

void returnZero() {
  wcHour = 0;
  wcMin = 0;
  wcSec = 0;
  wcday = 0;
}

void updateWC(String input) {

  String temp;
  int fileWC[4];
  int m = 0;
  for (int i = 0; i < input.length(); i++) {
    if (input[i] != '@') {
      temp += input[i];
    } else {
      Serial.println(temp);
      fileWC[m] = temp.toInt();
      m++;
      temp = "";
    }
  }
  wcHour = fileWC[0];
  wcMin = fileWC[1];
  wcSec = fileWC[2];
  wcday = fileWC[3];
}

void getWC(String &out) {
  out = "";
  out = wcHour + String("@") + wcMin + String("@") + wcSec + String("@") + wcday + String("@");
}

void showWarClock() {
  if (oldSecond != now.second) {
    oldSecond = now.second;
    if (wcSec == 59) {
      wcSec = 0;
      if (wcMin == 59) {
        wcMin = 0;
        if (wcHour == 23) {
          wcHour = 0;
          wcday++;
        } else
          wcHour++;
      } else
        wcMin++;
    } else
      wcSec++;
  }
  u8g2.setFont(u8g2_font_helvR14_tf);
  u8g2.setCursor(0, 60);
  /*---------------------------*/
  u8g2.printf("%02d:%02d:%02d", wcHour, wcMin, wcSec);
  /*---------------------------*/
  u8g2.setFont(u8g2_font_logisoso18_tf);
  u8g2.setCursor(101, 60);
  u8g2.printf("%02d", wcday);
  u8g2.drawRFrame(99, 39, 27, 24, 4);
}
