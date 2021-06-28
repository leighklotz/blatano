/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by Evandro Copercini
*/

#include <Arduino.h>
#include <Wire.h>
#include "SSD1306Wire.h"
#define OLED_SCL       22
#define OLED_SDA       21

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include <CircularBuffer.h>
#include <Arduino_CRC32.h>

#include "BLEScanner2.h"
#include "wifixbm.h"
#include "bluetoothxbm.h"

int scanTime = 5; //In seconds
BLEScan* pBLEScan;

#define D3 OLED_SDA
#define D5 OLED_SCL

// Initialize the OLED display using Wire library
// https://github.com/Xinyuan-LilyGO/LilyGo-W5500-Lite/tree/master/libdeps/esp8266-oled-ssd1306
// installed from Arduino IDE ThingPulse, Fabrice Weinberg v 4.2.0
SSD1306Wire  display(0x3C, D3, D5);

extern void test_crc();
extern void setup_crc();
extern char *robot_named_n(char *buf, uint32_t n);

const int TEXT_FIRST_LINE = 8;
const int LEADING = 12;

char linebuf[64];

Arduino_CRC32 crc32;

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

#define PRINTF(s_, ...) snprintf(linebuf, 64, (s_), ##__VA_ARGS__); Serial.println(linebuf)


void setup() {
  Serial.begin(115200);

  display.init();
  display.setContrast(255);
  display.clear();
  // display.setFont(ArialMT_Plain_10);
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.display();
  PRINTF("Startup");

  PRINTF("Setup CRC");
  setup_crc();
#ifdef TEST
  PRINTF("Testing CRC");
  test_crc();
  PRINTF("CRC Test Done");
  PRINTF("Testing robot names");
  test_names();
  PRINTF("Robot names done");
#endif
  
  delay(500);
  PRINTF("Scanning\n");
  BLEDevice::init("Blatano_0x00");
  pBLEScan = BLEDevice::getScan(); //create new scan
#if 0
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
#endif
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(60);
  pBLEScan->setWindow(59);  // less or equal setInterval value
}

// __attribute__((packed))


#define N_BLATANI 16
CircularBuffer<blatano_t,16> blatani;

void enhance(blatano_t *blat);

extern void printDevice(BLEAdvertisedDevice d);

void queueDevice(BLEScanResults *foundDevices, int i, uint8_t *pdup_index) {
  blatano_t blat;
  BLEAdvertisedDevice d = foundDevices->getDevice(i);
  Serial.printf("Advertised Device: %s \n", d.toString().c_str());
  printDevice(d);
  convert(&d, &blat, pdup_index);
  enhance(&blat);
  blatani.push(blat);
  PRINTF("%d %X %s\n", i, blat.crc32, blat.name);
}

#if 1
/**
 * @brief Convert an esp_ble_addr_type_t to a string representation.
 */
const char* addressTypeToString(esp_ble_addr_type_t type) {
  switch (type) {
  case BLE_ADDR_TYPE_PUBLIC:
    return "BLE_ADDR_TYPE_PUBLIC";
  case BLE_ADDR_TYPE_RANDOM:
    return "BLE_ADDR_TYPE_RANDOM";
  case BLE_ADDR_TYPE_RPA_PUBLIC:
    return "BLE_ADDR_TYPE_RPA_PUBLIC";
  case BLE_ADDR_TYPE_RPA_RANDOM:
    return "BLE_ADDR_TYPE_RPA_RANDOM";
  default:
    return " esp_ble_addr_type_t";
  }
}

