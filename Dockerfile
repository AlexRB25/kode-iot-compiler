# K-ode · servicio de compilación de IoT (arduino-cli real) — ver server.mjs. Se construye UNA vez con todo
# el toolchain pesado (núcleos ESP32/ESP8266 + librerías) ya instalado, así cada /compile solo tarda lo que
# tarda compilar el .ino, no la instalación del entorno.
FROM node:20-bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends curl ca-certificates python3 && rm -rf /var/lib/apt/lists/*

# arduino-cli (binario oficial, instalado en /usr/local/bin).
RUN curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh -s -- -b /usr/local/bin

# Índices de placas que no vienen en el catálogo oficial de Arduino (ESP32/ESP8266 son de Espressif).
RUN arduino-cli config init && \
    arduino-cli config add board_manager.additional_urls \
      https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json \
      https://arduino.esp8266.com/stable/package_esp8266com_index.json && \
    arduino-cli core update-index && \
    arduino-cli core install esp32:esp32 && \
    arduino-cli core install esp8266:esp8266

# Librerías de terceros que algunos pasos del editor pueden generar (ver cpp.ts: DHT11, Firebase). Se
# instalan por URL de git (no por nombre del Library Manager): así no depende de acertar el nombre EXACTO
# con el que cada una está registrada ahí — el repo es inequívoco. Las de Firebase están [DEPRECATED] en favor
# de "FirebaseClient" (API nueva), pero cpp.ts genera código contra esta API clásica (Firebase.begin/getFloat/…),
# así que son las que hace falta instalar aquí.
RUN arduino-cli lib install --git-url https://github.com/DFRobot/DFRobot_DHT11 \
 && arduino-cli lib install --git-url https://github.com/mobizt/Firebase-ESP32 \
 && arduino-cli lib install --git-url https://github.com/mobizt/Firebase-ESP8266

# Nuestra propia librería (begin/sendReading/readCommand) — va en la carpeta de librerías del usuario de arduino-cli.
COPY platformiot/PlatformIoT /root/Arduino/libraries/PlatformIoT

WORKDIR /app
COPY server.mjs ./
ENV PORT=8080
EXPOSE 8080
CMD ["node", "server.mjs"]
