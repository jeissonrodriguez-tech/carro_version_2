#ifndef WEBSERVER_SETUP_H
#define WEBSERVER_SETUP_H

#include <ESPAsyncWebServer.h>

// --- Variables globales ---
extern AsyncWebServer server;

// Modo actual: "manual" o "autonomo"
extern String currentMode;

// Estado del control (para modo manual)
extern bool controlEnabled;

// Estado del modo autónomo (encendido o apagado)
extern bool modoAutonomoActivo;

// --- Funciones principales ---
void setupWebServer();
void handleSensorData(AsyncWebServerRequest *request);

// --- Funciones auxiliares ---
void setModoManual();
void setModoAutonomo();
bool estaAutonomoActivo();

#endif
