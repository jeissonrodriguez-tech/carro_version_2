#ifndef SERVO_RELE_H
#define SERVO_RELE_H

#include <Arduino.h>
#include <ESP32Servo.h>

// ==== Pines ====
#define SERVO_PIN 13
#define RELAY_PIN 17  // relé que controla el ventilador o purificador

// ==== Variables globales ====
extern Servo servoMotor;
extern int servoPos;

// ==== Variables del ventilador ====
extern bool ventiladorEncendido;

// ==== Funciones principales ====
void iniciarServoYRele();

// ==== Servo ====
void moverServo(int angulo);
void moverServoIzquierda();
void moverServoCentro();
void moverServoDerecha();
void barridoServo();

// ==== Ventilador / Relé ====
void encenderVentilador();
void apagarVentilador();
// Control del ventilador por parámetro
void setVentilador(bool estado);

#endif
