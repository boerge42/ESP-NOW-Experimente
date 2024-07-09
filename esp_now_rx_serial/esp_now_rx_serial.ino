// *****************************************************************************
//
// ESP-NOW Empfaenger (Wetterstation)
// ==================================
//        Uwe Berger, 2024
//
// Die empfangenen Sensordaten werden seriell weitergeschickt.
//
// Die empfangenen Daten werden in einen JSON-String verpackt und ueber die serielle
// Schnittstelle ausgegeben. 
// Beispiel:
//
// {
//   "topic":{"TX_NAME":"ESP_WEATHERSTATION"},
//   "payload":
//     {
//       "BME280":{"temperature":24.3,"humidity":40.2,"pressure_abs":1023.7,"pressure_rel":1028.6}, 
//       "SHT21":{"temperature":25.1,"humidity":42.1}, 
//       "TMP36":{"temperature":16.4}, 
//       "BH1750":{"luminosity":0.0}, 
//       "ESP":{"awake_time":183,"vcc":3.32,"vbat":-0.01, "t1":115, "t2":116, "t3":116, "t4":142, "t5":168}
//     }
// }
//
// Hinweis: der Pfad zu my_esp_now.h muss in der include-Anweisung
//          angepasst werden!
//
// ---------
// Have fun!
//
// *****************************************************************************

#include <ESP8266WiFi.h>
#include <espnow.h>

#include "/home/bergeruw/work/esp_now/espnow/my_esp_now.h"

sensor_values_t sensor_values;



// **************************************************************
void serial_write_values(void) {
  
  char json[550];
  char tx_name[50];
  char bme280[100];
  char sht21[100];
  char tmp36[100];
  char bh1750[100];
  char esp[100];
  
  // JSON-String zusammenbauen...
  sprintf(tx_name, "\"TX_NAME\":\"%s\"", sensor_values.tx_name);
  sprintf(bme280, "\"BME280\":{\"temperature\":%.1f,\"humidity\":%.1f,\"pressure_abs\":%.1f,\"pressure_rel\":%.1f}", 
         sensor_values.bme_temperature, sensor_values.bme_humidity, sensor_values.bme_pressure_abs, sensor_values.bme_pressure_rel);
  sprintf(sht21, "\"SHT21\":{\"temperature\":%.1f,\"humidity\":%.1f}", 
         sensor_values.sht_temperature, sensor_values.sht_humidity);
  sprintf(tmp36, "\"TMP36\":{\"temperature\":%.1f}", 
         sensor_values.tmp36_temperature);
  sprintf(bh1750, "\"BH1750\":{\"luminosity\":%.1f}", 
         sensor_values.bh1750_luminosity);
  sprintf(esp, "\"ESP\":{\"awake_time\":%lu,\"vcc\":%.2f,\"vbat\":%.2f, \"t1\":%lu, \"t2\":%lu, \"t3\":%lu, \"t4\":%lu, \"t5\":%lu}", 
         sensor_values.awake_time, sensor_values.vcc, sensor_values.vbat, sensor_values.t1, sensor_values.t2, sensor_values.t3, sensor_values.t4, sensor_values.t5);
  sprintf(json, "{\"topic\":{%s},\"payload\":{%s, %s, %s, %s, %s}}", tx_name, bme280, sht21, tmp36, bh1750, esp);

  // ...und schreiben
  //~ Serial.println(json);
  //~ Serial.println(strlen(json));
  Serial.write(json, strlen(json));
  Serial.write("\n");
}

// **************************************************************
// Callback function that will be executed when data is received
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  
  memcpy(&sensor_values, incomingData, sizeof(sensor_values));
  
  //~ Serial.printf("From MAC: %X:%X:%X:%X:%X:%X --> %i bytes received\r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], len);
  //~ Serial.println("**** Data ****");
  //~ Serial.printf("awake_time........: %ims\r\n", sensor_values.awake_time);
  //~ Serial.printf("bh1750_luminosity.: %.2flux\r\n", sensor_values.bh1750_luminosity);
  //~ Serial.printf("bme_humidity......: %.2f%%H\r\n", sensor_values.bme_humidity);
  //~ Serial.printf("bme_pressure_abs..: %.2fhPa\r\n", sensor_values.bme_pressure_abs);
  //~ Serial.printf("bme_pressure_rel..: %.2fhPa\r\n", sensor_values.bme_pressure_rel);
  //~ Serial.printf("bme_temperature...: %.2f°C\r\n", sensor_values.bme_temperature);
  //~ Serial.printf("sht_humidity......: %.2f%%H\r\n", sensor_values.sht_humidity);
  //~ Serial.printf("sht_temperature...: %.2f°C\r\n", sensor_values.sht_temperature);
  //~ Serial.printf("tmp36_temperature.: %.2f°C\r\n", sensor_values.tmp36_temperature);
  //~ Serial.printf("Vbat..............: %.2fV\r\n", sensor_values.vbat);
  //~ Serial.printf("Vcc...............: %.2fV\r\n", sensor_values.vcc);
  //~ Serial.printf("t1................: %dms\r\n", sensor_values.t1);
  //~ Serial.printf("t2................: %dms\r\n", sensor_values.t2);
  //~ Serial.printf("t3................: %dms\r\n", sensor_values.t3);
  //~ Serial.printf("t4................: %dms\r\n", sensor_values.t4);
  //~ Serial.printf("t5................: %dms\r\n", sensor_values.t5);
  //~ Serial.println();
  
  //~ Serial.println("...send this data as json via serial!");
  serial_write_values();
  
}
 
// **************************************************************
void setup() {

  Serial.begin(115200);
  
  Serial.println();
  Serial.print("ESP8266 Board MAC Address (original): ");
  Serial.println(WiFi.macAddress());

  // wifi station mode
  WiFi.mode(WIFI_STA);

  // MAC-Adresse des ESP setzen
  wifi_set_macaddr(STATION_IF, new_mac_addr);
  Serial.print("ESP8266 Board MAC Address (new).....: ");
  Serial.println(WiFi.macAddress());

  wifi_promiscuous_enable(true);
  wifi_set_channel(ESP_NOW_CHANAL);
  wifi_promiscuous_enable(false);
  
  Serial.print("WiFi Channel: ");
  Serial.println(WiFi.channel());

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // ESP-NOW-Rolle setzen (Sender) 
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);

  // Callback-Funktion, wenn Daten empfangen wurden
  esp_now_register_recv_cb(OnDataRecv);
}

// **************************************************************
// **************************************************************
// **************************************************************
void loop() {
  
}
