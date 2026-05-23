#include "webserver_setup.h"
#include "motores.h"
#include "modo_autonomo.h"
#include "sensores.h"
#include "servo_rele.h"
#include <SPIFFS.h>
#include <Arduino.h>

bool modoReal = false; // false = simulación, true = MQ135 real

AsyncWebServer server(80);

String currentMode = "manual";
bool controlEnabled = false;
static unsigned long lastSpeedChange = 0;
static const unsigned long SPEED_CHANGE_MIN_MS = 750;

// --- Sirve archivos desde SPIFFS ---
void handleFileRequest(AsyncWebServerRequest *request) {
  String path = request->url();
  if (path == "/") path = "/index.html";
  if (!SPIFFS.exists(path)) {
    request->send(404, "text/plain", "Archivo no encontrado");
    return;
  }

  String contentType = "text/html";
  if (path.endsWith(".css")) contentType = "text/css";
  else if (path.endsWith(".js")) contentType = "application/javascript";

  request->send(SPIFFS, path, contentType);
}

// --- Lectura de sensores en formato JSON ---
void handleSensorData(AsyncWebServerRequest *request) {
  long front = medirDistanciaFront();
  long left  = medirDistanciaLeft();
  long right = medirDistanciaRight();

  char json[128];
  snprintf(json, sizeof(json),
    "{\"front\":%ld,\"left\":%ld,\"right\":%ld}",
    front, left, right);

  request->send(200, "application/json", json);
}

