#include <WiFi.h>
#include "BluetoothSerial.h"
#include <ArduinoJson.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <Preferences.h>
#include "index_html.h"

#define JSON_BUFFER_SIZE 240

char ssid[32];
char pwd[32];

int wifiTimeout = 20;
WiFiServer server(80);
bool wifiConnected = false;
bool hasCredentials = false;
bool connStatusChanged = false;


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

byte channel;
int onTime;

BluetoothSerial SerialBT;
char bluetoothName[32] = "VintLabs Controller";


byte debugLevel = 5;

void serialOutput(char *text, byte debug)
{
	if (debugLevel < debug)
		return;

	Serial.print(text);
}


void connectWifi()
{
	serialOutput("Connecting to ", 1);
	serialOutput(ssid, 1);
	serialOutput("\n", 1);

	WiFi.disconnect();

	WiFi.begin(ssid, pwd);

	int i = 0;
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(250);
		serialOutput(".", 1);
		i++;
		if ( i / 4 > wifiTimeout )
		{
			serialOutput("Timeout connecting to ", 1);
			serialOutput(ssid, 1);
			serialOutput("\n", 1);
			return;
		}
	}

	serialOutput("\nConnected to ", 1);
	serialOutput(ssid, 1);
	serialOutput(" IP: ", 1);
	serialOutput((char*)WiFi.localIP().toString().c_str(), 1);
	serialOutput("\n", 1);

	// start the web server
	server.begin();

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

                        serialOutput("Rx over BT: SSID = ", 2);
                        serialOutput(ssid, 2);
			serialOutput("\n", 2);

                        connStatusChanged = true;
                        hasCredentials = true;
                } // containsKey ssid
        } // if jsonIn.success
}



void setup()
{
	Serial.begin(115200);
	SerialBT.begin(bluetoothName);

	// Set timeout for readBytesUntil etc (default is 1000ms)
	SerialBT.setTimeout(250);

	serialOutput("Bluetooth started, connect to \"", 1);
	serialOutput(bluetoothName, 1);
	serialOutput("\"\n",1);

	Preferences prefs;
	prefs.begin("WiFiCred", false);
	
	if (prefs.getBool("valid", false))
	{
		strcpy(ssid, prefs.getString("ssid","").c_str());
		strcpy(pwd, prefs.getString("pwd", "").c_str());

		serialOutput("Found SSID from saved preferences: ", 1);
		serialOutput(ssid, 1);
		serialOutput("\n", 1);

		hasCredentials = true;
	} // if prefs valid

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


	
} // setup()

void loop()
{
	serialOutput("Entering main loop\n", 2);

	// Poll for BT or WiFi requests
	while (1)
	{
		// check if WiFi connection status is to be changed
		if (connStatusChanged)
		{
			connectWifi();
			connStatusChanged = false;
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
