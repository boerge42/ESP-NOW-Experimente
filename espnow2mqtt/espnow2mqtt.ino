// *****************************************************************************
//
// ESP-NOW Empfaenger (Wetterstation)
// ==================================
//        Uwe Berger, 2025
//
//
// Hardware:
// ---------
// Ein WT32-ETH01-Modul: 
// * Datasheet (z.B.) --> https://www.jacobsparts.com/static/images/WT32-ETH01_datasheet_V1.4.pdf
//
// Die, via ESP-NOW, empfangenen Sensordaten werden als MQTT-Nachricht via Ethernet
// weitergeschickt.
//
//
// Inspiration (u.a):
// ------------------
// * https://sebastianlang.net/arduino/wt32-eth01-mit-arduino/
// * https://github.com/tayyabtahir/ESP_WT32-ETH01-ESPNow-WithMQTTOverEthernet/blob/main/WT32-ETH01-ESPNowSamples/WT32-ETH01-ESPNow-WithMQTTOverEthernet/WT32-ETH01-ESPNow-WithMQTTOverEthernet.ino
// * https://wolles-elektronikkiste.de/esp-now#1_transm_1_recv_advanced
//
//
// Hinweis: der Pfad zu my_esp_now.h muss in der include-Anweisung
//          angepasst werden!
//
// ---------
// Have fun!
//
// *****************************************************************************
#include <PubSubClient.h>
#include <ETH.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

#define ARDUINOJSON_USE_DOUBLE 0
#include <ArduinoJson.h>

#include "my_esp_now.h"

#define HOSTNAME            "WT32-ETH01"
#define MQTT_BUFFER_SIZE    500

// Ethernet-Client
WiFiClient ethClient;

// MQTT-Client
PubSubClient mqtt_client(ethClient);

char buf[MQTT_BUFFER_SIZE];         // Buffer fuer MQTT-Nachricht
sensor_values_t sensor_values;      // Struktur ESPNow-Nachricht
static bool eth_connected = false;  // mit Ethernet verbunden?


// ******************************************************************************************
// This function handels Ethernet Event Call Backs
void WiFiEvent(WiFiEvent_t event) {
    
    switch (event) {
        case ARDUINO_EVENT_ETH_START:
            Serial.println("ETH Started");
            //set eth hostname here
            ETH.setHostname(HOSTNAME);
            break;
        case ARDUINO_EVENT_ETH_CONNECTED:
            Serial.println("ETH Connected");
            break;
        case ARDUINO_EVENT_ETH_GOT_IP:
            Serial.print("ETH MAC: ");
            Serial.print(ETH.macAddress());
            Serial.print(", IPv4: ");
            Serial.print(ETH.localIP());
            if (ETH.fullDuplex()) {
                Serial.print(", FULL_DUPLEX");
            }
            Serial.print(", ");
            Serial.print(ETH.linkSpeed());
            Serial.println("Mbps");
            eth_connected = true;
            break;
        case ARDUINO_EVENT_ETH_DISCONNECTED:
            Serial.println("ETH Disconnected");
            eth_connected = false;
            break;
        case ARDUINO_EVENT_ETH_STOP:
            Serial.println("ETH Stopped");
            eth_connected = false;
            break;
        default:
            break;
    }
}

// **************************************************************
void mqtt_connect(void) {
  uint8_t count = 0;

  Serial.println("Connect to MQTT-Broker: ");
  mqtt_client.setServer(MQTT_BROKER, MQTT_PORT);
  while (!mqtt_client.connected()) {
    count++;
    if (mqtt_client.connect("WT32-ETH0", MQTT_USER, MQTT_PWD)) {
       Serial.println("MQTT connected!");  
    } else {
       Serial.print(".");
       Serial.print(mqtt_client.state());
    }
    if (count > 10) {
       Serial.println("...no connection (MQTT-Broker)!!!");
       Serial.flush();
       while(1){};
    }
    delay(100);
  } 
  mqtt_client.setBufferSize(MQTT_BUFFER_SIZE); 
}

// ******************************************************************************************
double round_dec(double value, int dec) {
   return (int)(value * pow(10, dec) + 0.5) / pow(10, dec);
}

