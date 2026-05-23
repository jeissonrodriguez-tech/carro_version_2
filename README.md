# 🤖 Carro Robótico Autónomo ESP32 - Versión 2

## 📋 Descripción General

Proyecto avanzado de **carro robótico autónomo** basado en **ESP32** con capacidades de:
- ✅ **Control manual** por interfaz web (navegador)
- ✅ **Modo autónomo inteligente** con 8 estados de máquina
- ✅ **Evitación de obstáculos** con 3 sensores ultrasónicos
- ✅ **Detección de gas** con sensor MQ135 (real o simulado)
- ✅ **Servo para escaneo** y **relé para ventilador/purificador**
- ✅ **Velocidad adaptativa** según distancia y estado
- ✅ **Servidor web asincrónico** servido desde SPIFFS

---

## 🏗️ Estructura del Proyecto

```
parts_car/
├── src/
│   ├── main.cpp              # Inicialización y loop principal
│   ├── webserver_setup.cpp   # Servidor HTTP y rutas
│   ├── modo_autonomo.cpp     # Máquina de 8 estados
│   ├── motores.cpp           # Control PWM de motores
│   ├── sensores.cpp          # Lecturas ultrasónicas y MQ135
│   └── servo_rele.cpp        # Control del servo y relé
├── include/
│   ├── modo_autonomo.h
│   ├── webserver_setup.h
│   ├── motores.h
│   ├── sensores.h
│   └── servo_rele.h
├── data/
│   ├── index.html            # Panel principal
│   └── control.html          # Control manual (joystick/D-pad)
└── platformio.ini            # Configuración PlatformIO
```

---

## ⚙️ Componentes de Hardware

### 🔌 Pines de Control

| Componente | Descripción | Pines GPIO |
|---|---|---|
| **Motor A (Izquierdo)** | IN1, IN2 (dirección) | 25, 26 |
| **Motor B (Derecho)** | IN3, IN4 (dirección) | 32, 33 |
| **PWM Motor A** | ENA (velocidad) | 27 |
| **PWM Motor B** | ENB (velocidad) | 14 |

### 📡 Sensores Ultrasónicos

| Sensor | TRIG | ECHO | Posición |
|---|---|---|---|
| **Frontal** | GPIO 5 | GPIO 18 | Centro delantero |
| **Izquierdo** | GPIO 19 | GPIO 21 | Costado izquierdo |
| **Derecho** | GPIO 22 | GPIO 23 | Costado derecho |

### 🎛️ Otros Periféricos

| Dispositivo | Pin | Función |
|---|---|---|
| **Servo** | GPIO 13 | Control de ángulo (0-180°) |
| **Relé** | GPIO 17 | Ventilador/Purificador (activo en LOW) |
| **MQ135** | GPIO 34 (ADC) | Sensor de gas (aire) |

---

## 🚀 Características Principales

### 1️⃣ **Modo Manual**
- Control por joystick táctil (móviles) o teclado (escritorio)
- D-pad visual para direcciones (arriba, abajo, izquierda, derecha)
- **3 presets de velocidad**: Vel 1 (30%), Vel 2 (60%), Vel 3 (90%)
- Interfaz neón futurista en `control.html`

### 2️⃣ **Modo Autónomo (Máquina de 8 Estados)**

La lógica autónoma implementa una **máquina de estados** sofisticada:

```
┌─ AS_IDLE ──────────────┐
│                        │
├─ AS_NAVIGATE ◄─────────┤  (Estado principal)
│   ├─► AS_ALERT         │  (zona alerta, 30-45cm)
│   └─► AS_CRITICAL      │  (zona crítica, 15-30cm)
│       └─► AS_SCANNING  │  (3 puntos con servo)
│           └─► AS_TURNING ──────┐
│               └─► AS_ESCAPE    │  (Atrapamiento)
│                   └─► AS_BACKING ─→ AS_SCANNING
```

**Distancias de decisión:**
- `> 45cm`: TODO LIBRE (velocidad máxima)
- `30-45cm`: ALERTA (reduce 20%)
- `15-30cm`: CRÍTICA (reduce 50%)
- `< 8cm`: BLOQUEADA (retrocede)

### 3️⃣ **Velocidad Adaptativa**
- **Presets de límite máximo**: 30% (Baja 🐢), 50% (Media 🚗), 80% (Alta 🚀)
- Velocidad actual puede **reducirse por obstáculos** pero NO supera el límite
- Sistema independiente para modo manual y autónomo

