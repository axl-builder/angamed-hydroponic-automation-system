#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"

// 1. La IP de mi PC en Windows, porque uso wsl (la que vimos en el ipconfig)
const char* mqtt_server = "192.168.1.6";


// 2. Declarar WiFiClient y PubSubClient globalmente. Con ClientId y topicDatos generar el topic dinámicamente y guardarlo en una variable global
WiFiClient espClient;
PubSubClient client(espClient);

String clientId;
String topicDatos;

// 3.Crear una función connectMQTT() que intente conectarse al broker con un Client ID único — usá la MAC address del chip para garantizar unicidad: WiFi.macAddress() 
void connectMQTT() {
  // Mantener el bucle hasta que logre conectarse
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    
    
    // 2. Intentar conectar (convirtiendo el String a const char*)
    if (client.connect(clientId.c_str())) {
      Serial.println(" ¡Conectado al broker!");
      
      // 3. (Opcional) Publicar que este nodo específico revivió
      client.publish(topicDatos.c_str(), "En linea");
      
      // Si tuvieras que escuchar órdenes (ej: prender bomba), te suscribís acá:
      // client.subscribe("invernadero/tanque/bomba/set");
      
    } else {
      Serial.print(" Falló, rc=");
      Serial.print(client.state());
      Serial.println(" - Intentando de nuevo en 5 segundos...");
      
      // Esperar 5 segundos antes de reintentar
      delay(5000);
    }
  }
}


// 4. En setup(): inicializar Serial, conectar WiFi, configurar el servidor MQTT con setServer()
void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");


  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - inicio > 20000) {
        Serial.println("\nWiFi timeout - no se pudo conectar. Reiniciando placa...");
        // 2. Reiniciar el chip para evitar que se cuelgue intentando conectar a MQTT sin red
        ESP.restart();
    }
    Serial.print('.');
    delay(1000);
  }
    Serial.println(WiFi.localIP());
  
  //  Armar el ID único una vez que el WiFi está encendido
  clientId = "NodoTanque-" + WiFi.macAddress();
  
  //  Construir el topic siguiendo la convención de Angamed
  topicDatos = "angamed/" + clientId + "/datos";
  
  Serial.print("Topic de publicación configurado: ");
  Serial.println(topicDatos);
  
  //  Configurar el servidor y el puerto por defecto de MQTT
  client.setServer(mqtt_server, 1883);
}


// 5. En loop(): llamar client.loop() y verificar si la conexión MQTT sigue activa — si no, reconectar
void loop() {
  // Si se cae la conexión, volver a levantarla
  if (!client.connected()) {
    connectMQTT();
  }
  
  // Mantener la conexión viva (el latido)
  client.loop();

  // Ejemplo: publicar algo cada 10 segundos sin usar delay() bloqueantes
  static unsigned long lastMsg = 0;
  unsigned long now = millis();
  if (now - lastMsg > 10000) {
    lastMsg = now;
    // Acá iría la lectura real del sensor
    String payload = "{\"nivel\": 100}";
    client.publish(topicDatos.c_str(), payload.c_str()); 
    Serial.println("Mensaje publicado en: " + topicDatos);
  }
}