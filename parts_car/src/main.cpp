#include <Arduino.h>
#include <WiFi.h>
#include "webserver_setup.h"
#include "sensores.h"
#include "motores.h"
#include "modo_autonomo.h"
#include "servo_rele.h"  

// --- Configuración WiFi ---
const char* ssid = "RobotESP32";
const char* password = "12345678";

void setup() {
  Serial.begin(115200);

  // --- WiFi como punto de acceso ---
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.printf("📶 WiFi AP iniciado: %s\n", ssid);

  // --- Inicialización de hardware ---
  iniciarServoYRele();   // 👈 AGREGA ESTA LÍNEA
  inicializarMotores();
  iniciarSensores();
  iniciarMQ135();

  // --- Servidor web ---
  setupWebServer();

  Serial.println("✅ Configuración completa y lista para operar");
}

void loop() {
  // ⚠️ AsyncWebServer NO necesita server.handleClient()

  // 🧠 Ejecuta solo si el modo autónomo está activado
  if (estaAutonomoActivo()) {
    ejecutarModoAutonomo(MODE_AUTO);
  } 
  else {
    static unsigned long ultimoPrint = 0;
    if (millis() - ultimoPrint > 2000) {
      ultimoPrint = millis();

      long front = medirDistanciaFront();
      long left  = medirDistanciaLeft();
      long right = medirDistanciaRight();

      int mq = leerMQ135();

      Serial.print("Frontal: ");
      Serial.print(front);
      Serial.print(" cm | Izquierdo: ");
      Serial.print(left);
      Serial.print(" cm | Derecho: ");
      Serial.print(right);
      Serial.print(" cm | MQ135: ");
      Serial.println(mq);
    }
  }
}