### 4️⃣ **Detección de Atrapamiento**
- Memoria de 8 últimos giros
- Si detecta 3+ giros consecutivos al mismo lado → **Escape 180°**
- Evita loops infinitos

### 5️⃣ **Sensor MQ135**
- Dos modos: **Real** (lectura ADC) y **Simulación** (valores aleatorios)
- Configurable desde API `/setRealMode?active=1`

---

## 📡 API REST (Rutas HTTP)

### **Navegación**
| Método | Ruta | Parámetro | Función |
|---|---|---|---|
| GET | `/` | — | Sirve `index.html` |
| GET | `/index.html` | — | Panel principal |
| GET | `/control.html` | — | Control manual |

### **Control Manual**
| GET | `/forward` | `vel` (0-100) | Mover adelante |
| GET | `/backward` | `vel` (0-100) | Mover atrás |
| GET | `/left` | `vel` (0-100) | Girar izquierda |
| GET | `/right` | `vel` (0-100) | Girar derecha |
| GET | `/stop` | — | Detener motores |

### **Modos**
| GET | `/setMode` | `mode=manual\|autonomo` | Cambiar modo |
| GET | `/getMode` | — | Obtener modo actual |

### **Velocidad Manual**
| GET | `/setVelocidad` | `valor` (0-100) | Establecer velocidad |
| GET | `/getVelocidad` | — | Obtener velocidad actual (JSON) |
| GET | `/setSpeedLevel` | `mode`, `level` (1-3) | Aplicar preset |

### **Velocidad Autónoma**
| GET | `/setPresetAutonomo` | `nivel` (1-3) | Establecer límite máximo |
| GET | `/setVelocidadAutonomo` | `valor` (0-100) | Establecer velocidad actual |
| GET | `/getVelocidadesAutonomo` | — | Info de velocidades (JSON) |
| GET | `/quickPreset` | `p` (1-3) | Cambio rápido |

### **Autonomía**
| GET | `/startAutonomo` | — | Iniciar modo autónomo |
| GET | `/stopAutonomo` | — | Detener autonomía |
| GET | `/emergencyStop` | — | Parada de emergencia total |
| GET | `/debugAutono` | — | Info de estado (JSON) |

### **Sensores**
| GET | `/ultrasonic` | — | Distancias 3 sensores (JSON) |
| GET | `/mq135` | — | Lectura gas (JSON) |
| GET | `/setRealMode` | `active` (0\|1) | Modo real/simulación MQ135 |

### **Ventilador**
| GET | `/ventilador` | `state=on\|off` | Encender/apagar ventilador |

### **Control**
| GET | `/toggleControl` | `state=on\|off` | Activar/desactivar control manual |

---

## ✅ Estado Actual del Código

### ✨ **Lo que funciona bien:**

1. ✅ **Arquitetura modular** - Código bien organizado en módulos (motores, sensores, servo)
2. ✅ **Máquina de estados robusta** - Lógica autónoma sofisticada y no bloqueante
3. ✅ **Velocidad adaptativa** - Sistema inteligente de límites y reducción
4. ✅ **Servidor asincrónico** - Usa `AsyncWebServer` (no bloquea con `handleClient()`)
5. ✅ **Escaneo 3-puntos** - Servo para medir izquierda, centro, derecha
6. ✅ **Detección de atrapamiento** - Memoria de giros con escape automático
7. ✅ **Interfaz web moderna** - Diseño neón futurista en `control.html`
8. ✅ **Dos modos MQ135** - Real (ADC) y simulado
9. ✅ **JSON responses** - API retorna datos estructurados
10. ✅ **Inicialización completa** - `setup()` llama todas las inicializaciones

---

## ⚠️ Problemas Detectados

### 🔴 **Críticos**

1. **❌ `sensores.cpp` línea 42: `delay(60)` bloquea el loop**
   - Cada medición tiene dos `delay()`: en `medirDistancia()` y entre mediciones
   - Afecta responsividad del servidor web
   - **Solución**: Usar timers no bloqueantes (millis())

2. **❌ `sensores.cpp` línea 38: `pulseIn()` sin timeout efectivo**
   - Si sensor falla, puede esperar hasta 30ms
   - **Solución**: Usar `pulseInLong()` o interrupciones

### 🟡 **Importantes**

3. **⚠️ `motores.cpp` línea 74-75: `ledcWrite()` con PWM ambiguo**
   - Función `moverAdelante()` asume valores PWM para `vel` si `vel >= 0`
   - Confusión entre porcentaje y PWM en la API
   - **Solución**: Documentar claramente qué espera cada función

