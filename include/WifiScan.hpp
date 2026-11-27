#pragma once
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_err.h>
#include <atomic>

// Use Arduino's result/event ownership, but retain esp_wifi_scan_start's error.
// No framework patch and no second consumer of the SDK's scan result list.
class DiagnosticWifiScan : public WiFiScanClass, protected WiFiGenericClass {
 public:
  esp_err_t start() {
    scanDelete();
    wifi_scan_config_t config{};
    config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
    config.scan_time.active.min = 100;
    config.scan_time.active.max = 160;
    _scanAsync = true;
    _scanTimeout = 15000;
    _scanStarted = millis() ? millis() : 1;
    WiFiGenericClass::clearStatusBits(WIFI_SCAN_DONE_BIT);
    WiFiGenericClass::setStatusBits(WIFI_SCANNING_BIT);
    const esp_err_t error = esp_wifi_scan_start(&config, false);
    if (error != ESP_OK) {
      _scanStarted = 0;
      WiFiGenericClass::clearStatusBits(WIFI_SCANNING_BIT);
    }
    return error;
  }
  void cancel() {
    esp_wifi_scan_stop();
    _scanStarted = 0;
    WiFiGenericClass::clearStatusBits(WIFI_SCANNING_BIT);
    scanDelete();
  }
};

class WifiSurvey {
 public:
  enum class State { Idle, Queued, Settling, Scanning, Ready, Failed };
  State state = State::Idle;
  esp_err_t error = ESP_OK;
  const char *phase = "idle";
  uint32_t attempts = 0, started = 0, duration = 0;
  int count = 0;
  std::atomic<uint32_t> eventStatus{UINT32_MAX};

  bool busy() const {
    return state == State::Queued || state == State::Settling ||
           state == State::Scanning;
  }
  const char *stateName() const {
    switch (state) {
      case State::Queued: case State::Settling: case State::Scanning: return "running";
      case State::Ready: return "ready";
      case State::Failed: return "failed";
      default: return "idle";
    }
  }
  bool request() {
    if (busy()) return false;
    state = State::Queued;
    phase = "prepare";
    error = ESP_OK;
    eventStatus.store(UINT32_MAX);
    attempts = duration = 0;
    count = 0;
    started = millis();
    return true;
  }
  // Called under the application's state lock by the web task, even if the
  // browser leaves. Automatic reconnect and the radio are always released.
  void service() {
    if (!busy()) return;
    const uint32_t now = millis();
    if (now - started >= 15000U) {
      scanner.cancel();
      finish(State::Failed, ESP_ERR_TIMEOUT, "timeout");
      return;
    }
    if (state == State::Queued) {
      if (!eventRegistered) {
        WiFi.onEvent([this](WiFiEvent_t, WiFiEventInfo_t info) {
          eventStatus.store(info.wifi_scan_done.status);
        }, ARDUINO_EVENT_WIFI_SCAN_DONE);
        eventRegistered = true;
      }
      previousAutoReconnect = WiFi.getAutoReconnect();
      WiFi.setAutoReconnect(false);
      radioOwned = true;
      // Keep AP+STA and the AP credentials intact. Abort only an unconnected
      // station's association attempt; never erase the stored station config.
      if (WiFi.status() != WL_CONNECTED) {
        const esp_err_t result = esp_wifi_disconnect();
        if (result != ESP_OK && result != ESP_ERR_WIFI_NOT_CONNECT) {
          finish(State::Failed, result, "disconnect");
          return;
        }
      }
      due = now + 300U; // allow pending station disconnect events to drain
      state = State::Settling;
      phase = "settle";
      return;
    }
    if (state == State::Settling) {
      if (int32_t(now - due) < 0) return;
      phase = "start";
      ++attempts;
      eventStatus.store(UINT32_MAX);
      const esp_err_t result = scanner.start();
      error = result;
      if (result == ESP_ERR_WIFI_STATE && attempts < 4) {
        due = now + 300U;
        return;
      }
      if (result != ESP_OK) {
        finish(State::Failed, result, "start");
        return;
      }
      state = State::Scanning;
      phase = "scan";
    }
    if (state == State::Scanning) {
      if (eventStatus.load() == UINT32_MAX) return;
      const int result = scanner.scanComplete();
      if (result == WIFI_SCAN_RUNNING) return;
      if (result == WIFI_SCAN_FAILED) {
        scanner.cancel();
        finish(State::Failed, ESP_ERR_TIMEOUT, "completion");
      } else if (eventStatus.load() != UINT32_MAX && eventStatus.load() != 0) {
        scanner.scanDelete();
        finish(State::Failed, ESP_FAIL, "scan_done");
      } else {
        count = result;
        finish(State::Ready, ESP_OK, "complete");
      }
    }
  }
 private:
  DiagnosticWifiScan scanner;
  bool previousAutoReconnect = false, radioOwned = false, eventRegistered = false;
  uint32_t due = 0;
  void finish(State next, esp_err_t result, const char *where) {
    state = next;
    phase = where;
    error = result;
    duration = millis() - started;
    if (radioOwned) {
      WiFi.setAutoReconnect(previousAutoReconnect);
      radioOwned = false;
    }
    Serial.printf("Wi-Fi scan: state=%s phase=%s error=%s (0x%X) event=%ld attempts=%lu duration=%lu ms mode=%u AP-clients=%u\r\n",
                  stateName(), phase, esp_err_to_name(error), unsigned(error),
                  eventStatus.load() == UINT32_MAX ? -1L : long(eventStatus.load()),
                  (unsigned long)attempts, (unsigned long)duration,
                  unsigned(WiFi.getMode()), unsigned(WiFi.softAPgetStationNum()));
  }
};
