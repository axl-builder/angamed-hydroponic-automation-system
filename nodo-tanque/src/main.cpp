#include <Arduino.h>
#include <WiFi.h>
#include "secrets.h"


void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");


  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - inicio > 20000) {
        Serial.print("WiFi timeout - no se pudo conectar");
        break;
    }
    Serial.print('.');
    delay(1000);
  }
    Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  initWiFi();
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());
}


void loop() {}