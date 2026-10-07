#pragma once
// PlatformIoT — librería real (no un nombre puesto) que usa el .ino que genera K-ode para los pasos
// «Enviar lectura»/«Leer comando». Funciona en ESP32 y ESP8266 (usa HTTPClient de cada core).
//
// El backend de K-ode YA tiene las rutas /api/iot/reading y /api/iot/command (2026-10-07, migración 0113:
// device_readings) — begin()/sendReading()/readCommand() hablan con ellas de verdad. sendReading() guarda
// la última lectura de ese (tabla, campo); readCommand() lee esa MISMA fila (todavía no hay un paso que
// ESCRIBA un comando desde la app — por ahora readCommand() solo devuelve lo último que el propio
// dispositivo reportó, listo para cuando exista ese paso).
#include <Arduino.h>

#ifndef PLATFORMIOT_API_HOST
#define PLATFORMIOT_API_HOST "https://editor.k-ode.io"
#endif

class PlatformIoTClass {
public:
  void begin(const char *deviceToken);
  // Manda una lectura (tabla + campo + valor) a la base de datos en la nube del proyecto dueño de este token.
  void sendReading(const char *table, const char *field, float value);
  void sendReading(const char *table, const char *field, const String &value);
  // Pide el último valor de un campo (para que la placa reaccione a algo que cambiaste desde tu app).
  String readCommand(const char *table, const char *field);

private:
  String token;
  String postJson(const String &path, const String &body);
};

extern PlatformIoTClass PlatformIoT;
