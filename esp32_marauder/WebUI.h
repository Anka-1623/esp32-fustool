#pragma once

#ifndef WebUI_h
#define WebUI_h

#include "configs.h"
#include "ESPAsyncWebServer.h"
#include <AsyncTCP.h>
#include <WiFi.h>
#include <LinkedList.h>

// ---- Change these to whatever you want the network to look like ----
#define WEBUI_AP_SSID "ESP32_Marauder_AP"
#define WEBUI_AP_PASS "12345678910"   // WPA2, min 8 chars
#define WEBUI_PORT 8080
// ----------------------------------------------------------------------

class WebUI {
  private:
    AsyncWebServer server{WEBUI_PORT};
    LinkedList<String>* cmd_queue;
    portMUX_TYPE cmd_mux = portMUX_INITIALIZER_UNLOCKED;

    void ensureAP();
    void queueCommand(String cmd);

    void handleRoot(AsyncWebServerRequest *request);

    void handleWifiDiscoverStart(AsyncWebServerRequest *request);
    void handleWifiDiscoverStop(AsyncWebServerRequest *request);
    void handleWifiList(AsyncWebServerRequest *request);
    void handleWifiLock(AsyncWebServerRequest *request);
    void handleWifiInfo(AsyncWebServerRequest *request);
    void handleWifiAttack(AsyncWebServerRequest *request);

    void handleBtScanStart(AsyncWebServerRequest *request);
    void handleBtScanStop(AsyncWebServerRequest *request);
    void handleBtList(AsyncWebServerRequest *request);

    void handleCmd(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
    void handleStatus(AsyncWebServerRequest *request);

  public:
    void begin();
    void loop();
};

#endif