4. **⚠️ `sensores.cpp` línea 55-60: `medirPromedio()` toma 3 medidas**
   - Cada llamada son 3×medición = 3×(2+60ms delay) = ~186ms
   - Muy lento para loop principal
   - **Solución**: Usar 1 sola medición o caché

5. **⚠️ `servo_rele.cpp` línea 14: Relé inicializa con `HIGH`**
   - Asume relé activo en bajo (LOW=encendido)
   - Si relé tiene lógica inversa, inicia con ventilador apagado
   - **Solución**: Comentar claramente el tipo de relé

6. **⚠️ No hay validación de limites en `setVelocidad()`**
   - `constrain()` valida, pero se permite 0-100%
   - Si PWM es 0, motores no avanzan (comportamiento esperado)
   - **Solución**: Agregar valor mínimo (ej: 20% si está en movimiento)

### 🟢 **Menores / Sugerencias**

7. 📌 `webserver_setup.cpp` línea 9: Variable `modoReal` sin validación
   - Se importa desde webserver pero se declara en sensores
   - Podría causar confusión
   - **Solución**: Definir en header compartido

8. 📌 `modo_autonomo.cpp` línea 202: Sobreescribe velocidad autónoma
   - `ejecutarModoAutonomo()` recalcula `velocidadAutonomaPorcentaje` cada vez
   - Puede conflictuar con `setVelocidadAutonoma()`
   - **Solución**: Usar velocidad de `calcularVelocidadAdaptativa()` solo para cálculo, no para sobrescribir

9. 📌 `data/control.html` línea 149: Link correcto a `/index.html`
   - ✅ Ya está corregido (no tiene `/data/`)
   - Viejo README reportaba error que YA fue solucionado

10. 📌 No hay manejo de errores SPIFFS
    - Si `SPIFFS.begin()` falla, servidor inicia sin archivos
    - **Solución**: Reintentar o servir JSON fallback

---

## 🔧 Mejoras Recomendadas (Prioridad)

### 🔴 **URGENTE**

```cpp
// PROBLEMA: sensores.cpp usa delay() bloqueante
// SOLUCIÓN: Implementar lectura no bloqueante con timers

unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL = 100;  // ms

void actualizarSensores() {
  unsigned long now = millis();
  if (now - lastSensorRead < SENSOR_INTERVAL) return;
  
  lastSensorRead = now;
  // Leer UN sensor a la vez (rotar)
  medirDistancia(TRIG_FRONT, ECHO_FRONT);
}
```

### 🟠 **MUY IMPORTANTE**

```cpp
// PROBLEMA: motores.cpp confunde porcentaje y PWM
// SOLUCIÓN: Cambiar firma de funciones

// Antes: void moverAdelante(int porcentaje)  // confuso
// Después: 
void moverAdelante(int porcentaje = -1) {
  if (porcentaje >= 0) {
    setVelocidad(porcentaje);  // Siempre porcentaje
  }
  // Usar velocidadPWM actual
  ledcWrite(PWM_CH_A, velocidadPWM);
  ledcWrite(PWM_CH_B, velocidadPWM);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}
```

### 🟡 **RECOMENDADO**

```cpp
// MEJORA: Agregar modo de depuración
#define DEBUG 1

void logDebug(const char* formato, ...) {
  #if DEBUG
    Serial.printf("[%lu] ", millis());
    Serial.printf(formato);
    Serial.println();
  #endif
}

// En lugar de:
Serial.printf("📶 WiFi AP iniciado: %s\n", ssid);

// Usar:
logDebug("📶 WiFi AP iniciado: %s", ssid);
```

---

## 📝 Cambios Aplicados (vs. versión anterior)

| Característica | Antes | Ahora |
|---|---|---|
| Inicialización MQ135 | ❌ No llamado | ✅ `iniciarMQ135()` en `setup()` |
| Velocidad adaptativa | ❌ No existía | ✅ Presets 1-3 con límites |
| Máquina de estados | ❌ Simple (3-4 estados) | ✅ 8 estados completos |
| Detección de atrapamiento | ❌ No | ✅ Escape 180° automático |
| Escaneo servo | ❌ Básico | ✅ 3 puntos (izq, centro, der) |
| Link volver | ❌ `/data/index.html` | ✅ `/index.html` |
| Función duplicada | ❌ `enviarMovimiento()` x2 | ✅ Una sola definición |
| Relé ventilador | ❌ Sin estado | ✅ `ventiladorEncendido` bool |
| API velocidades | ❌ Solo manual | ✅ Manual + autónoma |
| Debug JSON | ❌ No | ✅ `/debugAutono` endpoint |

