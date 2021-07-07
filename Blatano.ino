/*
 * Leigh Klotz <klotz@klotz.me> ESP32 Bluetooth Friend "Blatano" for Steve Harrison
 * Completed Tue 06 Jul 2021 09:06:16 PM PDT
 * https://github.com/leighklotz/blatano
 *
 *
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
#include <WiFi.h>

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

#include "wifi-logo-icon.h"
#include "bluetooth-logo-icon.h"
#include "blatano-github-qr-xbm.h"

int n_wifi_found = 0;
int scanTime = 10; //In seconds
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

PixelRobot pixel_robot;
int16_t robot_max_width;
int16_t robot_max_height;
// Theoretically useful:
// const int16_t MIN_BLE_DB = -105;
// const int16_t MAX_BLE_DB = -60;
// Practically seen:
// We just want three sizes: weak, normal, strong
const int16_t MIN_BLE_DB = -90;
const int16_t MAX_BLE_DB = -80;
// weak=3 normal=4 strong=5
const int MIN_ROBOT_SCALE = 3;
const int MAX_ROBOT_SCALE = 5;

const int16_t MIN_WIFI_RSSI=-90;
const int16_t MAX_WIFI_RSSI=-30;

void draw_pixel_robot(uint32_t robot_num);
void enhance(blatano_t *blat);
void printDevice(BLEAdvertisedDevice d);
void displayDevice(int i, blatano_t blat);
void draw_splash_screen();

void setup() {
  Serial.begin(115200);

  display.init();
  display.flipScreenVertically();
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
  pixel_robot.setScales(MAX_ROBOT_SCALE, MAX_ROBOT_SCALE);
  pixel_robot.setMargins(1, 0);
  pixel_robot.generate(0);
  robot_max_width = pixel_robot.getWidth();
  robot_max_height = pixel_robot.getHeight();

  PRINTF("Setup WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

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
  printDevice(i, d);
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

void printDevice(int i, BLEAdvertisedDevice d) {
  PRINTF("* Device %d %s", i, d.getAddress().toString().c_str());

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
    PRINTF("- ServiceUUID: %s", d.getServiceUUID().toString().c_str());
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
  PRINTF("Loop");
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false); // is async maybe
  wifiScan();			// is not async, maybe
  bleScan(foundDevices);	// is sync
  pBLEScan->clearResults();     // delete results fromBLEScan buffer to release memory
}

void wifiScan() {
  PRINTF("wifiScan");
  // WiFi.scanNetworks will return the number of networks found
  n_wifi_found = WiFi.scanNetworks(false, true);
  PRINTF("Wifi found %d networks", n_wifi_found);
}

void displayWifi() {
  for (int i = 0; i < n_wifi_found; ++i) {
#if 0
    PRINTF("- wifi %d: %s (%d %d) %s %s", 
	   i, WiFi.SSID(i).c_str(),
	   WiFi.channel(i),
	   WiFi.RSSI(i), 
	   WiFi.BSSIDstr(i).c_str(),
	   ((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?"O":"X"));
#endif
    int32_t n_channels = 10;
    int16_t width = 10;
    int16_t x = min(WiFi.channel(i), n_channels) * width;
    // convert rssi to 1-8, higher means stronger.
    const int16_t min_height = 1;
    const int16_t max_height = 8;
    int16_t height = constrain(map(WiFi.RSSI(i), MIN_WIFI_RSSI, MAX_WIFI_RSSI, min_height, max_height), min_height, max_height);
#if 0
    PRINTF("- WiFi channel=%d rssi=%d height=%d", WiFi.channel(i), WiFi.RSSI(i), height);
#endif
    int16_t y = 64-height;
    display.drawRect(x, y, width, height);
#if 0
    PRINTF("- rssi=%d display.drawRect(%d, %d, %d, %d)", WiFi.RSSI(i), x, y, width, height);
#endif
    // draw landscape at bottom
    display.drawLine(0, 63, 128, 63);
  }
}

void bleScan(BLEScanResults foundDevices) {
  PRINTF("BLEScan Found: %d", foundDevices.getCount());
  int k = foundDevices.getCount();
  uint8_t dup_index = 0;
  int display_width = display.getWidth();
  for (int i = 0; i < k; i++)  {
    blatano_t blat = {};	// {} to clear to zero
    display.clear();
    int progress = k==1 ? 1 : (i * 100/(k-1));
    display.drawProgressBar(robot_max_width+1, 0, display_width-(robot_max_width+1)-1, TEXT_FIRST_LINE, progress);
    processDevice(&blat, &foundDevices, i, &dup_index);
    displayWifi();
    displayDevice(i, blat);
    display.display();
    delay(i == k-1 ? 2000 : 4000);
  }
  PRINTF("ble+wifi done");
}

void displayDevice(int i, blatano_t blat) {
#if 1
  PRINTF("- %d %X %s", i, blat.crc32, blat.name);
#endif
#ifdef DISPLAY_CRC32
  // i CRC32
  snprintf(linebuf, sizeof linebuf, "%d %X %s", i, blat.crc32, blat.name);
  display.setFont(ArialMT_Plain_10);
  display.drawStringf(robot_max_width+10, 64-LEADING, linebuf, "%d %x",i, blat.crc32);
#endif

  int display_width = display.getWidth();
  int display_height = display.getHeight();

  // blat.name
  display.setFont(ArialMT_Plain_16);
  display.drawStringMaxWidth(robot_max_width+3, TEXT_FIRST_LINE, display_width-(robot_max_width+3), blat.name);

  // draw signal strength
#if 0
  // first calculate robot_scale from blat.rssi
  // df["RSSI"].min() -> -105; df["RSSI"].max() -> -61
  {
    int16_t asu = map(blat.rssi, MIN_BLE_DB, MAX_BLE_DB, 63, 0);
    PRINTF("- BT rssi=%d asu=%d", blat.rssi, asu);
    // draw blat.RSSI
    // df["RSSI"].min() -> -105; df["RSSI"].max() -> -61
    display.drawLine(display_width=1, display_height-1, display_width-1, asu);
  }
#endif

  // draw_pixel_robot
  // PixelRobot uses 24-bits, so we lose 8 bits here.
  // For my neighborhood, top 24 bits looked better so I took those.
  // Maybe investigate spined robots.
  {
    int16_t size = constrain(map(blat.rssi, MIN_BLE_DB, MAX_BLE_DB, MIN_ROBOT_SCALE, MAX_ROBOT_SCALE),
			     MIN_ROBOT_SCALE, MAX_ROBOT_SCALE);
#if 0
    PRINTF("size = map(blat.rssi=%d, MIN_BLE_DB=%d, MAX_BLE_DB=%d, MIN_ROBOT_SCALE=%d, MAX_ROBOT_SCALE=%d",
	   blat.rssi, MIN_BLE_DB, MAX_BLE_DB, MIN_ROBOT_SCALE, MAX_ROBOT_SCALE);
#endif
    draw_pixel_robot((blat.crc32 & 0xffffff00) >> 8, size);
  }
}


// tested with robot_num = 0x1af824;
void draw_pixel_robot(uint32_t robot_num, int16_t robot_scale) {
  PRINTF("- draw_pixel_robot robot_num %x robot_scale=%d", robot_num, robot_scale);
  pixel_robot.clear();
  pixel_robot.generate(robot_num);
  pixel_robot.setScales(robot_scale, robot_scale);
  int16_t robot_width = pixel_robot.getWidth();
  int16_t robot_height = pixel_robot.getHeight();
  pixel_robot.draw((robot_max_width - robot_width)/2,
		   (robot_max_height - robot_height)/2);
}

void draw_splash_screen() {
  // Self Announcement
  display.setFont(ArialMT_Plain_16);
  display.drawStringMaxWidth(0, 0, 64, "Ego Blatano");

  // Source on GitHub QR Code
  display.drawXbm(64, 0, 
		  blatano_github_width, blatano_github_height, blatano_github_bits);

  // Cheezy WiFi icon
  display.drawXbm(4, 64-wifi_logo_icon_height,
		  wifi_logo_icon_width, wifi_logo_icon_height, wifi_logo_icon_bits);

  // Cheezy Bluetooth icon
  display.drawXbm(64-bluetooth_logo_icon_width-8, 64-bluetooth_logo_icon_height,
		  bluetooth_logo_icon_width, bluetooth_logo_icon_height, bluetooth_logo_icon_bits);


  display.display();
  delay(2000);



}
