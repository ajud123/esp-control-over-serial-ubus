# ESP8266 control over serial UBUS

A package for the OpenWRT system meant to control ESP8266 devices connected via serial over UBUS. The ESP8266 device must be running [this](https://github.com/janenasl/esp_control_over_serial) firmware for it to communicate successfully.
Doesn't accept any additional arguments and is intended to be ran as an init.d daemon. Currently provides the following methods:
* `devices`
* `on`. Accepts the following parameters: `{"port": "pathToSerialPort", "pin": pinNumber}`
* `off`. Accepts the following parameters: `{"port": "pathToSerialPort", "pin": pinNumber}`
* `get`. Accepts the following parameters: `{"port": "pathToSerialPort", "pin": pinNumber, "sensor": "sensorType", "model": "sensorModel"}`

For detailed information about the pins and sensor arguments, check the documentation of the linked ESP8266 firmware


## Dependencies
This package depends on libserialport.
The package makefile can be found in this forum post [https://forum.openwrt.org/t/sigrok-libserialport/119245/2](https://forum.openwrt.org/t/sigrok-libserialport/119245/2)