---

## 🚀 Cómo Usar

### 1️⃣ **Instalación**

```bash
# Instalar PlatformIO (VS Code extension)
cd parts_car
pio run  # Compilar
pio run -t upload  # Subir firmware al ESP32
pio run -t uploadfs  # Subir archivos SPIFFS (HTML/CSS/JS)
```

### 2️⃣ **Conectar**

1. El ESP32 crea red WiFi: **`RobotESP32`** (contraseña: **`12345678`**)
2. Abrir navegador: **`http://192.168.4.1`**
3. Ver monitor serial a 115200 baud para debug

### 3️⃣ **Panel Principal** (`index.html`)

- Visualizar tanque de gas (MQ135)
- Alertas en tiempo real
- Control del ventilador (botones)
- Modo oscuro (toggle)

### 4️⃣ **Control Manual** (`control.html`)

1. Botón **⏻** para encender control
2. **D-pad** o teclado (W/A/S/D) para mover
3. **Joystick táctil** en móviles
4. **3 botones de velocidad** (solo en modo manual)

### 5️⃣ **Modo Autónomo**

1. Ir a `index.html` → Cambiar a "AUTONOMO"
2. Click en "Iniciar autonomía"
3. Robot navega evitando obstáculos
4. Ajustar velocidad con presets 🐢/🚗/🚀
5. Parar con botón "Detener autonomía"

---

## 📊 Información Técnica

### **ESP32 Configuration**

| Parámetro | Valor |
|---|---|
| **Placa** | ESP32 Dev Module |
| **Frecuencia** | 240 MHz |
| **RAM** | 520 KB |
| **Flash** | 4 MB |
| **Puerto Serial** | 115200 baud |
| **Upload Speed** | 921600 baud |

### **Librerías Principales**

```ini
lib_deps =
  madhephaestus/ESP32Servo@^0.7.0
  https://github.com/me-no-dev/ESPAsyncWebServer.git
  https://github.com/me-no-dev/AsyncTCP.git
```

### **SPIFFS**

- Tamaño: Automático (resto de flash)
- Archivos: `index.html`, `control.html`
- Compresión: GZIP en servidor

---

## 🧪 Pruebas Recomendadas

### Manual
```
1. ✅ Conectar a WiFi
2. ✅ Cargar index.html
3. ✅ Encender control (botón ⏻)
4. ✅ Mover en 4 direcciones
5. ✅ Cambiar velocidades
6. ✅ Cambiar a modo autónomo
7. ✅ Iniciar autonomía
8. ✅ Verificar escaneo servo (serial)
9. ✅ Probar ventilador
```

### API
```bash
# Cambiar a autónomo
curl http://192.168.4.1/setMode?mode=autonomo

# Iniciar autonomía
curl http://192.168.4.1/startAutonomo

# Ver velocidad
curl http://192.168.4.1/getVelocidadesAutonomo

# Debug
curl http://192.168.4.1/debugAutono

# Sensores
curl http://192.168.4.1/ultrasonic
```

---

## 📈 Rendimiento Estimado

| Métrica | Valor | Notas |
|---|---|---|
| **FPS Loop** | ~5-10 fps | Limitado por sensores |
| **Latencia HTTP** | ~50-100 ms | Servidor asincrónico |
| **Rango ultrasónico** | 2-400 cm | Teórico, 45cm práctico |
| **Servo** | 0-180° | ~100-140ms por movimiento |
| **MQ135** | 0-1023 ADC | Real o simulado |
| **Batería** | ~2-4 hrs | Depende del motor y carga |

---

## 🐛 Próximos Pasos / TO-DO

- [ ] Implementar lectura de sensores no bloqueante
- [ ] Agregar PID para corrección de trayectoria
- [ ] Logging a SD card (opcional)
- [ ] OTA (Over The Air) updates
- [ ] Múltiples robots con comunicación
- [ ] Mapa 2D del entorno
- [ ] Calibración automática de sensores
- [ ] Pruebas de carga en batería
- [ ] Carcasa 3D impresa

---

## 📞 Soporte y Contacto

**Desarrollador:** jeissonrodriguez-tech
**Repositorio:** https://github.com/jeissonrodriguez-tech/carro_version_2
**Issues:** Reportar en GitHub

---

## 📄 Licencia

Este proyecto es de código abierto. Úsalo libremente, pero menciona la fuente.

---

**Última actualización:** Mayo 2026
**Versión de firmware:** 2.0.1
**Estado:** ✅ FUNCIONAL Y ESTABLE
