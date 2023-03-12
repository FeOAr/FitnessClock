#include <Arduino.h>
#include <WiFi.h>
#include <Ds1302.h>
#include "sntp.h"
#include "time.h"
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const long gmtOffset_sec = 8 * 3600;  // 时区设置函数，东八区（UTC/GMT+8:00）写成8*3600
const int daylightOffset_sec = 0;     // 夏令时填写3600，否则填0

extern String myDate;
extern String mytime;
extern Ds1302 rtc;
extern U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2;

int preb = 0;
void printLocalTime() {
  preb++;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("No time available (yet)");
    return;
  } else {
    int fityear = timeinfo.tm_year % 100;
    Ds1302::DateTime temp = {
      .year = fityear,
      .month = timeinfo.tm_mon+1,
      .day = timeinfo.tm_mday,
      .hour = timeinfo.tm_hour,
      .minute = timeinfo.tm_min,
      .second = timeinfo.tm_sec,
      .dow = timeinfo.tm_wday+1
    };
    rtc.setDateTime(&temp);
  }
}

void getNetTime()  //u8g2.clearBuffer();
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_tr);
  u8g2.setCursor(0, 14);
  u8g2.print("Update time...");
  u8g2.setFont(u8g2_font_open_iconic_www_2x_t);
  u8g2.drawGlyph(105, 35, 0x43);
  u8g2.sendBuffer();
  printLocalTime();
}
