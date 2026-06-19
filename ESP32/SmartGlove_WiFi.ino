// WiFi library — built into the ESP32, lets us create a server that the laptop connects to
#include <WiFi.h>

//       WiFi credentials
// Must match your phone hotspot exactly (case-sensitive, no extra spaces)
// Both the ESP32 and the laptop need to be on this same network
const char* ssid     = "YOUR_WIFI_NAME";     // <-- Change to your hotspot name
const char* password = "YOUR_WIFI_PASSWORD";  // <-- Change to your hotspot password

// Create a WiFi server on port 8080 — the Python script will connect to this
// Think of it like opening a shop: we pick a door number (8080) and wait for customers
WiFiServer wifiServer(8080);

// GPIO 13 receives UART data from the PIC microcontroller
// The PIC sends 'L', 'R', 'F', or 'C' through a voltage divider into this pin
#define RX_PIN 13

void setup() {
    // Serial: USB connection to the laptop for debugging (visible in Arduino Serial Monitor)
    Serial.begin(9600);

    // Serial2: second UART channel that listens on GPIO 13 for data from the PIC
    // Parameters: 9600 baud, 8 data bits, no parity, 1 stop bit, RX on GPIO 13, TX unused (-1)
    Serial2.begin(9600, SERIAL_8N1, RX_PIN, -1);

    // Connect to the WiFi hotspot
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");

    // Wait until connected — prints a dot every 500ms so we know it's still trying
    // If this loops forever, the SSID or password is wrong
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    // Connected! Print the IP address — the Python script needs this exact address
    Serial.println();
    Serial.print("Connected! IP: ");
    Serial.println(WiFi.localIP());

    // Start listening for incoming connections from the laptop
    wifiServer.begin();
    Serial.println("Server started on port 8080");
}

void loop() {
    // Check if the Python script on the laptop is trying to connect
    WiFiClient client = wifiServer.available();

    if (client) {
        // A client (the laptop) just connected
        Serial.println("Client connected!");

        // Stay in this loop as long as the laptop stays connected
        while (client.connected()) {

            // Check if the PIC sent a command byte over UART
            if (Serial2.available()) {
                char cmd = Serial2.read();    // Read the character ('L', 'R', 'F', or 'C')
                client.print(cmd);            // Forward it to the laptop over WiFi
                Serial.print("Forwarded: ");  // Also print to Serial Monitor for debugging
                Serial.println(cmd);
            }

            delay(10);  // Small pause to avoid overwhelming the CPU
        }

        // Laptop disconnected (closed the Python script or lost WiFi)
        client.stop();
        Serial.println("Client disconnected");
    }
    // If no client is connected, loop() just keeps checking — ready for the next connection
}
