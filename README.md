# ESP32-BT-WiFi-PWM
Control PWM on ESP32 GPIO outputs over Bluetooth or WiFi

This can receive JSON data over Bluetooth or HTTP GET request (using simple built-in web interface) to control 8 PWM outputs, and WiFi SSID and password can be configured over Bluetooth and saved in flash. PWM value (marked as "dc" in JSON and GET requests) is 12 bit, so 0-4096.

Example Bluetooth request to adjust Channel 0 to 2000:
```
{
        "ch": "0",
        "dc": "2000"
}
```

Example HTTP GET request for the same settings:
`http://192.168.1.79/?ch=0&dc=1000`

## Dependencies
ArduinoJson https://github.com/bblanchon/ArduinoJson
