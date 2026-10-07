#include "PlatformIoT.h"
// WiFi.status() se usa aquí aunque el propio paso "Conectar WiFi" no se haya agregado al flujo —
// PlatformIoT.begin() siempre se emite en setup() (ver cpp.ts), así que esta librería trae su propio WiFi.h.
#if defined(ESP32)
#include <WiFi.h>
#include <HTTPClient.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#endif

PlatformIoTClass PlatformIoT;

void PlatformIoTClass::begin(const char *deviceToken) { token = String(deviceToken); }

// Un solo POST JSON reutilizado por sendReading/readCommand — misma forma para las dos, cambia la ruta.
// Si WiFi no está conectado (o el token está vacío), no intenta nada: evita bloquear loop() esperando una
// conexión que no existe. El código de error se imprime por Serial para que se vea al depurar por USB.
String PlatformIoTClass::postJson(const String &path, const String &body) {
#if defined(ESP32) || defined(ESP8266)
  if (token.length() == 0 || WiFi.status() != WL_CONNECTED) return "";
  HTTPClient http;
  String url = String(PLATFORMIOT_API_HOST) + path;
#if defined(ESP32)
  http.begin(url);
#else
  WiFiClientSecure client; client.setInsecure();
  http.begin(client, url);
#endif
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", String("Bearer ") + token);
  int code = http.POST(body);
  String resp = code > 0 ? http.getString() : "";
  if (code <= 0) { Serial.print("PlatformIoT: error de red ("); Serial.print(code); Serial.println(")"); }
  http.end();
  return resp;
#else
  return "";
#endif
}

void PlatformIoTClass::sendReading(const char *table, const char *field, float value) {
  String body = String("{\"table\":\"") + table + "\",\"field\":\"" + field + "\",\"value\":" + String(value, 4) + "}";
  postJson("/api/iot/reading", body);
}
void PlatformIoTClass::sendReading(const char *table, const char *field, const String &value) {
  String body = String("{\"table\":\"") + table + "\",\"field\":\"" + field + "\",\"value\":\"" + value + "\"}";
  postJson("/api/iot/reading", body);
}
String PlatformIoTClass::readCommand(const char *table, const char *field) {
  String body = String("{\"table\":\"") + table + "\",\"field\":\"" + field + "\"}";
  return postJson("/api/iot/command", body);
}
