#!/usr/bin/env node
/**
 * Local API + static host for scrcpy GUI.
 * Endpoints:
 *   GET  /api/scan-devices
 *   GET  /api/sessions
 *   POST /api/start   { commands: string[] }
 *   POST /api/stop    { serial?: string }  // omit = stop all
 *   GET  /api/presets
 *   POST /api/presets { name, config, command }
 *   DELETE /api/presets/:id
 */
import http from 'node:http';
import { spawn, execFile } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import { fileURLToPath } from 'node:url';
import { randomUUID } from 'node:crypto';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const PORT = Number(process.env.SCRCPY_GUI_PORT || 3000);
const DIST = path.join(__dirname, 'dist');

const sessions = new Map(); // serial -> { proc, command, startedAt }

function configDir() {
  if (process.platform === 'darwin') {
    return path.join(os.homedir(), 'Library', 'Application Support', 'scrcpy');
  }
  if (process.platform === 'win32') {
    return path.join(process.env.APPDATA || path.join(os.homedir(), 'AppData', 'Roaming'), 'scrcpy');
  }
  return path.join(process.env.XDG_CONFIG_HOME || path.join(os.homedir(), '.config'), 'scrcpy');
}

function presetsPath() {
  return path.join(configDir(), 'gui-presets.json');
}

function ensureConfigDir() {
  fs.mkdirSync(configDir(), { recursive: true });
}

function loadPresets() {
  try {
    const raw = fs.readFileSync(presetsPath(), 'utf8');
    const data = JSON.parse(raw);
    return Array.isArray(data) ? data : [];
  } catch {
    return [];
  }
}

function savePresets(list) {
  ensureConfigDir();
  fs.writeFileSync(presetsPath(), JSON.stringify(list, null, 2));
}

function findBinary(name) {
  const envKey = name.toUpperCase();
  if (process.env[envKey]) return process.env[envKey];
  const candidates = [
    path.join(__dirname, '..', 'build-gui', 'app', name),
    path.join(__dirname, '..', 'dist', 'macos-portable', name),
    path.join(__dirname, name),
    name,
  ];
  if (process.platform === 'win32') {
    candidates.unshift(path.join(__dirname, '..', 'build-gui', 'app', `${name}.exe`));
  }
  for (const c of candidates) {
    if (c === name) return name;
    try {
      if (fs.existsSync(c) && fs.statSync(c).isFile()) return c;
    } catch {
      /* ignore */
    }
  }
  return name;
}

function adbPath() {
  return process.env.ADB || findBinary('adb');
}

function scrcpyPath() {
  return process.env.SCRCPY || findBinary('scrcpy');
}

function parseAdbDevices(stdout) {
  const lines = stdout.split(/\r?\n/).slice(1);
  const devices = [];
  for (const line of lines) {
    const trimmed = line.trim();
    if (!trimmed) continue;
    const parts = trimmed.split(/\s+/);
    const serial = parts[0];
    const state = parts[1] || 'unknown';
    if (!serial || state === 'List') continue;
    let model = '';
    const modelMatch = trimmed.match(/model:(\S+)/);
    if (modelMatch) model = modelMatch[1].replace(/_/g, ' ');
    const tcpip = serial.includes(':') ? serial : '';
    devices.push({ serial, state, model, tcpip });
  }
  return devices;
}

function scanDevices() {
  return new Promise((resolve, reject) => {
    execFile(adbPath(), ['devices', '-l'], { timeout: 15000 }, (err, stdout, stderr) => {
      if (err) {
        reject(new Error(stderr?.toString() || err.message));
        return;
      }
      resolve(parseAdbDevices(stdout.toString()));
    });
  });
}

