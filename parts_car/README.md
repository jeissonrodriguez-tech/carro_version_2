# Carro Robótico con ESP32

Este proyecto es un carro robótico basado en ESP32 que puede operar en modo manual y modo autónomo. Utiliza un servidor web embebido para controlar el vehículo desde un navegador y leer sensores ultrasónicos y un sensor de gas MQ135.

## Funcionalidad principal

- Control manual por web mediante botones y joystick.
- Modo autónomo con lógica de evitación de obstáculos y giro automático.
- Lectura de tres sensores ultrasónicos: frontal, izquierdo y derecho.
- Lectura de sensor MQ135 (modo real o simulado).
- Control de un servo para escaneo y un relé para encendido de ventilador/purificador.
- Interfaz web servida desde SPIFFS.

## Componentes del proyecto

- `src/main.cpp` - Inicializa WiFi, SPIFFS, servidor web y hardware.
- `src/webserver_setup.cpp` - Define rutas HTTP para control, modo, sensores y actuadores.
- `src/modo_autonomo.cpp` - Implementa la lógica de conducción autónoma.
- `src/motores.cpp` - Maneja los motores, PWM y comandos de movimiento.
- `src/sensores.cpp` - Lee los sensores ultrasónicos y el MQ135.
- `src/servo_rele.cpp` - Controla el servo y el relé del ventilador.
- `include/` - Contiene los encabezados con las APIs públicas.
- `data/` - Archivos web estáticos servidos desde SPIFFS.

## Conexiones de hardware

### Motor driver / ruedas
- `IN1` (ESP32 GPIO25) -> entrada 1 del controlador H-bridge
- `IN2` (ESP32 GPIO26) -> entrada 2 del controlador H-bridge
- `IN3` (ESP32 GPIO32) -> entrada 3 del controlador H-bridge
- `IN4` (ESP32 GPIO33) -> entrada 4 del controlador H-bridge
- `ENA` (ESP32 GPIO27) -> PWM para motor A
- `ENB` (ESP32 GPIO14) -> PWM para motor B

> Estos pines controlan dirección y velocidad de los dos motores. El controlador H-bridge debe tener alimentación separada para los motores y masa común con el ESP32.

### Sensores ultrasónicos
- `TRIG_FRONT` (ESP32 GPIO5) -> trigger sensor frontal
- `ECHO_FRONT` (ESP32 GPIO18) -> echo sensor frontal
- `TRIG_LEFT` (ESP32 GPIO19) -> trigger sensor izquierdo
- `ECHO_LEFT` (ESP32 GPIO21) -> echo sensor izquierdo
- `TRIG_RIGHT` (ESP32 GPIO22) -> trigger sensor derecho
- `ECHO_RIGHT` (ESP32 GPIO23) -> echo sensor derecho

### Sensor de gas MQ135
- `MQ135_PIN` (ESP32 GPIO34) -> salida analógica MQ135

### Servo y relé
- `SERVO_PIN` (ESP32 GPIO13) -> señal PWM del servo
- `RELAY_PIN` (ESP32 GPIO17) -> control del módulo de relé

> El relé controla el ventilador/purificador. El código asume relé activo en bajo: `LOW = encendido`, `HIGH = apagado`.

## Cómo usar

1. Abrir el proyecto en PlatformIO.
2. Compilar con `pio run`.
3. Subir el firmware al ESP32 con `pio run -t upload`.
4. Subir los archivos SPIFFS con `pio run -t uploadfs`.
5. Conectar un navegador a la red WiFi creada por el ESP32 (`RobotESP32`).
6. Abrir `http://192.168.4.1` o la IP mostrada en el monitor serial.

## Rutas web importantes

- `/control.html` - Interfaz de control manual.
- `/ultrasonic` - Lectura de sensores de distancia.
- `/mq135` - Lectura de MQ135.
- `/setMode?mode=manual` - Cambiar a modo manual.
- `/setMode?mode=autonomo` - Cambiar a modo autónomo.
- `/startAutonomo` - Iniciar autonomía.
- `/stopAutonomo` - Detener autonomía.
- `/toggleControl?state=on|off` - Activar/desactivar control manual.
- `/ventilador?state=on|off` - Encender/apagar ventilador.

## Posibles errores detectados

1. `data/control.html` contiene dos definiciones de la función `enviarMovimiento(...)`. Esto puede generar comportamientos inesperados en el navegador.
2. En `data/control.html`, el enlace de regreso usa `/data/index.html`, pero el servidor SPIFFS sirve archivos desde la raíz. El enlace correcto probablemente debe ser `/index.html`.
3. En `src/modo_autonomo.cpp`, dentro del caso `AS_FORWARD`, la línea `moverAdelante(velocidadPWM);` usa un valor PWM en lugar de un porcentaje. La función `moverAdelante()` espera porcentaje y volverá a convertirlo, lo que puede provocar velocidad máxima no deseada.
4. `sensores.cpp` usa `pulseIn()` y `delay()` para medir distancias, lo cual bloquea el loop; en el ESP32 es funcional, pero puede afectar la responsividad del servidor y la UI.
5. `webserver_setup.cpp` construye muchas cadenas `String` dinámicas. En ESP32 puede funcionar, pero conviene tener cuidado con la fragmentación de heap si se usa intensivamente.
6. En `main.cpp`, las variables `ssid` y `password` se usaban de forma inconsistente y el AP tenía credenciales codificadas.
7. `iniciarMQ135()` no se llamaba en `setup()`, aunque existe la función de inicialización del sensor.

## Cambios aplicados

- `data/control.html`: se eliminó la definición duplicada de `enviarMovimiento()` y se actualizó el enlace de regreso a `/index.html`.
- `src/modo_autonomo.cpp`: se corrigió la llamada a `moverAdelante()` para usar porcentaje de velocidad en lugar de PWM.
- `src/main.cpp`: ahora el AP usa `ssid` y `password` declarados, y se inicializa el MQ135 con `iniciarMQ135()`.

## Recomendaciones

- Corrige `control.html` eliminando la definición duplicada de `enviarMovimiento()`.
- Cambia el enlace de `/data/index.html` a `/index.html`.
- Ajusta `modo_autonomo.cpp` para pasar el porcentaje correcto a `moverAdelante()`.
- Si planeas usar el modo MQ135 real, asegúrate de inicializar `iniciarMQ135()` y de configurar correctamente el pin ADC.
- Considera reducir el uso de `String` en el servidor y privilegiar `char[]` o `send_P()` para evitar problemas de memoria.

## Notas adicionales
        
- El proyecto usa `ESPAsyncWebServer` y `AsyncTCP`, por lo que el loop principal no necesita `server.handleClient()`.
- El modo autónomo está controlado por una variable global `modoAutonomoActivo` y sólo se ejecuta cuando se solicita.
- El sistema presenta tanto modo de simulación como modo real para el sensor MQ135.
