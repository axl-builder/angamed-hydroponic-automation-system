// src/sensores.cpp
#include "sensores.h"

void inicializarSensores() {
  // Inicializar el generador aleatorio (luego acá inicializarás las sondas reales)
  randomSeed(analogRead(34));
}

float leerTemperatura() {
  return random(2000, 3001) / 100.0;
}

float leerPh() {
  return random(600, 901) / 100.0;
}