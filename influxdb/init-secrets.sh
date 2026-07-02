#!/bin/bash

# Cargar las variables del archivo .env si existe
if [ -f .env ]; then
  export $(echo $(grep -v '^#' .env | xargs))
else
  echo "Error: Archivo .env no encontrado."
  exit 1
fi

# Validar que ambas variables existan en el .env
if [ -z "$INFLUX_ADMIN_TOKEN" ]; then
  echo "Error: INFLUX_ADMIN_TOKEN no está definido en el .env."
  exit 1
fi

# Crear la carpeta secrets si no existe
mkdir -p secrets

# Crear el archivo JSON con los datos del .env
cat << EOF > secrets/influxdb-key.json
{
  "token": "$INFLUX_ADMIN_TOKEN",
  "name": "_admin"
}
EOF

echo "Archivo secrets/influxdb-key.json generado correctamente."

# Crear la carpeta config si no existe
mkdir -p config

# Crear el archivo config.json con los datos fijos solicitados
cat << EOF > config/config.json
{
  "DEFAULT_INFLUX_SERVER": "http://influxdb3-core:8181",
  "DEFAULT_INFLUX_DATABASE": "mydb",
  "DEFAULT_API_TOKEN": "$INFLUX_ADMIN_TOKEN",
  "DEFAULT_SERVER_NAME": "Local InfluxDB 3"
}
EOF

echo "Archivo config/config.json generado correctamente."
mkdir -p data/influxdb/data
sudo chown -R 1500:1500 data/