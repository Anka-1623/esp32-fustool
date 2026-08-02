#include "WebUI.h"
#include "WiFiScan.h"
#include "CommandLine.h"
#include <ArduinoJson.h>

extern WiFiScan wifi_scan_obj;
extern CommandLine cli_obj;
extern LinkedList<BleDevice>* ble_devices;
extern LinkedList<AccessPoint>* access_points;

static const char WEBUI_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="tr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>MARAUDER // control</title>
<style>
  :root { --grn:#00ff6a; --grn-dim:#0a5c30; --bg:#050705; --panel:#0b100c; --red:#ff3b3b; }
  * { box-sizing: border-box; }
  body {
    font-family: 'Courier New', ui-monospace, monospace;
    background: var(--bg); color: var(--grn); margin:0; padding:16px;
    background-image: radial-gradient(circle at 50% 0%, #071a0d 0%, #050705 70%);
  }
  h1 { font-size:18px; letter-spacing:2px; margin:0 0 4px 0; text-shadow: 0 0 6px var(--grn); }
  .sub { color:#5a9; font-size:12px; margin-bottom:16px; }
  .panel {
    background: var(--panel); border:1px solid var(--grn-dim); border-radius:4px;
    padding:12px; margin-bottom:16px; box-shadow: 0 0 12px rgba(0,255,106,0.08);
  }
  .panel h2 { font-size:13px; margin:0 0 10px 0; text-transform:uppercase; letter-spacing:1px; color:var(--grn); border-bottom:1px dashed var(--grn-dim); padding-bottom:6px; }
  button {
    background: transparent; color: var(--grn); border:1px solid var(--grn);
    padding:7px 12px; border-radius:3px; cursor:pointer; font-family:inherit; font-size:12px;
    margin-right:6px; margin-bottom:6px; text-transform:uppercase; letter-spacing:0.5px;
    transition: all .15s;
  }
  button:hover { background: var(--grn); color:#000; box-shadow:0 0 10px var(--grn); }
  button.danger { border-color: var(--red); color: var(--red); }
  button.danger:hover { background: var(--red); color:#000; box-shadow:0 0 10px var(--red); }
  table { width:100%; border-collapse: collapse; margin-top:10px; font-size:12px; }
  th, td { text-align:left; padding:6px 4px; border-bottom:1px solid #123; }
  th { color:#5a9; font-weight:normal; text-transform:uppercase; font-size:11px; }
  tr.dev-row { cursor:pointer; }
  tr.dev-row:hover { background: rgba(0,255,106,0.06); }
  tr.dev-row.open { background: rgba(0,255,106,0.1); }
  .detail-row td { background:#020402; border-bottom:1px solid var(--grn-dim); padding:10px; }
  .detail-actions button { min-width:110px; }
  input[type=text] {
    width:70%; padding:8px; border-radius:3px; border:1px solid var(--grn-dim);
    background:#020402; color:var(--grn); font-family:inherit;
  }
  .note { color:#5a9; font-size:11px; margin-top:8px; }
  #cmdOut { background:#020402; padding:8px; border-radius:3px; margin-top:8px; font-size:12px; min-height:20px; white-space:pre-wrap; border:1px solid var(--grn-dim); }
  .status { font-size:11px; color:#5a9; margin-left:8px; }
  .recon-field { font-size:12px; margin:2px 0; }
  .recon-field b { color:var(--grn); }
  .blink { animation: blink 1.2s infinite; }
  @keyframes blink { 50% { opacity: 0.2; } }
</style>
</head>
<body>
<h1>&gt; MARAUDER_CONTROL <span class="blink">_</span></h1>
<div class="sub">root@esp32-s3:~# uplink established</div>

<div class="panel">
  <h2>[ WiFi Kesif ]</h2>
  <button onclick="startWifiDiscover()">baslat</button>
  <button class="danger" onclick="stopWifiDiscover()">durdur</button>
  <span id="wifiStatus" class="status"></span>
  <table id="wifiTable">
    <thead><tr><th>SSID</th><th>Sinyal</th><th>Kanal</th><th>Guvenlik</th><th>Uretici</th></tr></thead>
    <tbody></tbody>
  </table>
  <div class="note">Not: kesif calisirken (radyo kanal atlar) bu ag birkac saniyeligine dusebilir, durdurunca geri gelir.</div>
</div>

<div class="panel">
  <h2>[ Bluetooth Kesif ]</h2>
  <button onclick="startBtScan()">baslat</button>
  <button class="danger" onclick="stopBtScan()">durdur</button>
  <span id="btStatus" class="status"></span>
  <table id="btTable">
    <thead><tr><th>Isim</th><th>MAC</th><th>Sinyal</th></tr></thead>
    <tbody></tbody>
  </table>
</div>

<div class="panel">
  <h2>[ Komut Konsolu ]</h2>
  <input type="text" id="cmdInput" placeholder="orn: help" onkeydown="if(event.key==='Enter')sendCmd()">
  <button onclick="sendCmd()">calistir</button>
  <div id="cmdOut"></div>
</div>

<script>
let wifiData = [];
let openIdx = -1;

function startWifiDiscover() {
  document.getElementById('wifiStatus').innerText = 'taraniyor...';
  fetch('/api/wifi/discover/start');
  if (window.wifiTimer) clearInterval(window.wifiTimer);
  window.wifiTimer = setInterval(pollWifi, 1500);
}
function stopWifiDiscover() {
  fetch('/api/wifi/discover/stop');
  document.getElementById('wifiStatus').innerText = 'durduruldu';
  if (window.wifiTimer) clearInterval(window.wifiTimer);
}

function pollWifi() {
  fetch('/api/wifi/list').then(r => r.json()).then(d => {
    wifiData = d.networks;
    document.getElementById('wifiStatus').innerText = wifiData.length + ' ag bulundu';
    renderWifiTable();
  });
}

function renderWifiTable() {
  const tbody = document.querySelector('#wifiTable tbody');
  tbody.innerHTML = '';
  wifiData.forEach((n, i) => {
    const tr = document.createElement('tr');
    tr.className = 'dev-row' + (openIdx === i ? ' open' : '');
    tr.innerHTML = `<td>${n.essid || '(gizli)'}</td><td>${n.rssi} dBm</td><td>${n.channel}</td><td>${n.sec}</td><td>${n.man || '-'}</td>`;
    tr.onclick = () => toggleDetail(i);
    tbody.appendChild(tr);

    if (openIdx === i) {
      const dr = document.createElement('tr');
      dr.className = 'detail-row';
      const td = document.createElement('td');
      td.colSpan = 5;
      td.innerHTML = `
        <div class="detail-actions">
          <button onclick="wifiAction(${i}, 'lock')">veri topla</button>
          <button onclick="wifiAction(${i}, 'info')">kesif</button>
          <button class="danger" onclick="wifiAction(${i}, 'attack')">saldiri</button>
        </div>
        <div id="reconOut-${i}"></div>
      `;
      dr.appendChild(td);
      tbody.appendChild(dr);
    }
  });
}

function toggleDetail(i) {
  openIdx = (openIdx === i) ? -1 : i;
  renderWifiTable();
}

function wifiAction(i, type) {
  const out = document.getElementById('reconOut-' + i);
  if (type === 'info') {
    fetch('/api/wifi/info?idx=' + i).then(r => r.json()).then(d => {
      out.innerHTML = `
        <div class="recon-field"><b>BSSID:</b> ${d.bssid}</div>
        <div class="recon-field"><b>Kanal:</b> ${d.channel}</div>
        <div class="recon-field"><b>Sinyal:</b> ${d.rssi} dBm</div>
        <div class="recon-field"><b>Guvenlik:</b> ${d.sec}</div>
        <div class="recon-field"><b>WPS:</b> ${d.wps ? 'evet' : 'hayir'}</div>
        <div class="recon-field"><b>Uretici:</b> ${d.man}</div>
        <div class="recon-field"><b>Paket sayisi:</b> ${d.packets}</div>
      `;
    });
  } else if (type === 'lock') {
    fetch('/api/wifi/lock?idx=' + i);
    out.innerText = '-> kanala kilitlendi, veri toplaniyor';
  } else if (type === 'attack') {
    fetch('/api/wifi/attack?idx=' + i);
    out.innerText = '-> deauth saldirisi gonderildi';
  }
}

let btTimer = null;
function startBtScan() {
  fetch('/api/bt/start');
  document.getElementById('btStatus').innerText = 'taraniyor...';
  if (btTimer) clearInterval(btTimer);
  btTimer = setInterval(pollBt, 1500);
}
function stopBtScan() {
  fetch('/api/bt/stop');
  document.getElementById('btStatus').innerText = 'durduruldu';
  if (btTimer) clearInterval(btTimer);
}
function pollBt() {
  fetch('/api/bt/list').then(r => r.json()).then(d => {
    document.getElementById('btStatus').innerText = d.devices.length + ' cihaz bulundu';
    const tbody = document.querySelector('#btTable tbody');
    tbody.innerHTML = '';
    d.devices.forEach(n => {
      const tr = document.createElement('tr');
      tr.innerHTML = `<td>${n.name || '(isimsiz)'}</td><td>${n.mac}</td><td>${n.rssi} dBm</td>`;
      tbody.appendChild(tr);
    });
  });
}

function sendCmd() {
  const cmd = document.getElementById('cmdInput').value;
  if (!cmd) return;
  document.getElementById('cmdOut').innerText = '> ' + cmd + '\n(kuyruga alindi, ana dongude calisacak)';
  fetch('/api/cmd', { method: 'POST', body: cmd });
  document.getElementById('cmdInput').value = '';
}
</script>
</body>
</html>
)=====";

void WebUI::queueCommand(String cmd) {
  portENTER_CRITICAL(&cmd_mux);
  cmd_queue->add(cmd);
  portEXIT_CRITICAL(&cmd_mux);
}

void WebUI::ensureAP() {
  if (!(WiFi.getMode() & WIFI_MODE_AP)) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(WEBUI_AP_SSID, WEBUI_AP_PASS);
  }
}

void WebUI::begin() {
  cmd_queue = new LinkedList<String>();

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(WEBUI_AP_SSID, WEBUI_AP_PASS);

  server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleRoot(request);
  });

  server.on("/api/wifi/discover/start", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleWifiDiscoverStart(request);
  });
  server.on("/api/wifi/discover/stop", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleWifiDiscoverStop(request);
  });
  server.on("/api/wifi/list", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleWifiList(request);
  });
  server.on("/api/wifi/lock", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleWifiLock(request);
  });
  server.on("/api/wifi/info", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleWifiInfo(request);
  });
  server.on("/api/wifi/attack", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleWifiAttack(request);
  });

  server.on("/api/bt/start", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleBtScanStart(request);
  });
  server.on("/api/bt/stop", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleBtScanStop(request);
  });
  server.on("/api/bt/list", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleBtList(request);
  });

  server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleStatus(request);
  });

  server.on("/api/cmd", HTTP_POST,
    [](AsyncWebServerRequest *request) { request->send(200, "text/plain", "ok"); },
    NULL,
    [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      this->handleCmd(request, data, len, index, total);
    }
  );

  server.begin();
}

