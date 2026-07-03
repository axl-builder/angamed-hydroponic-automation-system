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