void printDevice(BLEAdvertisedDevice d) {
  PRINTF("%s: RSSI %d\n", d.getAddress().toString().c_str(), d.getRSSI());

  PRINTF("- Address Type = 0x%x %s", ((uint8_t)(d.getAddressType())),
	 addressTypeToString(d.getAddressType()));

  if (d.haveName()) {
    PRINTF("- Name: %s\n", d.getName().c_str());
  }

  if (d.haveAppearance()) {
    PRINTF("- Appearance: %d\n", d.getAppearance());
  }

  if (d.haveManufacturerData()) {
    std::string md = d.getManufacturerData();
    uint8_t *mdp = (uint8_t *)d.getManufacturerData().data();
    char *pHex = BLEUtils::buildHexData(nullptr, mdp, md.length());
    PRINTF("- ManufacturerData: %s\n", pHex);
    free(pHex);
  }

  if (d.haveServiceUUID()) {
    PRINTF("- ServiceUUID: %s\b", d.getServiceUUID().toString().c_str());
  }

  if (d.haveTXPower()) {
    PRINTF("- TxPower: %d\n", (int)d.getTXPower());
  }
}
#endif


#define BLAT_STRING(blat_field, s) memcpy(blat_field, s.data(), min(s.length(), sizeof(blat_field)))
#define BLAT_DATA(blat_field, data) memcpy(blat_field, data, min(sizeof(data), sizeof(blat_field)))

void convert(BLEAdvertisedDevice *d, blatano_t* blat, uint8_t *pdup_index) {
  //  d.getAddress().toString().c_str()
  blat->address_type = ((int)(d->getAddressType()));

  if (blat->address_type == BLE_ADDR_TYPE_PUBLIC ||
      blat->address_type == BLE_ADDR_TYPE_RPA_PUBLIC) {
    BLAT_DATA(blat->mac, d->getAddress().getNative());
  } else {
    // for random addresses, just take their sequence number in scan order
    // and use that as the first byte of the MAC and the rest zero.
    // This gives repeatable names for random devices if the same devices
    // show up together in/ groups.
    // - can't keep track of individuals
    // - but each set of random devices will experience the same names, though
    // - some names will change when groups merge.
    ((uint8_t *)(blat->mac))[0] = (*pdup_index)++;
  }

  if (d->haveRSSI()) {
    blat->rssi = d->getRSSI();
  }

  if (d->haveName()) {
    BLAT_STRING(blat->name, d->getName());
  }

  if (d->haveAppearance()) {
    blat->appearance = d->getAppearance();
  }

  if (d->haveManufacturerData()) {
    BLAT_STRING(blat->manufacturer_data, d->getManufacturerData());
  }

  if (d->haveServiceUUID()) {
    BLAT_DATA(blat->service_uuid, d->getServiceUUID().to128().getNative()->uuid.uuid128);
  }

  if (d->haveTXPower()) {
    blat->tx_power = d->getTXPower();
  }
 
}

void enhance(blatano_t *blat) {
  uint8_t const *blatp = ((uint8_t const *) blat);
  blat->crc32 = crc32.calc(blatp, offsetof(blatano_t, BLATANO_T_CRC_END));
  robot_named_n(blat->name, blat->crc32);
}

void loop() {
  // put your main code here, to run repeatedly:
  PRINTF("LOOP");
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  PRINTF("BLEScanResults DONE");
  {
    display.clear();
    display.drawXbm(0 + 17, 0 + 17, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
    display.drawXbm(128-32, 0 + 17, bluetooth_logo_width, bluetooth_logo_height, bluetooth_logo_bits);
    display.display();
    delay(1000);
  }

  {
    display.clear();

    display.display();
    delay(1000);
  }
  PRINTF("Found: %d", foundDevices.getCount(), 0);
  int k = foundDevices.getCount();
  uint8_t dup_index = 0;
  for (int i = 0; i < k; i++)  {
    display.clear();
    int progress = k==1 ? 1 : (i * 100/(k-1));
    display.drawProgressBar(64, 0, 63, TEXT_FIRST_LINE, progress);
    queueDevice(&foundDevices, i, &dup_index);
    blatano_t blat = blatani.last();
    snprintf(linebuf, sizeof linebuf, "%d %X %s\n", i, blat.crc32, blat.name);
    // i*LEADING+TEXT_FIRST_LINE
    display.drawStringMaxWidth(0, 1*LEADING, 128, linebuf);
    display.display();
    delay(1000);
  }
  // delete results fromBLEScan buffer to release memory
  pBLEScan->clearResults();   
}
