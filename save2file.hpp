#include "FS.h"
#include "SPIFFS.h"


void readFile(fs::FS &fs, const char *path, String &out) {
  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }
  while (file.available()) {
    out = file.readString();
  }
  file.close();
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  file.print(message);
  file.close();
}
