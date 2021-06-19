/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by Evandro Copercini
*/

#include <Arduino.h>
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h"
#define OLED_SCL       22
#define OLED_SDA       21

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include <sstream>

int scanTime = 5; //In seconds
BLEScan* pBLEScan;

#define D3 OLED_SDA
#define D5 OLED_SCL

// Initialize the OLED display using Wire library
// https://github.com/Xinyuan-LilyGO/LilyGo-W5500-Lite/tree/master/libdeps/esp8266-oled-ssd1306
// installed from Arduino IDE ThingPulse, Fabrice Weinberg v 4.2.0
SSD1306Wire  display(0x3C, D3, D5);
extern void PRINT(const char *x);
extern void PRINTLN(const char *x);
extern void PRINTFN(const char *x, const char *y);
extern void PRINTF1(const char *x, int y);
extern void PRINTFN2(const char *x, const char *y, int z);
extern void PRINTLNI(int i);
#define LEADING (12)

int lineno = 0;
int colno = 0;

void clear() {
  display.clear();
  lineno = 0;
  colno = 0;
}

void newline() {
  if (++lineno == 5) {
    delay(500);
    clear();
  } else {
    colno = 0;
  }
}


// Can't have both callback and returned list, and callback doesn't seem
// to work well with the display.  So just Serial.
#if 0
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.println("Advertised Device: %s \n", advertisedDevice.toString().c_str());
    }
};
#endif


// todo: this print stuff has gotten out of hand

void PRINT(const char *x) {
  Serial.print(x);
  display.drawStringMaxWidth(colno, lineno *16, 96, x);
  // todo line wrapp effect on lineno indeterminate since it wraps at hyphens and spaces
  colno += display.getStringWidth(x);	
  display.display();
  delay(500);
}

void PRINTLN(const char *x) {
  Serial.println(x);
  display.drawStringMaxWidth(colno, lineno * 16, 96, x);
  newline();
  display.display();
  delay(500);
}

void PRINTLNI(int i) {
  char buf[16];
  snprintf(buf, 16, "%d", i);
  PRINTLN(buf);
}


void PRINTFN(const char *x, const char *y) {
  Serial.printf(x, y);
  char buf[64];
  snprintf(buf, 64, x, y);
  display.drawStringMaxWidth(colno, lineno * 16, 96, buf);
  // Display it on the screen
  display.display();
  delay(500);
  newline();
}

void PRINTFN1(const char *x, int y) {
  Serial.printf(x, y);
  char buf[64];
  snprintf(buf, 64, x, y);
  display.drawStringMaxWidth(colno, lineno * 16, 96, buf);
  // Display it on the screen
  display.display();
  delay(500);
  newline();
}

void PRINTFN2(const char *x, const char *y, int z) {
  Serial.printf(x, y);
  char buf[128];
  snprintf(buf, 64, x, y, z);
  display.drawStringMaxWidth(colno, lineno * 16, 96, buf);
  // Display it on the screen
  display.display();
  delay(500);
  newline();
}

void setup() {
  Serial.begin(115200);

  display.init();
  display.setContrast(255);
  clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  PRINTFN("Startup: %s\n", "foo");
  display.display();
  delay(2000);

  PRINTFN("Scanning...: %s\n", "bar");

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
#if 0
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
#endif
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
}


void printDevice(BLEScanResults foundDevices, int i) {
  BLEAdvertisedDevice d = foundDevices.getDevice(i);
  
  PRINTFN2("%s: RSSI %d\n", d.getAddress().toString().c_str(), d.getRSSI());

  if (d.haveName()) {
    PRINTFN("Name: %s", d.getName().c_str());
  }

  if (d.haveAppearance()) {
    PRINTFN1("Appearance: %d", d.getAppearance());
  }

  if (d.haveManufacturerData()) {
    std::string md = d.getManufacturerData();
    uint8_t *mdp = (uint8_t *)d.getManufacturerData().data();
    char *pHex = BLEUtils::buildHexData(nullptr, mdp, md.length());
    PRINTFN("ManufacturerData: %s", pHex);
    free(pHex);
  }

  if (d.haveServiceUUID()) {
    PRINTFN("ServiceUUID: %s", d.getServiceUUID().toString().c_str());
  }

  if (d.haveTXPower()) {
    PRINTFN1("TxPower: %d", (int)d.getTXPower());
  }
}

void loop() {
  delay(2000);
  // put your main code here, to run repeatedly:
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  clear();
  PRINT("Devices found: ");
  PRINTLNI(foundDevices.getCount());
  delay(2000);
  clear();
  for (int i = 0; i < foundDevices.getCount(); i++)  {
    printDevice(foundDevices, i);
  }
  // delete results fromBLEScan buffer to release memory
  pBLEScan->clearResults();   
  delay(2000);
}
