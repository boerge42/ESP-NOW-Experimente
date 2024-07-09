// *****************************************************************************
//
// ESP-NOW Empfaenger (Wetterstation)
// ==================================
//        Uwe Berger, 2024
//
// Die empfangenen Sensordaten werden via MQTT weitergeschickt.
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
#include <PubSubClient.h>

#include "/home/bergeruw/work/esp_now/espnow/my_esp_now.h"

sensor_values_t sensor_values;

WiFiClient espClient;
PubSubClient mqtt_client(espClient);


// **************************************************************
void mqtt_publish_values(void) {
  char json[500];
  char bme280[100];
  char sht21[100];
  char tmp36[100];
  char bh1750[100];
  char esp[100];
  
  // JSON-String zusammenbauen
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
  sprintf(json, "{%s, %s, %s, %s, %s}", bme280, sht21, tmp36, bh1750, esp);

  // MQTT-Nachricht versenden
  mqtt_client.publish(MQTT_TOPIC, json, false);
}
 
// **************************************************************
void wifi_connect(void) {
  uint8_t count = 0;

  Serial.print("Connect to WiFi: ") ;
  WiFi.begin(WIFI_SSID, WIFI_PWD);
  while (WiFi.status() != WL_CONNECTED) {
    count++;
    if (count > 50) {
      // keine Verbindung ins WLAN --> ESP reset
      Serial.println("...no connection (WLAN)!!!");
      Serial.flush();
      ESP.reset();
    }
      delay(100);
      Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.println(WiFi.localIP());
  Serial.println("");
}

// **************************************************************
void mqtt_connect(void) {
  uint8_t count = 0;

  Serial.println("Connect to MQTT-Broker: ");
  mqtt_client.setServer(MQTT_BROKER, MQTT_PORT);
  while (!mqtt_client.connected()) {
    count++;
    if (mqtt_client.connect("espnow", MQTT_USER, MQTT_PWD)) {
       Serial.println("MQTT connected!");  
    } else {
       Serial.print(".");
       Serial.print(mqtt_client.state());
    }
    if (count > 10) {
      // keine Verbindung zum MQTT-Broker --> ESP reset
        Serial.println("...no connection (MQTT-Broker)!!!");
        Serial.flush();
        ESP.reset();
    }
    delay(100);
  } 
  mqtt_client.setBufferSize(500); 
}

// **************************************************************
// Callback function that will be executed when data is received
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  
  memcpy(&sensor_values, incomingData, sizeof(sensor_values));
  Serial.println();
  Serial.printf("From MAC: %X:%X:%X:%X:%X:%X --> %i bytes received\r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], len);
  Serial.println("**** Data ****");
  Serial.printf("awake_time........: %ims\r\n", sensor_values.awake_time);
  Serial.printf("bh1750_luminosity.: %.2flux\r\n", sensor_values.bh1750_luminosity);
  Serial.printf("bme_humidity......: %.2f%%H\r\n", sensor_values.bme_humidity);
  Serial.printf("bme_pressure_abs..: %.2fhPa\r\n", sensor_values.bme_pressure_abs);
  Serial.printf("bme_pressure_rel..: %.2fhPa\r\n", sensor_values.bme_pressure_rel);
  Serial.printf("bme_temperature...: %.2f°C\r\n", sensor_values.bme_temperature);
  Serial.printf("sht_humidity......: %.2f%%H\r\n", sensor_values.sht_humidity);
  Serial.printf("sht_temperature...: %.2f°C\r\n", sensor_values.sht_temperature);
  Serial.printf("tmp36_temperature.: %.2f°C\r\n", sensor_values.tmp36_temperature);
  Serial.printf("Vbat..............: %.2fV\r\n", sensor_values.vbat);
  Serial.printf("Vcc...............: %.2fV\r\n", sensor_values.vcc);
  Serial.println();
  Serial.flush();
  
  Serial.println("...send this data as json payload to the mqtt broker!");
  mqtt_publish_values();
}
 
// **************************************************************
void setup() {
  
  Serial.begin(115200);
  
  Serial.println();
  Serial.print("ESP8266 Board MAC Address (original): ");
  Serial.println(WiFi.macAddress());

  // MAC-Adresse des ESP setzen
  wifi_set_macaddr(STATION_IF, new_mac_addr);
  Serial.print("ESP8266 Board MAC Address (new).....: ");
  Serial.println(WiFi.macAddress());

  // WiFi-Kanal setzen (Achtung: gilt fuer ESP-Now & WiFi!)
  wifi_promiscuous_enable(true);
  wifi_set_channel(WIFI_CHANNEL);
  wifi_promiscuous_enable(false);
  Serial.print("WiFi Channel: ");
  Serial.println(WiFi.channel());

  // WiFi mode setzen
  WiFi.mode(WIFI_AP_STA);

  // Anmeldung im WLAN und am MQTT-Server erfolgen im loop()!
  
  // ESP-NOW initialisieren
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // ESP-NOW-Rolle setzen (Sender) 
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);

  // Callback-Funktion, wenn Daten via ESP-Now empfangen wurden
  esp_now_register_recv_cb(OnDataRecv);
}

// **************************************************************
// **************************************************************
// **************************************************************
void loop() {
  // WLAN...
  if (WiFi.status() != WL_CONNECTED) {
    wifi_connect();
  }
  // MQTT-Broker...
  if (!mqtt_client.connected()) {
    mqtt_connect();
  }

}
