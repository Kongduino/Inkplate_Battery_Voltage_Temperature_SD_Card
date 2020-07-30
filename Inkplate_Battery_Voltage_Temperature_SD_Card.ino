/*
A sample project adapted from the original Inkplate 6 samples.
Displays battery voltage and Temperature with icons loaded from the SD card.
*/
#include "Inkplate.h" //Include Inkplate library to the sketch
#include "SdFat.h" //Include library for SD card
#include <time.h>
#include <sys/time.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "nvs_flash.h"
#include "apps/sntp/sntp.h"

Inkplate display(INKPLATE_3BIT); //Create an object on Inkplate library and also set library into 1-bit mode (Monochrome)
SdFile file; //Create SdFile object used for accessing files on SD card
int temperature;
float voltage;
int n = 0;

void displayTD() {
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  // Is time set? If not, tm_year will be (1970 - 1900).
  if (timeinfo.tm_year < (2016 - 1900)) {
    Serial.print(F("Time is not set yet. Connecting to WiFi and getting time over NTP.\n"));
    obtain_time();
    // update the 'now' variable with the current time
    time(&now);
  }
  char strftime_buf[64];
  // Set timezone to China Standard Time
  setenv("TZ", "CST-8", 1);
  tzset();
  localtime_r(&now, &timeinfo);
  display.setTextSize(3);
  strftime(strftime_buf, sizeof(strftime_buf), "%F [%a]", &timeinfo);
  Serial.printf("The current datein HK is: %s\n", strftime_buf);
  display.setCursor(250, 10);
  display.print(strftime_buf);
  strftime(strftime_buf, sizeof(strftime_buf), "%H:%M:%S", &timeinfo);
  Serial.printf("The current time in HK is: %s\n", strftime_buf);
  display.setCursor(300, 40);
  display.print(strftime_buf);
  display.setTextSize(4);
}

void displayTB() {
  temperature = display.readTemperature();
  voltage = display.readBattery();
  display.setCursor(70, 20);
  display.print(voltage, 2);
  display.print('V');
  display.setCursor(660, 20);
  display.print(temperature, DEC); //Print temperature
  display.print('C');
  Serial.print("Temperature: "); Serial.print(temperature, DEC); Serial.println('*C');
  Serial.print("Battery: "); Serial.print(voltage, 2); Serial.println('V');
}

void loadPicture(char *fName, uint16_t offsetX, uint16_t offsetY) {
  if (!file.open(fName, O_RDONLY)) {
    //If it fails to open, send error message to display, otherwise read the file.
    Serial.println("File open error");
    return;
  }
  uint16_t myWidth, myHeight;
  int len = file.fileSize(), readLen = 0;
  myWidth = file.read() + file.read() * 256;
  myHeight = file.read() + file.read() * 256;
  Serial.println("len: " + String(len));
  Serial.println("w: " + String(myWidth) + ", h: " + String(myHeight));
  char myBuffer[myWidth];
  uint16_t ix = 0, jx = 0, px = 0, w2 = myWidth / 2;
  while (readLen < len) {
    file.read(myBuffer, w2);
    // Read a line's worth: width/2
    uint8_t v;
    for (ix = 0; ix < w2; ix++) {
      v = myBuffer[ix];
      // we cannot assume that both nibbles are for the same line --> width = odd number
      // the last pixel of a line will be the high nibble, followed by the first pixel of the next line
      // so we must treat each half separately and check
      display.drawPixel(offsetX + px++, offsetY + jx, (v >> 4) >> 1);
      if (px == myWidth) {
        px = 0;
        jx++;
      }
      display.drawPixel(offsetX + px++, offsetY + jx, (v & 0x0F) >> 1);
      if (px == myWidth) {
        px = 0;
        jx++;
      }
    }
    readLen += w2;
  } Serial.println("Read: " + String(readLen));
  file.close();
}

void obtain_time(void) {
  initialize_sntp();
  // wait for time to be set
  time_t now = 0;
  struct tm timeinfo = { 0 };
  int retry = 0;
  const int retry_count = 12;
  while (timeinfo.tm_year < (2018 - 1900) && ++retry < retry_count) {
    Serial.printf("Waiting for system time to be set... (%d/%d)\n", retry, retry_count);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    time(&now);
    localtime_r(&now, &timeinfo);
  }
}

void initialize_sntp(void) {
  Serial.println("Initializing SNTP");
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "pool.ntp.org");
  sntp_init();
}

boolean checkConnection() {
  Serial.print(F("Waiting for Wi-Fi connection"));
  uint8_t count = 0;
  while (count < 30) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected!");
      return (true);
    }
    delay(500);
    count += 1;
    Serial.print(".");
  }
  Serial.println(F("Timed out."));
  return false;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  while (Serial.available()) char c = Serial.read();
  Serial.println("\n\nRaw Bitmap Test");
  display.begin();
  display.clearDisplay();
  display.display();
  String wifi_ssid = "SSID";
  String wifi_password = "PWD";
  Serial.print(F("WIFI-SSID: "));
  Serial.println(wifi_ssid);
  Serial.print(F("WIFI-PASSWD: "));
  Serial.println(wifi_password);
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
  bool x = false;
  while (!x) {
    Serial.write(' ');
    x = checkConnection();
  }
  Serial.println(WiFi.localIP());
  if (display.sdCardInit()) {
    Serial.println("SD Card ok! Reading data...");
  } else {
    //If card init was not successful, display error on screen and stop the program (using infinite loop)
    Serial.println("SD Card error!");
    while (true);
  }
  loadPicture("Battery64.raw", 0, 0);
  loadPicture("Temperature64.raw", 740, 0);
  display.setTextSize(4);
  display.setTextColor(0, 7);
  displayTB();
  displayTD();
  display.display();
}

void loop() {
  displayTB();
  displayTD();
  if (n > 10) {
    //Check whether you need to do a full refresh or you can do a partial update
    display.display();
    n = 0;
  } else {
    display.partialUpdate();
    n++;
  }
  delay(1000);
}
