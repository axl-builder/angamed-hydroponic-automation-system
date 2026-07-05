# Angamed Hydroponic Automation — Concentrador IoT

Stack de concentrador local para el sistema de automatización del invernadero hidropónico Angamed. Recibe datos de sensores desde nodos ESP32 vía MQTT, los almacena en InfluxDB 3 Core y los visualiza en Grafana en tiempo real.

---

## Demo

![Video demostrativo](assets/demo.gif)

---

## Arquitectura (en producción)

![Arquitectura del sistema](assets/screenshot.png)

Todos los servicios corren como contenedores Docker en un único host (Raspberry Pi 4 en producción, cualquier máquina Linux para desarrollo).

---

## Stack

| Servicio | Imagen | Puerto | Función |
|---|---|---|---|
| Mosquitto | `eclipse-mosquitto:latest` | 1883 | Broker MQTT |
| InfluxDB 3 Core | `influxdb:3-core` | 8181 | Base de datos de series temporales |
| InfluxDB Explorer | `influxdata/influxdb3-ui:1.9.0` | 8080 | UI de la base de datos |
| Node-RED | `nodered/node-red:5.0-debian` | 1880 | Motor de flujos — bridge MQTT → InfluxDB |
| Grafana | `grafana/grafana:13.0.3` | 3000 | Dashboard |

---

## Requisitos

- Docker y Docker Compose
- `openssl` disponible en el shell
- Linux o macOS (WSL2 en Windows también funciona — ver nota más abajo)

---

## Configuración inicial

### 1. Clonar el repositorio

```bash
git clone <repo-url>
cd angamed-hydroponic-automation-system/concentrador-iot
```

### 2. Ejecutar el script de inicialización

```bash
chmod +x init-secrets.sh
bash init-secrets.sh
```

Este script hace lo siguiente de forma automática:

- Genera `INFLUX_ADMIN_TOKEN` y `SESSION_SECRET_KEY` en `.env` si no existen
- Genera `NODERED_CREDENTIAL_SECRET` en `.env` si no existe
- Crea `secrets/influxdb-key.json` con el token de admin para InfluxDB Core
- Crea `config/config.json` con la conexión del Explorer preconfigurada
- Crea todos los directorios de datos bajo `data/` con los permisos correctos por UID de contenedor
- Copia `nodered/settings.js` y `nodered/flows.json` a `data/node-red/`

> Si los directorios de datos ya existen, el script preguntará si querés borrarlos para una inicialización limpia.

### 3. Levantar el stack

```bash
docker compose up -d
```

### 4. Verificar los servicios

| Servicio | URL | Credenciales |
|---|---|---|
| InfluxDB Explorer | http://localhost:8080 | Token del `.env` |
| Node-RED | http://localhost:1880 | Sin credenciales |
| Grafana | http://localhost:3000 | admin / admin |

### 5. Configuración de Node-RED

5. Configuración de Node-RED
Una vez que los servicios estén levantados, es necesario instalar el nodo de integración con InfluxDB dentro de Node-RED. Para hacerlo, sigue estos pasos desde la interfaz web de Node-RED:

Haz clic en el menú principal (el ícono de las tres líneas horizontales en la esquina superior derecha).

Selecciona la opción Manage palette.

En la ventana de configuración que se abre, dirígete a la pestaña Install.

En la barra de búsqueda, escribe influxdb.

Busca en la lista de resultados el paquete llamado exactamente node-red-contrib-influxdb3 (Node-RED nodes for InfluxDB v3 integration).

Haz clic en el botón Install de ese paquete y espera a que finalice el proceso.

Una vez instalado el plugin, debes configurar la conexión del nodo de InfluxDB en tu flujo para que pueda comunicarse con la base de datos. Sigue estos pasos:

Haz doble clic sobre el nodo de InfluxDB en tu flujo para abrir el panel de propiedades lateral.

Junto al campo del servidor, haz clic en el ícono de edición (el lápiz) para añadir o modificar la conexión.

En la configuración del servidor, completa los siguientes campos clave:

Version: Selecciona 2.0 (o la versión requerida por el nodo).

URL: Ingresa http://influxdb3-core:8181 (este es el hostname y puerto interno del contenedor de InfluxDB en la red de Docker).

Token: Pega aquí el valor exacto de la variable INFLUX_ADMIN_TOKEN que se generó automáticamente en tu archivo .env.

