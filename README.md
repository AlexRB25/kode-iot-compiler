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

## Desplegar en Railway (elegido)

Capa gratis para arrancar (poco volumen: Bronce tope 5/mes); el plan de $20/mes (con metered aparte, ~$30
totales con uso normal de este servicio) cuando ya haya usuarios de verdad usándolo seguido.

1. Sube este repo a GitHub (uno nuevo, vacío — "kode-iot-compiler" en tu cuenta): `git remote add origin <url>` y `git push -u origin master`.
2. En railway.app: **New Project → Deploy from GitHub repo** → autoriza acceso a GitHub → elige `kode-iot-compiler`.
3. Railway detecta el `Dockerfile` solo y empieza a construir — la primera vez tarda varios minutos (instala
   arduino-cli + los núcleos de ESP32/ESP8266 + las librerías). Espera a que el deploy termine (estado verde).
4. Pestaña **Variables** del servicio → agrega `COMPILE_SECRET` con un valor largo y aleatorio (genera uno
   nuevo con `node -e "console.log(require('crypto').randomBytes(32).toString('hex'))"` — nunca lo
   subas a git, solo se pega en las Variables de Railway y de Vercel).
5. Pestaña **Settings → Networking** → **Generate Domain** — te da una URL pública tipo
   `https://kode-iot-compiler-production.up.railway.app`.
6. En `kode` (Vercel, variables de entorno del proyecto): agrega
   - `IOT_COMPILE_URL` = la URL del paso 5.
   - `IOT_COMPILE_SECRET` = el MISMO valor de `COMPILE_SECRET` del paso 4.
   Vuelve a desplegar `kode` (o espera al siguiente push) para que tome las variables nuevas.
7. Prueba con el `curl` de abajo contra la URL de Railway antes de probarlo desde el editor.

Nunca expongas `COMPILE_SECRET`/`IOT_COMPILE_SECRET` al navegador: la llamada a este servicio la hace el
servidor de K-ode, no el cliente — el navegador solo recibe de vuelta los binarios ya compilados para
flashearlos él mismo con WebSerial.

**Otros hosts** (si más adelante conviene cambiar): cualquiera que corra un contenedor Docker con un puerto
expuesto sirve igual (Fly.io, Render, un VPS con Docker…) — mismos pasos 4-7, solo cambia cómo se despliega
el contenedor en el paso 1-3.

## Probar a mano

```bash
curl -X POST http://localhost:8080/compile \
  -H "Authorization: Bearer TU_COMPILE_SECRET" \
  -H "Content-Type: application/json" \
  -d '{"board":"esp32","sketch":"void setup(){} void loop(){}"}'
```

Responde `{ "ok": true, "parts": [{ "address": 4096, "data": "<base64>" }, ...] }` o
`{ "ok": false, "error": "..." }` con el motivo real de arduino-cli si algo no compiló.
