#pragma once
#ifndef _SUPABASE_REALTIME_CLIENT_H_
#define _SUPABASE_REALTIME_CLIENT_H_

#include <Arduino.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

class SupabaseRealtimeClient {
public:
  static constexpr size_t JSON_DOC_SIZE = 4096;

  enum State {
    STATE_DISCONNECTED = 0,
    STATE_CONNECTING,
    STATE_CONNECTED,
    STATE_JOINED,
    STATE_ERROR
  };

  enum LogLevel {
    LOG_DEBUG = 0,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_NONE
  };

  struct Change {
    long long id = -1;
    String message;
    String sender;
    String update_at;
    String commit_timestamp;
    String table;
    String type;
    String schema;
    String old_record_json;
  };

  typedef void (*ChangeCallback)(const Change& change);
  typedef void (*StatusCallback)(State state, const String& reason);
  typedef void (*RawJsonCallback)(const JsonDocument& doc);

  SupabaseRealtimeClient()
  : _refCounter(2),
    _heartbeatIntervalMs(25000),
    _lastHeartbeat(0),
    _messageCb(nullptr),
    _statusCb(nullptr),
    _rawJsonCb(nullptr),
    _state(STATE_DISCONNECTED),
    _connectAttempts(0),
    _lastConnectMillis(0),
    _lastDisconnectMillis(0),
    _reconnectDelayMs(5000),
    _useSSL(true),
    _logLevel(LOG_INFO)
  {}

  void init(const String& host, const String& apikey, bool useSSL=true, uint16_t port=443) {
    _host = host;
    _apikey = apikey;
    _useSSL = useSSL;
    _port = port;
    _path = "/realtime/v1/websocket?apikey=" + _apikey + "&vsn=1.0.0";
  }

  void begin() {
    log(LOG_INFO, "begin() called, starting websocket");
    setState(STATE_CONNECTING, "begin()");
    if (_useSSL) _ws.beginSSL(_host.c_str(), _port, _path.c_str());
    else _ws.begin(_host.c_str(), _port, _path.c_str());

    _instance = this;
    _ws.onEvent(_staticEventCallback);
    _ws.setReconnectInterval(_reconnectDelayMs);
  }

  void loop() {
    _ws.loop();
    unsigned long now = millis();

    if (now - _lastHeartbeat >= _heartbeatIntervalMs) {
      sendHeartbeat();
      _lastHeartbeat = now;
    }
  }

  void close() {
    log(LOG_INFO, "close() called");
    _ws.disconnect();
  }

  void setChangeCallback(ChangeCallback cb) { _messageCb = cb; }
  void setStatusCallback(StatusCallback cb) { _statusCb = cb; }
  void setRawJsonCallback(RawJsonCallback cb) { _rawJsonCb = cb; }

  void setHeartbeatInterval(unsigned long ms) { _heartbeatIntervalMs = ms; }
  void setReconnectDelay(unsigned long ms) {
    _reconnectDelayMs = ms;
    _ws.setReconnectInterval(_reconnectDelayMs);
  }

  void setLogLevel(LogLevel level) { _logLevel = level; }

  State getState() const { return _state; }
  String getStateString() const {
    switch (_state) {
      case STATE_CONNECTING: return "CONNECTING";
      case STATE_CONNECTED:  return "CONNECTED";
      case STATE_JOINED:     return "JOINED";
      case STATE_ERROR:      return "ERROR";
      default:               return "DISCONNECTED";
    }
  }
  String getLastError() const { return _lastError; }
  unsigned long getConnectAttempts() const { return _connectAttempts; }
  unsigned long getLastConnectMillis() const { return _lastConnectMillis; }
  unsigned long getLastDisconnectMillis() const { return _lastDisconnectMillis; }
  bool isConnected() const { return _state == STATE_CONNECTED || _state == STATE_JOINED; }
  bool isJoined() const { return _state == STATE_JOINED; }

private:
  WebSocketsClient _ws;
  String _host;
  String _apikey;
  String _path;
  uint16_t _port;
  unsigned long _refCounter;
  unsigned long _heartbeatIntervalMs;
  unsigned long _lastHeartbeat;
  ChangeCallback _messageCb;
  StatusCallback _statusCb;
  RawJsonCallback _rawJsonCb;
  State _state;
  String _lastError;
  unsigned long _connectAttempts;
  unsigned long _lastConnectMillis;
  unsigned long _lastDisconnectMillis;
  unsigned long _reconnectDelayMs;
  bool _useSSL;
  LogLevel _logLevel;

  static SupabaseRealtimeClient* _instance;

  void log(LogLevel level, const String& msg) const {
    if (level < _logLevel || _logLevel == LOG_NONE) return;
    const char* prefix = "";
    switch (level) {
      case LOG_DEBUG: prefix = "DEBUG"; break;
      case LOG_INFO:  prefix = "INFO "; break;
      case LOG_WARN:  prefix = "WARN "; break;
      case LOG_ERROR: prefix = "ERROR"; break;
      default: prefix = "LOG "; break;
    }
    Serial.print("[");
    Serial.print(prefix);
    Serial.print("] ");
    Serial.println(msg);
  }

