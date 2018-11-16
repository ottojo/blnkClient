#include <Adafruit_NeoPixel.h>

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>


constexpr String WIFI_SSID = "Toolbox";
constexpr String WIFI_PW = "WIFIPASSWORD"
constexpr uint16_t COMMAND_PORT = 7891;

//#define DEBUG_ALL_DATA

enum class ReceivingState {WAITING, WAITING_L1, WAITING_L2, RECEIVING_DATA};
ReceivingState state = ReceivingState::WAITING;


WiFiServer server(COMMAND_PORT);
WiFiUDP udp;

uint16_t receiveLength;
uint8_t receiveBuffer[2048];
int byteCounter = 0;
int pixelcount = 50;

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
    for (int i = 0; i < pixelcount; i++) {
        pixels.setPixelColor(i - 1, pixels.Color(0, 0, 0));
        pixels.setPixelColor(i, pixels.Color(50, 50, 50));
        pixels.show();
        delay(100);
    }
    pixels.setPixelColor(pixelcount - 1, 0, 0, 0);
    pixels.show();

    // Server
    server.begin();

    pinMode(D8, INPUT);
    attachInterrupt(digitalPinToInterrupt(D8), sendDiscoverMessage, FALLING);

    // Connection LED 2
    pinMode(D7, OUTPUT);
    digitalWrite(D7, LOW);

    // Data LED 1
    pinMode(D6, OUTPUT);
    digitalWrite(D6, LOW);
}

void sendDiscoverMessage() {
    udp.begin(1234);
    udp.beginPacketMulticast  (IPAddress(239, 1, 3, 37), 1337, WiFi.localIP());
    udp.write(ID);
    udp.write(" ");
    char cstr[16];
    itoa(COMMAND_PORT, cstr, 10);
    udp.write(cstr);
    udp.write(" blnk protocol discovery");
    udp.endPacket();
    udp.stop();
}


void loop() {
    sendDiscoverMessage();

    delay(1000);

    WiFiClient client = server.available();
    if (client)
    {
        digitalWrite(D7, HIGH);
        printDebug("Client connected");
        while (client.connected())
        {
            while (client.available() > 0)
            {
                digitalWrite(D6, HIGH);
                printDebug("Data! yay!");
                switch (state) {
                    case ReceivingState::WAITING:
                        if (client.read() == 0xaf) {
#ifdef DEBUG_ALL_DATA
                            Serial.println("Got startbyte.");
#endif
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
            digitalWrite(D6, LOW);
        }

        digitalWrite(D7, LOW);
        printDebug("Client disconnected");
    }
}

void handleMessage() {
    printDebug("Handling");
    if (receiveBuffer[0] == 2) {
        // Command 2 = interval
        printDebug("Interval packet");

        pixels.show();
    } else if (receiveBuffer[0] == 1) {
        // Command 1 = frame
        printDebug("Frame packet");
        for (int i = 0; i < receiveLength && i < 3 * pixelcount; i++) {
            pixels.setPixelColor(i, pixels.Color(receiveBuffer[3 * i + 1], receiveBuffer[3 * i + 2], receiveBuffer[3 * i + 3]));
        }
        pixels.show();
    } else if (receiveBuffer[0] == 0) {
        // Command 0 = discard
        printDebug("Discard Packet");
    }
    printDebug("Done");
}

void printDebug(String s) {
#ifdef DEBUG
    Serial.println(s);
#endif
}
