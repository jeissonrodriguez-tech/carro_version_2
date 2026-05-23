#ifndef MODO_AUTONOMO_H
#define MODO_AUTONOMO_H

#include <Arduino.h>

// --- Modos de operación ---
#define MODE_MANUAL 0
#define MODE_AUTO   1

// --- Variable global ---
extern bool modoAutonomoActivo;

// --- Funciones principales ---
void ejecutarModoAutonomo(int modoActual);
void iniciarModoAutonomo();
void detenerModoAutonomo();
bool estaAutonomoActivo();

// --- Control de velocidad del modo autónomo ---
void setPresetVelocidadAutonoma(int preset);  // Establecer límite máximo (1=30%, 2=50%, 3=80%)
void setVelocidadAutonoma(int porcentaje);    // Establecer velocidad actual (respeta límite)
int getVelocidadAutonoma();                   // Obtener velocidad actual
int getVelocidadMaximaAutonoma();             // Obtener velocidad máxima del preset

// --- Estado / depuración ---
String getAutonomoDebug();

#endif
