// *****************************************************************************
//
// ESP-NOW Sender (Wetterstation)
// ==============================
//        Uwe Berger, 2024
//
// Diverse Sensoren auslesen und Messwerte via ESP-Now versenden.
//
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

#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_ADS1X15.h>
#include <BH1750FVI.h>
#include "SHT2x.h"

#include "/home/bergeruw/work/esp_now/espnow/my_esp_now.h"

sensor_values_t sensor_values;

#define ESP_NAME  "ESP_WEATHERSTATION"

// Struktur fuer ESP-RTC-Memory
union rtc_t {
  struct {
	uint32_t crc32;
	byte data[4];  
  } data;
  struct {
    uint32_t crc32;
    uint32_t awake_time;        // 4
  } vars;
} rtc;

#define DEEPSLEEP_TIME 5e6  // 5s
#define SEND_TIMEOUT   500  // 245ms


uint32_t old_awake_time, awake_time;
volatile boolean call_OnDataSend;

Adafruit_BME280 bme;

#define MY_ALTITUDE 39.0
float altitude = MY_ALTITUDE;

SHT2x sht;

BH1750FVI myLux(0x23);

Adafruit_ADS1115 ads;


// **************************************************************
// https://github.com/esp8266/Arduino/blob/master/libraries/esp8266/examples/RTCUserMemory/RTCUserMemory.ino
//
uint32_t calculateCRC32(const uint8_t *data, size_t length) 
{
  uint32_t crc = 0xffffffff;
  while (length--) {
    uint8_t c = *data++;
    for (uint32_t i = 0x80; i > 0; i >>= 1) {
      bool bit = crc & 0x80000000;
      if (c & i) {
        bit = !bit;
      }
      crc <<= 1;
      if (bit) {
        crc ^= 0x04c11db7;
      }
    }
  }
  return crc;
}

// **************************************************************
void write_rtc_memory(uint32_t time)
{
  rtc.vars.awake_time = time;
  rtc.vars.crc32 = calculateCRC32((uint8_t*) &rtc.data.data[0], sizeof(rtc.data.data));
  ESP.rtcUserMemoryWrite(0, (uint32_t *) &rtc.data, sizeof(rtc.data));
}

// **************************************************************
void esp_goto_deepsleep(uint32_t deepsleep_time)
{
  ESP.deepSleep(deepsleep_time, RF_NO_CAL);
}

// **************************************************************
void OnDataSend(uint8_t *mac_addr, uint8_t sendStatus) 
{
  //~ Serial.printf("send_cb, send done, status = %i\n", sendStatus);
  call_OnDataSend = true;
}
 
// **************************************************************
void bme280_sleep(void)
{
  // BME280 in Sleep-Mode versetzen --> RTFM	
  Wire.beginTransmission(0x76);   // i2c-Adr. BME280         
  Wire.write(0xF4);               // BME280_REGISTER_CONTROL  
  Wire.write(0b00);               // MODE_SLEEP              
  Wire.endTransmission();         
}
 
// **************************************************************
float tmp36_readTemperatureC_via_ads1115 (void)
{
  // 4x Gain (+/- 1.024V 15Bit-Aufloesung)
  ads.setGain(GAIN_FOUR);
  // TMP36 an ADC0 angeschlossen
  int16_t adc = ads.readADC_SingleEnded(0);
  float voltage = adc * 1024.0 / 32768.0 / 1000.0;             // x*1024/2^15/1000
  return (voltage - 0.5) * 100;               // RTFM TMP36
}

// **************************************************************
float read_vcc_via_ads1115 (void)
{
  // 1x Gain (+/- 4.096V --> Aufloesung 2mV)
  ads.setGain(GAIN_ONE);
  // Vcc liegt an ADC1 an
  int16_t adc = ads.readADC_SingleEnded(1);
  return adc*0.000125;                       // 4.096/2^15/1000
} 
 
// **************************************************************
// ...wir brauchen einen Spannungsteiler, da max. Vcc + 0.3V anliegen
// duerfen! Vcc ist 3.3V (MCP1700-3302). Voll geladene Akkus kommen 
// locker ueber 3.6V...
float read_vbat_via_ads1115 (void)
{
  // 2x Gain (+/- 2.048V --> 15Bit-Aufloesung)
  ads.setGain(GAIN_TWO);
  // Vcc liegt an ADC2 an
  int16_t adc = ads.readADC_SingleEnded(2);
  return adc*3.2*0.0000625;                       // 2048/2^15/1000 * 3.2 (3.2 --> Spannungsteiler)
}
 
