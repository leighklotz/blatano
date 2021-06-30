/*
 * Leigh Klotz <klotz@klotz.me> ESP32 Bluetooth friend finder
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
#include <BLEUUID.h>

#include <Arduino_CRC32.h>

#include "BLEScanner2.h"

int scanTime = 5; //In seconds
BLEScan* pBLEScan;

#define D3 OLED_SDA
#define D5 OLED_SCL

// Initialize the OLED display using Wire library
// https://github.com/Xinyuan-LilyGO/LilyGo-W5500-Lite/tree/master/libdeps/esp8266-oled-ssd1306
// installed from Arduino IDE ThingPulse, Fabrice Weinberg v 4.2.0
SSD1306Wire  display(0x3C, D3, D5);

enum servicetype_t getWellKnownServiceType(BLEAdvertisedDevice *d);
BLEUUID COVID_TRACKER_SERVICEUUID = BLEUUID("0000fd6f-0000-1000-8000-00805f9b34fb");
BLEUUID GATT_SERVICEUUID = BLEUUID("e20a39f4-73f5-4bc4-a12f-17d1ad07a961");
BLEUUID FITBIT_CHARGE_3_SERVICEUUID = BLEUUID("adabfb00-6e7d-4601-bda2-bffaa68956ba");

const int TEXT_FIRST_LINE = 8;
const int LEADING = 12;
char linebuf[64];
Arduino_CRC32 crc32;


#define PRINTF(s_, ...) snprintf(linebuf, 64, (s_), ##__VA_ARGS__); Serial.println(linebuf)

extern void test_crc();
extern void setup_crc();
extern char *robot_named_n(char *buf, uint32_t n);
extern void enhance(blatano_t *blat);
extern void printDevice(BLEAdvertisedDevice d);

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
#if 0
  PRINTF("Testing CRC");
  test_crc();
  PRINTF("CRC Test Done");
  PRINTF("Testing robot names");
  test_names();
  PRINTF("Robot names done");
#endif
  
  PRINTF("Scanning");
  BLEDevice::init("Blatano_0x00");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(60);
  pBLEScan->setWindow(59);  // less or equal setInterval value
}

void processDevice(blatano_t *pblat, BLEScanResults *foundDevices, int i, uint8_t *pdup_index) {
  BLEAdvertisedDevice d = foundDevices->getDevice(i);
#if 1
  printDevice(d);
#endif
  convert(&d, pblat, pdup_index);
  enhance(pblat);
}

enum servicetype_t getWellKnownServiceType(BLEAdvertisedDevice *d) {
  BLEUUID serviceUUID = d->getServiceUUID();
  servicetype_t r = UNKNOWN;
  if (serviceUUID.equals(COVID_TRACKER_SERVICEUUID)) r = COVID_TRACKER;
  else if (serviceUUID.equals(GATT_SERVICEUUID)) r = GATT;
  else if (serviceUUID.equals(FITBIT_CHARGE_3_SERVICEUUID)) r = FITBIT_CHARGE_3;
  return r;
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
  PRINTF("* %s", d.getAddress().toString().c_str());

  char *service_type_names[] = { "UNKNOWN", "COVID_TRACKER", "GATT", "FITBIT_CHARGE_3" };
  int s_type = (int)getWellKnownServiceType(&d);
  if (s_type < sizeof(service_type_names) / sizeof (char *)) {
    PRINTF("- ServiceType %d %s", s_type, service_type_names[s_type]);
  } else {
    PRINTF("- ServiceType %d [offlist]", s_type);
  }

  PRINTF("- RSSI %d", d.getRSSI());

  PRINTF("- Address Type = 0x%x %s", ((uint8_t)(d.getAddressType())),
	 addressTypeToString(d.getAddressType()));

  if (d.haveName()) {
    PRINTF("- Name: %s", d.getName().c_str());
  }

  if (d.haveAppearance()) {
    PRINTF("- Appearance: %d", d.getAppearance());
  }

  if (d.haveManufacturerData()) {
    std::string md = d.getManufacturerData();
    uint8_t *mdp = (uint8_t *)d.getManufacturerData().data();
    char *pHex = BLEUtils::buildHexData(nullptr, mdp, md.length());
    PRINTF("- ManufacturerData: %s", pHex);
    free(pHex);
  }

  if (d.haveServiceUUID()) {
    PRINTF("- ServiceUUID: %s\b", d.getServiceUUID().toString().c_str());
  }

  if (d.haveTXPower()) {
    PRINTF("- TxPower: %d", (int)d.getTXPower());
  }
}
#endif


#define BLAT_STRING(blat_field, s) memcpy(blat_field, s.data(), min(s.length(), sizeof(blat_field)))
#define BLAT_DATA(blat_field, data) memcpy(blat_field, data, sizeof(blat_field))

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
    // manufacturer_code is 2 bytes since that's the code
    // the rest of the data varies
    BLAT_STRING(blat->manufacturer_code, d->getManufacturerData());
  }

  if (d->haveServiceUUID()) {
    BLAT_DATA(blat->service_uuid, d->getServiceUUID().to128().getNative()->uuid.uuid128);
    blat->serviceType=getWellKnownServiceType(d);
  }

  if (d->haveTXPower()) {
    blat->tx_power = d->getTXPower();
  }
 
}

void enhance(blatano_t *blat) {
  uint8_t const *blatp = ((uint8_t const *) blat);
  blat->crc32 = crc32.calc(blatp, offsetof(blatano_t, BLATANO_T_CRC_END));
#if 0
  PRINTF("- 0x%x = crc32.calc(blatp, %d)", blat->crc32, offsetof(blatano_t, BLATANO_T_CRC_END));
  for (int i = 0; i < offsetof(blatano_t, BLATANO_T_CRC_END); i++) {
    PRINTF("-- blat[0x%x] = 0x%x", i, blatp[i]);
  }
#endif
  robot_named_n(blat->name, blat->crc32);
}

void loop() {
  PRINTF("Scanning");
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  {
    // display.clear();
    // display.drawXbm(0 + 17, 0 + 17, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
    // display.drawXbm(128-32, 0 + 17, bluetooth_logo_width, bluetooth_logo_height, bluetooth_logo_bits);
    // display.display();
    // delay(1000);
    display.clear();
    display.display();
  }

  PRINTF("Found: %d", foundDevices.getCount());
  int k = foundDevices.getCount();
  uint8_t dup_index = 0;
  for (int i = 0; i < k; i++)  {
    blatano_t blat = {};	// {} to clear to zero
    display.clear();
    int progress = k==1 ? 1 : (i * 100/(k-1));
    display.drawProgressBar(64, 0, 63, TEXT_FIRST_LINE, progress);
    processDevice(&blat, &foundDevices, i, &dup_index);
    PRINTF("- %d %X %s\n", i, blat.crc32, blat.name);
    snprintf(linebuf, sizeof linebuf, "%d %X %s", i, blat.crc32, blat.name);
    // i*LEADING+TEXT_FIRST_LINE
    display.drawStringMaxWidth(0, 1*LEADING, 128, linebuf);
    display.display();
    delay(1000);
  }
  // delete results fromBLEScan buffer to release memory
  pBLEScan->clearResults();   
}
