#ifndef MOTORES_H
#define MOTORES_H

#include <Arduino.h>

// ==== Pines de control ====
#define IN1 25
#define IN2 26
#define IN3 32
#define IN4 33
#define ENA 27
#define ENB 14

// ==== Configuración PWM ====
#define PWM_FREQ 5000
#define PWM_RES 8
#define PWM_CH_A 0
#define PWM_CH_B 1

// ==== Variables globales ====
// Solo se declaran aquí (sin asignar valores)
extern int velocidadPWM;         // Control de velocidad actual (0-255)
extern int velocidadPorcentaje;  // Velocidad en porcentaje (0-100)

// ==== Funciones principales ====
void inicializarMotores();
void moverAdelante(int vel = -1);
void moverAtras(int vel = -1);
void girarIzquierda(int vel = -1);
void girarDerecha(int vel = -1);
void detenerMotores();

// --- Utilidades de velocidad ---
int getVelocidadPWM();

// ==== Movimientos suaves ====
void girarSuaveIzquierda();
void girarSuaveDerecha();
void frenarSuave();

// ==== Control de velocidad (necesario para webserver_setup.cpp) ====
void setVelocidad(int nuevaVelocidad);
int getVelocidad();

// === Presets y utilidades ===
void setPresetSpeed(bool autonoma, int level); // level: 1..3

#endif
