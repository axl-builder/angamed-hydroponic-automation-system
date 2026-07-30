// ==============================================================================
// INCLUSIÓN DE LIBRERÍAS
// ==============================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>  // Cliente MQTT ligero y estándar para microcontroladores.
#include "secrets.h"       // Tus credenciales (SSID, contraseñas). Mantenelo fuera de GitHub.
#include "sensores.h"      // Tu módulo separado donde vive la lógica de los sensores de la hidroponía.
#include "esp_task_wdt.h"  // Watchdog Timer: el "perro guardián" que reinicia el ESP32 si se cuelga.
#include <ArduinoJson.h>   // Gestiona la creación de JSON de forma segura en la RAM (evita fragmentación).
#include <time.h>          // Librería estándar de C para manejar el tiempo y timestamps.

// ==============================================================================
// 1. CONFIGURACIÓN DEL SERVIDOR DE HORA (NTP)
// ==============================================================================
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -10800; // Offset en segundos respecto a Greenwich (-3 horas * 3600 = Argentina)
const int   daylightOffset_sec = 0; // Argentina no usa horario de verano actualmente.

// ==============================================================================
// 2. OBJETOS GLOBALES (WiFi, MQTT y Tópicos)
// ==============================================================================
WiFiClient espClient;           // Objeto que maneja la conexión física TCP/IP.
PubSubClient client(espClient); // Objeto MQTT que usa la conexión TCP/IP anterior.

// Usamos String temporalmente acá para armarlos de forma dinámica en el setup(),
// luego los convertiremos a const char* al momento de enviarlos.
String clientId;
String topicDatos;

// ==============================================================================
// 3. FUNCIÓN DE RECONEXIÓN MQTT
// ==============================================================================
// Esta función atrapa al código si se cae el broker o la red, y no lo deja salir
// hasta que vuelva a estar conectado, garantizando que no se pierdan datos en el vacío.
void connectMQTT() {
  while (!client.connected()) { // Mientras el estado sea "desconectado"...
    
    // Acariciamos al perro guardián: le decimos al Watchdog que no estamos colgados, 
    // solo estamos esperando a que vuelva internet. Si no hacemos esto, el chip se reinicia.
    esp_task_wdt_reset(); 
    
    Serial.print("Intentando conexión MQTT...");
    
    // client.connect() exige un texto inmutable (const char*). 
    // Usamos .c_str() para extraer ese formato compatible desde nuestra variable String.
    if (client.connect(clientId.c_str())) {
      Serial.println(" ¡Conectado al broker!");
      
      // Publicamos un mensaje de "estado" para avisar que este nodo volvió a la vida.
      client.publish(topicDatos.c_str(), "En linea");
      
      // client.subscribe("invernadero/tanque/bomba/set"); // (Descomentar para recibir órdenes)
      
    } else {
      Serial.print(" Falló, rc=");
      Serial.print(client.state()); // Imprime el código de error para diagnosticar (ej: -2 = error de red)
      Serial.println(" - Intentando de nuevo en 5 segundos...");
      
      delay(5000); // Pausa bloqueante antes de volver a martillar el servidor.
    }
  }
}

// ==============================================================================
// 4. CONFIGURACIÓN INICIAL (Se ejecuta una sola vez al arrancar)
// ==============================================================================
void setup() {
  Serial.begin(115200); // Iniciamos el puerto serie para ver logs en la PC.
  delay(2000);          // Pausa táctica para darte tiempo a abrir el monitor serie.

  // --- CONFIGURACIÓN DEL WATCHDOG ---
  // Le damos al sistema 30 segundos de tolerancia. Si en 30 segundos el código no llama
  // a esp_task_wdt_reset(), asume que hubo un error fatal y reinicia físicamente la placa.
  esp_err_t wdt_status = esp_task_wdt_init(30, true); 
  Serial.print("Resultado esp_task_wdt_init: ");
  Serial.println(esp_err_to_name(wdt_status)); // Debería imprimir "ESP_OK"
  esp_task_wdt_add(NULL); // Suscribe el hilo actual (loop principal) a la vigilancia.

  // --- INICIALIZACIÓN DE SENSORES ---
  inicializarSensores(); // Llama a la función de tu archivo sensores.h

  // --- CONEXIÓN WIFI ---
  WiFi.mode(WIFI_STA); // Modo Estación (se conecta a un router, no crea su propia red).
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");

  unsigned long inicio = millis();
  
  // Bucle de espera para el WiFi con un Timeout (válvula de escape)
  while (WiFi.status() != WL_CONNECTED) {
    esp_task_wdt_reset(); // Avisamos que estamos ocupados, no colgados.
    
    // Si pasaron más de 20 segundos intentando conectar...
    if (millis() - inicio > 20000) {
        Serial.println("\nWiFi timeout - no se pudo conectar. Reiniciando placa...");
        ESP.restart(); // Cortamos por lo sano y reiniciamos el ciclo desde cero.
    }
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP()); // Imprime la IP que le asignó el router.

  // --- SINCRONIZACIÓN NTP (EL RELOJ) ---
  // Disparamos la petición asincrónica al servidor de internet.
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Sincronizando hora con NTP (pool.ntp.org)...");

  struct tm timeinfo;
  unsigned long ntpInicio = millis();
  bool ntpSincronizado = false;

  // Bucle sincrónico: obligamos al setup a esperar hasta 10 segundos por la hora correcta.
  // Esto evita que las primeras lecturas se envíen con fecha de 1970.
  while (!ntpSincronizado && (millis() - ntpInicio < 10000)) {
    esp_task_wdt_reset();
    // getLocalTime chequea si el ESP32 ya recibió la hora. Devuelve true si la tiene.
    ntpSincronizado = getLocalTime(&timeinfo, 1000); 
    if (!ntpSincronizado) {
      Serial.print('.');
    }
  }

  if (ntpSincronizado) {
    Serial.println("\nHora NTP sincronizada correctamente.");
  } else {
    Serial.println("\nADVERTENCIA: Timeout de NTP. Los primeros timestamps podrían ser de 1970.");
  }

  // --- GENERACIÓN DE IDENTIFICADORES DINÁMICOS ---
  // Usamos la MAC Address (dirección física única del chip de red) para asegurar que 
  // si el día de mañana agregás otro ESP32, nunca tengan el mismo ID y choquen en MQTT.
  clientId = "NodoTanque-" + WiFi.macAddress();
  topicDatos = "angamed/" + clientId + "/datos"; // Ej: angamed/NodoTanque-24:6F.../datos
  
  Serial.print("Topic de publicación configurado: ");
  Serial.println(topicDatos);
  
  // Le decimos al cliente MQTT a qué IP/URL apuntar y a qué puerto (1883 es el estándar).
  client.setServer(MQTT_SERVER, 1883);
}