void WebUI::loop() {
  // Drain queued commands on the main task (NOT the async TCP task) since the
  // WiFi/BT drivers are not safe to poke from arbitrary FreeRTOS tasks.
  while (true) {
    String cmd = "";
    bool got = false;
    portENTER_CRITICAL(&cmd_mux);
    if (cmd_queue->size() > 0) {
      cmd = cmd_queue->get(0);
      cmd_queue->remove(0);
      got = true;
    }
    portEXIT_CRITICAL(&cmd_mux);
    if (!got) break;
    cli_obj.runCommand(cmd);
  }

  static uint32_t last_check = 0;
  uint32_t now = millis();
  if (now - last_check > 3000) {
    last_check = now;
    if (!wifi_scan_obj.scanning()) {
      this->ensureAP();
    }
  }
}

void WebUI::handleRoot(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", WEBUI_HTML);
}

void WebUI::handleWifiDiscoverStart(AsyncWebServerRequest *request) {
  this->queueCommand("sniffbeacon");
  request->send(200, "text/plain", "started");
}

void WebUI::handleWifiDiscoverStop(AsyncWebServerRequest *request) {
  this->queueCommand("stopscan");
  request->send(200, "text/plain", "stopped");
}

void WebUI::handleWifiList(AsyncWebServerRequest *request) {
  DynamicJsonDocument doc(8192);
  JsonArray arr = doc.createNestedArray("networks");
  if (access_points != NULL) {
    for (int i = 0; i < access_points->size(); i++) {
      AccessPoint ap = access_points->get(i);
      JsonObject o = arr.createNestedObject();
      o["essid"] = ap.essid;
      o["rssi"] = ap.rssi;
      o["channel"] = ap.channel;
      o["sec"] = wifi_scan_obj.security_int_to_string(ap.sec);
      o["man"] = ap.man;
    }
  }
  String out;
  serializeJson(doc, out);
  request->send(200, "application/json", out);
}

