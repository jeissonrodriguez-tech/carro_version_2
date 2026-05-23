#include "motores.h"
#include <Arduino.h>

// ==== Pines ====
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
int velocidadPWM = 128;           // Velocidad real (0–255)
int velocidadPorcentaje = 50;     // Velocidad en porcentaje (0–100)
// Presets: niveles 1..3 (porcentaje)
static const int presetsManual[3] = {30, 60, 90};
static const int presetsAutonoma[3] = {30, 50, 80};

// ===== FUNCIONES =====

// --- Ajustar velocidad desde web o joystick (0–100%) ---
void setVelocidad(int porcentaje) {
  velocidadPorcentaje = constrain(porcentaje, 0, 100);
  velocidadPWM = map(velocidadPorcentaje, 0, 100, 0, 255);
  Serial.printf("⚙️ Velocidad establecida: %d%% (%d PWM)\n", velocidadPorcentaje, velocidadPWM);
}

// --- Obtener velocidad actual en porcentaje (para web) ---
int getVelocidad() {
  return velocidadPorcentaje;
}

// --- Obtener velocidad real PWM (para motores) ---
int getVelocidadPWM() {
  return velocidadPWM;
}

// === Establecer preset por nivel (1..3) ===
void setPresetSpeed(bool autonoma, int level) {
  if (level < 1) level = 1;
  if (level > 3) level = 3;
  int pct = autonoma ? presetsAutonoma[level-1] : presetsManual[level-1];
  setVelocidad(pct);
}

// --- Inicializar pines y PWM ---
void inicializarMotores() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  ledcSetup(PWM_CH_A, PWM_FREQ, PWM_RES);
  ledcSetup(PWM_CH_B, PWM_FREQ, PWM_RES);
  ledcAttachPin(ENA, PWM_CH_A);
  ledcAttachPin(ENB, PWM_CH_B);

  detenerMotores();
}

// --- Movimiento hacia adelante ---
void moverAdelante(int porcentaje) {
  if (porcentaje >= 0) setVelocidad(porcentaje);
  ledcWrite(PWM_CH_A, velocidadPWM);
  ledcWrite(PWM_CH_B, velocidadPWM);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

// --- Movimiento hacia atrás ---
void moverAtras(int porcentaje) {
  if (porcentaje >= 0) setVelocidad(porcentaje);
  ledcWrite(PWM_CH_A, velocidadPWM);
  ledcWrite(PWM_CH_B, velocidadPWM);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

// --- Giro a la izquierda ---
void girarIzquierda(int porcentaje) {
  if (porcentaje >= 0) setVelocidad(porcentaje);
  ledcWrite(PWM_CH_A, velocidadPWM);
  ledcWrite(PWM_CH_B, velocidadPWM);
  // Motor izquierdo hacia atrás, motor derecho hacia adelante
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

// --- Giro a la derecha ---
void girarDerecha(int porcentaje) {
  if (porcentaje >= 0) setVelocidad(porcentaje);
  ledcWrite(PWM_CH_A, velocidadPWM);
  ledcWrite(PWM_CH_B, velocidadPWM);
  // Motor izquierdo hacia adelante, motor derecho hacia atrás
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

// --- Detener motores ---
void detenerMotores() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  ledcWrite(PWM_CH_A, 0);
  ledcWrite(PWM_CH_B, 0);
}
