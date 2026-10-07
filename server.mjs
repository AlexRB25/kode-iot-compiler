// Servicio de compilación de K-ode (IoT): recibe un .ino (el que genera cpp.ts) y una placa, compila con
// arduino-cli de verdad y devuelve los binarios listos para flashear por USB desde el navegador (esptool-js,
// ver kode/src/components/IotFlashButton.tsx) — con sus direcciones de flasheo correctas por placa.
// Aparte del editor (Vercel no puede correr esto: el toolchain de Arduino pesa cientos de MB y tarda minutos
// en compilar — necesita filesystem persistente y tiempo de ejecución largo).
import http from 'node:http';
import { execFile } from 'node:child_process';
import { promisify } from 'node:util';
import { mkdir, mkdtemp, readdir, readFile, rm, writeFile } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import path from 'node:path';

const run = promisify(execFile);
const PORT = process.env.PORT || 8080;
const SECRET = process.env.COMPILE_SECRET; // kode lo manda en el header Authorization — nadie más debe poder llamar este servicio
if (!SECRET) throw new Error('Falta COMPILE_SECRET en el entorno.');

// FQBN real de arduino-cli por cada placa que SÍ se puede flashear por USB desde el navegador (WebSerial solo
// habla el protocolo de arranque de los chips Espressif — por eso no hay Arduino Uno/Nano/Mega/RP2040 aquí,
// esas siguen solo con "Copiar"/"Descargar" + Arduino IDE). offsets: dónde va cada parte del binario en la
// flash (confirmado contra la documentación real de arduino-cli/esptool, no inventado).
const BOARDS = {
  esp32: { fqbn: 'esp32:esp32:esp32', parts: [
    { file: '.bootloader.bin', address: 0x1000 },
    { file: '.partitions.bin', address: 0x8000 },
    { file: '.bin', address: 0x10000 },
  ] },
  esp8266: { fqbn: 'esp8266:esp8266:nodemcuv2', parts: [
    { file: '.bin', address: 0x0 },
  ] },
};

async function compile(board, sketch) {
  const cfg = BOARDS[board];
  if (!cfg) throw new Error(`Placa no soportada para flasheo por USB: ${board}`);
  const dir = await mkdtemp(path.join(tmpdir(), 'kode-iot-'));
  const sketchDir = path.join(dir, 'sketch');
  const outDir = path.join(dir, 'out');
  await mkdir(sketchDir, { recursive: true });
  await mkdir(outDir, { recursive: true });
  await writeFile(path.join(sketchDir, 'sketch.ino'), sketch, 'utf8');
  try {
    await run('arduino-cli', ['compile', '--fqbn', cfg.fqbn, '--export-binaries', '--output-dir', outDir, sketchDir], { timeout: 180_000, maxBuffer: 20 * 1024 * 1024 });
    // ".bin" también es el final de ".bootloader.bin"/".partitions.bin" — se van quitando de la lista a medida
    // que se encuentran, así el match genérico ".bin" solo puede caer en el binario principal del sketch.
    let outFiles = await readdir(outDir);
    const parts = [];
    for (const p of cfg.parts) {
      const file = outFiles.find(f => f.endsWith(p.file));
      if (!file) throw new Error(`No se encontró el binario esperado (${p.file}) en la salida de arduino-cli.`);
      outFiles = outFiles.filter(f => f !== file);
      const data = await readFile(path.join(outDir, file));
      parts.push({ address: p.address, data: data.toString('base64') });
    }
    return { ok: true, parts };
  } finally {
    await rm(dir, { recursive: true, force: true });
  }
}

const server = http.createServer(async (req, res) => {
  if (req.method !== 'POST' || req.url !== '/compile') { res.writeHead(404).end(); return; }
  if (req.headers.authorization !== `Bearer ${SECRET}`) { res.writeHead(401).end('unauthorized'); return; }
  let raw = '';
  req.on('data', c => { raw += c; if (raw.length > 2_000_000) req.destroy(); });
  req.on('end', async () => {
    try {
      const { board, sketch } = JSON.parse(raw);
      if (!board || !sketch) throw new Error('Faltan board o sketch.');
      const result = await compile(board, sketch);
      res.writeHead(200, { 'content-type': 'application/json' }).end(JSON.stringify(result));
    } catch (e) {
      const message = e?.stderr ? String(e.stderr).slice(-4000) : (e instanceof Error ? e.message : String(e));
      res.writeHead(200, { 'content-type': 'application/json' }).end(JSON.stringify({ ok: false, error: message }));
    }
  });
});

server.listen(PORT, () => console.log(`K-ode IoT compiler escuchando en :${PORT}`));
