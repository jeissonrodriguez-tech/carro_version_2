#include "modo_autonomo.h"
#include "motores.h"
#include "sensores.h"
#include "webserver_setup.h"
#include "servo_rele.h"
#include <Arduino.h>

bool modoAutonomoActivo = false;

// ========== PARÁMETROS OPTIMIZADOS PARA CARRO 27×17cm ==========

// --- Distancias de decisión ---
const int DISTANCIA_SEGURA = 45;      // > 45cm: TODO libre
const int DISTANCIA_ALERTA = 30;      // 30-45cm: Reducir velocidad
const int DISTANCIA_CRITICA = 15;     // 15-30cm: Crítico
const int DISTANCIA_BLOQUEADA = 8;    // < 8cm: Completamente bloqueado

// --- Tiempos de acción ---
const int RETARDO_GIRO = 350;         // ms (optimizado para 27cm)
const int RETARDO_DETECCION = 100;    // ms (intervalo entre bucles)
const int BACKUP_TIME = 400;          // ms cuando retrocede
const int ESCANEO_DELAY = 120;        // ms espera tras mover servo

// --- PRESETS DE VELOCIDAD AUTÓNOMA (LÍMITES MÁXIMOS) ---
static const int PRESET_VELOCIDADES[3] = {30, 50, 80};  // Baja, Media, Alta
static int presetActualAutonomo = 2;  // Por defecto: MEDIA (50%)
static int velocidadMaximaAutonoma = PRESET_VELOCIDADES[1];  // 50%

// --- Variable de velocidad dinámica adaptativa ---
int velocidadAutonomaPorcentaje = 50;

// --- Estado de la máquina ---
enum AutonoState { 
  AS_IDLE=0, 
  AS_NAVIGATE, 
  AS_ALERT, 
  AS_CRITICAL, 
  AS_SCANNING, 
  AS_TURNING, 
  AS_BACKING, 
  AS_ESCAPE 
};
static AutonoState autEstado = AS_IDLE;
static unsigned long estadoMillis = 0;
static unsigned long accionMillis = 0;

// Telemetría
static long lastFront = 999;
static long lastLeft = 999;
static long lastCenter = 999;
static long lastRight = 999;
static String lastAction = "idle";
static String lastDecision = "INIT";

// Escaneo no bloqueante (3 puntos: izq, centro, der)
enum ScanPhase { SP_INIT = 0, SP_SCAN_LEFT, SP_WAIT_LEFT, SP_SCAN_CENTER, 
                 SP_WAIT_CENTER, SP_SCAN_RIGHT, SP_WAIT_RIGHT, SP_DONE };
static ScanPhase scanPhase = SP_INIT;
static unsigned long scanMillis = 0;
static long scanLeftVal = 999;
static long scanCenterVal = 999;
static long scanRightVal = 999;

// Detección de atrapamiento (memoria de giros)
static int memoriaGiros[8] = {0};
static int indiceMemoria = 0;
static int giroConsecutivos = 0;
static int ultimoGiro = 0;  // 0=izq, 1=der, 2=recto

const int TIMEOUT_ESCANEO = 3000;  // 3 segundos
static unsigned long scanStartTime = 0;

// ========== FUNCIONES DE VELOCIDAD ADAPTATIVA ==========

// --- Establecer preset de velocidad máxima (límite que NO puede superar) ---
void setPresetVelocidadAutonoma(int preset) {
  if (preset < 1) preset = 1;
  if (preset > 3) preset = 3;
  presetActualAutonomo = preset - 1;
  velocidadMaximaAutonoma = PRESET_VELOCIDADES[presetActualAutonomo];
  
  // Limitar la velocidad actual si está por encima del nuevo máximo
  if (velocidadAutonomaPorcentaje > velocidadMaximaAutonoma) {
    velocidadAutonomaPorcentaje = velocidadMaximaAutonoma;
  }
  
  const char* nombres[] = {"🐢 BAJA", "🚗 MEDIA", "🚀 ALTA"};
  Serial.printf("⚙️ Preset velocidad autónoma: %s (%d%% máximo)\n", 
    nombres[presetActualAutonomo], velocidadMaximaAutonoma);
}

// --- Establecer velocidad actual (respetando el límite del preset) ---
void setVelocidadAutonoma(int porcentaje) {
  porcentaje = constrain(porcentaje, 0, 100);
  
  // NO PUEDE SUPERAR el límite máximo del preset
  if (porcentaje > velocidadMaximaAutonoma) {
    porcentaje = velocidadMaximaAutonoma;
  }
  
  velocidadAutonomaPorcentaje = porcentaje;
  Serial.printf("⚙️ Velocidad autónoma: %d%% (Máximo: %d%%)\n", 
    velocidadAutonomaPorcentaje, velocidadMaximaAutonoma);
}

