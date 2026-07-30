# Nodo Tanque — Angamed Hydroponic Automation System

Firmware del nodo periférico de tanque para el sistema de automatización hidropónica Angamed. Corre sobre un ESP32-S3-N16R8 y publica telemetría (pH, temperatura del agua, RSSI) vía MQTT hacia el concentrador local definido en la rama `feature/concentrador-iot`.

Este nodo corresponde a la Capa 1 (Campo) de la arquitectura de tres capas descrita en el documento técnico de base del proyecto.

## Estado actual

Fase 2 en curso. Publicación funcional con datos simulados de pH y temperatura. Watchdog timer implementado y validado. Sincronización horaria vía NTP implementada. Sensores reales (pH DFRobot, EC, DS18B20, nivel ultrasónico) pendientes de integración de hardware.

## Hardware

- **Placa:** ESP32-S3-N16R8 (16 MB flash, 8 MB PSRAM)
- **Entorno de desarrollo:** PlatformIO CLI sobre WSL2 (Ubuntu), framework Arduino
- **Sensores reales del proyecto** (pendientes de integración): pH DFRobot SEN0161-V2, EC DFRobot SEN0244, nivel JSN-SR04T, temperatura DS18B20

## Estructura del repositorio

```
nodo-tanque/
├── include/
│   └── sensores.h        # Declaraciones del módulo de sensores
├── src/
│   ├── main.cpp           # Lógica principal: WiFi, MQTT, NTP, watchdog, publicación
│   ├── sensores.cpp       # Implementación de lectura de sensores (simulada por ahora)
│   ├── secrets.h          # Credenciales locales — NO se sube a git
│   └── secrets.h.example  # Plantilla de secrets.h
└── platformio.ini
```

## Configuración inicial

1. Cloná el repositorio y entrá a la carpeta `nodo-tanque/`.
2. Copiá `src/secrets.h.example` a `src/secrets.h` y completá tus credenciales:

```cpp
#pragma once

const char* WIFI_SSID     = "tu-red";
const char* WIFI_PASSWORD = "tu-password";
const char* MQTT_SERVER   = "ip-del-concentrador";
const int   MQTT_PORT     = 1883;
```

`secrets.h` está en `.gitignore` y nunca debe subirse al repositorio.

3. Compilá y subí el firmware:

```bash
pio run -t upload -t monitor
```

## Arquitectura del firmware

### Flujo de datos

```
ESP32 (nodo-tanque) → MQTT (Mosquitto) → Node-RED → InfluxDB 3 Core → Grafana
```

### Identificación del nodo

Cada nodo genera su Client ID MQTT dinámicamente a partir de su dirección MAC (`WiFi.macAddress()`), garantizando unicidad sin configuración manual:

```
clientId    = "NodoTanque-<MAC>"
topicDatos  = "angamed/<clientId>/datos"
```

### Formato del payload

El `device_id` viaja en el tópico, no en el payload. El JSON publicado sigue este schema:

```json
{
  "ts": 1785429706,
  "pH": 6.65,
  "temp_agua": 26.88,
  "rssi": -27
}
```

Este mismo schema es utilizado por el simulador Python del concentrador (`envio-datos-mqtt.py`) y por la función de transformación en Node-RED, manteniendo consistencia end-to-end.

### Watchdog Timer (WDT)

El firmware usa la API del Task Watchdog Timer expuesta por el core `framework-arduinoespressif32` instalado (`esp_task_wdt_init(timeout, panic)`, con timeout en segundos), y no la API basada en struct de configuración que documentan algunas versiones más recientes de ESP-IDF — la disponibilidad de una u otra depende de la versión específica del core instalada por PlatformIO y conviene verificarla contra el header local antes de asumir la firma.

Configuración actual:

- **Timeout:** 30 segundos, alineado con la especificación Fail-Safe del documento técnico de base.
- **Panic:** activado (`true`). Si el firmware queda bloqueado más de 30 segundos sin alimentar al watchdog, el chip se reinicia físicamente.

El watchdog se alimenta (`esp_task_wdt_reset()`) en tres puntos:

- En cada iteración del `loop()` principal.
- Dentro del bucle de espera de conexión WiFi en `setup()`.
- Dentro del bucle de reintento de conexión MQTT (`connectMQTT()`).

Esto asegura que las esperas controladas de reconexión (WiFi caído, broker caído) no disparen el watchdog — el disparo queda reservado para bloqueos genuinos e imprevistos del firmware. Validado mediante pruebas de corte de red: el nodo reconecta solo, sin intervención manual, una vez restablecida la conectividad.

### Sincronización horaria (NTP)

El `setup()` sincroniza la hora contra `pool.ntp.org` (offset fijo UTC-3, sin horario de verano) antes de iniciar el ciclo normal de publicación. Como `configTime()` es asincrónica, se usa un bucle de espera acotado (10 segundos, con reintentos internos de 1 segundo vía `getLocalTime()`) para confirmar la sincronización antes de continuar.

Si la sincronización no se logra dentro del timeout, el firmware registra una advertencia por Serial y continúa igual, en vez de reiniciar la placa — a diferencia del timeout de WiFi, no se considera una condición fatal.

### Publicación de telemetría

Cada 10 segundos, de forma no bloqueante (vía `millis()`, sin `delay()` en el ciclo principal), el nodo arma el JSON con ArduinoJson v7 y lo publica en su tópico. Se optó por ArduinoJson en lugar de concatenación manual de `String` para evitar la fragmentación de memoria heap que genera la concatenación repetida a lo largo del tiempo de operación continua del dispositivo.

## Convenciones de tópicos MQTT

| Tópico | Dirección | Contenido |
|---|---|---|
| `angamed/<device_id>/datos` | Nodo → Gateway | Telemetría periódica (JSON) |

## Pendientes

- [ ] Logs remotos por MQTT (`logMQTT()`), publicando en `angamed/<device_id>/log` además de por Serial.
- [ ] Integración de sensores reales, comenzando por DS18B20 (temperatura), luego pH DFRobot cuando esté disponible el hardware.
- [ ] Migración de credenciales WiFi/MQTT desde `secrets.h` a almacenamiento cifrado NVS para el build de producción.
- [ ] Validación explícita del comportamiento del watchdog ante un bloqueo real no controlado (no solo ante pérdida de red, que ya está cubierta por la lógica de reintento).

## Referencias

Este firmware implementa parcialmente la especificación del documento técnico de arquitectura del proyecto Angamed (nodos ESP32-S3 para tanques, protocolo MQTT, lógica Fail-Safe con watchdog de 30s). Las simplificaciones actuales respecto al documento (sensores simulados, ausencia de TLS y VLAN en esta etapa de desarrollo local) corresponden al alcance de la Fase 2 y se resuelven en fases posteriores del plan de trabajo.