import json
import time
import paho.mqtt.client as mqtt
import random
import os

# Definir los parametros de conexion
device_id = "tanque1" # Node-RED va a extraer este nombre automáticamente del tópico
broker = os.environ.get("MQTT_BROKER", "localhost")
port = 1883
topic = f"angamed/{device_id}/datos" # Tópico dinámico
intervalo = 3  # Intervalo de publicacion en segundos

# Definimos un callback
def on_connect(client, userdata, flags, rc):
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

# Publicacion de la telemetría
try:
    while True:
        # Simulamos la lectura de datos del tanque y red
        ph = round(random.uniform(6.0, 9.0), 2)  
        temperatura = round(random.uniform(20.0, 30.0), 2)  
        rssi_simulado = random.randint(-85, -45) # Simulamos la intensidad del WiFi

        # -----------------------------------------------------
        # NUEVO SCHEMA ESTANDARIZADO (Igual al del ESP32):
        # 1. Ya no enviamos "device_id" (viaja en el tópico)
        # 2. "ph" cambia a "pH"
        # 3. "timestamp" cambia a "ts"
        # 4. Sumamos el "rssi"
        # -----------------------------------------------------
        datos_tanque = {
            "ts": int(time.time()),
            "pH": ph,
            "temp_agua": temperatura,
            "rssi": rssi_simulado
        }

        # convertimos todo a JSON string
        payload_json = json.dumps(datos_tanque)

        # Publicamos los datos en los topics correspondientes
        client.publish(topic, payload_json)

        print(f"Publicado en {topic}: {payload_json}")

        # Esperamos antes de la siguiente publicacion
        time.sleep(intervalo)

except KeyboardInterrupt:
    print("\nInterrupcion del programa. Cerrando conexion con el broker MQTT.")
    client.loop_stop()
    client.disconnect()
    print("Conexion cerrada.")