#include <Arduino.h>
#include <Ds1302.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>
#include "nettime.hpp"
#include "fun.hpp"
#include "warclock.hpp"
#include "ws2812.hpp"
#include "save2file.hpp"
#include "TheBme280.hpp"
#include "OneButton.h"
#define KEY_update 26
#define KEY_rst 27
#define NUMPIXELS 1
#define PIN_NEOPIXEL 13

OneButton button1(KEY_update, true);
OneButton button2(KEY_rst, true);

Ds1302 rtc(17, 22, 21);                                                                     // DS1302时钟实例
U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R2, /* cs=*/5, /* dc=*/16, /* reset=*/4);  //显示对象
Adafruit_NeoPixel pixels(NUMPIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
const char *ssid = "301-1";            // wifi nane
const char *password = "15868495985";  // wifi password
Ds1302::DateTime now;                  // 当前时间对象
String myDate = "20";                  // 年份字符串首部，uint8_t存超过255会溢出
String mytime;                         // 时间字符串
float temperature = 0;
float presure = 0;
float altitude = 0;
float humidity = 0;
bool wifiConnect = true;
int brightness[6] = { 10, 50, 100, 150, 200, 255 };  //亮度值
int bnPerb = 4;                                      //上边亮度数组的指针
uint8_t invertS = 0xa7;                              //反色指令
bool invertSF = false;                               //反色标志
bool powersave = false;                              //节电标志
bool rotateS = false;                                //旋转标志
int MaxConnectTimes = 10;                            //wifi的最大重新连接次数
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
extern void bme280_Init();
extern void bme280_GetVal();
volatile SemaphoreHandle_t timerSemaphore;

volatile uint32_t isrCounter = 0;
volatile uint32_t lastIsrAt = 0;

void ARDUINO_ISR_ATTR onTimer() {
  xSemaphoreGiveFromISR(timerSemaphore, NULL);
}

void show(Ds1302::DateTime timeinfo)  // 屏幕布局
{
  now = timeinfo;
  u8g2.clearBuffer();
  u8g2.setContrast(brightness[bnPerb]);
  u8g2.setPowerSave(powersave);
  u8g2.sendF("c", invertS);
  if (rotateS)
    u8g2.setDisplayRotation(U8G2_R0);
  else
    u8g2.setDisplayRotation(U8G2_R2);
  /*--------------war clock---------------*/
  showWarClock();
  /*-------------日期----------------*/
  u8g2.setFont(u8g2_font_helvR14_tf);
  u8g2.setCursor(0, 14);
  u8g2.printf("20%d-%02d-%02d", timeinfo.year, timeinfo.month, timeinfo.day);
  /*-------------周----------------*/
  // u8g2.setFont(u8g2_font_7x13_mf);
  u8g2.setCursor(94, 14);
  u8g2.println(WeekDays[timeinfo.dow - 1]);
  /*-------------时间----------------*/
  u8g2.setFont(u8g2_font_helvR14_tf);
  u8g2.setCursor(0, 30);
  u8g2.print(timeinfo.hour);
  u8g2.printf(":%02d", timeinfo.minute);
  u8g2.printf(":%02d", timeinfo.second);
  /*-------------图标----------------*/
  /*https://github.com/olikraus/u8g2/wiki/fntgrpiconic#open_iconic_arrow_1x*/
  u8g2.setFont(u8g2_font_open_iconic_www_2x_t);
  if (wifiConnect) {
    u8g2.drawGlyph(106, 33, 0x51);
  } else {
    u8g2.drawGlyph(106, 33, 0x45);
  }
  /*-------------气温-------------*/
  u8g2.setFont(u8g2_font_ncenR08_tf);
  u8g2.setCursor(1, 62);
  u8g2.printf("%.2f°C, %.2f%c, %.2fhPa", temperature, humidity, 0x25, presure);
  u8g2.setCursor(82, 52);
  u8g2.printf("%.2f m", altitude);
  u8g2.drawBox(0, 49, 75, 2);
  /*-------------亮度-------------*/
  u8g2.setFont(u8g2_font_ncenR08_tf);
  u8g2.setCursor(106, 43);
  u8g2.printf("%d", brightness[bnPerb]);
  /*----------------------------------*/
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
  int connectTimer = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_unifont_tr);
    u8g2.setCursor(0, 14);
    u8g2.println("WIFI connect...");
    u8g2.setFont(u8g2_font_open_iconic_www_2x_t);
    u8g2.drawGlyph(105, 35, 0x53);
    u8g2.sendBuffer();
    connectTimer++;
    if (connectTimer > MaxConnectTimes)
      break;
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
  // Serial.printf("<wctime> %s\n", wctime);
  updateWC(wctime);

  // 设置ds1302初始时间
  // Ds1302::DateTime dt = {
  //   .year = 24,
  //   .month = Ds1302::MONTH_JAN,
  //   .day = 17,
  //   .hour = 12,
  //   .minute = 27,
  //   .second = 30,
  //   .dow = Ds1302::DOW_SUN-
  // };
  // rtc.setDateTime(&dt);

  bme280_Init();
  // link the button 1 functions.
  button1.attachClick(updateTime);
  button1.attachDoubleClick(invertScreen);
  button1.attachLongPressStart(rotateScreen);

  // link the button 2 functions.
  button2.attachClick(powsave);
  button2.attachDoubleClick(changeBN);
  button2.attachLongPressStart(timeRst);
}

void loop() {
  // 按键联网对时
  //  Serial.println("here");
  button1.tick();
  button2.tick();

  if (!WiFi.isConnected()) {
    wifiConnect = false;
  }else{
    wifiConnect = true;
  }

  if (xSemaphoreTake(timerSemaphore, 0) == pdTRUE) {
    RGB_turnOn(255, 255, 255);
    String temp;
    getWC(temp);
    // Serial.printf("<temp> %s\n", temp);
    writeFile(SPIFFS, "/wcsave.txt", temp.c_str());
    bme280_GetVal();
    temperature = bme.readTemperature();
    presure = bme.readPressure() / 100.0F;
    altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
    humidity = bme.readHumidity();
    // delay(100);
    RGB_turnOn(0, 0, 0);
  }
  refreshtime();  // 每秒刷新一次字符串时间
  show(now);      // oled输出
}
void updateTime() {
  if (!WiFi.isConnected()) {
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
  }
  RGB_turnOn(0, 255, 0);
  getNetTime();
  RGB_turnOn(0, 0, 0);
}
void invertScreen() {
  if (invertSF) {
    invertS = 0xa7;
  } else {
    invertS = 0xa6;
  }
  invertSF = !invertSF;
}
void rotateScreen() {
  rotateS = !rotateS;
}
void timeRst() {
  RGB_turnOn(255, 0, 0);
  delay(100);
  returnZero();
  delay(100);
  RGB_turnOn(0, 0, 0);
}
void powsave() {
  powersave = !powersave;
}
void changeBN() {
  bnPerb++;
  bnPerb = bnPerb % 6;
}