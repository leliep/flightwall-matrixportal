#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <cstdint>
#include <cstring>

struct MqttSettings {
  String host, user, password;
  uint16_t port = 1883;
  String topics[4];
};

// Observe ACKs without changing PubSubClient's protocol implementation. Only
// the first eight body bytes are retained; PUBLISH bodies are never copied here.
struct MqttWireObserver {
  uint16_t pending[4] = {};
  int16_t subscription[4] = {-1,-1,-1,-1}; // -1 empty, -2 pending, -3 write failed
  int16_t connack = -1;
  uint32_t pongs = 0, malformed = 0;
  uint8_t phase = 0, header = 0, digits = 0, prefix[8] = {};
  uint32_t remaining = 0, multiplier = 1, position = 0;
  void byte(uint8_t value);
  void complete();
  void subscriptionWrite(int slot, const uint8_t *data, size_t length);
};

struct MqttReport {
  bool dnsOk = false, tcpOk = false, connected = false;
  int code = -1;
  char ip[16] = {}, clientId[32] = {};
  uint32_t dnsMs = 0, tcpMs = 0, connackMs = 0, connectedAtMs = 0;
  int16_t subscription[4] = {-1,-1,-1,-1};
  uint32_t received[4] = {}, valid[4] = {}, invalid[4] = {}, updated[4] = {};
  float values[4] = {};
  bool lastValid[4] = {};
  uint32_t pongs = 0;
};

const char *mqttStateText(int code);
void mqttClientId(char *out, size_t size, const uint8_t mac[6], bool test);

class MqttObservedClient : public Client {
 public:
  WiFiClient socket;
  MqttWireObserver wire;
  int subscriptionSlot = -1;
  bool writeFailed = false;
  int connect(IPAddress ip, uint16_t port) override { return socket.connect(ip,port,4000); }
  int connect(const char *host, uint16_t port) override { return socket.connect(host,port,4000); }
  size_t write(uint8_t value) override { return write(&value,1); }
  size_t write(const uint8_t *data, size_t size) override {
    wire.subscriptionWrite(subscriptionSlot,data,size);
    size_t written=socket.write(data,size);if(written!=size)writeFailed=true;return written;
  }
  int available() override { int n=socket.available(); if (!n) delay(1); return n; }
  int read() override { int c=socket.read(); if (c>=0) wire.byte(static_cast<uint8_t>(c)); return c; }
  int read(uint8_t *data, size_t size) override {
    int n=socket.read(data,size);for(int i=0;i<n;++i)wire.byte(data[i]);return n;
  }
  int peek() override { return socket.peek(); }
  void flush() override { socket.flush(); }
  void stop() override { socket.stop(); }
  uint8_t connected() override { return socket.connected(); }
  operator bool() override { return static_cast<bool>(socket); }
};

// One owner: the MQTT task. All methods involving sockets run without the
// application lock; only private settings, samples and diagnostics are touched.
class MqttSession {
 public:
  using Validator = bool (*)(uint8_t, const char *, float &);
  explicit MqttSession(Validator validator);
  MqttSession(const MqttSession&) = delete;
  MqttSession &operator=(const MqttSession&) = delete;
  MqttSettings settings;
  MqttReport report;
  bool connect(const char *id);
  bool step();
  void close();
  bool subscriptionsReady() const;
  bool subscriptionsDenied() const;
  bool allDataValid() const;
  bool bufferReady() const { return bufferOk_; }
 private:
  MqttObservedClient transport_;
  PubSubClient client_;
  Validator validator_;
  bool bufferOk_ = false;
  void syncReport();
  void message(char *topic, uint8_t *payload, unsigned length);
};