// --- Configura todas las rutas del servidor ---
void setupWebServer() {
  if (!SPIFFS.begin(true)) {
    Serial.println("❌ Error al montar SPIFFS");
    return;
  }

  // --- Páginas principales ---
  server.on("/", HTTP_GET, handleFileRequest);
  server.on("/index.html", HTTP_GET, handleFileRequest);
  server.on("/control.html", HTTP_GET, handleFileRequest);

  // --- Ruta de sensores ---
  server.on("/ultrasonic", HTTP_GET, handleSensorData);

  // --- Lectura MQ135 (gas) ---
  server.on("/mq135", HTTP_GET, [](AsyncWebServerRequest *request) {
    int valor = leerMQ135();
    char json[64];
    snprintf(json, sizeof(json), "{\"mq135\":%d}", valor);
    request->send(200, "application/json", json);
  });

  // --- Obtener modo actual ---
  server.on("/getMode", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", currentMode);
  });

  // --- Encender / apagar control manual ---
  server.on("/toggleControl", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("state")) {
      String state = request->getParam("state")->value();
      controlEnabled = (state == "on");
      detenerMotores();
      setVelocidad(0);
      request->send(200, "text/plain", controlEnabled ? "on" : "off");
    } else {
      request->send(400, "text/plain", "Falta parámetro 'state'");
    }
  });

  // --- Movimiento manual ---
  auto validarManual = [](AsyncWebServerRequest *request) -> bool {
    if (!controlEnabled || currentMode != "manual") {
      request->send(403, "text/plain", "Control deshabilitado");
      return false;
    }
    return true;
  };

  server.on("/forward", HTTP_GET, [=](AsyncWebServerRequest *request) {
    if (!validarManual(request)) return;
    int vel = request->hasParam("vel") ? request->getParam("vel")->value().toInt() : -1;
    moverAdelante(vel);
    request->send(200, "text/plain", "ok");
  });

  server.on("/backward", HTTP_GET, [=](AsyncWebServerRequest *request) {
    if (!validarManual(request)) return;
    int vel = request->hasParam("vel") ? request->getParam("vel")->value().toInt() : -1;
    moverAtras(vel);
    request->send(200, "text/plain", "ok");
  });

  server.on("/left", HTTP_GET, [=](AsyncWebServerRequest *request) {
    if (!validarManual(request)) return;
    int vel = request->hasParam("vel") ? request->getParam("vel")->value().toInt() : -1;
    girarIzquierda(vel);
    request->send(200, "text/plain", "ok");
  });

  server.on("/right", HTTP_GET, [=](AsyncWebServerRequest *request) {
    if (!validarManual(request)) return;
    int vel = request->hasParam("vel") ? request->getParam("vel")->value().toInt() : -1;
    girarDerecha(vel);
    request->send(200, "text/plain", "ok");
  });

  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request) {
    detenerMotores();
    request->send(200, "text/plain", "stopped");
  });

  // --- Velocidad actual MANUAL ---
  server.on("/getVelocidad", HTTP_GET, [](AsyncWebServerRequest *request) {
    char json[64];
    snprintf(json, sizeof(json), "{\"velocidad\":%d}", getVelocidad());
    request->send(200, "application/json", json);
  });

  // --- Obtener velocidades (manual y autonoma) ---
  server.on("/getSpeeds", HTTP_GET, [](AsyncWebServerRequest *request) {
    char json[128];
    snprintf(json, sizeof(json),
      "{\"manual\":%d,\"autonomo\":%d,\"autonomo_max\":%d}",
      getVelocidad(), getVelocidadAutonoma(), getVelocidadMaximaAutonoma());
    request->send(200, "application/json", json);
  });

  // --- Actualizar velocidad MANUAL ---
  server.on("/setVelocidad", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("valor")) {
      int valor = request->getParam("valor")->value().toInt();
      setVelocidad(valor);
      request->send(200, "text/plain", "Velocidad actualizada");
    } else {
      request->send(400, "text/plain", "Falta el parámetro 'valor'");
    }
  });

  // --- Establecer preset de velocidad (1..3) para MANUAL ---
  server.on("/setSpeedLevel", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("mode") || !request->hasParam("level")) {
      request->send(400, "text/plain", "Falta parámetro 'mode' o 'level'");
      return;
    }

    unsigned long now = millis();
    if (now - lastSpeedChange < SPEED_CHANGE_MIN_MS) {
      request->send(429, "text/plain", "Cambios de velocidad muy rápidos");
      return;
    }

    String mode = request->getParam("mode")->value();
    int level = request->getParam("level")->value().toInt();
    bool auton = (mode == "autonomo");
    if (level < 1 || level > 3) {
      request->send(400, "text/plain", "Level debe ser 1..3");
      return;
    }

    setPresetSpeed(auton, level);
    lastSpeedChange = now;
    Serial.printf("⚙️ Preset velocidad aplicado: modo=%s nivel=%d\n", mode.c_str(), level);
    request->send(200, "text/plain", "ok");
  });

  // --- Cambiar modo entre manual / autónomo ---
  server.on("/setMode", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("mode")) {
      request->send(400, "text/plain", "Falta parámetro 'mode'");
      return;
    }

    String mode = request->getParam("mode")->value();
    detenerMotores();
    detenerModoAutonomo();

    if (mode == "autonomo") {
      currentMode = "autonomo";
      controlEnabled = false;
      Serial.println("🤖 Modo AUTÓNOMO seleccionado (en espera de inicio)");
    } else {
      currentMode = "manual";
      controlEnabled = true;
      Serial.println("🕹️ Modo MANUAL activado");
    }
    detenerMotores();
    setVelocidad(0);

    request->send(200, "text/plain", "ok");
  });

  // --- Iniciar modo autónomo ---
  server.on("/startAutonomo", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (currentMode != "autonomo") {
      request->send(403, "text/plain", "Debe estar en modo autónomo");
      return;
    }

    iniciarModoAutonomo();
    Serial.println("🚗 Iniciando modo autónomo...");
    request->send(200, "text/plain", "autonomo iniciado");
  });

  // --- Detener modo autónomo ---
  server.on("/stopAutonomo", HTTP_GET, [](AsyncWebServerRequest *request) {
    detenerModoAutonomo();
    Serial.println("🛑 Modo autónomo detenido");
    request->send(200, "text/plain", "autonomo detenido");
  });

  // --- Parada de emergencia ---
  server.on("/emergencyStop", HTTP_GET, [](AsyncWebServerRequest *request) {
    detenerModoAutonomo();
    detenerMotores();
    setVelocidad(0);
    controlEnabled = false;
    currentMode = "manual";
    Serial.println("‼️ Emergency STOP activado");
    request->send(200, "text/plain", "emergency_stop");
  });

  // --- Favicon ---
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(204);
  });

  // --- Error 404 ---
  server.onNotFound([](AsyncWebServerRequest *request) {
    handleFileRequest(request);
  });

  // ========== RUTAS NUEVAS PARA VELOCIDAD ADAPTATIVA ==========

  // --- Ruta de depuración: muestra el estado del modo autónomo ---
  server.on("/debugAutono", HTTP_GET, [](AsyncWebServerRequest *request) {
    String info = getAutonomoDebug();
    request->send(200, "application/json", info);
  });

  // === 🎚️ ESTABLECER PRESET DE VELOCIDAD MÁXIMA DEL AUTÓNOMO ===
  // Establece el LÍMITE MÁXIMO que el robot NO puede superar
  server.on("/setPresetAutonomo", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("nivel")) {
      request->send(400, "application/json", "{\"error\":\"Falta parámetro 'nivel' (1,2,3)\"}");
      return;
    }

    int nivel = request->getParam("nivel")->value().toInt();
    
    if (nivel < 1 || nivel > 3) {
      request->send(400, "application/json", "{\"error\":\"Nivel debe ser 1, 2 o 3\"}");
      return;
    }

    setPresetVelocidadAutonoma(nivel);
    
    char response[128];
    snprintf(response, sizeof(response),
      "{\"preset\":%d,\"velocidad_maxima\":%d,\"mensaje\":\"Límite máximo establecido\"}",
      nivel, getVelocidadMaximaAutonoma());
    
    request->send(200, "application/json", response);
  });

  // === 🎚️ ESTABLECER VELOCIDAD ACTUAL (respetando límite del preset) ===
  // La velocidad NO puede superar el límite del preset, pero puede reducirse por obstáculos
  server.on("/setVelocidadAutonomo", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("valor")) {
      request->send(400, "application/json", "{\"error\":\"Falta parámetro 'valor' (0-100)\"}");
      return;
    }

    int valor = request->getParam("valor")->value().toInt();
    valor = constrain(valor, 0, 100);

    setVelocidadAutonoma(valor);

    char response[128];
    snprintf(response, sizeof(response),
      "{\"velocidad_solicitada\":%d,\"velocidad_actual\":%d,\"velocidad_maxima\":%d}",
      valor, getVelocidadAutonoma(), getVelocidadMaximaAutonoma());

    request->send(200, "application/json", response);
  });

  // === 📊 OBTENER INFORMACIÓN DE VELOCIDADES AUTÓNOMAS ===
  server.on("/getVelocidadesAutonomo", HTTP_GET, [](AsyncWebServerRequest *request) {
    char response[256];
    snprintf(response, sizeof(response),
      "{\"velocidad_actual\":%d,\"velocidad_maxima_preset\":%d,\"presets\":[30,50,80],"
      "\"explicacion\":\"El robot puede reducir por obstáculos pero NO supera el límite máximo\"}",
      getVelocidadAutonoma(), getVelocidadMaximaAutonoma());

    request->send(200, "application/json", response);
  });

  // === 🎚️ CAMBIO RÁPIDO ENTRE PRESETS (1=Baja, 2=Media, 3=Alta) ===
  server.on("/quickPreset", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("p")) {
      request->send(400, "text/plain", "Falta parámetro 'p' (1,2,3)");
      return;
    }

    int preset = request->getParam("p")->value().toInt();
    setPresetVelocidadAutonoma(preset);

    const char* nombres[] = {"BAJA", "MEDIA", "ALTA"};
    const int valores[] = {30, 50, 80};
    
    char response[128];
    snprintf(response, sizeof(response),
      "{\"preset\":\"%s\",\"velocidad\":%d}",
      nombres[preset-1], valores[preset-1]);

    request->send(200, "application/json", response);
  });

  // --- Ruta para activar o desactivar el modo real (MQ135) ---
  server.on("/setRealMode", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("active")) {
      String value = request->getParam("active")->value();
      if (value == "1") {
        modoReal = true;
        Serial.println("✅ Modo REAL activado: MQ135 ON");
      } else {
        modoReal = false;
        Serial.println("🧪 Modo SIMULACIÓN activado: MQ135 OFF");
      }
      request->send(200, "text/plain", "OK");
    } else {
      request->send(400, "text/plain", "Falta parámetro 'active'");
    }
  });

  // --- Ruta para encender o apagar el ventilador ---
  server.on("/ventilador", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("state")) {
      request->send(400, "text/plain", "Falta parametro 'state'");
      return;
    }

    String estado = request->getParam("state")->value();
    estado.toLowerCase();

    if (estado == "on") {
      encenderVentilador();
      request->send(200, "text/plain", "Ventilador encendido");
    } 
    else if (estado == "off") {
      apagarVentilador();
      request->send(200, "text/plain", "Ventilador apagado");
    } 
    else {
      request->send(400, "text/plain", "Valor invalido (use on/off)");
    }
  });

  server.begin();
  Serial.println("🌐 Servidor web iniciado en puerto 80");
}
