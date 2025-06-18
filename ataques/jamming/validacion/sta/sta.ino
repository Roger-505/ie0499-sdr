#include <WiFi.h>
#include <HTTPClient.h>

// Credenciales del AP al que se conecta (el que creaste antes)
const char* ssid = "ESP32_AP";
const char* password = "12345678";

// Dirección IP del AP (por defecto en modo SoftAP)
const char* serverIP = "192.168.4.1";

int i = 0;
void setup() {
  Serial.begin(115200);
  delay(1000);

  // Conectar al AP
  Serial.printf("Conectando a %s", ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConectado a AP");
  Serial.print("Dirección IP local: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String("http://") + serverIP + "/";
    
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("Respuesta del servidor:");
      Serial.print(payload);
      Serial.print(" ");
      Serial.println(i);
    } else {
      Serial.printf("Error en la solicitud HTTP: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  } else {
    Serial.println("No conectado a WiFi.");
  }
  i += 1;
  delay(5000);  // Espera 5 segundos antes de la próxima solicitud
}
