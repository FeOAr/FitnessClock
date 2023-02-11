#include "WString.h"
#include <Arduino.h>
#include <Ds1302.h>
#include <U8g2lib.h>

extern U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2;
extern Ds1302::DateTime now;
Ds1302::DateTime wcTimeStamp;
static uint8_t oldSecond = 0;
int wcHour, wcMin, wcSec, wcday;

void returnZero() {
  wcHour = 0;
  wcMin = 0;
  wcSec = 0;
  wcday = 0;
}

void updateWC(String wctime, String nowtime) {

  String temp;
  int fileWC[4];
  int fileNow[4];
  int m = 0;
  //拆以下计时串
  for (int i = 0; i < wctime.length(); i++) {
    if (wctime[i] != '@') {
      temp += wctime[i];
    } else {
      fileWC[m] = temp.toInt();
      m++;
      temp = "";
    }
  }
  Serial.printf("fileWC ->%d:%d:%d-%d\n", fileWC[0], fileWC[1], fileWC[2], fileWC[3]);
  m = 0;
  temp = "";
  //拆断电时附近时间
  for (int i = 0; i < nowtime.length(); i++) {
    if (nowtime[i] != '@') {
      temp += nowtime[i];
    } else {
      fileNow[m] = temp.toInt();
      m++;
      temp = "";
    }
  }
  Serial.printf("fileNow ->%d:%d:%d-%d\n", fileNow[0], fileNow[1], fileNow[2], fileNow[3]);  //时:分:秒-日
  rtc.getDateTime(&wcTimeStamp);
  Serial.printf("temp ->%d:%d:%d-%d\n", wcTimeStamp.hour, wcTimeStamp.minute, wcTimeStamp.second, wcTimeStamp.day);

  /*--------时间作差----------*/
  int tempTimeVal[4];  //时间差中间量
  tempTimeVal[0] = wcTimeStamp.second - fileNow[2];
  tempTimeVal[1] = wcTimeStamp.minute - fileNow[1];
  tempTimeVal[2] = wcTimeStamp.hour - fileNow[0];
  tempTimeVal[3] = wcTimeStamp.day - fileNow[3];
  for (int i = 0; i < 3; i++) {
    if (tempTimeVal[i] < 0) {
      tempTimeVal[i] += 60;
      tempTimeVal[i + 1]--;
    }
  }
  Serial.printf("temp 1 ->%d:%d:%d-%d\n", tempTimeVal[2], tempTimeVal[1], tempTimeVal[0], tempTimeVal[3]);

  /*--------时间求和----------*/
  int tempAddVal[4];                           //计算中间量
  tempAddVal[0] = tempTimeVal[0] + fileWC[2];  //秒
  tempAddVal[1] = tempTimeVal[1] + fileWC[1];  //分
  tempAddVal[2] = tempTimeVal[2] + fileWC[0];  //时
  tempAddVal[3] = tempTimeVal[3] + fileWC[3];  //日
  for (int i = 0; i < 3; i++) {
    if (tempAddVal[i] >60) {
      tempAddVal[i] -= 60;
      tempAddVal[i + 1]++;
    }
  }
  Serial.printf("temp 2 ->%d:%d:%d-%d\n", tempAddVal[2], tempAddVal[1], tempAddVal[0], tempAddVal[3]);


  wcHour = tempAddVal[2];
  wcMin = tempAddVal[1];
  wcSec = tempAddVal[0];
  wcday = tempAddVal[3];
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
  u8g2.setCursor(12, 60);
  /*---------------------------*/
  u8g2.printf("%02d:%02d:%02d", wcHour, wcMin, wcSec);
  /*---------------------------*/
  u8g2.setFont(u8g2_font_logisoso18_tf);
  u8g2.setCursor(101, 60);
  u8g2.printf("%02d", wcday);
  u8g2.drawRFrame(99, 39, 27, 24, 4);
}