  void setState(State st, const String& reason = String()) {
    if (st == _state && reason.length() == 0) return;
    _state = st;
    if (st == STATE_ERROR && reason.length() > 0) _lastError = reason;
    if (st == STATE_CONNECTED) {
      _connectAttempts++;
      _lastConnectMillis = millis();
      _lastError = String();
    }
    if (st == STATE_DISCONNECTED) {
      _lastDisconnectMillis = millis();
    }
    if (_statusCb) {
      _statusCb(_state, reason);
    }
    if (reason.length()) log(LOG_INFO, String("State -> ") + getStateString() + " : " + reason);
    else log(LOG_INFO, String("State -> ") + getStateString());
  }

  void sendJoinMessage() {
    String joinMessage = R"({
      "topic":"realtime:public",
      "event":"phx_join",
      "ref":"1",
      "payload":{
        "config":{
          "broadcast":{"self":false},
          "postgres_changes":[{
            "event":"UPDATE",
            "schema":"public",
            "table":"messages"
          }]
        }
      }
    })";
    log(LOG_DEBUG, "Enviando phx_join");
    _ws.sendTXT(joinMessage);
  }

  void sendHeartbeat() {
    String heartbeat = String("{") +
      "\"topic\":\"realtime:public\"," +
      "\"event\":\"phx_heartbeat\"," +
      "\"payload\":{}," +
      "\"ref\":\"" + String(_refCounter++) + "\"" +
      "}";
    log(LOG_DEBUG, "Enviando heartbeat ref=" + String(_refCounter-1));
    _ws.sendTXT(heartbeat);
  }

  // Parsing con ArduinoJson v7: usar JsonDocument (no DynamicJsonDocument)
  void handleTextPayload(const char* payload, size_t length) {
    log(LOG_DEBUG, "Texto recibido, size=" + String(length));

    // JsonDocument ahora es el tipo recomendado en v7 (capacidad elástica).
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload, length);
    if (err) {
      String errStr = String("deserializeJson failed: ") + err.c_str();
      log(LOG_WARN, errStr);
      return;
    }

    if (_rawJsonCb) {
      _rawJsonCb(doc);
    }

    // event (si existe)
    const char* event = doc["event"] | nullptr;
    if (event != nullptr) {
      String ev(event);
      if (ev == "phx_reply") {
        const char* ref = doc["ref"] | nullptr;
        if (ref && String(ref) == "1") {
          const char* status = doc["payload"]["status"] | nullptr;
          if (status && String(status) == "ok") {
            setState(STATE_JOINED, "phx_join OK");
            log(LOG_INFO, "phx_join confirmado (JOINED).");
          } else {
            String reason = "phx_join reply not OK";
            setState(STATE_ERROR, reason);
            log(LOG_WARN, reason);
          }
        }
      } else if (ev == "system") {
        log(LOG_INFO, "system event recibido");
      }
    }

    // Procesar postgres_changes
    if (doc["event"].is<const char*>() && String(doc["event"].as<const char*>()) == "postgres_changes") {
      JsonObject data = doc["payload"]["data"].as<JsonObject>();
      if (!data.isNull()) {
        Change ch;
        ch.table = String(data["table"] | "");
        ch.type = String(data["type"] | "");
        ch.schema = String(data["schema"] | "");
        ch.commit_timestamp = String(data["commit_timestamp"] | "");

        JsonObject record = data["record"].as<JsonObject>();
        if (!record.isNull()) {
          // Evitar containsKey (deprecated): usar is<>() o isNull()
          if (!record["id"].isNull()) {
            // as<long long>() funciona si el valor es numérico o string numérico
            ch.id = record["id"].as<long long>();
          }
          ch.message = String(record["message"] | "");
          ch.sender = String(record["sender"] | "");
          ch.update_at = String(record["update_at"] | "");
        }

        // old_record: serializar directamente si existe
        if (!data["old_record"].isNull()) {
          String oldJson;
          serializeJson(data["old_record"], oldJson);
          ch.old_record_json = oldJson;
        }

        log(LOG_INFO, String("Cambio detectado: table=") + ch.table + " type=" + ch.type +
            " id=" + String(ch.id) + " message=" + ch.message);

        if (_messageCb) _messageCb(ch);
      } else {
        log(LOG_WARN, "postgres_changes sin campo payload.data");
      }
    }
  }

  static void _staticEventCallback(WStype_t type, uint8_t* payload, size_t length) {
    if (_instance) _instance->_eventCallback(type, payload, length);
  }

  void _eventCallback(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
      case WStype_CONNECTED:
        setState(STATE_CONNECTED, "WebSocket connected");
        log(LOG_INFO, "WebSocket conectado");
        sendJoinMessage();
        break;

      case WStype_DISCONNECTED:
        setState(STATE_DISCONNECTED, "WebSocket disconnected");
        log(LOG_WARN, "WebSocket desconectado");
        break;

      case WStype_TEXT:
        handleTextPayload((const char*)payload, length);
        break;

      case WStype_ERROR:
        {
          String reason;
          if (payload && length) reason = String((const char*)payload, length);
          else reason = "WebSocket error (no payload)";
          setState(STATE_ERROR, reason);
          log(LOG_ERROR, "WebSocket ERROR: " + reason);
        }
        break;

      case WStype_PONG:
        log(LOG_DEBUG, "PONG recibido");
        break;

      default:
        break;
    }
  }
};

SupabaseRealtimeClient* SupabaseRealtimeClient::_instance = nullptr;

#endif