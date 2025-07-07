#include <WiFi.h>
#include "esp_wifi.h"

// Access Point credentials
const char* ssid = "ESP32_AP";
const char* password = "esp32pass";  // Must be at least 8 characters

void printMAC(const uint8_t* mac) {
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 0x10) Serial.print("0");
    Serial.print(mac[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Setting up ESP32 as Access Point...");

  // Set WiFi to AP mode
  WiFi.mode(WIFI_AP);

  // Start the soft AP
  if (WiFi.softAP(ssid, password)) {
    Serial.println("Access Point started successfully!");
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
    uint8_t esp_mac[6];
    esp_wifi_get_mac(WIFI_IF_AP, esp_mac);
    Serial.print("ESP32 AP MAC address: ");
    printMAC(esp_mac);
  } else {
    Serial.println("Failed to start Access Point.");
  }
}

void loop() {
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 5000) {
    Serial.print("- Clients connected: ");
    Serial.println(WiFi.softAPgetStationNum());
    lastPrint = millis();
  }
}
