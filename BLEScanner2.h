// BLATANO_T_CRC_END marks end of CRC data.
// Put stuff after that that is too variable.
struct blatano_t {
  esp_bd_addr_t mac;
  int address_type;
  char name[16];
  int appearance;
  uint8_t manufacturer_data[16];
  uint8_t service_uuid[16];  
  uint32_t crc32;
#define BLATANO_T_CRC_END crc32
  int tx_power;
  int rssi;
};
