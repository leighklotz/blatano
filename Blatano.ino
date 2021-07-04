/*
 * Leigh Klotz <klotz@klotz.me> ESP32 Bluetooth Friend "Blatano"
 * Bluetooth scanner based on Neil Kolban example for IDF
 * - https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp 
 * - Ported to Arduino ESP32 by Evandro Copercini
 * Robot name is due to me
 * PixelRobot is due to 
 * http://www.jake.dk/programmering/html/gamlehjemmesider/xnafan/xnafan.net/2010/01/xna-pixel-robots-library/index.html
 * - Dave Bollinger, Pixel Robot Generator, Wayback Machine](http://web.archive.org/web/20080228054405/http://www.davebollinger.com/works/pixelrobots
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

#include "Blatano.h"
#include "PixelRobot.h"

#include "wifixbm.h"
#include "bluetoothxbm.h"

int scanTime = 5; //In seconds
BLEScan* pBLEScan;

#define D3 OLED_SDA
#define D5 OLED_SCL

// Initialize the OLED display using Wire library
// https://github.com/Xinyuan-LilyGO/LilyGo-W5500-Lite/tree/master/libdeps/esp8266-oled-ssd1306
// installed from Arduino IDE ThingPulse, Fabrice Weinberg v 4.2.0
SSD1306Wire display(0x3C, D3, D5);

enum servicetype_t getWellKnownServiceType(BLEAdvertisedDevice *d);
BLEUUID COVID_TRACKER_SERVICEUUID = BLEUUID("0000fd6f-0000-1000-8000-00805f9b34fb");
BLEUUID GATT_SERVICEUUID = BLEUUID("e20a39f4-73f5-4bc4-a12f-17d1ad07a961");
BLEUUID FITBIT_CHARGE_3_SERVICEUUID = BLEUUID("adabfb00-6e7d-4601-bda2-bffaa68956ba");

const int TEXT_FIRST_LINE = 8;
const int LEADING = 12;
char linebuf[128];		// 128 characters ought to be enough for anyone
Arduino_CRC32 crc32;


#define PRINTF(s_, ...) snprintf(linebuf, 128, (s_), ##__VA_ARGS__); Serial.println(linebuf)

const int ROBOT_SCALE = 5;
PixelRobot pixel_robot;
memory_t memory[32];

void draw_pixel_robot(uint32_t robot_num);
void enhance(blatano_t *blat);
void printDevice(BLEAdvertisedDevice d);
void displayDevice(int i, blatano_t blat);
void draw_splash_screen();

void setup() {
  Serial.begin(115200);

  display.init();
  display.setContrast(255);
  display.clear();
  display.display();
  PRINTF("Startup");

  PRINTF("Setup Names");
  setup_names();
#if TEST_NAMES
  PRINTF("Test robot names");
  test_names();
  PRINTF("Robot names done");
#endif
  
  PRINTF("Setup PixelRobot");
  pixel_robot = PixelRobot();
  pixel_robot.setScales(5, 5);
  pixel_robot.setMargins(1, 0);
  pixel_robot.generate(0);

  PRINTF("Create Scanner");
  BLEDevice::init("Blatano_0x00");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(60);
  pBLEScan->setWindow(59);  // less or equal setInterval value

  PRINTF("Draw Splash Screen");
  draw_splash_screen();
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
  // robot_named_n doesn't cover the whole crc32 bits
  robot_named_n(blat->name, blat->crc32); 
}

void loop() {
  PRINTF("Scanning");
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  {
    display.clear();
    display.display();
  }

  PRINTF("Found: %d", foundDevices.getCount());
  int k = foundDevices.getCount();
  uint8_t dup_index = 0;
  int display_width = display.getWidth();
  int robot_width = pixel_robot.getWidth();
  for (int i = 0; i < k; i++)  {
    blatano_t blat = {};	// {} to clear to zero
    display.clear();
    int progress = k==1 ? 1 : (i * 100/(k-1));
    display.drawProgressBar(robot_width+1, 0, display_width-(robot_width+1)-1, TEXT_FIRST_LINE, progress);
    processDevice(&blat, &foundDevices, i, &dup_index);
    displayDevice(i, blat);
  }
  // delete results fromBLEScan buffer to release memory
  pBLEScan->clearResults();   
}

void displayDevice(int i, blatano_t blat) {
  PRINTF("- %d %X %s\n", i, blat.crc32, blat.name);
  snprintf(linebuf, sizeof linebuf, "%d %X %s", i, blat.crc32, blat.name);

  int display_width = display.getWidth();
  int robot_width = pixel_robot.getWidth();

  // i CRC32
  display.setFont(ArialMT_Plain_10);
  display.drawStringf(robot_width+10, 64-LEADING, linebuf, "%d %x",i, blat.crc32);
  // blat.name
  display.setFont(ArialMT_Plain_16);
  display.drawStringMaxWidth(robot_width+5, TEXT_FIRST_LINE, display_width-(robot_width+5)-1, blat.name);

  // draw_pixel_robot
  // PixelRobot uses 24-bits, so we lose 8 bits here.
  // For my neighborheed, top 24 bits looked better so I took those.
  // Maybe investigate spined robots
  draw_pixel_robot((blat.crc32 & 0xffffff00) >> 8);
  display.display();
  delay(1000);
}


// test with robot_num = 0x1af824;
void draw_pixel_robot(uint32_t robot_num) {
  PRINTF("- robot_num %x\n", robot_num);
  pixel_robot.clear();
  pixel_robot.generate(robot_num);
  pixel_robot.draw(0, 0);//1*ROBOT_SCALE
}

void draw_splash_screen() {
  display.setFont(ArialMT_Plain_24);
  display.drawStringMaxWidth(8, 0, 127, "Ego Blatano");
  display.display();
  delay(5000);
  display.clear();
  display.drawXbm(0 + 32, 0 + 32, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
  display.drawXbm(128-32, 0 + 32, bluetooth_logo_width, bluetooth_logo_height, bluetooth_logo_bits);
  display.display();
}