function tokenizeCommand(cmd) {
  // Simple shell-ish split respecting quotes
  const tokens = [];
  let cur = '';
  let quote = null;
  for (let i = 0; i < cmd.length; i++) {
    const ch = cmd[i];
    if (quote) {
      if (ch === quote) quote = null;
      else cur += ch;
      continue;
    }
    if (ch === '"' || ch === "'") {
      quote = ch;
      continue;
    }
    if (/\s/.test(ch)) {
      if (cur) {
        tokens.push(cur);
        cur = '';
      }
      continue;
    }
    cur += ch;
  }
  if (cur) tokens.push(cur);
  return tokens;
}

function extractSerial(tokens) {
  for (let i = 0; i < tokens.length; i++) {
    if (tokens[i] === '-s' && tokens[i + 1]) return tokens[i + 1];
    if (tokens[i].startsWith('--serial=')) return tokens[i].slice('--serial='.length);
  }
  return `session-${randomUUID().slice(0, 8)}`;
}

function startCommand(command) {
  const tokens = tokenizeCommand(command.trim());
  if (!tokens.length) throw new Error('Empty command');
  if (tokens[0] === 'scrcpy') tokens[0] = scrcpyPath();

  const serial = extractSerial(tokens);
  // Stop existing session for same serial
  stopSession(serial);

  const proc = spawn(tokens[0], tokens.slice(1), {
    stdio: ['ignore', 'pipe', 'pipe'],
    env: { ...process.env },
    detached: false,
  });

  const entry = {
    serial,
    command,
    pid: proc.pid,
    startedAt: Date.now(),
    logs: [],
    proc,
  };
  const pushLog = (buf) => {
    const text = buf.toString();
    for (const line of text.split(/\r?\n/)) {
      if (line) {
        entry.logs.push(line);
        if (entry.logs.length > 500) entry.logs.shift();
      }
    }
  };
  proc.stdout.on('data', pushLog);
  proc.stderr.on('data', pushLog);
  proc.on('exit', (code) => {
    entry.exitCode = code;
    entry.alive = false;
    // keep for a while in map for UI status
  });
  entry.alive = true;
  sessions.set(serial, entry);
  return { serial, pid: proc.pid, command };
}

function stopSession(serial) {
  const entry = sessions.get(serial);
  if (!entry) return false;
  try {
    if (entry.proc && entry.alive !== false) {
      entry.proc.kill('SIGTERM');
      setTimeout(() => {
        try {
          if (!entry.proc.killed) entry.proc.kill('SIGKILL');
        } catch {
          /* ignore */
        }
      }, 2000);
    }
  } catch {
    /* ignore */
  }
  sessions.delete(serial);
  return true;
}

function stopAll() {
  for (const serial of [...sessions.keys()]) stopSession(serial);
}

function readBody(req) {
  return new Promise((resolve, reject) => {
    const chunks = [];
    req.on('data', (c) => chunks.push(c));
    req.on('end', () => {
      const raw = Buffer.concat(chunks).toString('utf8');
      if (!raw) return resolve({});
      try {
        resolve(JSON.parse(raw));
      } catch (e) {
        reject(e);
      }
    });
    req.on('error', reject);
  });
}

function sendJson(res, status, obj) {
  const body = JSON.stringify(obj);
  res.writeHead(status, {
    'Content-Type': 'application/json',
    'Access-Control-Allow-Origin': '*',
    'Access-Control-Allow-Methods': 'GET,POST,DELETE,OPTIONS',
    'Access-Control-Allow-Headers': 'Content-Type',
  });
  res.end(body);
}

function contentType(file) {
  if (file.endsWith('.html')) return 'text/html; charset=utf-8';
  if (file.endsWith('.js')) return 'text/javascript; charset=utf-8';
  if (file.endsWith('.css')) return 'text/css; charset=utf-8';
  if (file.endsWith('.svg')) return 'image/svg+xml';
  if (file.endsWith('.png')) return 'image/png';
  if (file.endsWith('.json')) return 'application/json';
  return 'application/octet-stream';
}

