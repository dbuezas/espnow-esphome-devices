//
// Original Author: Phong Vu. https://github.com/iphong/lib-esp-rc/blob/3569f91a17817edcf6ad19c09679086c2a0d46ca/EspRC.h
// Modified here: https://github.com/dbuezas/espnow-esphome-devices
// Changes: 
//  * removed "channel" parameter
//  * fixed bug that only allows for 1 listener
//  * added raw logging of handled and unhandled messages
#ifndef __ESP_RC_H__
#define __ESP_RC_H__

#include <esp8266wifi.h>
#include <espnow.h>

u8 broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef std::function<void()> esp_rc_cb_t;

struct esp_rc_listener_t {
  String prefix;
  esp_rc_cb_t callback;
};

class EspRCClass;

extern EspRCClass EspRC;

class EspRCClass {
 public:
  esp_rc_listener_t listeners[255];
  u8 _listeners_num;
  u8 *_sender;
  u8 *_data;
  u8 _size;
  u32 _sentTime;
  bool _isSending;

  static bool equals(u8 *a, u8 *b, u8 size, u8 offset = 0) {
    for (auto i = offset; i < offset + size; i++) {
      if (a[i] != b[i]) {
        return false;
      }
    }
    return true;
  }
  u8 *getSender() {
    return _sender;
  }
  u8 *getBytes() {
    return _data;
  }
  String getValue() {
    String d = "";
    for (auto i = 0; i < _size; i++) d.concat((const char)_data[i]);
    return d;
  }
  void send(String data, u8 *receiver = broadcast) {
    while (_isSending && _sentTime + 5000 > micros()) {
      delay(1);
    }
    _isSending = true;
    _sentTime = micros();
    esp_now_send(receiver, (u8 *)data.c_str(), (u8)data.length());
  }
  void send(String data, String value) {
    send(String(data + value));
  }
  void send(String data, double value) {
    send(String(data + String(value)));
  }
  void on(String prefix, esp_rc_cb_t callback) {
    listeners[_listeners_num++] = {prefix, callback};
  }
  void end() {
    esp_now_del_peer(broadcast);
    esp_now_unregister_recv_cb();
    esp_now_deinit();
  }
  void begin() {
    if (WiFi.getMode() == WIFI_OFF) {
      WiFi.mode(WIFI_AP_STA);
    }
    if (esp_now_init() == OK) {
      esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
      if (esp_now_is_peer_exist(broadcast))
        esp_now_del_peer(broadcast);
      esp_now_add_peer(broadcast, ESP_NOW_ROLE_COMBO, 0, 0, 0);

      esp_now_register_send_cb([](u8 *receiver, u8 err) {
        EspRC._isSending = false;
      });
      esp_now_register_recv_cb([](u8 *sender, u8 *data, u8 size) {
        bool handled = false;
        for (auto i = 0; i < EspRC._listeners_num; i++) {
          esp_rc_listener_t listener = EspRC.listeners[i];
          u8 *prefix = (u8 *)listener.prefix.c_str();
          u8 len = listener.prefix.length();
          if (equals(prefix, data, len)) {
            if (listener.callback) {
              EspRC._data = data + len;
              EspRC._size = size - len;
              listener.callback();
              handled = true;
            }
          }
        }
        if (handled) {
          ESP_LOGW("ESPNow", "handled OK: %.*s", size, data);
        }else {
          ESP_LOGW("ESPNow", "ignored !!: %.*s", size, data);
        }
      });
    }
  }

} EspRC;

#endif  //__ESP_RC_H__