// ==============================================================================
// 5. BUCLE PRINCIPAL (Se repite infinitamente)
// ==============================================================================
void loop() {
  esp_task_wdt_reset(); // "Todo bien Watchdog, seguimos vivos en cada vuelta del loop"

  // Si por algún motivo nos desconectamos de MQTT (ej: microcorte de red), 
  // frenamos acá y volvemos a intentar reconectar.
  if (!client.connected()) {
    connectMQTT();
  }
  
  // client.loop() procesa mensajes entrantes (si estuvieras suscripto a algo)
  // y mantiene vivo el "Ping" de fondo con el servidor MQTT. Debe llamarse siempre.
  client.loop();

  // --- TEMPORIZADOR NO BLOQUEANTE ---
  // En lugar de usar delay(10000) (que colgaría todo el procesador y haría fallar el MQTT y el Watchdog),
  // calculamos la diferencia de tiempo desde la última vez que mandamos un mensaje.
  static unsigned long lastMsg = 0; // 'static' hace que esta variable no se borre al terminar el loop.
  unsigned long now = millis();     // Tiempo actual desde que arrancó el micro.
  
  if (now - lastMsg > 10000) {      // ¿Pasaron 10.000 milisegundos (10 segundos)?
    lastMsg = now;                  // Actualizamos la marca de tiempo para la próxima vuelta.
    
    // 1. OBTENEMOS DATOS FÍSICOS
    float ph_actual = leerPh();
    float temp_actual = leerTemperatura();

    // 2. OBTENEMOS EL TIEMPO ACTUAL (TS)
    // time_t es un tipo de dato preparado para guardar segundos desde 1970.
    time_t ts_actual;
    time(&ts_actual); // Carga la hora real (que ya sincronizamos en el setup) en la variable.

    // 3. ARMAMOS EL PAQUETE JSON
    // Usamos JsonDocument (ArduinoJson v7). Esto se crea en el "Stack" (memoria temporal).
    // Cuando termine el 'if', este documento se destruye automáticamente, liberando la RAM
    // perfectamente intacta, sin generar fragmentación.
    JsonDocument doc; 
    
    // Asignamos las claves y los valores. El tipo de dato (float, int, string) se maneja solo.
    doc["ts"]        = ts_actual;             // Timestamp para la serie temporal (InfluxDB)
    doc["pH"]        = ph_actual;             
    doc["temp_agua"] = temp_actual;
    doc["rssi"]      = WiFi.RSSI();           // Métrica de intensidad de señal WiFi (muy útil en IoT)
    
    // (A futuro podés agregar más sensores acá simplemente sumando líneas doc["clave"] = valor)

    // 4. SERIALIZACIÓN Y ENVÍO
    String payload;
    // Convierte la estructura de datos 'doc' a un texto estructurado en formato JSON y lo guarda en 'payload'.
    serializeJson(doc, payload); 

    // Publicamos en el broker. Pasamos tanto el topic como el payload a 'const char*' con .c_str()
    client.publish(topicDatos.c_str(), payload.c_str()); 
    
    Serial.println("Publicado: " + payload); // Dejamos un registro en consola para depuración.
  }
}