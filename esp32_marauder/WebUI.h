#pragma once

#ifndef WebUI_h
#define WebUI_h

#include "configs.h"
#include "ESPAsyncWebServer.h"
#include <AsyncTCP.h>
#include <WiFi.h>
#include <LinkedList.h>
#include <FS.h>
#ifndef HAS_SD
  #include <FFat.h>
#endif
#ifdef HAS_NEOPIXEL_LED
  #include "LedInterface.h"
#endif

// ---- Change these to whatever you want the network to look like ----
#define WEBUI_AP_SSID "FUSTOOL_ESP32"
#define WEBUI_AP_PASS "12345678910"   // WPA2, min 8 chars
#define WEBUI_PORT 8080
// ----------------------------------------------------------------------

#define WEBUI_CMD_MAXLEN 160

class WebUI {
  private:
    AsyncWebServer server{WEBUI_PORT};
    QueueHandle_t cmd_queue = NULL;
    uint32_t auto_stop_at = 0;
    uint32_t capture_started_at = 0;
    uint32_t capture_duration_ms = 0;
    String capture_label = "";
    bool is_attack = false;
    bool bt_scanning = false;
    bool led_state_on = false;
    uint32_t last_led_toggle = 0;
    uint32_t loop_count = 0;
    uint32_t loop_hz = 0;
    uint32_t last_loop_hz_calc = 0;
    // WiFi.* calls (scanNetworks/scanDelete/softAP) must only ever run from
    // the main loop task, never directly from an AsyncTCP callback - the
    // ESP32 WiFi driver isn't safe to poke from another task and doing so
    // can hard-hang the whole chip with no crash log at all. These flags
    // defer that work into loop(), same as the command queue does.
    volatile bool pending_scan_start = false;
    volatile bool pending_scan_stop = false;
    // Set after queuing "stopscan" post-capture/attack; a bit later we force
    // a full WiFi.mode(WIFI_AP_STA)+softAP(ssid,pass) unconditionally, since
    // just checking "is the AP mode bit set" isn't enough to guarantee the
    // actual SSID/password/DHCP got re-applied correctly.
    uint32_t force_ap_restart_at = 0;

    void ensureAP();
    void forceApRestart();
    void queueCommand(String cmd);
    void scheduleAutoStop(uint32_t ms_from_now, String label);
    void updateActivityLed();
    void handleSystemStatus(AsyncWebServerRequest *request);

    void handleRoot(AsyncWebServerRequest *request);

    void handleWifiDiscoverStart(AsyncWebServerRequest *request);
    void handleWifiDiscoverStop(AsyncWebServerRequest *request);
    void handleWifiList(AsyncWebServerRequest *request);
    void handleWifiLock(AsyncWebServerRequest *request);
    void handleWifiAttack(AsyncWebServerRequest *request);
    void handleCaptureStatus(AsyncWebServerRequest *request);

    void handleBtScanStart(AsyncWebServerRequest *request);
    void handleBtScanStop(AsyncWebServerRequest *request);
    void handleBtList(AsyncWebServerRequest *request);

    void handleCmd(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
    void handleStatus(AsyncWebServerRequest *request);

    fs::FS* captureFS();
    void handleFilesList(AsyncWebServerRequest *request);
    void handleFilesDownload(AsyncWebServerRequest *request);
    void handleFilesDelete(AsyncWebServerRequest *request);

  public:
    void begin();
    void loop();
};

#endif