function serveStatic(req, res) {
  let urlPath = decodeURIComponent(req.url.split('?')[0]);
  if (urlPath === '/') urlPath = '/index.html';
  const file = path.normalize(path.join(DIST, urlPath));
  if (!file.startsWith(DIST)) {
    res.writeHead(403);
    res.end('Forbidden');
    return;
  }
  if (!fs.existsSync(file) || fs.statSync(file).isDirectory()) {
    // SPA fallback
    const index = path.join(DIST, 'index.html');
    if (fs.existsSync(index)) {
      res.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8' });
      fs.createReadStream(index).pipe(res);
      return;
    }
    res.writeHead(404);
    res.end('Not found. Run npm run build first, or use npm run dev.');
    return;
  }
  res.writeHead(200, { 'Content-Type': contentType(file) });
  fs.createReadStream(file).pipe(res);
}

const server = http.createServer(async (req, res) => {
  const url = req.url.split('?')[0];
  const method = req.method || 'GET';

  if (method === 'OPTIONS') {
    sendJson(res, 204, {});
    return;
  }

  try {
    if (method === 'GET' && url === '/api/health') {
      sendJson(res, 200, { ok: true, scrcpy: scrcpyPath(), adb: adbPath() });
      return;
    }

    if (method === 'GET' && url === '/api/scan-devices') {
      const devices = await scanDevices();
      sendJson(res, 200, { devices });
      return;
    }

    if (method === 'GET' && url === '/api/sessions') {
      const list = [...sessions.values()].map((s) => ({
        serial: s.serial,
        command: s.command,
        pid: s.pid,
        startedAt: s.startedAt,
        alive: s.alive !== false,
        exitCode: s.exitCode ?? null,
        logs: s.logs.slice(-50),
      }));
      sendJson(res, 200, { sessions: list });
      return;
    }

    if (method === 'POST' && url === '/api/start') {
      const body = await readBody(req);
      const commands = body.commands || (body.command ? [body.command] : []);
      if (!commands.length) {
        sendJson(res, 400, { error: 'No commands provided' });
        return;
      }
      const started = [];
      for (const cmd of commands) {
        started.push(startCommand(cmd));
      }
      sendJson(res, 200, { started });
      return;
    }

    if (method === 'POST' && url === '/api/stop') {
      const body = await readBody(req);
      if (body.serial) {
        sendJson(res, 200, { stopped: stopSession(body.serial) });
      } else {
        stopAll();
        sendJson(res, 200, { stopped: true });
      }
      return;
    }

    if (method === 'GET' && url === '/api/presets') {
      sendJson(res, 200, { presets: loadPresets() });
      return;
    }

    if (method === 'POST' && url === '/api/presets') {
      const body = await readBody(req);
      if (!body.name || !body.config) {
        sendJson(res, 400, { error: 'name and config required' });
        return;
      }
      const list = loadPresets();
      const now = new Date().toISOString();
      const preset = {
        id: randomUUID(),
        name: String(body.name).trim(),
        config: body.config,
        command: body.command || '',
        created_at: now,
        updated_at: now,
      };
      list.unshift(preset);
      savePresets(list);
      sendJson(res, 200, { preset });
      return;
    }

    if (method === 'DELETE' && url.startsWith('/api/presets/')) {
      const id = decodeURIComponent(url.slice('/api/presets/'.length));
      const list = loadPresets().filter((p) => p.id !== id);
      savePresets(list);
      sendJson(res, 200, { ok: true });
      return;
    }

    if (url.startsWith('/api/')) {
      sendJson(res, 404, { error: 'Unknown API route' });
      return;
    }

    serveStatic(req, res);
  } catch (err) {
    sendJson(res, 500, { error: err.message || String(err) });
  }
});

server.listen(PORT, () => {
  console.log(`scrcpy GUI server on http://localhost:${PORT}`);
  console.log(`  adb:    ${adbPath()}`);
  console.log(`  scrcpy: ${scrcpyPath()}`);
});

function shutdown() {
  stopAll();
  server.close(() => process.exit(0));
}
process.on('SIGINT', shutdown);
process.on('SIGTERM', shutdown);
