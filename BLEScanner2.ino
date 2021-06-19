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

// Can't have both callback and returned list, and callback doesn't seem
// to work well with the display. So just Serial. And turn it off
// completely since they don't build an array if you have a callback.
#if 0
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.println("Advertised Device: %s \n", advertisedDevice.toString().c_str());
    }
};
#endif

void SCREEN(char buf[]) {
  Serial.println(buf);
  display.clear();
  display.drawStringMaxWidth(0, 0, 96, buf);
  display.display();
  delay(2000);

}

void SCREEN_I(int idx, const char *fmt, int i) {
  char buf[128];
  snprintf(buf, 128, fmt, idx, i);
  SCREEN(buf);
}

void SCREEN_SI(int idx, const char *fmt, const char *s, int i) {
  char buf[128];
  snprintf(buf, 128, fmt, idx, s, i);
  SCREEN(buf);
}

void SCREEN_S(int idx, const char *fmt, const char *s) {
  char buf[128];
  snprintf(buf, 128, fmt, idx, s);
  SCREEN(buf);
}

void setup() {
  Serial.begin(115200);

  display.init();
  display.setContrast(255);
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.display();
  SCREEN("[-] Startup: BLEScanner2.ino");
  SCREEN("[-] Scanning: BT");
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  // Don't use callbacks since we want to need to wait to use the display till after the data loads
#if 0
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
#endif
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
}

void printDevice(BLEScanResults foundDevices, int i) {
  BLEAdvertisedDevice dev = foundDevices.getDevice(i);

  SCREEN_SI(i, "[%d] %s: RSSI %d\n", dev.getAddress().toString().c_str(), dev.getRSSI());
  SCREEN_S(i, "[%d] %s", dev.toString().c_str());
#if 1
  printDeviceDetails(i, dev);
#endif
}

struct blatano {
  uint8_t signature[8];
  uint8_t rssi;
  char name[16];
  uint8_t appearance;
  uint8_t mdp[16];
  uint8_t serviceUUID[16];
  uint8_t txpower;
};

#include <CircularBuffer.h>

CircularBuffer<struct blatano,32> correspondents;

void printDeviceDetails(int i, BLEAdvertisedDevice dev) {
  SCREEN_SI(i, "[%d] %s: RSSI %d\n", dev.getAddress().toString().c_str(), dev.getRSSI());

  if (dev.haveName()) {
    SCREEN_S(i, "[%d] Name: %s", dev.getName().c_str());
  }

  if (dev.haveAppearance()) {
    SCREEN_I(i, "[%d] Appearance: %d", dev.getAppearance());
  }

  if (dev.haveManufacturerData()) {
    std::string md = dev.getManufacturerData();
    uint8_t *mdp = (uint8_t *)dev.getManufacturerData().data();
    char *pHex = BLEUtils::buildHexData(nullptr, mdp, md.length());
    SCREEN_S(i, "[%d] ManufacturerData: %s", pHex);
    free(pHex);
  }

  if (dev.haveServiceUUID()) {
    SCREEN_S(i, "[%d] ServiceUUID: %s", dev.getServiceUUID().toString().c_str());
  }

  if (dev.haveTXPower()) {
    SCREEN_I(i, "[%d] TxPower: %d", (int)dev.getTXPower());
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  SCREEN_I(-1, "[%d], Devices found: %d", foundDevices.getCount());
  for (int i = 0; i < foundDevices.getCount(); i++)  {
    printDevice(foundDevices, i);
  }
  // delete results fromBLEScan buffer to release memory
  pBLEScan->clearResults();   
  display.clear();
}
