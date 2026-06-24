#!/usr/bin/env node
/**
 * Smart Campus Ecosystem – Web Server
 * Cairo University · Faculty of Computers and AI
 *
 * Zero npm dependencies – runs with plain: node server.js
 * Data persists to campus_resources.json, campus_users.json, campus_transactions.log
 */

const http = require('http');
const fs   = require('fs');
const path = require('path');
const url  = require('url');

const PORT = 3000;
const DATA_DIR = path.join(__dirname, 'data');

// ─── Ensure data directory ───────────────────────────────────────────────────
if (!fs.existsSync(DATA_DIR)) fs.mkdirSync(DATA_DIR);

const RESOURCES_FILE    = path.join(DATA_DIR, 'campus_resources.json');
const USERS_FILE        = path.join(DATA_DIR, 'campus_users.json');
const TRANSACTIONS_FILE = path.join(DATA_DIR, 'campus_transactions.log');

// ─── In-memory state ─────────────────────────────────────────────────────────
let resources = [];      // { id, name, category, price, stock, extra }
let users     = [];      // { campusID, name, role }
let nextId    = 1;
let nextOrderId = 1;

// ─── Persistence ─────────────────────────────────────────────────────────────
function loadData() {
  try {
    if (fs.existsSync(RESOURCES_FILE)) {
      resources = JSON.parse(fs.readFileSync(RESOURCES_FILE, 'utf8'));
      if (resources.length)
        nextId = Math.max(...resources.map(r => r.id)) + 1;
      console.log(`[DataManager] Loaded ${resources.length} resource(s).`);
    }
  } catch (e) { console.error('[DataManager] Could not load resources:', e.message); resources = []; }

  try {
    if (fs.existsSync(USERS_FILE)) {
      users = JSON.parse(fs.readFileSync(USERS_FILE, 'utf8'));
      console.log(`[DataManager] Loaded ${users.length} user(s).`);
    }
  } catch (e) { console.error('[DataManager] Could not load users:', e.message); users = []; }

  try {
    if (fs.existsSync(TRANSACTIONS_FILE)) {
      const lines = fs.readFileSync(TRANSACTIONS_FILE, 'utf8').trim().split('\n').filter(Boolean);
      if (lines.length) {
        const last = lines[lines.length - 1];
        const m = last.match(/^Order#(\d+)/);
        if (m) nextOrderId = parseInt(m[1]) + 1;
      }
    }
  } catch (_) {}
}

function saveData() {
  try { fs.writeFileSync(RESOURCES_FILE, JSON.stringify(resources, null, 2)); } catch (e) { console.error(e.message); }
  try { fs.writeFileSync(USERS_FILE, JSON.stringify(users, null, 2)); } catch (e) { console.error(e.message); }
}

function logTransaction(entry) {
  try { fs.appendFileSync(TRANSACTIONS_FILE, entry + '\n'); } catch (_) {}
}

// ─── Helpers ─────────────────────────────────────────────────────────────────
function daysUntilExpiry(dateStr) {
  const d = new Date(dateStr);
  if (isNaN(d)) return Infinity;
  return Math.round((d - Date.now()) / 86400000);
}

function isValidCard(num) {
  return /^\d{16}$/.test(num);
}

function getDiscount(role) {
  return role === 'Staff' ? 0.10 : 0.0;
}

function jsonResponse(res, status, obj) {
  const body = JSON.stringify(obj);
  res.writeHead(status, {
    'Content-Type': 'application/json',
    'Access-Control-Allow-Origin': '*',
    'Content-Length': Buffer.byteLength(body)
  });
  res.end(body);
}

function readBody(req) {
  return new Promise((resolve, reject) => {
    let data = '';
    req.on('data', c => data += c);
    req.on('end', () => {
      try { resolve(data ? JSON.parse(data) : {}); }
      catch (e) { reject(e); }
    });
    req.on('error', reject);
  });
}

// ─── Auto-save every 60 s ────────────────────────────────────────────────────
setInterval(() => { saveData(); console.log('[AutoSave] Data saved.'); }, 60000);

// ─── Background monitors ──────────────────────────────────────────────────────
// Low-stock every 30 s
setInterval(() => {
  const low = resources.filter(r => r.stock <= 5);
  if (low.length) {
    console.log('\n[LOW STOCK ALERT]');
    low.forEach(r => console.log(`  ! ${r.name} — only ${r.stock} unit(s) left`));
  }
}, 30000);

// Expiry every 60 s
setInterval(() => {
  const warn = resources
    .filter(r => r.category === 'Cafeteria Perishables')
    .map(r => ({ name: r.name, days: daysUntilExpiry(r.extra) }))
    .filter(w => w.days <= 3);
  if (warn.length) {
    console.log('\n[EXPIRY ALERT]');
    warn.forEach(w => {
      if      (w.days < 0) console.log(`  ! ${w.name}: EXPIRED ${Math.abs(w.days)} day(s) ago!`);
      else if (w.days === 0) console.log(`  ! ${w.name}: EXPIRES TODAY!`);
      else console.log(`  ! ${w.name}: expires in ${w.days} day(s)`);
    });
  }
}, 60000);

// ─── Route handlers ───────────────────────────────────────────────────────────
const routes = {};

function route(method, path, handler) {
  routes[`${method} ${path}`] = handler;
}

// GET /api/resources
route('GET', '/api/resources', (req, res) => {
  jsonResponse(res, 200, resources);
});

// GET /api/resources/:id
route('GET', '/api/resources/:id', (req, res, params) => {
  const r = resources.find(r => r.id === parseInt(params.id));
  if (!r) return jsonResponse(res, 404, { error: `No resource with ID ${params.id}` });
  jsonResponse(res, 200, r);
});

// POST /api/resources
route('POST', '/api/resources', async (req, res) => {
  const b = await readBody(req);
  const { name, price, stock, type, extra } = b;
  if (!name || !name.trim()) return jsonResponse(res, 400, { error: 'Name cannot be empty.' });
  if (!price || price <= 0)  return jsonResponse(res, 400, { error: 'Price must be positive.' });
  if (stock < 0)             return jsonResponse(res, 400, { error: 'Stock cannot be negative.' });

  const cats = { 'Lab Hardware': 'Lab Hardware', 'Cafeteria Perishables': 'Cafeteria Perishables', 'Bookstore Media': 'Bookstore Media' };
  const category = cats[type] || type;
  const id = nextId++;
  const resource = { id, name: name.trim(), category, price: parseFloat(price), stock: parseInt(stock), extra: extra || '' };
  resources.push(resource);
  saveData();
  jsonResponse(res, 201, resource);
});

// PATCH /api/resources/:id/restock
route('PATCH', '/api/resources/:id/restock', async (req, res, params) => {
  const b = await readBody(req);
  const r = resources.find(r => r.id === parseInt(params.id));
  if (!r) return jsonResponse(res, 404, { error: `Resource ID ${params.id} not found.` });
  const qty = parseInt(b.qty);
  if (!qty || qty <= 0) return jsonResponse(res, 400, { error: 'Quantity must be a positive integer.' });
  r.stock += qty;
  saveData();
  jsonResponse(res, 200, r);
});

// GET /api/users
route('GET', '/api/users', (req, res) => {
  jsonResponse(res, 200, users);
});

// POST /api/users
route('POST', '/api/users', async (req, res) => {
  const b = await readBody(req);
  const { name, campusID, role } = b;
  if (!name || !name.trim())       return jsonResponse(res, 400, { error: 'Name cannot be empty.' });
  if (!campusID || !campusID.trim()) return jsonResponse(res, 400, { error: 'Campus ID cannot be empty.' });
  if (role !== 'Student' && role !== 'Staff') return jsonResponse(res, 400, { error: 'Role must be Student or Staff.' });
  if (users.find(u => u.campusID === campusID.trim()))
    return jsonResponse(res, 409, { error: `Campus ID '${campusID}' already registered.` });
  const user = { campusID: campusID.trim(), name: name.trim(), role };
  users.push(user);
  saveData();
  jsonResponse(res, 201, user);
});

// GET /api/users/:campusID
route('GET', '/api/users/:campusID', (req, res, params) => {
  const u = users.find(u => u.campusID === params.campusID);
  if (!u) return jsonResponse(res, 404, { error: `No user with ID '${params.campusID}'` });
  jsonResponse(res, 200, u);
});

// POST /api/orders/checkout
route('POST', '/api/orders/checkout', async (req, res) => {
  const b = await readBody(req);
  const { campusID, delivery, items, paymentMethod, cardNumber } = b;

  const user = users.find(u => u.campusID === campusID);
  if (!user) return jsonResponse(res, 404, { error: `User '${campusID}' not found.` });

  if (!items || !items.length) return jsonResponse(res, 400, { error: 'Cart is empty.' });

  if (paymentMethod === 'Card') {
    if (!isValidCard(cardNumber))
      return jsonResponse(res, 400, { error: 'Card number must be exactly 16 numeric digits.' });
  }

  // Validate stock
  for (const item of items) {
    const r = resources.find(r => r.id === item.id);
    if (!r) return jsonResponse(res, 404, { error: `Resource ID ${item.id} not found.` });
    if (r.stock < item.qty) return jsonResponse(res, 400, { error: `Insufficient stock for '${r.name}' (have ${r.stock}, need ${item.qty}).` });
  }

  // Deduct stock
  let rawSubtotal = 0;
  const lineItems = [];
  for (const item of items) {
    const r = resources.find(r => r.id === item.id);
    r.stock -= item.qty;
    const sub = r.price * item.qty;
    rawSubtotal += sub;
    lineItems.push({ id: r.id, name: r.name, qty: item.qty, price: r.price, subtotal: sub });
  }

  const discount   = rawSubtotal * getDiscount(user.role);
  const deliveryFee = delivery === 'Dorm Delivery' ? 5.00 : 0.00;
  const total      = rawSubtotal - discount + deliveryFee;
  const orderId    = nextOrderId++;

  saveData();

  const now = new Date().toISOString();
  logTransaction(
    `Order#${orderId} | ${user.name} (${user.campusID}) | EGP ${total.toFixed(2)} | ${paymentMethod} | ${now}`
  );

  jsonResponse(res, 200, {
    orderId, user, lineItems, rawSubtotal, discount, deliveryFee, total,
    paymentMethod, delivery, timestamp: now
  });
});

// GET /api/transactions
route('GET', '/api/transactions', (req, res) => {
  try {
    const log = fs.existsSync(TRANSACTIONS_FILE)
      ? fs.readFileSync(TRANSACTIONS_FILE, 'utf8').trim().split('\n').filter(Boolean)
      : [];
    jsonResponse(res, 200, log);
  } catch (e) { jsonResponse(res, 500, { error: e.message }); }
});

// DELETE /api/resources/:id
route('DELETE', '/api/resources/:id', (req, res, params) => {
  const idx = resources.findIndex(r => r.id === parseInt(params.id));
  if (idx === -1) return jsonResponse(res, 404, { error: `No resource with ID ${params.id}` });
  const removed = resources.splice(idx, 1)[0];
  saveData();
  jsonResponse(res, 200, { message: `Resource '${removed.name}' removed.`, removed });
});

// DELETE /api/users/:campusID
route('DELETE', '/api/users/:campusID', (req, res, params) => {
  const idx = users.findIndex(u => u.campusID === params.campusID);
  if (idx === -1) return jsonResponse(res, 404, { error: `No user with ID '${params.campusID}'` });
  const removed = users.splice(idx, 1)[0];
  saveData();
  jsonResponse(res, 200, { message: `User '${removed.name}' removed.`, removed });
});

// GET /api/compare?id1=X&id2=Y
route('GET', '/api/compare', (req, res) => {
  const q = url.parse(req.url, true).query;
  const a = resources.find(r => r.id === parseInt(q.id1));
  const b = resources.find(r => r.id === parseInt(q.id2));
  if (!a || !b) return jsonResponse(res, 404, { error: 'One or both resource IDs not found.' });
  let msg;
  if (a.price > b.price)      msg = `${a.name} (EGP ${a.price.toFixed(2)}) costs more than ${b.name} (EGP ${b.price.toFixed(2)})`;
  else if (b.price > a.price) msg = `${b.name} (EGP ${b.price.toFixed(2)}) costs more than ${a.name} (EGP ${a.price.toFixed(2)})`;
  else                        msg = `Equal cost — EGP ${a.price.toFixed(2)}`;
  jsonResponse(res, 200, { a, b, comparison: msg });
});

// ─── Dynamic route matching ───────────────────────────────────────────────────
function matchRoute(method, pathname) {
  for (const key of Object.keys(routes)) {
    const [m, p] = key.split(' ');
    if (m !== method) continue;
    const parts  = p.split('/');
    const actual = pathname.split('/');
    if (parts.length !== actual.length) continue;
    const params = {};
    let ok = true;
    for (let i = 0; i < parts.length; i++) {
      if (parts[i].startsWith(':')) params[parts[i].slice(1)] = actual[i];
      else if (parts[i] !== actual[i]) { ok = false; break; }
    }
    if (ok) return { handler: routes[key], params };
  }
  return null;
}

// ─── HTTP server ──────────────────────────────────────────────────────────────
const server = http.createServer((req, res) => {
  const { pathname } = url.parse(req.url);

  // CORS pre-flight
  if (req.method === 'OPTIONS') {
    res.writeHead(204, { 'Access-Control-Allow-Origin': '*', 'Access-Control-Allow-Methods': '*', 'Access-Control-Allow-Headers': '*' });
    return res.end();
  }

  // Serve the frontend
  if (req.method === 'GET' && (pathname === '/' || pathname === '/index.html')) {
    const htmlPath = path.join(__dirname, 'index.html');
    if (!fs.existsSync(htmlPath)) {
      res.writeHead(404); return res.end('index.html not found');
    }
    const html = fs.readFileSync(htmlPath);
    res.writeHead(200, { 'Content-Type': 'text/html' });
    return res.end(html);
  }

  // API routes
  const match = matchRoute(req.method, pathname);
  if (match) {
    Promise.resolve(match.handler(req, res, match.params)).catch(e => {
      jsonResponse(res, 500, { error: e.message });
    });
    return;
  }

  jsonResponse(res, 404, { error: 'Not found' });
});

loadData();
server.listen(PORT, () => {
  console.log(`\n╔════════════════════════════════════════╗`);
  console.log(`║   Smart Campus Ecosystem  – Running    ║`);
  console.log(`║   http://localhost:${PORT}                ║`);
  console.log(`║   Cairo University · Faculty of CS&AI  ║`);
  console.log(`╚════════════════════════════════════════╝\n`);
});

// Graceful shutdown
process.on('SIGINT', () => {
  saveData();
  console.log('\n[Shutdown] Data saved. Goodbye!');
  process.exit(0);
});
