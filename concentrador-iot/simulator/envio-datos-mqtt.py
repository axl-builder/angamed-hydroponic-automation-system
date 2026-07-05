import json
import time
import paho.mqtt.client as mqtt
import random
import os

# Definir los parametros de conexion
device_id = "tanque1"
broker = os.environ.get("MQTT_BROKER", "localhost")
port = 1883
topic = "angamed/tanque1/datos"
intervalo = 3  # Intervalo de publicacion en segundos

# Definimos un callback


def on_connect(client, userdate, flags, rc):
    if rc == 0:
        print("Conexion exitosa con el broker MQTT.")
    else:
        print(f"Error al conectar con el broker MQTT. Codigo de error: {rc}")


# Creamos un cliente MQTT
client = mqtt.Client()

# Asignamos la funcion de callback para la conexion
client.on_connect = on_connect

# Conectar el broker
client.connect(broker, port, 60)

# Dejar la conexion en un loop 
client.loop_start()

# Publicacion de la temperatura cada 5 segundos
try:
    while True:
        # Simulamos la lectura de datos del tanque
        ph = round(random.uniform(6, 9), 2)  # Valor simulado de pH
        temperatura = round(random.uniform(20, 30), 2)  # Valor simulado de temperatura

        # Agrupamos los datos en un diccionario de python
        datos_tanque = {
            "device_id": device_id,
            "ph": ph,
            "temp_agua": temperatura,
            "timestamp": int(time.time())
        }

        # convertimos todo a JSON string
        payload_json = json.dumps(datos_tanque)

        # Publicamos los datos en los topics correspondientes
        client.publish(topic, payload_json)

        print(f"Publicado: {payload_json}")

        # Esperamos 5 segundos antes de la siguiente publicacion
        time.sleep(intervalo)

except KeyboardInterrupt:
    print("Interrupcion del programa. Cerrando conexion con el broker MQTT.")
    client.loop_stop()
    client.disconnect()
    print("Conexion cerrada.")
