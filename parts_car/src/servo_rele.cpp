#include "servo_rele.h"

// ==== Variables globales ====
Servo servoMotor;
int servoPos = 90;  // posición inicial al centro
bool ventiladorEncendido = false;  // 🆕 estado actual del ventilador

// ==== INICIALIZACIÓN ====
void iniciarServoYRele() {
  servoMotor.attach(SERVO_PIN);
  servoMotor.write(servoPos);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // HIGH = apagado si relé es activo en bajo

  // Escaneo inicial (opcional)
  for (int ang = 0; ang <= 180; ang += 15) {
    servoMotor.write(ang);
    delay(40);
  }
  servoMotor.write(90);

  Serial.println("Servo y relé inicializados");
}

// ==== SERVO ====
void moverServo(int angulo) {
  angulo = constrain(angulo, 0, 180);
  servoMotor.write(angulo);
  servoPos = angulo;
}

void moverServoIzquierda() {
  servoPos = 40;
  servoMotor.write(servoPos);
  Serial.println("Servo → izquierda");
}

void moverServoCentro() {
  servoPos = 90;
  servoMotor.write(servoPos);
  Serial.println("Servo → centro");
}

void moverServoDerecha() {
  servoPos = 140;
  servoMotor.write(servoPos);
  Serial.println("Servo → derecha");
}

// 👇 Nuevo: función de barrido (30°, 90°, 170°, 170°, 90°, 30°)
void barridoServo() {
  int angulos[] = {30, 90, 170, 170, 90, 30};
  for (int i = 0; i < 6; i++) {
    servoMotor.write(angulos[i]);
    servoPos = angulos[i];
    delay(300); // ajusta velocidad del barrido (300–500 ms recomendado)
  }
  Serial.println("Servo → barrido completo");
}

// ==== RELÉ (PURIFICADOR / VENTILADOR) ====
void encenderVentilador() {
  digitalWrite(RELAY_PIN, LOW); // LOW = encendido (si relé activo en bajo)
  ventiladorEncendido = true;   // 🆕 guardamos estado
  Serial.println("Ventilador encendido");
}

void apagarVentilador() {
  digitalWrite(RELAY_PIN, HIGH); // HIGH = apagado
  ventiladorEncendido = false;   // 🆕 guardamos estado
  Serial.println("Ventilador apagado");
}

// 🆕 Nuevo: función general para controlar el ventilador con parámetro
void setVentilador(bool estado) {
  if (estado) {
    encenderVentilador();
  } else {
    apagarVentilador();
  }
}
