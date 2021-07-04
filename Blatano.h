enum servicetype_t { UNKNOWN=0, 
		     COVID_TRACKER=1,
		     GATT=2,
		     FITBIT_CHARGE_3=3,
		     N_SERVICE_TYPES=4
};

// BLATANO_T_CRC_END marks end of CRC data.
// Put stuff after that that is too variable.
struct blatano_t {
  uint8_t mac[6];		/* mac address is converted to seqno if it's random */
  int address_type;
  char name[16];
  int appearance;
  uint8_t manufacturer_code[2];	/* exclude all but manufacturer_code since it's too random */
  servicetype_t serviceType;
#define BLATANO_T_CRC_END crc32
  uint32_t crc32;
  uint8_t service_uuid[16];
  int tx_power;
  int rssi;
};

struct memory_t {
  uint32_t crc32;
  int count;
};
