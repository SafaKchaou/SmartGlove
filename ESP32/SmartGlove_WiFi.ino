#include <WiFi.h>

const char* ssid     = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

WiFiServer wifiServer(8080);

#define RX_PIN 13

void setup() {
    Serial.begin(9600);
    Serial2.begin(9600, SERIAL_8N1, RX_PIN, -1);

    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("Connected! IP: ");
    Serial.println(WiFi.localIP());

    wifiServer.begin();
    Serial.println("Server started on port 8080");
}

void loop() {
    // UART forwarding will be added next
}