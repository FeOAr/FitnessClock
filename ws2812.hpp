#include <Adafruit_NeoPixel.h>

// 初始化灯珠控制实例
extern Adafruit_NeoPixel pixels;

void RGB_Init(int val) {
  pixels.begin();
  pixels.setBrightness(val);
}


void RGB_turnOn(int val1, int val2, int val3) {
  pixels.setPixelColor(0, pixels.Color(val1, val2, val3));
  pixels.show();
}

void RGB_turnOff() {
  pixels.clear();
}
