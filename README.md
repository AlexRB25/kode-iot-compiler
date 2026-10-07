# K-ode · servicio de compilación de IoT

Compila el `.ino` que genera el editor de K-ode con `arduino-cli` de verdad y devuelve los binarios listos
para flashear por USB desde el navegador (WebSerial, vía `esptool-js` — ver
`kode/src/components/IotFlashButton.tsx`).

Aparte del editor (Vercel) a propósito: el toolchain de Arduino (núcleos ESP32/ESP8266 + librerías) pesa
cientos de MB y una función serverless no tiene filesystem persistente ni el tiempo de ejecución que hace
falta — esto necesita un servidor/contenedor de verdad, siempre prendido.

Solo compila para **ESP32** y **ESP8266** (son las únicas placas del catálogo de K-ode que se pueden
flashear por USB sin instalar nada, vía el protocolo de arranque de Espressif). Arduino Uno/Nano/Mega y
Raspberry Pi Pico siguen solo con "Copiar"/"Descargar" + Arduino IDE manual.

## Desplegar

Cualquier host que corra un contenedor Docker con un puerto expuesto sirve (Railway, Fly.io, Render, un VPS
con Docker…). Pasos:

1. `docker build -t kode-iot-compiler .`
2. Corre el contenedor con la variable de entorno `COMPILE_SECRET` puesta a un valor largo y aleatorio (ej.
   `openssl rand -hex 32`) — es el secreto compartido que valida que solo el servidor de K-ode pueda pedir
   compilaciones. Expón el puerto `8080` (o el que pongas en `PORT`).
3. En `kode` (el editor), configura las variables de entorno del servidor:
   - `IOT_COMPILE_URL` — la URL pública de este servicio (ej. `https://kode-iot-compiler.up.railway.app`).
   - `IOT_COMPILE_SECRET` — el MISMO valor que `COMPILE_SECRET` de arriba.

Nunca expongas `COMPILE_SECRET`/`IOT_COMPILE_SECRET` al navegador: la llamada a este servicio la hace el
servidor de K-ode, no el cliente — el navegador solo recibe de vuelta los binarios ya compilados para
flashearlos él mismo con WebSerial.

## Probar a mano

```bash
curl -X POST http://localhost:8080/compile \
  -H "Authorization: Bearer TU_COMPILE_SECRET" \
  -H "Content-Type: application/json" \
  -d '{"board":"esp32","sketch":"void setup(){} void loop(){}"}'
```

Responde `{ "ok": true, "parts": [{ "address": 4096, "data": "<base64>" }, ...] }` o
`{ "ok": false, "error": "..." }` con el motivo real de arduino-cli si algo no compiló.
