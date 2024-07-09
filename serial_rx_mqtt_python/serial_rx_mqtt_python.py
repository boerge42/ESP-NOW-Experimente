# ******************************************************************************
#
# ...Python und fuer einen Raspberry Pi bestimmt...
#
# Lesen serielle Schnittstelle und empfangene Daten via MQTT weitersenden...
# ==========================================================================
#                            Uwe Berger, 2024
#
# Die Daten, welche ueber die serielle Schnittstelle eingelesen werden, muessen
# in einem validen JSON-Format vorliegen. Ist dies der Fall, werden sie via MQTT 
# weitergesendet.
#
# Aufbau des empfangenen JSON-Strings (Beispiel):
#
# {
#   "topic":{"TX_NAME":"ESP_WEATHERSTATION"},
#   "payload":
#     {
#       "BME280":{"temperature":24.3,"humidity":40.2,"pressure_abs":1023.7,"pressure_rel":1028.6}, 
#       "SHT21":{"temperature":25.1,"humidity":42.1}, 
#       "TMP36":{"temperature":16.4}, 
#       "BH1750":{"luminosity":0.0}, 
#       "ESP":{"awake_time":183,"vcc":3.32,"vbat":-0.01, "t1":115, "t2":116, "t3":116, "t4":142, "t5":168}
#     }
# }
#
# Hinweise: 
#    * Fehlerbehandlung koennte man sicherlich noch weiter optimieren
#    * serielle Schnittstelle via raspi-conig auf dem RPI aktivieren
#
#
# ---------
# Have fun!
#
# ******************************************************************************

import signal

import serial
import json
import paho.mqtt.client as mqtt

MQTT_BROKER = "nanotuxedo"
MQTT_PORT   = 1883
MQTT_USER   = ""
MQTT_PWD    = ""


# Name des seriellen Port auf dem Pin-Header des Raspberry
SERIAL_PORT = '/dev/ttyS0'

# Baudrate
SERIAL_RATE = 115200        # ...analog Sender ;-)

# *******************************************************
def signal_handler(signal, frame):
    print("...if ctrl+c doesn't work, try ctrl+z...")
    exit(0)
  
# *******************************************************
# *******************************************************
# *******************************************************

# ctrl+c
signal.signal(signal.SIGINT, signal_handler)

# serielles Port initialisieren
ser = serial.Serial(SERIAL_PORT, SERIAL_RATE)

# MQTT...
mqtt_client = mqtt.Client()
mqtt_client.username_pw_set(username=MQTT_USER, password=MQTT_PWD)
# ...mqtt-connect erfolgt im loop!

# endless loop...
while True:
    
    # mqtt-connect, wenn nicht schon erfolgt...
    if mqtt_client.is_connected() == False:
        print(f"...connect to mqtt-broker... ")
        mqtt_client.connect(MQTT_BROKER, MQTT_PORT)
        mqtt_client.loop_start()
    
    # "try" wg. nicht-utf8-konformen empfangenen Uetzelbruetzel
    try:
        rx_buf = ser.readline().decode('utf-8')
        
        print(rx_buf)

        # json-encode...
        rx_payload = json.loads(rx_buf)
        topic = rx_payload["topic"]["TX_NAME"]
        payload = rx_payload["payload"]             # Ergebnis ist ein Dict.!
        
        # payload-Dict. als String
        payload = json.dumps(payload)
        
        # ~ print(topic)
        # ~ print(payload)
        
        # als MQTT-Nachricht versenden
        mqtt_client.publish(topic, payload, qos=0)
        
    except:
        pass
