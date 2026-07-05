# angamed-hydroponic-automation-system
1. cp .env.example .env  (y completar el token)
2. ./init-secrets.sh
3. sudo chown -R 1500:1500 ./data
4. docker compose up -d


# En WSL2 con Docker Desktop:
export MQTT_BROKER=$(grep nameserver /etc/resolv.conf | awk '{print $2}')

# En Linux nativo o Mac:
export MQTT_BROKER=localhost

# En producción con ESP32:
# configurar directamente la IP del servidor en el firmware

# Angamed Hydroponic Automation — IoT Concentrator

Local concentrator stack for the Angamed hydroponic greenhouse automation system. Receives sensor data from ESP32 nodes via MQTT, stores it in InfluxDB 3 Core, and visualizes it in Grafana in real time.

---

## Demo

<!-- Replace this comment with an embedded video or GIF showing the live dashboard -->
> 📹 _Video demo goes here_

---

## Architecture

```
ESP32 nodes  ──MQTT──►  Mosquitto  ──►  Node-RED  ──►  InfluxDB 3 Core  ──►  Grafana
                            │                                                    │
                            └──────────────── InfluxDB Explorer ◄───────────────┘
```

All services run as Docker containers on a single host (Raspberry Pi 4 in production, any Linux machine for development).

---

## Stack

| Service | Image | Port | Purpose |
|---|---|---|---|
| Mosquitto | `eclipse-mosquitto:latest` | 1883 | MQTT broker |
| InfluxDB 3 Core | `influxdb:3-core` | 8181 | Time series database |
| InfluxDB Explorer | `influxdata/influxdb3-ui:1.9.0` | 8080 | Database UI |
| Node-RED | `nodered/node-red:5.0-debian` | 1880 | Flow engine — MQTT to InfluxDB bridge |
| Grafana | `grafana/grafana:13.0.3` | 3000 | Dashboard |

---

## Requirements

- Docker and Docker Compose
- `openssl` available in the shell
- Linux or macOS (WSL2 on Windows also works — see note below)

---

## Setup

### 1. Clone the repository

```bash
git clone <repo-url>
cd angamed-hydroponic-automation-system
```

### 2. Run the initialization script

```bash
chmod +x init-secrets.sh
./init-secrets.sh
```

This script does the following automatically:

- Generates `INFLUX_ADMIN_TOKEN` and `SESSION_SECRET_KEY` in `.env` if they don't exist
- Generates `NODERED_CREDENTIAL_SECRET` in `.env` if it doesn't exist
- Creates `secrets/influxdb-key.json` with the admin token for InfluxDB Core
- Creates `config/config.json` with the Explorer connection pre-configured
- Creates all required data directories under `data/` with correct ownership per container UID
- Copies `nodered/settings.js` and `nodered/flows.json` to `data/node-red/`

> If data directories already exist, the script will ask whether to wipe them for a clean initialization.

### 3. Start the stack

```bash
docker compose up -d
```

### 4. Verify services

| Service | URL | Default credentials |
|---|---|---|
| InfluxDB Explorer | http://localhost:8080 | Token from `.env` |
| Node-RED | http://localhost:1880 | None |
| Grafana | http://localhost:3000 | admin / admin |

---

## WSL2 note

If running on WSL2 with Docker Desktop, `localhost` inside WSL2 does not resolve to Docker container ports. Use the WSL2 gateway IP instead:

```bash
export MQTT_BROKER=$(grep nameserver /etc/resolv.conf | awk '{print $2}')
```

Set this in your shell profile (`.bashrc` or `.zshrc`) so it persists across sessions. The simulation scripts read `MQTT_BROKER` from the environment with `localhost` as fallback.

---

## Simulating sensor data

A Python publisher script is included for development and testing. It simulates ESP32 sensor readings and publishes them to the broker every 3 seconds.

```bash
cd simulator
python envio-datos-mqtt.py
```

The script reads `MQTT_BROKER` from the environment. On Linux or macOS, the default `localhost` works. On WSL2, set the variable as described above.

---

## MQTT topic structure

```
angamed/<device_id>/datos
```

Payload format (JSON):

```json
{
  "device_id": "tanque1",
  "ph": 6.72,
  "temp_agua": 22.5,
  "timestamp": 1700000000
}
```

---

## InfluxDB data model

| Element | Value |
|---|---|
| Database | `mydb` |
| Measurement | `lecturas_tanque1` |
| Fields | `ph`, `temp_agua` |
| Tags | `device_id` |

---

## Grafana dashboard

The Angamed dashboard includes:

- pH over time (time series)
- Water temperature over time (time series)
- Current values (stat panel — last reading)

After starting the stack, import the dashboard from `grafana/angamed-dashboard.json` via Grafana UI: Dashboards → Import.

---

## Repository structure

```
.
├── docker-compose.yaml       # Service orchestration
├── init-secrets.sh           # Initialization script (run before first docker compose up)
├── .env.example              # Environment variable template
├── nodered/
│   ├── flows.json            # Node-RED flow definition
│   └── settings.js           # Node-RED settings with credentialSecret
├── simulator/
│   ├── envio-datos-mqtt.py   # MQTT publisher — simulates ESP32 sensor node
│   └── suscriptor-datos-mqtt.py  # MQTT subscriber — for debugging
└── grafana/
    └── angamed-dashboard.json    # Grafana dashboard export
```

---

## Credentials and secrets

Secrets are never committed to the repository. The `.env` file is git-ignored. On first run, `init-secrets.sh` generates all required secrets automatically.

To rotate the InfluxDB token, delete `.env` and re-run `./init-secrets.sh`. This wipes all generated secrets and recreates them — you will need to re-enter the token in Node-RED and Grafana.

---

## Troubleshooting

**InfluxDB Core fails to start with permission denied**
Run `docker run --rm influxdb:3-core id` to get the container UID, then `sudo chown -R <uid>:<uid> data/influxdb/data`.

**Node-RED credentials lost after restart**
This means `NODERED_CREDENTIAL_SECRET` changed between runs. Keep the `.env` file and do not delete it between restarts. If credentials are lost, re-enter the InfluxDB token in the Node-RED UI under the influxdb server node.

**MQTT messages not reaching Node-RED on WSL2**
The local Mosquitto service may be intercepting traffic on port 1883. Stop it with `sudo systemctl stop mosquitto && sudo systemctl disable mosquitto`.