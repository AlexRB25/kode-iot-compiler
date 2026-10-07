#pragma once
// PlatformIoT — librería real (no un nombre puesto) que usa el .ino que genera K-ode para los pasos
// «Enviar lectura»/«Leer comando». Funciona en ESP32 y ESP8266 (usa HTTPClient de cada core).
//
// IMPORTANTE (2026-10-07): el backend de K-ode todavía NO tiene las rutas /api/iot/reading y
// /api/iot/command — begin()/sendReading()/readCommand() SÍ hacen una petición HTTP real (no son un stub),
// pero hoy recibirán 404 hasta que esas rutas existan del lado del servidor. Cuando se construyan, esta
// librería no necesita cambiar: ya habla el protocolo real (POST JSON con el token de dispositivo).
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
