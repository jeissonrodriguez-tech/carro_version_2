#ifndef SENSORES_H
#define SENSORES_H

#include <Arduino.h>

// === Pines de los sensores ultrasónicos ===
#define TRIG_LEFT   19
#define ECHO_LEFT   21
#define TRIG_RIGHT  22
#define ECHO_RIGHT  23
#define TRIG_FRONT  5
#define ECHO_FRONT  18

// === Variables globales de distancia ===
extern long distanciaFront;
extern long distanciaLeft;
extern long distanciaRight;

// === Funciones principales ===
void iniciarSensores();
long medirDistancia(int trigPin, int echoPin);
long medirPromedio(int trig, int echo);
void ajustarVelocidad(long dFront);
void correccionTrayectoria(long dIzq, long dDer);

// === Funciones de memoria y entorno (opcional) ===
void guardarMemoria(int obstaculo);
bool zonaBloqueada(int dir);
int escanearEntorno();

// === Funciones auxiliares ===
long medirDistanciaFront();
long medirDistanciaLeft();
long medirDistanciaRight();

// ==== MQ-135 ====
#define MQ135_PIN 34
void iniciarMQ135();
int leerMQ135();

#endif
