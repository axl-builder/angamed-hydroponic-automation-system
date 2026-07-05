#!/bin/bash

# ==============================================================================
# 1. CARGA, VALIDACIÓN Y GENERACIÓN DINÁMICA DE CREDENCIALES (.env)
# ==============================================================================
# Asegurar la existencia del archivo .env para evitar fallos de lectura
touch .env

# Cargar variables de entorno actuales omitiendo comentarios
export $(echo $(grep -v '^#' .env | xargs)) 2>/dev/null

# Generar INFLUX_ADMIN_TOKEN de manera automática si no está definido
if [ -z "$INFLUX_ADMIN_TOKEN" ]; then
    echo "🔑 Generando un nuevo INFLUX_ADMIN_TOKEN aleatorio..."
    # Genera una cadena segura de 48 caracteres hexadecimales con el prefijo oficial apiv3_
    INFLUX_ADMIN_TOKEN="apiv3_$(openssl rand -hex 24)"
    echo "INFLUX_ADMIN_TOKEN=$INFLUX_ADMIN_TOKEN" >> .env
fi

# Generar SESSION_SECRET_KEY para el Explorer si no está definida
if [ -z "$SESSION_SECRET_KEY" ]; then
    echo "🔑 Generando una nueva SESSION_SECRET_KEY para el Explorer..."
    SESSION_SECRET_KEY=$(openssl rand -base64 32)
    echo "SESSION_SECRET_KEY=$SESSION_SECRET_KEY" >> .env
fi

if [ -z "$NODERED_CREDENTIAL_SECRET" ]; then
    NODERED_CREDENTIAL_SECRET=$(openssl rand -hex 32)
    echo "NODERED_CREDENTIAL_SECRET=$NODERED_CREDENTIAL_SECRET" >> .env
fi

# ==============================================================================
# 2. CONTROL DE IDEMPOTENCIA (Limpieza opcional de entornos previos)
# ==============================================================================
EXISTEN_CARPETAS=false

# Verificamos si alguna de las carpetas principales ya está en el disco
for dir in data secrets config; do
    if [ -d "$dir" ]; then
        EXISTEN_CARPETAS=true
        break
    fi
done

if [ "$EXISTEN_CARPETAS" = true ]; then
    echo "⚠️ Se detectaron estructuras previas (data/, secrets/ o config/)."
    read -p "¿Deseas eliminarlas para realizar una inicialización completamente limpia? [s/N]: " respuesta
    
    case "$respuesta" in
        [sS]|[sS][iI])
            echo "Eliminando directorios existentes con privilegios elevados..."
            # Es necesario usar sudo por los chown previos de los UIDs de cada contenedor
            sudo rm -rf data secrets config
            echo "✅ Carpetas eliminadas correctamente. Procediendo con la creación limpia."
            ;;
        *)
            echo "➡️ Manteniendo las carpetas y permisos existentes. Continuando..."
            ;;
    esac
fi

# ==============================================================================
# 3. GESTIÓN DE CONFIGURACIONES Y SECRETOS (Sincronización de tokens)
# ==============================================================================
echo "Configurando archivos de acceso y credenciales..."

# InfluxDB Secret (Mapeado de forma nativa por Docker Secrets)
mkdir -p secrets
cat << EOF > secrets/influxdb-key.json
{
  "token": "$INFLUX_ADMIN_TOKEN",
  "name": "_admin"
}
EOF
sudo chown -R 1500:1500 secrets
chmod 700 secrets
chmod 600 secrets/influxdb-key.json

# InfluxDB Explorer Config (Recibe exactamente el mismo token asignado arriba)
mkdir -p config
cat << EOF > config/config.json
{
  "DEFAULT_INFLUX_SERVER": "http://influxdb3-core:8181",
  "DEFAULT_INFLUX_DATABASE": "mydb",
  "DEFAULT_API_TOKEN": "$INFLUX_ADMIN_TOKEN",
  "DEFAULT_SERVER_NAME": "Local InfluxDB 3"
}
EOF

# ==============================================================================
# 4. CREACIÓN Y ASIGNACIÓN DE PERMISOS EN LA CARPETA CENTRALIZADA 'data/'
# ==============================================================================
echo "Inicializando árbol de directorios en ./data/ y aplicando chown..."

# Crear directorios para InfluxDB, Node-RED y Mosquitto de un solo golpe
mkdir -p data/influxdb/data \
         data/mosquitto/config \
         data/mosquitto/data \
         data/mosquitto/log \
         data/node-red \
         data/grafana

#copiando config para nodered
cp nodered/settings.js data/node-red/settings.js


# Configuración Base de Mosquitto (si no existe, evita carpetas vacías erróneas)
if [ ! -f data/mosquitto/config/mosquitto.conf ]; then
    cat << EOF > data/mosquitto/config/mosquitto.conf
persistence true
persistence_location /mosquitto/data/
log_dest file /mosquitto/log/mosquitto.log
allow_anonymous true
listener 1883
EOF
fi

# Aplicar permisos específicos por Contenedor para evitar errores de Linux EACCES
# InfluxDB 3 Core (UID/GID 1500)
sudo chown -R 1500:1500 data/influxdb/data
sudo chown -R 1500:1500 config

# Mosquitto (UID/GID 1883 sobre datos, logs y archivo config)
sudo chown -R 1883:1883 data/mosquitto/data data/mosquitto/log
sudo chown -R 1883:1883 data/mosquitto/config/mosquitto.conf 2>/dev/null || true

# Node-RED (UID/GID 1000)
sudo chown -R 1000:1000 data/node-red

# Grafana (UID/GID 472)
sudo chown -R 472:472 data/grafana

echo "======================================================================"
echo " Sistema centralizado en './data/'. Inicia con: docker compose up -d"
echo "======================================================================"