// **************************************************************
void sensors_read(void)
{
  // Achtung: der BH1750 scheint ein wenig Zeit zwischen 
  // Init --> Read --> poweroff zu brauchen, um einen sinnvollen
  // Wert lesen zu koennen; die folgende Anordnung der Kommandos
  // scheint aber zu reichen...
  
  // Sensoren init
  bme.begin(0x76);        // BME280
  myLux.powerOn();        // BH1750
  myLux.setContHighRes(); // ...
  ads.begin();            // ADS1115
  sht.begin();            // SHT21
  sht.setResolution(3);   // ...Temp./Hum. in 11Bit Aufloesung
  
  sensor_values.t1 = millis() - awake_time;

  // Sensoren auslesen
  // ...BME280
  sensor_values.bme_temperature   = bme.readTemperature();
  sensor_values.bme_humidity      = bme.readHumidity();
  sensor_values.bme_pressure_abs  = bme.readPressure()/100.0F;
  sensor_values.bme_pressure_rel  = sensor_values.bme_pressure_abs+(altitude/8.0);
  
  sensor_values.t2 = millis() - awake_time;
  
  // ...BH1750
  sensor_values.bh1750_luminosity = myLux.getLux();

  sensor_values.t3 = millis() - awake_time;
  
  // ...ADS1115
  sensor_values.tmp36_temperature = tmp36_readTemperatureC_via_ads1115();
  sensor_values.vcc               = read_vcc_via_ads1115();
  sensor_values.vbat              = read_vbat_via_ads1115();
  
  sensor_values.t4 = millis() - awake_time;

  // ...SHT21
  sht.read();
  sensor_values.sht_temperature   = sht.getTemperature();
  sensor_values.sht_humidity      = sht.getHumidity();

  sensor_values.t5 = millis() - awake_time;

  // ...vorhergehende Wachzeit :-)
  sensor_values.awake_time        = old_awake_time;

  // Sensoren, soweit notwendig, schlafen schicken
  myLux.powerOff();               // BH1750
  bme280_sleep();                 // BME280

}
 
// **************************************************************
void setup() 
{

  // Nullpunkt fuer Laufzeitmessung setzen
  awake_time = millis();
  
  // Name des Sender-Moduls setzen
  strncpy(sensor_values.tx_name, ESP_NAME, sizeof(sensor_values.tx_name));
  
  WiFi.persistent( false ); // ...ist schneller, aber warum?
  
  // Init Serial Monitor
  Serial.begin(115200);
  Serial.println();
 
  // RTC-Memory auslesen und validieren
  ESP.rtcUserMemoryRead(0, (uint32_t *) &rtc.data, sizeof(rtc.data));
  if (calculateCRC32((uint8_t*) &rtc.data.data[0], sizeof(rtc.data.data)) == rtc.vars.crc32) {
    old_awake_time = rtc.vars.awake_time;
  } else {
    old_awake_time = 0;
  }
  
  // I2C initialisieren
  Wire.begin();
  Wire.setClock(3400000);

  // Sensoren initialisieren/auslesen
  sensors_read();
 
  // WiFi-Device in Station-Mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    esp_goto_deepsleep(DEEPSLEEP_TIME);
  }

  // ESP-NOW-Rolle setzen (Sender) 
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  
  // Callback-Funktion, wenn Daten gesendet wurden
  esp_now_register_send_cb(OnDataSend);
  call_OnDataSend = false;
  
  // Empfaenger-MAC setzen
  esp_now_add_peer(new_mac_addr, ESP_NOW_ROLE_SLAVE, ESP_NOW_CHANAL, NULL, 0);
 
  // Daten senden
  esp_now_send(new_mac_addr, (uint8_t *) &sensor_values, sizeof(sensor_values));

}
 
// **************************************************************
// **************************************************************
// **************************************************************
void loop() 
{
  // warten bis Daten gesendet wurden oder ein Timeout zuschlaegt
  if (call_OnDataSend || (millis() > SEND_TIMEOUT)) {
    // aktuelle Laufzeit in RTC sichern
    write_rtc_memory((millis()-awake_time));
    
    // ...und gute Nacht!
    esp_goto_deepsleep(DEEPSLEEP_TIME);
  }
}
