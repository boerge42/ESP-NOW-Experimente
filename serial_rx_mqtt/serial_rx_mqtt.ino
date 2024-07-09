// *****************************************************************************
//
// Lesen serielle Schnittstelle und empfangene Daten via MQTT weitergesenden...
// ============================================================================
//                            Uwe Berger, 2024
//
// Die Daten, welche ueber die serielle Schnittstelle eingelesen werden, muessen
// in einem validen JSON-Format vorliegen. Ist dies der Fall, werden sie via MQTT 
// weitergesendet.
//
// Aufbau des empfangenen JSON-Strings (Beispiel):
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
#include <PubSubClient.h>
#include <jsonlib.h>

#include "/home/bergeruw/work/esp_now/espnow/my_esp_now.h"

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

char rx_buf[550]; 

// **************************************************************
void wifi_connect(void) {
  uint8_t count = 0;

  Serial.print("Connect to WiFi: ") ;
  WiFi.mode(WIFI_STA);  
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
void setup() {
  Serial.begin(115200);
  
  // Anmeldung im WLAN und am MQTT-Broker erfolgen im loop()!
  
}

// **************************************************************
// **************************************************************
// **************************************************************
void loop() {
    
  // WLAN?
  if (WiFi.status() != WL_CONNECTED) {
    wifi_connect();
  }
  // MQTT-Broker?
  if (!mqtt_client.connected()) {
    mqtt_connect();
  }    
  // serielle Daten?  
  if (Serial.available()) {
    Serial.readBytesUntil('\n', rx_buf, sizeof(rx_buf));
    Serial.println(rx_buf);
    String str_topic = jsonExtract(jsonExtract(rx_buf, "topic"), "TX_NAME");
    String str_payload = jsonExtract(rx_buf, "payload");
    //~ Serial.println(str_topic);
    //~ Serial.println(str_payload);
    char* topic = &str_topic[0];
    char* payload = &str_payload[0];
    //~ Serial.println(topic);
    //~ Serial.println(payload);
    mqtt_client.publish(topic, payload, false);
  }
}

