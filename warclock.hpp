#include "HardwareSerial.h"
#include "WString.h"
#include <Arduino.h>
#include <Ds1302.h>
#include <U8g2lib.h>

extern U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2;
extern Ds1302::DateTime now;
Ds1302::DateTime wcTimeStamp;
static uint8_t oldSecond = 0;
int wcHour = now.hour;
int wcMin = now.minute;
int wcSec = now.second;
int wcday = now.day;
int tempTimeVal[4];  //wc的结果数组

void str2array(String &input, int strRecv[4]) {
  String temp;
  int m = 0;
  for (int i = 0; i < input.length(); i++) {
    if (input[i] != '@') {
      temp += input[i];
    } else {
      strRecv[m] = temp.toInt();
      m++;
      temp = "";
    }
  }
}

void returnZero() {
  /*--------设置当下为wc起点-----------*/
  wcHour = now.hour;
  wcMin = now.minute;
  wcSec = now.second;
  wcday = now.day;
}

void updateWC(String wctime) {
  int temp[4];
  str2array(wctime, temp);
  wcHour = temp[0];
  wcMin = temp[1];
  wcSec = temp[2];
  wcday = temp[3];
}

void getWC(String &out) {
  out = "";
  out = wcHour + String("@") + wcMin + String("@") + wcSec + String("@") + wcday + String("@");
}

void showWarClock() {
  if (oldSecond != now.second) {
    oldSecond = now.second;
    /*--------时间差-----------*/
    tempTimeVal[0] = now.second - wcSec;  //补齐进位原因，调整顺序
    tempTimeVal[1] = now.minute - wcMin;
    tempTimeVal[2] = now.hour - wcHour;
    tempTimeVal[3] = now.day - wcday;
    for (int i = 0; i < 2; i++) {
      if (tempTimeVal[i] < 0) {
        tempTimeVal[i] += 60;  //分 秒
        tempTimeVal[i + 1]--;
      }
    }
    if (tempTimeVal[2] < 0) {
      tempTimeVal[2] += 24;  //时
      tempTimeVal[3]--;
    }
  }
  u8g2.setFont(u8g2_font_helvR14_tf);
  u8g2.setCursor(12, 60);
  /*---------------------------*/
  u8g2.printf("%02d:%02d:%02d", tempTimeVal[2], tempTimeVal[1], tempTimeVal[0]);
  /*---------------------------*/
  u8g2.setFont(u8g2_font_logisoso18_tf);
  u8g2.setCursor(101, 60);
  u8g2.printf("%02d", tempTimeVal[3]);
  u8g2.drawRFrame(99, 39, 27, 24, 4);
}
