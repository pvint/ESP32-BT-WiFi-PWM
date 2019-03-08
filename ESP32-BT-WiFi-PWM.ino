#include <WiFi.h>
#include "BluetoothSerial.h"
#include <ArduinoJson.h>
//#include <nvs.h>
//#include <nvs_flash.h>
#include <Preferences.h>
#include "fauxmoESP.h"
#include "esp_err.h"
#include "index_html.h"

#define JSON_BUFFER_SIZE 240

char ssid[32];
char pwd[32];

int wifiTimeout = 20;
WiFiServer server(8080);
bool wifiConnected = false;
bool hasCredentials = false;
bool connStatusChanged = false;

fauxmoESP fauxmo;

// VintLabs Module PWM GPIO Assignments
#define PWM0 12
#define PWM1 13
#define PWM2 14
#define PWM3 15
#define PWM4 16
#define PWM5 17
#define PWM6 18
#define PWM7 19

// Default PWM properties
int freq = 5000;
byte ledChannel = 0;
int ledValue[8] = {0, 0, 0, 0, 0, 0, 0, 0};
byte resolution = 12;
byte ledDelay = 1;

byte channel;
int onTime;

BluetoothSerial SerialBT;
char bluetoothName[32] = "VintLabs Starfish";


byte debugLevel = 5;

void connectWifi()
{
	Serial.printf("Connecting to %s...\n", ssid);

	WiFi.disconnect();

	WiFi.begin(ssid, pwd);

	int i = 0;
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(250);
		Serial.print(".");
		i++;
		if ( i / 4 > wifiTimeout )
		{
			Serial.printf("Timeout connecting to %s\n", ssid);
			return;
		}
	}

	Serial.printf("\nConnected to %s  IP: %s\n", ssid, WiFi.localIP().toString().c_str());

	// start the web server
	server.begin();

	wifiConnected = true;
	return;
} // connectWifi()

void decodeJson(JsonObject &json)
{
        //json.printTo(Serial);
        if (json.success())
        {
                if (json.containsKey("ch") && json.containsKey("dc"))
                {
                        channel = json["ch"].as<byte>();
                        onTime = json["dc"].as<int>();
                } // if containsKey ch && dc
                else if (json.containsKey("ssid") && json.containsKey("pwd"))
                {
                        strcpy(ssid, (char*)json["ssid"].as<char*>());
                        strcpy(pwd, (char*)json["pwd"].as<char*>());

                        Preferences prefs;
                        prefs.begin("WiFiCred", false);
                        prefs.putString("ssid", ssid);
                        prefs.putString("pwd", pwd);
                        prefs.putBool("valid", true);
                        prefs.end();

			Serial.printf("Rx ssid over BT: \"%s\"\n", ssid);

                        connStatusChanged = true;
                        hasCredentials = true;
                } // containsKey ssid
        } // if jsonIn.success
}

void fadeLed(byte led, unsigned char value)
{
	int val = value * 16;
	int s = ledcRead(led);

	if (s == val)
		return;

	byte step;

	if (val > s)
		step = 1;
	else
		step = -1;

	//Serial.printf("Was %d, set to %d, step %d\n", s, val, step);
	for (int v = s; v != val; v += step)
	{
		ledcWrite(led, v);
		delay(ledDelay);
		//Serial.println(v);
	}

	ledcWrite(led, val);
} // fadeLed()

