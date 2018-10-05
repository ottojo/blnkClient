#include <Adafruit_NeoPixel.h>

#include <ESP8266WiFi.h>

#define WIFI_SSID "Toolbox"
#define WIFI_PW "WIFIPASSWORD"
#define DEBUG

#define COMMAND_PORT 7891

WiFiServer server(COMMAND_PORT);

uint16_t receiveLength;
char receiveBuffer[2048];
int pixelcount = 60;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(pixelcount, D5, NEO_GRB + NEO_KHZ800);

void setup(void) {
  // Serial Port
#ifdef DEBUG
  Serial.begin(9600);
#endif

  // WiFi
#ifdef DEBUG
  Serial.println("Initializing WiFi");
  Serial.printf("Connecting to \"%s\"\n", WIFI_SSID);
#endif
  WiFi.mode(WIFI_STA);
  WiFi.hostname("BLNK-CLIENT");
  WiFi.begin(WIFI_SSID, WIFI_PW);
#ifdef DEBUG
  Serial.println("");
#endif
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
#ifdef DEBUG
    Serial.print(".");
#endif
  }
#ifdef DEBUG
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
#endif

  // NeoPixel
  pixels.begin();

  // Server
  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (client)
  {
    Serial.println("Client connected");
    long lastActivity = millis();
    while (client.connected())
    {
      if (client.available())
      {
        Serial.println("Data available");
        while (client.read() != 0xaf) {}        // Start byte
        receiveLength = client.read() << 8;     // Length high bye
        receiveLength |= client.read();         // Length low byte
        client.readBytes(receiveBuffer, receiveLength);
        handleMessage();
        lastActivity = millis();
      } else if (millis() < lastActivity || millis() > lastActivity + 30000) {
        Serial.println("Client timeout");
        client.stop();
      }
    }
    Serial.println("Client disconnected");
  }
}

void handleMessage() {
  Serial.println("message:");
  for (int i = 0; i < receiveLength; i++) {
    Serial.print(receiveBuffer[i], HEX);
  }
  if (receiveBuffer[0] == 1) {
    for (int i = 0; i < receiveLength && i < 3 * pixelcount; i++) {
      pixels.setPixelColor(i, pixels.Color(receiveBuffer[3 * i + 1], receiveBuffer[3 * i + 2], receiveBuffer[3 * i + 3]));
    }
    pixels.show();
  }
}