void WebUI::handleWifiLock(AsyncWebServerRequest *request) {
  if (request->hasParam("idx")) {
    int idx = request->getParam("idx")->value().toInt();
    if (access_points != NULL && idx >= 0 && idx < access_points->size()) {
      AccessPoint ap = access_points->get(idx);
      this->queueCommand("channel -s " + String(ap.channel));
    }
  }
  request->send(200, "text/plain", "ok");
}

void WebUI::handleWifiInfo(AsyncWebServerRequest *request) {
  DynamicJsonDocument doc(1024);
  if (request->hasParam("idx")) {
    int idx = request->getParam("idx")->value().toInt();
    if (access_points != NULL && idx >= 0 && idx < access_points->size()) {
      AccessPoint ap = access_points->get(idx);
      char mac_str[18];
      sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X", ap.bssid[0], ap.bssid[1], ap.bssid[2], ap.bssid[3], ap.bssid[4], ap.bssid[5]);
      doc["bssid"] = mac_str;
      doc["channel"] = ap.channel;
      doc["rssi"] = ap.rssi;
      doc["sec"] = wifi_scan_obj.security_int_to_string(ap.sec);
      doc["wps"] = ap.wps;
      doc["man"] = ap.man;
      doc["packets"] = ap.packets;
    }
  }
  String out;
  serializeJson(doc, out);
  request->send(200, "application/json", out);
}

