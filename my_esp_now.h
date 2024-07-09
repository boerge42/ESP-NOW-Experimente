// *****************************************************************************
//
// ESP-Now Sende/Empfaenger
// ========================
//     Uwe Berger; 2024
//
// Gemeinsame Defines, Datentypen etc. fuer ESP-Now Sender/Empfaenger der
// Wetterstation.
//
// ---------
// Have fun!
//
// *****************************************************************************

// Struktur Messwerte
struct sensor_values_t {
  float bme_temperature;
  float bme_humidity;
  float bme_pressure_abs;
  float bme_pressure_rel;
  float sht_temperature;
  float sht_humidity;
  float bh1750_luminosity;
  float tmp36_temperature;
  char tx_name[32]; 
  float vcc;
  float vbat;
  uint32_t awake_time;
  uint32_t t1;
  uint32_t t2;
  uint32_t t3;
  uint32_t t4;
  uint32_t t5;
};

// MAC-Adresse des ESP-Now-Empfaengers
uint8_t new_mac_addr[6] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};

// WiFi-Kanal
#define WIFI_CHANNEL    7

// ESP-Now-Kanal
#define ESP_NOW_CHANAL  1

// WLAN-Anmeldung
#define WIFI_SSID       "WLAN"
#define WIFI_PWD        "xyz"

// MQTT-Konfiguration
#define MQTT_BROKER     "nanotuxedo"
#define MQTT_PORT       1883
#define MQTT_USER       ""
#define MQTT_PWD        ""
#define MQTT_TOPIC      "espnow/"

// *****************************************************************************
