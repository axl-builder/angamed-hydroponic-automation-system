import json

import paho.mqtt.client as mqtt

# Definir los parametros de conexion
broker = "localhost"
port = 1883
topic = "angamed/tanque1/datos"
intervalo = 3  # Intervalo de publicacion en segundos

# Definimos un callback


def on_connect(client, userdate, flags, rc):
    if rc == 0:
        print("Conexion exitosa con el broker MQTT.")
        client.subscribe(topic)
        print(f"Suscrito a los topics: {topic}")
    else:
        print(f"Error al conectar con el broker MQTT. Codigo de error: {rc}")


def on_message(client, userdata, msg):
    try:
        payload = json.loads(msg.payload.decode())

        ph = payload["ph"] 
        temperatura = payload["temp_agua"]
        timestamp = payload["timestamp"]

        print(f"Mensaje recibido en el topic {msg.topic}: {ph}, {temperatura}, {timestamp}")

    except json.JSONDecodeError:
        print("Error al decodificar el mensaje JSON recibido.")

    except KeyError as e:
        print(f"Falta la clave en el mensaje JSON recibido: {e}")

    except Exception as e:
        print(f"Error al procesar el mensaje recibido: {e}")



# Creamos un cliente MQTT

client = mqtt.Client()

# Asignamos la funcion de callback para la conexion
client.on_connect = on_connect
client.on_message = on_message

#Conectar el broker
client.connect(broker, port, 60)

# Dejar la conexion en un loop 
client.loop_forever()

