# Sensordaten via ESP-Now

Datenübertragung allein via WLAN/MQTT ist "energetisch" nicht wirklich optimal! Wozu das ganze Halli-Galli, um sich in einem WLAN anzumelden, eine IP-Adresse zu erhalten etc.? Das verbraucht nur wertvolle Zeit! Expressif hat dafür [ESP-NOW](https://www.espressif.com/en/solutions/low-power-solutions/esp-now) erfunden!

In den Unterverzeichnissen sind ein paar Experimente dazu zu finden:

* [esp_now_tx](esp_now_tx/): diverse Sensordaten ermitteln und via ESP-NOW versenden
* [esp_now_rx_serial](esp_now_rx_serial/): die, via ESP-NOW, empfangenen Daten werden in ein (festgelegtes) json-Format konvertiert und über die serielle Schnittstelle weitergeleitet.
* [serial_rx_mqtt](serial_rx_mqtt/): wenn die, über die serielle Schnittstelle empfangenen Daten dem (festgelegten) json-Format entsprechen, werden sie entsprechend via MQTT weitergeleitet
* [serial_rx_mqtt_python](serial_rx_mqtt_python/): ...dito, aber für einen Raspberry Pi und in Python
* [esp_now_rx_mqtt](esp_now_rx_mqtt): der ESP-NOW-Empfänger sendet die empfangenen Daten 1:1 via MQTT (via WLAN!!!) weiter --> will keiner, weil ggf. die Reichweite des Senders darunter leidet (gleicher WiFi-Kanal für ESP-NOW und WLAN)! Deshalb auch nicht bis zum Ende weiterentwickelt...
* [espnow2mqtt](espnow2mqtt): Empfänger auf Basis eines WT32-ETH01-Moduls; die empfangenen Daten werden, im json-Format, als MQTT-Nachricht via Ethernet weitergeleitet


----
Have fun!

Uwe Berger; 2024