void setup()
{
	Serial.begin(115200);

	// Get MAC
	uint8_t mac[6];
	esp_efuse_mac_get_default(mac);

	Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x\n",mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	String bname = String(bluetoothName) + String("-") + String(mac[5],HEX);
	SerialBT.begin(bname);

	// Set timeout for readBytesUntil etc (default is 1000ms)
	SerialBT.setTimeout(250);

	Serial.printf("Bluetooth started. Connect to %s\n", bname.c_str());

	Preferences prefs;
	prefs.begin("WiFiCred", false);
	
	if (prefs.getBool("valid", false))
	{
		strcpy(ssid, prefs.getString("ssid","").c_str());
		strcpy(pwd, prefs.getString("pwd", "").c_str());

		Serial.printf("Found SSID from saved preferences: \"%s\"\n", ssid);
		//Serial.printf("Found pass from saved preferences: \"%s\"\n", pwd);

		hasCredentials = true;
	} // if prefs valid
strcpy(ssid,"RFBM2");
strcpy(pwd, "marchalfranck");
	if (hasCredentials)
		connectWifi();

	// Set up PWM
	for (byte i = 0; i < 8; i++)
	{
		ledcSetup(i, freq, resolution);
	}

	// attach channels to pins
	ledcAttachPin(PWM0, 0);
        ledcAttachPin(PWM1, 1);
        ledcAttachPin(PWM2, 2);
        ledcAttachPin(PWM3, 3);
        ledcAttachPin(PWM4, 4);
        ledcAttachPin(PWM5, 5);
        ledcAttachPin(PWM6, 6);
        ledcAttachPin(PWM7, 7);

        pinMode(PWM0, OUTPUT);
        pinMode(PWM1, OUTPUT);
        pinMode(PWM2, OUTPUT);
        pinMode(PWM3, OUTPUT);
        pinMode(PWM4, OUTPUT);
        pinMode(PWM5, OUTPUT);
        pinMode(PWM6, OUTPUT);
        pinMode(PWM7, OUTPUT);


	// Temp setup for fauxmoESP
	Serial.println("Setting up fauxmo stuff");
	fauxmo.createServer(true);
	fauxmo.setPort(80);	// required for gen3 devices apparently??
	if (wifiConnected)
	{
		Serial.println("Enabling fauxmo");
		fauxmo.enable(true);
	}
	fauxmo.addDevice("Starfish");

	fauxmo.onSetState([](unsigned char device_id, const char *device_name, bool state, unsigned char val)
	{
		//fadeLed(0, value);
		int value = val << 4;
		int led = 0;	
		int s = ledcRead(led);

		if (s == value)
			return;

		byte step;

		if (value > s)
			step = 1;
		else
			step = -1;

		//Serial.printf("Was %d, set to %d, step %d\n", s, val, step);
		for (int v = s; v != value; v += step)
		{
			ledcWrite(led, v);
			delay(ledDelay);
			//Serial.println(v);
		}

		ledcWrite(led, value);

	});

	// callback for getBinaryState
	/*
	fauxmo.onGetState([](unsigned char device_id, const char *device_name) {
		return !digitalRead(2);
	});*/
	
} // setup()

void loop()
{
	// Poll for BT or WiFi requests
	while (1)
	{
		// check if WiFi connection status is to be changed
		if (connStatusChanged)
		{
			wifiConnected = false;
			connectWifi();
			connStatusChanged = false;
			if (wifiConnected)
			{
				Serial.println("(Re)enabling fauxmo");
				fauxmo.enable(true);
			}

		}

		// fauxmo stuff
		if (wifiConnected)
		{
			//Serial.printf("[BEFOREHANDLE] Free heap: %d bytes\n", ESP.getFreeHeap());
			fauxmo.handle();
			//Serial.printf("[AFTERHANDLE] Free heap: %d bytes\n", ESP.getFreeHeap());

    // If your device state is changed by any other means (MQTT, physical button,...)
    // you can instruct the library to report the new state to Alexa on next request:
    // fauxmo.setState(ID_YELLOW, true, 255);
		}

		// Check for incoming Bluetooth data
		if (SerialBT.available())
		{
			const size_t bufferSize = JSON_OBJECT_SIZE(1) + JSON_BUFFER_SIZE;

			DynamicJsonBuffer jsonBuffer(bufferSize);
			JsonObject& root = jsonBuffer.parseObject(SerialBT);

			if (root.success())
			{
				decodeJson(root);
				ledcWrite(channel, onTime);
			} // if root.success
		} // if SerialBT.available()

		// Check for WiFi connection
		WiFiClient client = server.available();  // listen for incoming clients
		
		if (client)
		{
			// TODO: Determine a good size for the HTTP request
			// TODO Convert this to currentLine[]
			String currentLine = "";
			//char *currentLine = (char*)malloc(1024 * sizeof(char));

			while (client.connected())
			{
				if (client.connected())
				{
					char c = client.read();

					if (c == '\n')
					{
						// \n\n denotes the end of the request
						if (currentLine.length() == 0)
						{
							//serialOutput("Sending web page...", 4);
							//serialOutput("\n", 4);
							client.println("HTTP/1.1 200 OK");
							client.println("Content-type:text/html");
							client.println();

							// Send the web page
							client.println(INDEX_HTML);
							// End connection with second newline
							client.println();

							// Nothing more to do in the loop this time around
							break;

						} // if len(currentLine == 0)
						else
						{
							// Find variables in GET
							if ((currentLine.indexOf("ch=") > 0 ) && (currentLine.indexOf("HTTP/1.1") > 0 ))
							{
								//serialOutput("Found ch=\n", 2);
								// will look like GET /?ch=0&dc=558 HTTP/1.1
								// Really need to get rid of all Strings....
								String s = currentLine.substring(currentLine.indexOf("ch="));

								int c = s.substring(3, 4).toInt();

								int d = s.substring(s.indexOf("&dc=") + 4, s.indexOf(" HTTP")).toInt();

								Serial.print("Channel ");
								Serial.print(c);
								Serial.print(" Value ");
								Serial.println(d);

								// set the LED global variables
								channel = c;
								onTime = d;

								ledcWrite(channel, onTime);
							} // if contains
							
							currentLine = "";
						} // else
					} // if c == \n
					else if (c != '\r')
					{
						currentLine += c;
					}
				} // if clinet.connected
			} // while client.connected

			// close the connection
			client.stop();
		} // if client
	} // while 1
}
