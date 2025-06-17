#include <WiFi.h>
#include <WebServer.h>

// Parámetros del AP
const char* ssid = "ESP32_AP";
const char* password = "12345678";

// Instancia del servidor web en el puerto 80
WebServer server(80);

// Función para simular una respuesta de comunicación
void handleRoot() {
  server.send(200, "text/plain", "Simulación de comunicación exitosa.");
}

// Función de configuración
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Crear punto de acceso
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.println("Punto de acceso iniciado.");
  Serial.print("Dirección IP del AP: ");
  Serial.println(IP);

  // Rutas del servidor
  server.on("/", handleRoot);
  
  // Iniciar servidor
  server.begin();
  Serial.println("Servidor web iniciado.");
}

// Bucle principal
void loop() {
  server.handleClient();
}
