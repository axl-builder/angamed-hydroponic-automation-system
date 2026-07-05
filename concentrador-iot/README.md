Acá tenés el README completo y mejorado, listo para copiar y pegar en tu repo:Markdown# Angamed Hydroponic Automation — Concentrador IoT

Stack de concentrador local para el sistema de automatización del invernadero hidropónico Angamed. Recibe datos de sensores desde nodos ESP32 vía MQTT, los almacena en InfluxDB 3 Core y los visualiza en Grafana en tiempo real.

---

## Demo

![Video Demostrativo](assets/demo.gif)

---

## Arquitectura (en producción)

![Dashboard de Grafana](assets/screenshot.png)

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
- Gestor de paquetes `uv` (para ejecutar el simulador en Python)
- Linux o macOS (WSL2 en Windows también funciona — ver nota más abajo)

---

## Configuración inicial

### 1. Clonar el repositorio

```bash
git clone <repo-url>
cd angamed-hydroponic-automation-system/concentrador-iot
2. Ejecutar el script de inicializaciónBashchmod +x init-secrets.sh
bash init-secrets.sh
Este script hace lo siguiente de forma automática:Genera INFLUX_ADMIN_TOKEN y SESSION_SECRET_KEY en .env si no existen.Genera NODERED_CREDENTIAL_SECRET en .env si no existe.Crea secrets/influxdb-key.json con el token de admin para InfluxDB Core.Crea config/config.json con la conexión del Explorer preconfigurada.Crea todos los directorios de datos bajo data/ con los permisos correctos por UID de contenedor.Copia nodered/settings.js y nodered/flows.json a data/node-red/.Nota: Si los directorios de datos o el archivo .env ya existen, el script preguntará si querés borrarlos para realizar una inicialización completamente limpia.3. Levantar el stackBashdocker compose up -d
4. Verificar los serviciosServicioURLCredencialesInfluxDB Explorerhttp://localhost:8080Token del .envNode-REDhttp://localhost:1880Sin credencialesGrafanahttp://localhost:3000admin / admin5. Configurar la conexión en Node-REDUna vez que los servicios estén levantados, es necesario configurar Node-RED para que pueda enviar datos a InfluxDB.Instalar el plugin de InfluxDB:Ve al menú de opciones (≡) arriba a la derecha en la interfaz de Node-RED.Selecciona Manage palette.En la pestaña Install, busca e instala el nodo correspondiente a InfluxDB.Cargar el Token de seguridad:Haz doble clic en el nodo de InfluxDB dentro de tu flujo.Edita la configuración del servidor haciendo clic en el ícono del lápiz.Pega el token que se generó en la variable INFLUX_ADMIN_TOKEN de tu archivo .env.Nota para WSL2Si corrés el stack en WSL2 con Docker Desktop, localhost dentro de WSL2 no resuelve a los puertos de los contenedores Docker. Usá la IP del gateway de WSL2 en su lugar:Bashexport MQTT_BROKER=$(grep nameserver /etc/resolv.conf | awk '{print $2}')
Agregá esta línea a tu perfil de shell (.bashrc o .zshrc) para que persista entre sesiones. Los scripts de simulación leen MQTT_BROKER del entorno con localhost como fallback.Simulación de datos de sensoresSe incluye un script en Python (envio-datos-mqtt.py) para desarrollo y pruebas. Este script simula lecturas de un nodo sensor ESP32 y las publica al broker cada 3 segundos.Como el script requiere la librería paho-mqtt, primero debes inicializar el entorno virtual usando uv e instalar las dependencias:Bashcd simulator
uv sync
source .venv/bin/activate
python envio-datos-mqtt.py
Nota: El script lee la variable MQTT_BROKER del entorno. En Linux o macOS, el localhost por defecto funciona sin problemas. En WSL2, configurá la variable como se indica en la sección anterior.Estructura de topics MQTTPlaintextangamed/<device_id>/datos
Formato del payload esperado (JSON):JSON{
  "device_id": "tanque1",
  "ph": 6.72,
  "temp_agua": 22.5,
  "timestamp": 1700000000
}
Modelo de datos en InfluxDBElementoValorDatabasemydbMeasurementlecturas_tanque1Fieldsph, temp_aguaTagsdevice_idConfiguración de GrafanaPara visualizar los datos, primero debes conectar Grafana con tu base de datos InfluxDB:Accede a Grafana en http://localhost:3000 (Credenciales por defecto: admin / admin).Ve a la sección de Data Sources y agrega una nueva conexión.Configura los parámetros de conexión vinculando el servidor de InfluxDB y pegando el token de seguridad, tal como se muestra en la siguiente imagen:Dashboard de AngamedEl dashboard preconfigurado incluye:Nivel de pH a lo largo del tiempo (Time series).Temperatura del agua a lo largo del tiempo (Time series).Valores actuales (Stat panel — última lectura registrada).Una vez levantado el stack y configurado el Data Source, puedes importar el dashboard directamente desde el archivo grafana/angamed-dashboard.json en la UI de Grafana: Dashboards → Import.Estructura del repositorioPlaintext.
├── docker-compose.yaml       # Orquestación de servicios
├── init-secrets.sh           # Script de inicialización (correr antes del primer docker compose up)
├── .env.example              # Plantilla de variables de entorno
├── nodered/
│   ├── flows.json            # Definición del flow base de Node-RED
│   └── settings.js           # Configuración de Node-RED con credentialSecret
├── simulator/
│   ├── envio-datos-mqtt.py      # Publisher MQTT — simula nodo sensor ESP32
│   └── suscriptor-datos-mqtt.py # Subscriber MQTT — para debugging
└── grafana/
    └── angamed-dashboard.json   # Export del dashboard preconfigurado