# Angamed Hydroponic Automation System

Sistema IoT de monitoreo y control para un invernadero hidropónico comercial de 20×45 m. Automatiza la lectura de parámetros críticos del proceso (pH, temperatura del agua, y a futuro EC, nivel y caudal) mediante nodos periféricos ESP32, un concentrador local en Docker, y visualización en tiempo real.

Proyecto desarrollado bajo la marca AXL Builder, en colaboración con el equipo de Angamed.

## Arquitectura general

El sistema sigue una topología de tres capas, según el documento técnico de base del proyecto:

```
+--------------------------------------------------------------------------+
|                          CAPA 3 - REMOTA / CLOUD (planificada)            |
|      EMQX Cloud (Broker)  <-->  Thingsboard CE  <-->  App Móvil SCADA     |
+--------------------------------------------------------------------------+
                                    ▲
                                    ▼
+--------------------------------------------------------------------------+
|                          CAPA 2 - GATEWAY LOCAL                          |
|             concentrador-iot/ (Docker Compose: Mosquitto, InfluxDB,      |
|                        Node-RED, Grafana)                                |
+--------------------------------------------------------------------------+
                                    ▲
                                    │ MQTT / WiFi
                                    ▼
+--------------------------------------------------------------------------+
|                          CAPA 1 - CAMPO / NODOS                          |
|                nodo-tanque/ (ESP32-S3, firmware Arduino)                 |
+--------------------------------------------------------------------------+
```

Este repositorio contiene el desarrollo de las Capas 1 y 2. La Capa 3 (SCADA en la nube, alertas multicanal) está contemplada en el documento técnico de base pero todavía no implementada.

## Estado del proyecto

| Componente | Rama | Estado |
|---|---|---|
| Concentrador IoT (Capa 2) | `feature/concentrador-iot` | Completo — stack Docker operativo, recibiendo datos reales y simulados |
| Nodo de tanque (Capa 1) | `feature/nodo-tanque` | En curso — publicación funcional con datos simulados, sensores reales pendientes |

## Estructura del repositorio

```
angamed-hydroponic-automation-system/
├── concentrador-iot/      # Capa 2 — Gateway local (Docker Compose)
│   └── README.md          # Detalle de configuración, stack y troubleshooting
└── nodo-tanque/            # Capa 1 — Firmware ESP32-S3 del nodo de tanque
    └── README.md           # Detalle de arquitectura del firmware
```

Cada carpeta tiene su propio README con la documentación específica de esa capa. Este README general solo cubre cómo se relacionan entre sí y cómo levantar el sistema completo de punta a punta.

## Levantar el sistema completo (desarrollo local)

### 1. Concentrador

```bash
cd concentrador-iot
bash init-secrets.sh
docker compose up -d
```

Ver [`concentrador-iot/README.md`](concentrador-iot/README.md) para la configuración de Node-RED y el detalle de cada servicio.

### 2. Nodo de tanque

Con hardware ESP32-S3 disponible:

```bash
cd nodo-tanque
cp src/secrets.h.example src/secrets.h
# completar SSID, password y MQTT_SERVER en secrets.h
pio run -t upload -t monitor
```

Ver [`nodo-tanque/README.md`](nodo-tanque/README.md) para el detalle del firmware, watchdog y sincronización NTP.

### Sin hardware disponible

El concentrador incluye un simulador Python (`concentrador-iot/simulator/envio-datos-mqtt.py`) que publica telemetría con el mismo schema que el nodo real, permitiendo desarrollar y probar el resto del stack sin necesidad del ESP32 físico.

## Schema de datos compartido

Ambas ramas comparten la misma convención de tópicos y payload MQTT, punto de acoplamiento central entre el firmware y el concentrador:

**Tópico:**
```
angamed/<device_id>/datos
```

**Payload (JSON):**
```json
{
  "ts": 1785429706,
  "pH": 6.65,
  "temp_agua": 26.88,
  "rssi": -27
}
```

El `device_id` viaja en el tópico, no en el payload. Cualquier cambio a este schema debe reflejarse simultáneamente en tres lugares: el firmware del nodo (`nodo-tanque/src/main.cpp`), el simulador (`concentrador-iot/simulator/envio-datos-mqtt.py`) y la función de transformación en Node-RED (`concentrador-iot/nodered/flows.json`).

## Hoja de ruta

Según el plan de trabajo del documento técnico de base:

- **Fase 1 — Infraestructura núcleo y red:** completa en desarrollo local (Docker Compose). Pendiente: VLAN IoT dedicada y hardening de red para el despliegue en campo.
- **Fase 2 — Nodo piloto de tanque:** en curso. Firmware base, watchdog y NTP resueltos. Pendiente: sensores reales, logs remotos por MQTT, credenciales en NVS.
- **Fase 3 — Escalado industrial:** no iniciada. Nodos adicionales (Tanque 2, Tanque 3, Maternidad, Caudal), Plug & Play, alertas Telegram.
- **Fase 4 — Producción y OTA:** no iniciada. Actualizaciones remotas de firmware, monitoreo de deriva de sensores en operación real.

## Documentación técnica de base

El diseño de arquitectura, selección de hardware, protocolos de calibración y matriz de riesgos fueron especificados en un documento técnico previo al desarrollo, elaborado en conjunto con el liderazgo del proyecto. Ese documento es la referencia de diseño contra la cual se evalúan las decisiones tomadas en cada rama; las diferencias entre lo especificado y lo implementado en esta etapa (por ejemplo, ausencia de TLS o de aisladores galvánicos I2C mientras se trabaja con sensores simulados) están anotadas en los pendientes de cada README de rama.