// ******************************************************************************************
void messageReceived(const esp_now_recv_info *info, const uint8_t* incomingData, int len){
    
    // empfangene Daten auf sensor_values mappen
    memcpy(&sensor_values, incomingData, sizeof(sensor_values));

    // Ausgabe auf serieller Schnittstelle
    Serial.printf("Transmitter MAC Address: %02X:%02X:%02X:%02X:%02X:%02X \n\r", 
        info->src_addr[0], info->src_addr[1], info->src_addr[2], info->src_addr[3], info->src_addr[4], info->src_addr[5]);    
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
    //~ Serial.printf("t1................: %dms\r\n", sensor_values.t1);
    //~ Serial.printf("t2................: %dms\r\n", sensor_values.t2);
    //~ Serial.printf("t3................: %dms\r\n", sensor_values.t3);
    //~ Serial.printf("t4................: %dms\r\n", sensor_values.t4);
    //~ Serial.printf("t5................: %dms\r\n", sensor_values.t5);
    Serial.println(); 
    
    // json aufbereiten und via mqtt senden
    JsonDocument doc;
    JsonObject BME280 = doc["BME280"].to<JsonObject>();
    BME280["temperature"] = round_dec(sensor_values.bme_temperature, 1);
    BME280["humidity"] = round_dec(sensor_values.bme_humidity, 1);
    BME280["pressure_abs"] = round_dec(sensor_values.bme_pressure_abs, 1);
    BME280["pressure_rel"] = round_dec(sensor_values.bme_pressure_rel, 1);
    JsonObject SHT21 = doc["SHT21"].to<JsonObject>();
    SHT21["temperature"] = round_dec(sensor_values.sht_temperature, 1);
    SHT21["humidity"] = round_dec(sensor_values.sht_humidity, 1);
    doc["TMP36"]["temperature"] = round_dec(sensor_values.tmp36_temperature, 1);
    doc["BH1750"]["luminosity"] = round_dec(sensor_values.bh1750_luminosity, 1);
    JsonObject ESP = doc["ESP"].to<JsonObject>();
    ESP["awake_time"] = sensor_values.awake_time;
    ESP["vcc"] = round_dec(sensor_values.vcc, 2);
    ESP["vbat"] = round_dec(sensor_values.vbat, 2);
    ESP["t1"] = sensor_values.t1;
    ESP["t2"] = sensor_values.t2;
    ESP["t3"] = sensor_values.t3;
    ESP["t4"] = sensor_values.t4;
    ESP["t5"] = sensor_values.t5;
    doc.shrinkToFit();  // optional
    serializeJson(doc, buf);  
    mqtt_client.publish(MQTT_TOPIC, buf, false);  

}

// ******************************************************************************************
void setup()
{
  
    // serielle Schnittstelle
    Serial.begin(115200);
    while (!Serial);

    // ESPNow...
    Serial.println("Init ESPNow");
    WiFi.mode(WIFI_STA);
    // ...MAC setzen
    esp_wifi_set_mac(WIFI_IF_STA, new_mac_addr);
    // ...WiFi-Channel setzen
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(ESP_NOW_CHANAL, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);    
    // ...ESPNow init
    if (esp_now_init() == ESP_OK) {
        Serial.println("ESPNow Init success");
    }
    else {
        Serial.println("ESPNow Init fail");
        return;
    }
    // ...Rx-Callback
    esp_now_register_recv_cb(messageReceived);

    // Ethernet...
    Serial.println("Connect to LAN:");
    Serial.println();
    // ...Ethernet-Event callback
    WiFi.onEvent(WiFiEvent);
    // ...Ethernet starten (IP via DHCP)
    while (ETH.begin() != true) {
        Serial.print(".");
        delay(1000);    
    }
    Serial.println("LAN connected!");
    Serial.println();

    // MQTT-Broker connect
    mqtt_connect();
    
    // ein paar Gedenksekunden ;-)
    delay(5000);

}

// ******************************************************************************************
// ******************************************************************************************
// ******************************************************************************************
void loop()
{
    // Ethernet da?
    if (eth_connected) {
        // MQTT-Broker da?
        if (!mqtt_client.connected()) {
            mqtt_connect();
        } 
    }
    delay(1000);
}
