#include <WiFi.h>

extern "C" {
  #include "esp_wifi.h"
}

const char* ssid     = "ZLMP";         // ← Cambia por tu SSID
const char* password = "55552902";     // ← Cambia por tu contraseña

const int BOOT_BUTTON = 0;  // GPIO0 normalmente está conectado al botón BOOT

unsigned long lastCheck = 0;
const unsigned long checkInterval = 5000; // ms

bool manuallyDisconnected = false;

void connectToWiFi() {
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Conexión exitosa!");
    Serial.print("IP local: ");
    Serial.println(WiFi.localIP());

    Serial.print("Canal Wi-Fi: ");
    Serial.println(WiFi.channel());

    Serial.print("MAC del ESP32: ");
    Serial.println(WiFi.macAddress());

    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
      Serial.print("MAC del Access Point: ");
      char apMacStr[18];
      sprintf(apMacStr, "%02X:%02X:%02X:%02X:%02X:%02X",
              ap_info.bssid[0], ap_info.bssid[1], ap_info.bssid[2],
              ap_info.bssid[3], ap_info.bssid[4], ap_info.bssid[5]);
      Serial.println(apMacStr);
    } else {
      Serial.println("❌ No se pudo obtener la MAC del AP.");
    }
  } else {
    Serial.println("\n❌ Falló la conexión.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(BOOT_BUTTON, INPUT_PULLUP);  // Configurar GPIO0 con resistencia pull-up

  WiFi.mode(WIFI_STA);
  connectToWiFi();
}

void loop() {
  // Verificar conexión
  unsigned long now = millis();
  if (now - lastCheck >= checkInterval) {
    lastCheck = now;

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("✅ Conectado a Internet.");
    } else if (!manuallyDisconnected) {
      Serial.println("⚠️ Wi-Fi desconectado. Intentando reconectar...");
      connectToWiFi();
    } 
  }

  // Revisar botón BOOT
  if (digitalRead(BOOT_BUTTON) == LOW && WiFi.status() == WL_CONNECTED) {
    Serial.println("🛑 Botón BOOT presionado. Desconectando de WiFi...");
    WiFi.disconnect(true);  // Forzar desconexión
    manuallyDisconnected = true;
  }
}