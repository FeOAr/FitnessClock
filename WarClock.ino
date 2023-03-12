#include <Arduino.h>
#include <Ds1302.h>
#include <U8g2lib.h>
#include <SPI.h>
#include "nettime.hpp"
#include "fun.hpp"
#include "warclock.hpp"
#include "ws2812.hpp"
#include "save2file.hpp"
#define KEY_update 26
#define KEY_rst 27
#define NUMPIXELS 1
#define PIN_NEOPIXEL 13

Ds1302 rtc(17, 22, 21);                                                                     // DS1302时钟实例
U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R2, /* cs=*/5, /* dc=*/16, /* reset=*/4);  //显示对象
Adafruit_NeoPixel pixels(NUMPIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
const char *ssid = "K2P4A24GHZ";            // wifi nane
const char *password = "19491001newChina";  // wifi password
Ds1302::DateTime now;                       // 当前时间对象
String myDate = "20";                       // 年份字符串首部，uint8_t存超过255会溢出
String mytime;                              // 时间字符串
bool wifiConnect = true;
const char *WeekDays[] = {
  "Sun",
  "Mon",
  "Tue",
  "Wed",
  "Thur",
  "Fri",
  "Sat"
};  // 输出星期
hw_timer_t *timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;

volatile uint32_t isrCounter = 0;
volatile uint32_t lastIsrAt = 0;

void ARDUINO_ISR_ATTR onTimer() {
  xSemaphoreGiveFromISR(timerSemaphore, NULL);
}

void show(Ds1302::DateTime timeinfo)  // 屏幕布局
{
  u8g2.clearBuffer();
  /*--------------war clock---------------*/
  showWarClock();
  /*-------------日期----------------*/
  u8g2.setFont(u8g2_font_helvR14_tf);
  u8g2.setCursor(0, 14);
  u8g2.printf("20%d-%02d-%02d", timeinfo.year, timeinfo.month, timeinfo.day);
  /*-------------周----------------*/
  // u8g2.setFont(u8g2_font_7x13_mf);
  u8g2.setCursor(94, 14);
  u8g2.println(WeekDays[timeinfo.dow-1]);
  /*-------------时间----------------*/
  u8g2.setFont(u8g2_font_crox5h_tf);
  u8g2.setCursor(0, 38);
  u8g2.print(timeinfo.hour);
  u8g2.printf(":%02d", timeinfo.minute);
  u8g2.printf(":%02d", timeinfo.second);
  /*-------------图标----------------*/
  /*https://github.com/olikraus/u8g2/wiki/fntgrpiconic#open_iconic_arrow_1x*/
  u8g2.setFont(u8g2_font_open_iconic_www_2x_t);
  if (wifiConnect) {
    u8g2.drawGlyph(105, 35, 0x51);
  } else {
    u8g2.drawGlyph(105, 35, 0x45);
  }
  /*-----------------------------*/
  u8g2.sendBuffer();
  delay(50);
}

void setup() {
  Serial.begin(115200);
  RGB_Init(20);
  pinMode(KEY_update, INPUT);
  pinMode(KEY_rst, INPUT);
  RGB_turnOn(0, 0, 0);

  rtc.init();  // 时钟初始化
  u8g2.begin();
  u8g2.enableUTF8Print();

  sntp_servermode_dhcp(1);  // (optional)
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);

  WiFi.begin(ssid, password);  // 开启wifi
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_unifont_tr);
    u8g2.setCursor(0, 14);
    u8g2.println("WIFI connect...");
    u8g2.setFont(u8g2_font_open_iconic_www_2x_t);
    u8g2.drawGlyph(105, 35, 0x53);
    u8g2.sendBuffer();
  }

  /*-------定时器部分-------*/
  timerSemaphore = xSemaphoreCreateBinary();
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 10000000, true);
  timerAlarmEnable(timer);
  /*-------文件读写部分-------*/
  if (!SPIFFS.begin(1)) {
    Serial.println("Card Mount Failed");
    return;
  }
  String wctime;
  readFile(SPIFFS, "/wcsave.txt", wctime);
  updateWC(wctime);

  // 设置ds1302初始时间
  Ds1302::DateTime dt = {
    .year = 24,
    .month = Ds1302::MONTH_JAN,
    .day = 17,
    .hour = 12,
    .minute = 27,
    .second = 30,
    .dow = Ds1302::DOW_SUN
  };
  rtc.setDateTime(&dt);
}

void loop() {
  // 按键联网对时
  //  Serial.println("here");
  if (!digitalRead(KEY_update)) {
    delayMicroseconds(10);
    if (!digitalRead(KEY_update)) {
      RGB_turnOn(0, 255, 0);
      getNetTime();
      RGB_turnOn(0, 0, 0);
    }
  }
  if (!digitalRead(KEY_rst)) {
    delayMicroseconds(10);
    if (!digitalRead(KEY_rst)) {
      RGB_turnOn(255, 0, 0);
      delay(100);
      returnZero();
      delay(100);
      RGB_turnOn(0, 0, 0);
    }
  }

  if (!WiFi.isConnected()) {
    wifiConnect = false;
  }

  if (xSemaphoreTake(timerSemaphore, 0) == pdTRUE) {
    RGB_turnOn(255, 255, 255);
    String temp;
    getWC(temp);
    writeFile(SPIFFS, "/wcsave.txt", temp.c_str());
    delay(100);
    RGB_turnOn(0, 0, 0);
  }
  refreshtime();  // 每秒刷新一次字符串时间
  show(now);      // oled输出
}
