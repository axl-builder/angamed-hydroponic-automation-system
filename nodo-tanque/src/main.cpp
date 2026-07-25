#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Nodo Angamed arrancando...");
    Serial.printf("RAM libre: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("PSRAM total: %d bytes\n", ESP.getPsramSize());
    Serial.printf("PSRAM libre: %d bytes\n", ESP.getFreePsram());
}

void loop() {}