// --- Obtener velocidad actual ---
int getVelocidadAutonoma() {
  return velocidadAutonomaPorcentaje;
}

// --- Obtener velocidad máxima del preset actual ---
int getVelocidadMaximaAutonoma() {
  return velocidadMaximaAutonoma;
}

// ========== FUNCIONES DE DECISIÓN INTELIGENTE ==========

// --- Calcular velocidad adaptativa según obstáculos ---
int calcularVelocidadAdaptativa(long distFront) {
  int velocidadBase = velocidadMaximaAutonoma;  // Respetar límite del preset
  
  if (distFront >= DISTANCIA_SEGURA) {
    // TODO LIBRE: velocidad máxima
    return velocidadBase;
  }
  else if (distFront >= DISTANCIA_ALERTA) {
    // ZONA ALERTA: reduce 20%
    int reducida = (velocidadBase * 80) / 100;
    return max(15, reducida);
  }
  else if (distFront >= DISTANCIA_CRITICA) {
    // ZONA CRÍTICA: reduce 50%
    int reducida = (velocidadBase * 50) / 100;
    return max(15, reducida);
  }
  else {
    // ZONA PELIGROSA: reduce 70%
    int reducida = (velocidadBase * 30) / 100;
    return max(10, reducida);
  }
}

// --- Calcular espacio disponible (0-100%) ---
int calcularEspacioDisponible(long distancia) {
  if (distancia >= DISTANCIA_SEGURA) return 100;      // MUY LIBRE
  if (distancia >= DISTANCIA_ALERTA) return 75;       // LIBRE
  if (distancia >= DISTANCIA_CRITICA) return 50;      // ESTRECHO
  if (distancia >= DISTANCIA_BLOQUEADA) return 25;    // MUY ESTRECHO
  return 0;                                            // BLOQUEADO
}

// --- Detectar atrapamiento (3+ giros consecutivos al mismo lado) ---
bool estaAtrapado() {
  int conteo[2] = {0, 0};  // izq, der
  
  for (int i = 0; i < 8; i++) {
    if (memoriaGiros[i] == 0) conteo[0]++;      // izquierda
    else if (memoriaGiros[i] == 1) conteo[1]++;  // derecha
  }
  
  // Si hay 3+ giros al mismo lado = atrapado
  return (conteo[0] >= 3 || conteo[1] >= 3);
}

// --- Guardar giro en memoria ---
void guardarGiro(int direccion) {
  memoriaGiros[indiceMemoria] = direccion;
  indiceMemoria = (indiceMemoria + 1) % 8;
}

// --- Debug JSON del modo autónomo ---
String getAutonomoDebug() {
  char json[512];
  snprintf(json, sizeof(json),
    "{\"activo\":%d,\"estado\":%d,\"front\":%ld,\"left\":%ld,\"center\":%ld,\"right\":%ld,"
    "\"velocidad_actual\":%d,\"velocidad_maxima\":%d,\"preset\":%d,\"accion\":\"%s\","
    "\"decision\":\"%s\",\"espacio_disponible\":%d}",
    modoAutonomoActivo ? 1 : 0,
    (int)autEstado,
    lastFront, lastLeft, lastCenter, lastRight,
    velocidadAutonomaPorcentaje,
    velocidadMaximaAutonoma,
    presetActualAutonomo + 1,
    lastAction.c_str(),
    lastDecision.c_str(),
    calcularEspacioDisponible(lastFront)
  );
  return String(json);
}

// ========== MÁQUINA DE ESTADOS PRINCIPAL ==========

