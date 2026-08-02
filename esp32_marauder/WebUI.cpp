#include "WebUI.h"
#include "WiFiScan.h"
#include "CommandLine.h"
#include <ArduinoJson.h>

extern WiFiScan wifi_scan_obj;
extern CommandLine cli_obj;
extern LinkedList<BleDevice>* ble_devices;
#ifdef HAS_SD
  #include "SDInterface.h"
  extern SDInterface sd_obj;
#endif
#ifdef HAS_NEOPIXEL_LED
  extern LedInterface led_obj;
#endif

static const char WEBUI_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="tr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>FUSTOOL // control</title>
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
  button, select {
    background: transparent; color: var(--grn); border:1px solid var(--grn);
    padding:7px 12px; border-radius:3px; cursor:pointer; font-family:inherit; font-size:12px;
    margin-right:6px; margin-bottom:6px; text-transform:uppercase; letter-spacing:0.5px;
    transition: all .15s;
  }
  select { background: #020402; }
  select option { background: #020402; }
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
  #captureBar { display:none; }
  #captureBar.active { display:block; }
  .progress-outer { background:#020402; border:1px solid var(--grn-dim); border-radius:3px; height:14px; overflow:hidden; margin-top:6px; }
  .progress-inner { background:var(--grn); height:100%; width:0%; transition: width .3s; box-shadow: 0 0 8px var(--grn); }
</style>
</head>
<body>
<h1>&gt; FUSTOOL_CONTROL <span class="blink">_</span></h1>
<div class="sub">root@esp32-s3:~# uplink established</div>

<div class="panel">
  <h2>[ Sistem Monitoru ]</h2>
  <table>
    <tr><td>Sicaklik</td><td id="sysTemp">-</td></tr>
    <tr><td>Bos Bellek</td><td id="sysHeap">-</td></tr>
    <tr><td>CPU Frekansi</td><td id="sysFreq">-</td></tr>
    <tr><td>Ana Dongu Hizi (yuk gostergesi)</td><td id="sysLoopHz">-</td></tr>
    <tr><td>Calisma Suresi</td><td id="sysUptime">-</td></tr>
  </table>
  <div class="note">ESP32'de gercek "%CPU kullanimi" ozel bir ISP-IDF derlemesi gerektirir, bu SDK ile mumkun degil. Bunun yerine gercek sicaklik/bellek/frekans degerlerini ve dongu hizini gosteriyoruz - dongu hizi dusukse (norm. ~1000+ Hz) sistem o an tarama/saldiri gibi agir bir iş yapiyor demektir.</div>
  <div class="note">RGB LED: <b style="color:#00e5ff">mavi yanip sonme</b>=WiFi kesif, <b style="color:#ff00ff">mor yanip sonme</b>=Bluetooth kesif, <b style="color:#ffaa00">turuncu yanip sonme</b>=veri toplama, <b style="color:#ff3b3b">kirmizi yanip sonme</b>=saldiri, sonuk yesil=bosta.</div>
</div>

<div class="panel" id="captureBar">
  <h2>[ Aktif Islem ]</h2>
  <div id="captureLabel" class="status"></div>
  <div class="progress-outer"><div class="progress-inner" id="captureProgress"></div></div>
  <div id="captureTime" class="note"></div>
</div>

<div class="panel">
  <h2>[ WiFi Kesif ]</h2>
  <button onclick="startWifiDiscover()">baslat</button>
  <button class="danger" onclick="stopWifiDiscover()">durdur</button>
  <span id="wifiStatus" class="status"></span>
  <table id="wifiTable">
    <thead><tr><th>SSID</th><th>Sinyal</th><th>Kanal</th><th>Guvenlik</th></tr></thead>
    <tbody></tbody>
  </table>
  <div class="note">Bu tarama normal WiFi taramasi kullanir, bu ag'a baglantiniz kopmaz. Sadece "saldiri" veya "veri topla" butonlarina bastiginizda ESP32'nin tek radyosu hedef kanala gececegi icin baglanti birkac saniyeligine kesilir, otomatik geri doner.</div>
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
  <h2>[ Yakalanan Dosyalar ]</h2>
  <button onclick="loadFiles()">listeyi yenile</button>
  <table id="filesTable">
    <thead><tr><th>Dosya</th><th>Boyut</th><th></th></tr></thead>
    <tbody></tbody>
  </table>
  <div class="note">"veri topla" calisirken buraya dusen .pcap dosyasinin boyutu buyudukce gercek zamanli veri toplandigini gorursunuz (handshake dahil).</div>
</div>

<div class="panel">
  <h2>[ Komut Konsolu ]</h2>
  <input type="text" id="cmdInput" placeholder="orn: help" onkeydown="if(event.key==='Enter')sendCmd()">
  <button onclick="sendCmd()">calistir</button>
  <div id="cmdOut"></div>
  <div class="note">Marauder'in tum komutlari icin "help" yazip calistirin.</div>
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
    document.getElementById('wifiStatus').innerText = wifiData.length + ' ag bulundu' + (d.status === 'scanning' ? ' (taraniyor...)' : '');
    renderWifiTable();
  });
}

