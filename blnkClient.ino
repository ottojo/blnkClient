#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>

constexpr String WIFI_SSID = "Toolbox";
constexpr String WIFI_PW = "WIFIPASSWORD"
constexpr uint16_t COMMAND_PORT = 7891;

//#define DEBUG_ALL_DATA

enum class ReceivingState {WAITING, WAITING_L1, WAITING_L2, RECEIVING_DATA};
ReceivingState state = ReceivingState::WAITING;

uint16_t receiveLength;
uint8_t receiveBuffer[2048];
int byteCounter = 0;
int pixelcount = 60;

WiFiServer server(COMMAND_PORT);
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(pixelcount, D5, NEO_GRB + NEO_KHZ800);

void setup(void) {
    // Serial Port
    Serial.begin(9600);

    // WiFi
    Serial.println("Initializing WiFi");
    Serial.printf("Connecting to \"%s\"\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.hostname("BLNK-CLIENT");
    WiFi.begin(WIFI_SSID, WIFI_PW);
    Serial.println("");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(WIFI_SSID);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

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
        while (client.connected())
        {
            while (client.available() > 0)
            {
                switch (state) {
                    case ReceivingState::WAITING:
                        if (client.read() == 0xaf) {
                            state = ReceivingState::ReceivingState::WAITING_L1;
                        }
                        break;
                    case ReceivingState::WAITING_L1:
                        receiveLength = client.read() << 8;
                        state = ReceivingState::ReceivingState::WAITING_L2;
                        break;
                    case ReceivingState::WAITING_L2:
                        receiveLength |= client.read();
                        byteCounter = 0;
                        state = ReceivingState::RECEIVING_DATA;
                        break;
                    case ReceivingState::RECEIVING_DATA:
                        receiveBuffer[byteCounter] = client.read();
                        byteCounter++;
                        if (byteCounter >= receiveLength) {
#ifdef DEBUG_ALL_DATA
                            Serial.println("Received data packet:");
                            for (int i = 0; i < receiveLength; i++) {
                                Serial.println(receiveBuffer[i], HEX);
                            }
                            Serial.println();
#endif
                            handleMessage();
                            state = ReceivingState::WAITING;
                        }
                        break;
                }
            }
        }
        Serial.println("Client disconnected");
    }
}

void handleMessage() {
    Serial.println("Handling");
    if (receiveBuffer[0] == 1) {
        // Command 1 = frame
        Serial.println("Frame packet");
        for (int i = 0; i < receiveLength && i < 3 * pixelcount; i++) {
            pixels.setPixelColor(i, pixels.Color(receiveBuffer[3 * i + 1], receiveBuffer[3 * i + 2], receiveBuffer[3 * i + 3]));
        }
        pixels.show();
    } else if (receiveBuffer[0] == 0) {
        // Command 0 = discard
        Serial.println("Discard Packet");
    }
    Serial.println("Done");
}
