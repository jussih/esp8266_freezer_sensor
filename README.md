# ESP8266 fridge temperature sensor node

This is a firmware for an ESP8266 board which monitors fridge and freezer
temperatures using DS18B20 sensors, displaying them on a 16x2 LCD unit and
publishing the measurements to an MQTT topic.

Used as part of a Home Assistant setup to monitor refridgerators.

## Hardware

- Any ESP8266 dev board such as NodeMCU or Wemos D1 mini.
- 2 DS18B20 temperature sensors connected to ESP GPIO pin 14 (D5). Requires
  a 4.7k ohm pull-up resistor to 3.3V.
- 16x2 LCD unit with a PCF8574 I2C interface board connected to the default I2C data
  and clock pins (D1 = clock, D2 = data)

## Flashing

- Use Arduino IDE 1.8+
- Install ESP8266 support via board manager
- Select board in Tools menu
- Select serial port
- Insert WLAN and MQTT credentials
- Compile and upload