Haz clic en el botón Update para guardar la configuración del servidor.

Finalmente, presiona el botón rojo Deploy en la esquina superior derecha de Node-RED para aplicar y activar los cambios.

---

## Nota para WSL2

Si corrés el stack en WSL2 con Docker Desktop, `localhost` dentro de WSL2 no resuelve a los puertos de los contenedores Docker. Usá la IP del gateway de WSL2 en su lugar:

```bash
export MQTT_BROKER=$(grep nameserver /etc/resolv.conf | awk '{print $2}')
```

Agregá esta línea a tu perfil de shell (`.bashrc` o `.zshrc`) para que persista entre sesiones. Los scripts de simulación leen `MQTT_BROKER` del entorno con `localhost` como fallback.

---

## Simulación de datos de sensores

Se incluye un script Python publisher para desarrollo y pruebas. Simula lecturas de sensores ESP32 y las publica al broker cada 3 segundos.

```bash
cd simulator
python envio-datos-mqtt.py
```

Puede ser necesario inicializar el entorno virtual antes de correrlo:

```bash
uv sync
source .venv/bin/activate
```

Esto instala `paho-mqtt`, la biblioteca requerida por el script.

El script lee `MQTT_BROKER` del entorno. En Linux o macOS el `localhost` por defecto funciona. En WSL2 configurá la variable como se indica arriba.

---

## Estructura de topics MQTT

```
angamed/<device_id>/datos
```

Formato del payload (JSON):

```json
{
  "device_id": "tanque1",
  "ph": 6.72,
  "temp_agua": 22.5,
  "timestamp": 1700000000
}
```

---

## Modelo de datos en InfluxDB

| Elemento | Valor |
|---|---|
| Database | `mydb` |
| Measurement | `lecturas_tanque1` |
| Fields | `ph`, `temp_agua` |
| Tags | `device_id` |

---

## Configuración de Grafana

Hay que conectar InfluxDB como data source usando la configuración de la imagen:

![Configuración data source en Grafana](assets/data-source-grafana.JPG)

### Dashboard de Angamed

El dashboard incluye:

- pH a lo largo del tiempo (time series)
- Temperatura del agua a lo largo del tiempo (time series)
- Valores actuales (stat panel — última lectura)

Una vez levantado el stack, importar el dashboard desde `grafana/angamed-dashboard.json` en la UI de Grafana: **Dashboards → Import**.

---

## Estructura del repositorio

```
.
├── docker-compose.yaml           # Orquestación de servicios
├── init-secrets.sh               # Script de inicialización (correr antes del primer docker compose up)
├── .env.example                  # Plantilla de variables de entorno
├── nodered/
│   ├── flows.json                # Definición del flow de Node-RED
│   └── settings.js               # Configuración de Node-RED con credentialSecret
├── simulator/
│   ├── envio-datos-mqtt.py       # Publisher MQTT — simula nodo sensor ESP32
│   └── suscriptor-datos-mqtt.py  # Subscriber MQTT — para debugging
└── grafana/
    └── angamed-dashboard.json    # Export del dashboard de Grafana
```

---

## Credenciales y secretos

Los secretos nunca se commitean al repositorio. El archivo `.env` está en el `.gitignore`. En la primera ejecución, `init-secrets.sh` genera todos los secretos necesarios de forma automática.

Para rotar el token de InfluxDB, borrar el `.env` y volver a ejecutar `./init-secrets.sh`. Esto elimina y recrea todos los secretos — será necesario reingresar el token en Node-RED y Grafana.

---

## Solución de problemas

**Las credenciales de Node-RED se pierden tras reiniciar**
Significa que `NODERED_CREDENTIAL_SECRET` cambió entre ejecuciones. Conservar el archivo `.env` y no borrarlo entre reinicios. Si se pierden las credenciales, reingresar el token de InfluxDB en la UI de Node-RED en el nodo del servidor influxdb.

**InfluxDB Core no arranca — permission denied**
Ejecutar `docker run --rm influxdb:3-core id` para obtener el UID del contenedor, luego `sudo chown -R <uid>:<uid> data/influxdb/data`.

**Los mensajes MQTT no llegan a Node-RED en WSL2**
El servicio local de Mosquitto puede estar interceptando el tráfico en el puerto 1883. Detenerlo con `sudo systemctl stop mosquitto && sudo systemctl disable mosquitto`.