void ejecutarModoAutonomo(int modoActual) {
  if (modoActual != MODE_AUTO || !modoAutonomoActivo) return;

  long distFront = medirDistanciaFront();
  lastFront = distFront;
  unsigned long now = millis();

  // Calcular velocidad adaptativa
  int velocidadActual = calcularVelocidadAdaptativa(distFront);
  velocidadAutonomaPorcentaje = velocidadActual;

  switch (autEstado) {
    // ===== ESTADO: IDLE (Inicial) =====
    case AS_IDLE:
      moverServoCentro();
      moverAdelante(velocidadActual);
      lastAction = "start_forward";
      lastDecision = "INICIAR";
      autEstado = AS_NAVIGATE;
      estadoMillis = now;
      break;

    // ===== ESTADO: NAVIGATE (Principal - Navega libremente) =====
    case AS_NAVIGATE:
      if (distFront > DISTANCIA_SEGURA) {
        // TODO LIBRE
        moverAdelante(velocidadActual);
        lastAction = "navigate_free";
        lastDecision = "TODO_LIBRE";
      }
      else if (distFront > DISTANCIA_ALERTA) {
        // Zona de alerta
        moverAdelante(velocidadActual);
        lastAction = "navigate_alert";
        lastDecision = "ALERTA_REDUCIR";
        autEstado = AS_ALERT;
      }
      else if (distFront > DISTANCIA_CRITICA) {
        // Zona crítica
        autEstado = AS_CRITICAL;
      }
      else {
        // Obstáculo detectado
        detenerMotores();
        lastAction = "obstacle_detected";
        lastDecision = "OBSTÁCULO";
        autEstado = AS_SCANNING;
        estadoMillis = now;
        scanPhase = SP_INIT;
        scanStartTime = now;
      }
      break;

    // ===== ESTADO: ALERT (Reducir velocidad) =====
    case AS_ALERT:
      if (distFront > DISTANCIA_ALERTA) {
        // Volver a navegar normal
        autEstado = AS_NAVIGATE;
      }
      else if (distFront <= DISTANCIA_CRITICA) {
        // Pasar a crítico
        autEstado = AS_CRITICAL;
      }
      else {
        moverAdelante(velocidadActual);
        lastAction = "alert_slow";
        lastDecision = "ALERTA";
      }
      break;

    // ===== ESTADO: CRITICAL (Zona peligrosa) =====
    case AS_CRITICAL:
      if (distFront > DISTANCIA_ALERTA) {
        // Volver a alerta o navegar
        autEstado = (distFront > DISTANCIA_SEGURA) ? AS_NAVIGATE : AS_ALERT;
      }
      else if (distFront > DISTANCIA_CRITICA) {
        // Seguir lentamente
        moverAdelante(velocidadActual);
        lastAction = "critical_slow";
        lastDecision = "CRÍTICA";
      }
      else {
        // Pasar a escaneo
        detenerMotores();
        autEstado = AS_SCANNING;
        estadoMillis = now;
        scanPhase = SP_INIT;
        scanStartTime = now;
      }
      break;

    // ===== ESTADO: SCANNING (Escaneo 3 puntos con servo) =====
    case AS_SCANNING: {
      // Timeout de seguridad
      if (now - scanStartTime > TIMEOUT_ESCANEO) {
        Serial.println("⚠️ TIMEOUT escaneo - forzar retroceso");
        detenerMotores();
        moverAtras(60);
        autEstado = AS_BACKING;
        accionMillis = now;
        lastDecision = "TIMEOUT_ESCAPE";
        scanPhase = SP_INIT;
        break;
      }

      switch (scanPhase) {
        case SP_INIT:
          detenerMotores();
          moverServoIzquierda();  // 45°
          scanMillis = now;
          scanPhase = SP_WAIT_LEFT;
          lastAction = "scan_left";
          break;

        case SP_WAIT_LEFT:
          if (now - scanMillis >= ESCANEO_DELAY) {
            scanLeftVal = medirDistanciaFront();
            lastLeft = scanLeftVal;
            moverServoCentro();  // 90°
            scanMillis = now;
            scanPhase = SP_WAIT_CENTER;
            lastAction = "scan_center";
          }
          break;

        case SP_WAIT_CENTER:
          if (now - scanMillis >= ESCANEO_DELAY) {
            scanCenterVal = medirDistanciaFront();
            lastCenter = scanCenterVal;
            moverServoDerecha();  // 135°
            scanMillis = now;
            scanPhase = SP_WAIT_RIGHT;
            lastAction = "scan_right";
          }
          break;

        case SP_WAIT_RIGHT:
          if (now - scanMillis >= ESCANEO_DELAY) {
            scanRightVal = medirDistanciaFront();
            lastRight = scanRightVal;
            moverServoCentro();  // Volver al centro
            
            // Analizar resultados
            Serial.printf("📊 ESCANEO: L=%ld | C=%ld | R=%ld\n", 
              scanLeftVal, scanCenterVal, scanRightVal);

            // Decisión inteligente
            if (scanLeftVal < DISTANCIA_BLOQUEADA && 
                scanCenterVal < DISTANCIA_BLOQUEADA && 
                scanRightVal < DISTANCIA_BLOQUEADA) {
              // TODO BLOQUEADO
              lastAction = "all_blocked";
              lastDecision = "BLOQUEADO_RETROCEDER";
              moverAtras(70);
              autEstado = AS_BACKING;
              accionMillis = now;
              guardarGiro(2);  // Centro bloqueado
            }
            else if (scanCenterVal >= DISTANCIA_CRITICA) {
              // Centro libre - continuar recto
              lastAction = "center_free";
              lastDecision = "CENTRO_LIBRE";
              moverAdelante(velocidadActual);
              autEstado = AS_NAVIGATE;
            }
            else if (scanLeftVal > scanRightVal + 15) {
              // Izquierda significativamente más libre
              lastAction = "turn_left";
              lastDecision = "GIRAR_IZQ";
              girarIzquierda(max(30, velocidadActual));
              guardarGiro(0);
              autEstado = AS_TURNING;
              accionMillis = now;
            }
            else if (scanRightVal > scanLeftVal + 15) {
              // Derecha significativamente más libre
              lastAction = "turn_right";
              lastDecision = "GIRAR_DER";
              girarDerecha(max(30, velocidadActual));
              guardarGiro(1);
              autEstado = AS_TURNING;
              accionMillis = now;
            }
            else {
              // Similar en ambos lados, elegir derecha por defecto
              lastAction = "turn_right_default";
              lastDecision = "GIRAR_DER_DEFAULT";
              girarDerecha(max(30, velocidadActual));
              guardarGiro(1);
              autEstado = AS_TURNING;
              accionMillis = now;
            }

            scanPhase = SP_INIT;
          }
          break;

        default:
          scanPhase = SP_INIT;
          break;
      }
    }
    break;

    // ===== ESTADO: TURNING (Girando) =====
    case AS_TURNING:
      if (now - accionMillis >= RETARDO_GIRO) {
        detenerMotores();
        
        // Verificar si está atrapado
        if (estaAtrapado()) {
          Serial.println("🆘 ATRAPADO! Intentando escape 180°");
          lastAction = "escape_180";
          lastDecision = "ESCAPE_ATRAPAMIENTO";
          autEstado = AS_ESCAPE;
          accionMillis = now;
        } else {
          moverAdelante(velocidadActual);
          lastAction = "forward_after_turn";
          lastDecision = "NAVEGANDO";
          autEstado = AS_NAVIGATE;
          estadoMillis = now;
        }
      }
      break;

    // ===== ESTADO: BACKING (Retrocediendo) =====
    case AS_BACKING:
      if (now - accionMillis >= BACKUP_TIME) {
        detenerMotores();
        lastAction = "backup_complete";
        lastDecision = "ESCANEO_NUEVO";
        autEstado = AS_SCANNING;
        estadoMillis = now;
        scanPhase = SP_INIT;
        scanStartTime = now;
      }
      break;

    // ===== ESTADO: ESCAPE (Giro 180° para escapar atrapamiento) =====
    case AS_ESCAPE:
      if (now - accionMillis >= (RETARDO_GIRO * 2)) {  // Giro doble
        detenerMotores();
        Serial.println("✅ Escape 180° completado");
        lastAction = "escape_complete";
        lastDecision = "REINICIAR";
        moverAdelante(velocidadActual);
        autEstado = AS_NAVIGATE;
        estadoMillis = now;
        indiceMemoria = 0;  // Limpiar memoria de giros
      }
      break;
  }
}

// ========== CONTROL DE AUTONOMÍA ==========

void iniciarModoAutonomo() {
  detenerMotores();
  modoAutonomoActivo = true;
  moverServoCentro();
  autEstado = AS_IDLE;
  lastAction = "started";
  lastDecision = "INIT";
  
  // Inicializar memoria de giros
  memset(memoriaGiros, 0, sizeof(memoriaGiros));
  indiceMemoria = 0;
  giroConsecutivos = 0;
  
  Serial.println("🤖 Modo autónomo activado (máquina de 8 estados)");
  Serial.printf("📊 Velocidad máxima: %d%% (Preset %d)\n", 
    velocidadMaximaAutonoma, presetActualAutonomo + 1);
}

void detenerModoAutonomo() {
  modoAutonomoActivo = false;
  detenerMotores();
  moverServoCentro();
  autEstado = AS_IDLE;
  lastAction = "stopped";
  lastDecision = "STOP";
  Serial.println("🛑 Modo autónomo detenido");
}

bool estaAutonomoActivo() {
  return modoAutonomoActivo;
}
