#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"
#include "sensores.h"
#include "esp_task_wdt.h"
#include <ArduinoJson.h>
#include <time.h>

// 1. CONFIGURACIÓN DEL SERVIDOR DE HORA (NTP)
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -10800; // -10800 segundos = UTC-3 (Hora de Argentina)
const int   daylightOffset_sec = 0; // Sin horario de verano

// 2. Declarar WiFiClient y PubSubClient globalmente. Con ClientId y topicDatos generar el topic dinámicamente y guardarlo en una variable global
WiFiClient espClient;
PubSubClient client(espClient);
String clientId;
String topicDatos;

// 3.Crear una función connectMQTT() que intente conectarse al broker con un Client ID único — usá la MAC address del chip para garantizar unicidad: WiFi.macAddress() 
void connectMQTT() {
  // Mantener el bucle hasta que logre conectarse
  while (!client.connected()) {

    esp_task_wdt_reset();
    
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
  

  // dentro de setup(), después de Serial.begin(115200):
  delay(2000); // para que te dé tiempo a abrir el monitor serie
  esp_err_t wdt_status = esp_task_wdt_init(30, true); // 30 segundos de timeout, true para resetear el chip si se cuelga
  Serial.print("Resultado esp_task_wdt_init: ");
  Serial.println(esp_err_to_name(wdt_status));

  esp_task_wdt_add(NULL);
  // Llamamos a la inicialización de nuestro módulo separado
  inicializarSensores();
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");


  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED) {

    esp_task_wdt_reset();
    
    if (millis() - inicio > 20000) {
        Serial.println("\nWiFi timeout - no se pudo conectar. Reiniciando placa...");
        // 2. Reiniciar el chip para evitar que se cuelgue intentando conectar a MQTT sin red
        ESP.restart();
    }
    Serial.print('.');
    delay(1000);
  }
    Serial.println(WiFi.localIP());

    // 2. SINCRONIZAMOS LA HORA CON INTERNET (NTP)
    // Esto se hace una sola vez acá, y el ESP32 mantiene la hora solo
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println("Sincronizando hora con NTP (pool.ntp.org)...");

  
  //  Armar el ID único una vez que el WiFi está encendido
  clientId = "NodoTanque-" + WiFi.macAddress();
  
  //  Construir el topic siguiendo la convención de Angamed
  topicDatos = "angamed/" + clientId + "/datos";
  
  Serial.print("Topic de publicación configurado: ");
  Serial.println(topicDatos);
  
  //  Configurar el servidor y el puerto por defecto de MQTT
  client.setServer(MQTT_SERVER, 1883);
}


// 5. En loop(): llamar client.loop() y verificar si la conexión MQTT sigue activa — si no, reconectar
void loop() {
  esp_task_wdt_reset();
  // Si se cae la conexión, volver a levantarla
  if (!client.connected()) {
    connectMQTT();
  }
  
  // Mantener la conexión viva (el latido)
  client.loop();

  // Ejemplo: publicar algo cada 10 segundos
  static unsigned long lastMsg = 0;
  unsigned long now = millis();
  if (now - lastMsg > 10000) {
    lastMsg = now;
    
    // Obtenemos los datos limpios desde el otro archivo
    float ph_actual = leerPh();
    float temp_actual = leerTemperatura();

    // 3. OBTENEMOS LA HORA REAL (Epoch / Unix Timestamp)
    time_t ts_actual;
    time(&ts_actual);

    // 4. ARMAMOS EL JSON (El fin de la fragmentación de memoria)
    // ArduinoJson v7 maneja el tamaño automáticamente sin que tengas que calcular bytes
    // Armamos el JSON respetando el Schema de Producción
    JsonDocument doc;
    
    // doc["device_id"] = clientId; // LO SACAMOS, se envía en el Tópico
    
    doc["ts"]        = ts_actual;
    doc["pH"]        = ph_actual;             // Corregido a mayúscula
    doc["temp_agua"] = temp_actual;
    doc["rssi"]      = WiFi.RSSI();           // Agregamos métrica de red
    
    // (Cuando tengas los otros sensores, agregarás "EC" y "nivel_pct")

    String payload;
    serializeJson(doc, payload);

    client.publish(topicDatos.c_str(), payload.c_str()); 
    Serial.println("Publicado: " + payload);
  }
}