function renderWifiTable() {
  const tbody = document.querySelector('#wifiTable tbody');
  tbody.innerHTML = '';
  wifiData.forEach((n, i) => {
    const tr = document.createElement('tr');
    tr.className = 'dev-row' + (openIdx === i ? ' open' : '');
    tr.innerHTML = `<td>${n.ssid || '(gizli)'}</td><td>${n.rssi} dBm</td><td>${n.channel}</td><td>${n.enc}</td>`;
    tr.onclick = () => toggleDetail(i);
    tbody.appendChild(tr);

    if (openIdx === i) {
      const dr = document.createElement('tr');
      dr.className = 'detail-row';
      const td = document.createElement('td');
      td.colSpan = 4;
      td.innerHTML = `
        <div class="recon-field"><b>BSSID:</b> ${n.bssid}</div>
        <div class="detail-actions">
          <button onclick="wifiAction(${i}, 'lock')">veri topla (tum paketler, 12sn)</button>
        </div>
        <div class="detail-actions">
          <select id="atkType-${i}">
            <option value="deauth">Deauth (baglantiyi kopar)</option>
            <option value="probe">Probe Spam</option>
            <option value="rickroll">Rickroll Beacon Spam</option>
            <option value="funny">Komik SSID Beacon Spam</option>
          </select>
          <button class="danger" onclick="wifiAction(${i}, 'attack')">saldiri baslat (8sn)</button>
        </div>
        <div id="reconOut-${i}"></div>
        <div class="note">ESP32'nin TEK radyosu var: bu islem calisirken kendi agindan (bu paneli gordugunuz ag) dusersiniz - bu bir hata degil, fiziksel kisit. Islem bitince ESP32'nin agi geri gelir ama telefonunuz OTOMATIK baglanmaz (internetsiz ag oldugu icin) - WiFi ayarlarindan FUSTOOL_ESP32'yi elle tekrar secmeniz gerekir.</div>
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
  const n = wifiData[i];
  if (type === 'lock') {
    fetch('/api/wifi/lock?bssid=' + encodeURIComponent(n.bssid) + '&ch=' + n.channel)
      .then(() => { out.innerText = '-> kanal ' + n.channel + "'e kilitlendi, veri toplaniyor (baglanti bu sure boyunca kesik olabilir)"; });
  } else if (type === 'attack') {
    const atkType = document.getElementById('atkType-' + i).value;
    fetch('/api/wifi/attack?bssid=' + encodeURIComponent(n.bssid) + '&type=' + atkType)
      .then(() => { out.innerText = '-> ' + atkType + ' saldirisi baslatildi (baglanti bu sure boyunca kesik olabilir)'; });
  }
}

let btTimer = null;
function startBtScan() {
  fetch('/api/bt/start');
  document.getElementById('btStatus').innerText = 'taraniyor...';
  if (btTimer) clearInterval(btTimer);
  btTimer = setInterval(pollBt, 1200);
}
function stopBtScan() {
  fetch('/api/bt/stop');
  document.getElementById('btStatus').innerText = 'durduruldu';
  if (btTimer) clearInterval(btTimer);
}
function pollBt() {
  fetch('/api/bt/list').then(r => r.json()).then(d => {
    document.getElementById('btStatus').innerText = d.devices.length + ' cihaz bulundu (canli)';
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
  const out = document.getElementById('cmdOut');
  out.innerText = '> ' + cmd + '\n(gonderiliyor...)';
  fetch('/api/cmd', { method: 'POST', body: cmd })
    .then(r => r.text())
    .then(t => { out.innerText = '> ' + cmd + '\n(kuyruga alindi ve calistirildi: ' + t + ')\nCiktiyi gormek icin USB seri konsoluna bakin ya da "Yakalanan Dosyalar" panelini kontrol edin.'; })
    .catch(e => { out.innerText = '> ' + cmd + '\n(HATA: ' + e + ' - ESP32 agina bagli oldugunuzdan emin olun)'; });
  document.getElementById('cmdInput').value = '';
}

function fmtSize(b) {
  if (b > 1024*1024) return (b/1024/1024).toFixed(1) + ' MB';
  if (b > 1024) return (b/1024).toFixed(1) + ' KB';
  return b + ' B';
}

function loadFiles() {
  fetch('/api/files/list').then(r => r.json()).then(d => {
    const tbody = document.querySelector('#filesTable tbody');
    tbody.innerHTML = '';
    d.files.forEach(f => {
      const tr = document.createElement('tr');
      const nameCell = document.createElement('td');
      nameCell.innerText = f.name;
      const sizeCell = document.createElement('td');
      sizeCell.innerText = fmtSize(f.size);
      const actionCell = document.createElement('td');
      const dlBtn = document.createElement('a');
      dlBtn.href = '/api/files/download?name=' + encodeURIComponent(f.name);
      dlBtn.innerText = 'indir';
      dlBtn.style.color = 'var(--grn)';
      dlBtn.style.marginRight = '10px';
      const delBtn = document.createElement('button');
      delBtn.className = 'danger';
      delBtn.innerText = 'sil';
      delBtn.onclick = () => { fetch('/api/files/delete?name=' + encodeURIComponent(f.name)).then(loadFiles); };
      actionCell.appendChild(dlBtn);
      actionCell.appendChild(delBtn);
      tr.appendChild(nameCell);
      tr.appendChild(sizeCell);
      tr.appendChild(actionCell);
      tbody.appendChild(tr);
    });
  });
}

function pollCapture() {
  fetch('/api/capture/status').then(r => r.json()).then(d => {
    const bar = document.getElementById('captureBar');
    if (d.active) {
      bar.classList.add('active');
      document.getElementById('captureLabel').innerText = d.label;
      const pct = Math.min(100, Math.round((d.elapsed_ms / d.duration_ms) * 100));
      document.getElementById('captureProgress').style.width = pct + '%';
      document.getElementById('captureTime').innerText =
        'gecen: ' + (d.elapsed_ms/1000).toFixed(0) + 'sn / tahmini toplam: ' + (d.duration_ms/1000).toFixed(0) + 'sn';
      loadFiles();
    } else {
      bar.classList.remove('active');
    }
  });
}

function pollSystem() {
  fetch('/api/system/status').then(r => r.json()).then(d => {
    document.getElementById('sysTemp').innerText = d.temp_c.toFixed(1) + ' C';
    document.getElementById('sysHeap').innerText = fmtSize(d.free_heap) + ' / ' + fmtSize(d.total_heap);
    document.getElementById('sysFreq').innerText = d.cpu_mhz + ' MHz';
    document.getElementById('sysLoopHz').innerText = d.loop_hz + ' Hz';
    const u = d.uptime_s;
    document.getElementById('sysUptime').innerText = Math.floor(u/3600) + 's ' + Math.floor((u%3600)/60) + 'd ' + (u%60) + 'sn';
  });
}

loadFiles();
setInterval(loadFiles, 5000);
setInterval(pollCapture, 1000);
setInterval(pollSystem, 2000);
pollSystem();
</script>
</body>
</html>
)=====";

void WebUI::queueCommand(String cmd) {
  if (cmd_queue == NULL) return;
  char buf[WEBUI_CMD_MAXLEN];
  cmd.toCharArray(buf, WEBUI_CMD_MAXLEN);
  xQueueSend(cmd_queue, buf, 0);
}

void WebUI::ensureAP() {
  if (!(WiFi.getMode() & WIFI_MODE_AP)) {
    this->forceApRestart();
  }
}

void WebUI::forceApRestart() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(WEBUI_AP_SSID, WEBUI_AP_PASS);
}

void WebUI::begin() {
  cmd_queue = xQueueCreate(16, WEBUI_CMD_MAXLEN);

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
  server.on("/api/wifi/attack", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleWifiAttack(request);
  });
  server.on("/api/capture/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleCaptureStatus(request);
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
  server.on("/api/system/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleSystemStatus(request);
  });

  server.on("/api/files/list", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleFilesList(request);
  });
  server.on("/api/files/download", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleFilesDownload(request);
  });
  server.on("/api/files/delete", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleFilesDelete(request);
  });

  server.on("/api/cmd", HTTP_POST,
    [](AsyncWebServerRequest *request) { request->send(200, "text/plain", "queued"); },
    NULL,
    [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      this->handleCmd(request, data, len, index, total);
    }
  );

  server.begin();
}

void WebUI::loop() {
  loop_count++;
  uint32_t now0 = millis();
  if (now0 - last_loop_hz_calc >= 1000) {
    loop_hz = loop_count;
    loop_count = 0;
    last_loop_hz_calc = now0;
  }

  // Drain queued commands on the main task (NOT the async TCP task) since the
  // WiFi/BT drivers are not safe to poke from arbitrary FreeRTOS tasks.
  // Uses a FreeRTOS queue (not a manually-locked list) since that's the
  // correct primitive for handing data between the AsyncTCP task and the
  // main loop task.
  char buf[WEBUI_CMD_MAXLEN];
  while (cmd_queue != NULL && xQueueReceive(cmd_queue, buf, 0) == pdTRUE) {
    cli_obj.runCommand(String(buf));
  }

  // Same reasoning: WiFi.scanNetworks()/scanDelete() must run here, not in
  // the HTTP handler's AsyncTCP task context.
  if (pending_scan_stop) {
    pending_scan_stop = false;
    WiFi.scanDelete();
  }
  if (pending_scan_start) {
    pending_scan_start = false;
    if (WiFi.scanComplete() != -1) { // don't restart an already-running scan
      WiFi.scanNetworks(true /* async */, false, false, 300, 0);
    }
  }

  // If a bounded capture/attack was scheduled, auto-stop it so the AP
  // (and the client's connection to this UI) reliably comes back without
  // needing a request that can no longer reach us.
  if (auto_stop_at != 0 && (int32_t)(millis() - auto_stop_at) > 0) {
    auto_stop_at = 0;
    capture_started_at = 0;
    is_attack = false;
    this->queueCommand("stopscan");
    // Give "stopscan" a moment to actually run (it's still sitting in
    // cmd_queue right now) before we force the AP back - then force it
    // unconditionally rather than trusting the AP-mode-bit check, since
    // that bit can be set without the SSID/password/DHCP actually being
    // re-applied.
    force_ap_restart_at = millis() + 1000;
  }

  if (force_ap_restart_at != 0 && (int32_t)(millis() - force_ap_restart_at) > 0) {
    force_ap_restart_at = 0;
    this->forceApRestart();
  }

  static uint32_t last_check = 0;
  uint32_t now = millis();
  if (now - last_check > 3000) {
    last_check = now;
    if (!wifi_scan_obj.scanning()) {
      this->ensureAP();
    }
  }

  this->updateActivityLed();
}

void WebUI::scheduleAutoStop(uint32_t ms_from_now, String label) {
  auto_stop_at = millis() + ms_from_now;
  capture_started_at = millis();
  capture_duration_ms = ms_from_now;
  capture_label = label;
}

// Blinks the onboard RGB LED with a color that tells you what the radio is
// currently doing: cyan = WiFi discovery, magenta = Bluetooth discovery,
// orange = capturing packets, red = attack in progress, dim green = idle.
void WebUI::updateActivityLed() {
  #ifdef HAS_NEOPIXEL_LED
    uint32_t now = millis();
    bool active = false;
    int r = 0, g = 0, b = 0;

    if (is_attack) {
      active = true; r = 255; g = 0; b = 0;
    } else if (capture_started_at != 0) {
      active = true; r = 255; g = 120; b = 0;
    } else if (bt_scanning) {
      active = true; r = 200; g = 0; b = 200;
    } else if (WiFi.scanComplete() == -1) {
      active = true; r = 0; g = 180; b = 255;
    }

    if (!active) {
      led_state_on = false;
      led_obj.setMode(MODE_CUSTOM);
      led_obj.setColor(0, 10, 0);
      return;
    }

    if (now - last_led_toggle > 300) {
      last_led_toggle = now;
      led_state_on = !led_state_on;
      led_obj.setMode(MODE_CUSTOM);
      led_obj.setColor(led_state_on ? r : 0, led_state_on ? g : 0, led_state_on ? b : 0);
    }
  #endif
}

void WebUI::handleSystemStatus(AsyncWebServerRequest *request) {
  DynamicJsonDocument doc(256);
  doc["temp_c"] = temperatureRead();
  doc["free_heap"] = ESP.getFreeHeap();
  doc["total_heap"] = ESP.getHeapSize();
  doc["cpu_mhz"] = ESP.getCpuFreqMHz();
  doc["uptime_s"] = millis() / 1000;
  doc["loop_hz"] = loop_hz;
  String out;
  serializeJson(doc, out);
  request->send(200, "application/json", out);
}

void WebUI::handleRoot(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", WEBUI_HTML);
}

// WiFi Kesif uses the plain Arduino WiFi.scanNetworks() API instead of
// Marauder's own promiscuous scan engine. This is deliberate: Marauder's
// engine calls esp_wifi_set_mode(WIFI_MODE_NULL, ...) to sniff, which tears
// down the softAP entirely - anyone connected to this dashboard over WiFi
// would be disconnected and unable to reach the UI again. scanNetworks()
// works fine alongside an active softAP (WIFI_AP_STA), so this listing
// never drops the connection.
void WebUI::handleWifiDiscoverStart(AsyncWebServerRequest *request) {
  pending_scan_start = true;
  request->send(200, "text/plain", "started");
}

void WebUI::handleWifiDiscoverStop(AsyncWebServerRequest *request) {
  pending_scan_stop = true;
  request->send(200, "text/plain", "stopped");
}

void WebUI::handleWifiList(AsyncWebServerRequest *request) {
  int n = WiFi.scanComplete();
  DynamicJsonDocument doc(8192);
  doc["status"] = (n == -1) ? "scanning" : "done";
  JsonArray arr = doc.createNestedArray("networks");
  if (n > 0) {
    for (int i = 0; i < n; i++) {
      JsonObject o = arr.createNestedObject();
      o["ssid"] = WiFi.SSID(i);
      o["bssid"] = WiFi.BSSIDstr(i);
      o["rssi"] = WiFi.RSSI(i);
      o["channel"] = WiFi.channel(i);
      o["enc"] = wifi_scan_obj.security_int_to_string((int)WiFi.encryptionType(i));
    }
  }
  String out;
  serializeJson(doc, out);
  request->send(200, "application/json", out);
}

// These two DO use Marauder's real attack/capture engine, which means they
// DO briefly take the radio out of AP mode (unavoidable: ESP32 has one
// radio). They're bounded and auto-stop (see loop()/scheduleAutoStop) so
// the dashboard connection always recovers on its own.
void WebUI::handleWifiLock(AsyncWebServerRequest *request) {
  if (request->hasParam("ch")) {
    int ch = request->getParam("ch")->value().toInt();
    this->queueCommand("channel -s " + String(ch));
    // scanall captures beacons, probes, deauths and EAPOL/handshake frames
    // on the locked channel - i.e. "all the network's data", not just beacons.
    this->queueCommand("scanall");
    is_attack = false;
    this->scheduleAutoStop(12000, "Veri toplaniyor (kanal " + String(ch) + ")");
  }
  request->send(200, "text/plain", "ok");
}

void WebUI::handleWifiAttack(AsyncWebServerRequest *request) {
  if (request->hasParam("bssid")) {
    String bssid = request->getParam("bssid")->value();
    String type = request->hasParam("type") ? request->getParam("type")->value() : "deauth";
    if (type == "deauth") {
      this->queueCommand("attack -t deauth -s " + bssid);
    } else if (type == "probe") {
      this->queueCommand("attack -t probe");
    } else if (type == "rickroll") {
      this->queueCommand("attack -t rickroll");
    } else if (type == "funny") {
      this->queueCommand("attack -t funny");
    }
    is_attack = true;
    this->scheduleAutoStop(8000, type + " saldirisi");
  }
  request->send(200, "text/plain", "ok");
}

void WebUI::handleCaptureStatus(AsyncWebServerRequest *request) {
  DynamicJsonDocument doc(256);
  bool active = capture_started_at != 0;
  doc["active"] = active;
  if (active) {
    doc["label"] = capture_label;
    doc["elapsed_ms"] = millis() - capture_started_at;
    doc["duration_ms"] = capture_duration_ms;
  }
  String out;
  serializeJson(doc, out);
  request->send(200, "application/json", out);
}

void WebUI::handleBtScanStart(AsyncWebServerRequest *request) {
  this->queueCommand("sniffbt");
  bt_scanning = true;
  request->send(200, "text/plain", "started");
}

void WebUI::handleBtScanStop(AsyncWebServerRequest *request) {
  this->queueCommand("stopscan");
  bt_scanning = false;
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

fs::FS* WebUI::captureFS() {
  #ifdef HAS_SD
    if (sd_obj.supported) return &SD;
  #endif
  return &FFat;
}

void WebUI::handleFilesList(AsyncWebServerRequest *request) {
  DynamicJsonDocument doc(4096);
  JsonArray arr = doc.createNestedArray("files");
  fs::FS* fs = this->captureFS();
  File root = fs->open("/");
  if (root && root.isDirectory()) {
    File f = root.openNextFile();
    while (f) {
      if (!f.isDirectory()) {
        JsonObject o = arr.createNestedObject();
        String n = String(f.name());
        if (!n.startsWith("/")) n = "/" + n;
        o["name"] = n;
        o["size"] = f.size();
      }
      f = root.openNextFile();
    }
  }
  String out;
  serializeJson(doc, out);
  request->send(200, "application/json", out);
}

void WebUI::handleFilesDownload(AsyncWebServerRequest *request) {
  if (!request->hasParam("name")) {
    request->send(400, "text/plain", "missing name");
    return;
  }
  String name = request->getParam("name")->value();
  if (!name.startsWith("/")) name = "/" + name;
  fs::FS* fs = this->captureFS();
  if (!fs->exists(name)) {
    request->send(404, "text/plain", "not found");
    return;
  }
  request->send(*fs, name, "application/octet-stream", true);
}

void WebUI::handleFilesDelete(AsyncWebServerRequest *request) {
  if (request->hasParam("name")) {
    String name = request->getParam("name")->value();
    if (!name.startsWith("/")) name = "/" + name;
    this->captureFS()->remove(name);
  }
  request->send(200, "text/plain", "ok");
}

void WebUI::handleCmd(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  static String cmd_body_buffer;
  if (index == 0) cmd_body_buffer = "";
  for (size_t i = 0; i < len; i++) cmd_body_buffer += (char)data[i];
  if (index + len == total) {
    this->queueCommand(cmd_body_buffer);
  }
}