void WebUI::handleWifiAttack(AsyncWebServerRequest *request) {
  if (request->hasParam("idx")) {
    int idx = request->getParam("idx")->value().toInt();
    this->queueCommand("select -a " + String(idx));
    this->queueCommand("attack -t deauth");
  }
  request->send(200, "text/plain", "ok");
}

void WebUI::handleBtScanStart(AsyncWebServerRequest *request) {
  this->queueCommand("sniffbt");
  request->send(200, "text/plain", "started");
}

void WebUI::handleBtScanStop(AsyncWebServerRequest *request) {
  this->queueCommand("stopscan");
  request->send(200, "text/plain", "stopped");
}

void WebUI::handleBtList(AsyncWebServerRequest *request) {
  DynamicJsonDocument doc(4096);
  JsonArray arr = doc.createNestedArray("devices");
  if (ble_devices != NULL) {
    for (int i = 0; i < ble_devices->size(); i++) {
      BleDevice d = ble_devices->get(i);
      JsonObject o = arr.createNestedObject();
      char mac_str[18];
      sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X", d.mac[0], d.mac[1], d.mac[2], d.mac[3], d.mac[4], d.mac[5]);
      o["mac"] = mac_str;
      o["name"] = d.name;
      o["rssi"] = d.rssi;
    }
  }
  String out;
  serializeJson(doc, out);
  request->send(200, "application/json", out);
}

void WebUI::handleStatus(AsyncWebServerRequest *request) {
  DynamicJsonDocument doc(256);
  doc["scanning"] = wifi_scan_obj.scanning();
  doc["ap_ssid"] = WEBUI_AP_SSID;
  doc["ap_ip"] = WiFi.softAPIP().toString();
  String out;
  serializeJson(doc, out);
  request->send(200, "application/json", out);
}

void WebUI::handleCmd(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  static String cmd_body_buffer;
  if (index == 0) cmd_body_buffer = "";
  for (size_t i = 0; i < len; i++) cmd_body_buffer += (char)data[i];
  if (index + len == total) {
    this->queueCommand(cmd_body_buffer);
  }
}
