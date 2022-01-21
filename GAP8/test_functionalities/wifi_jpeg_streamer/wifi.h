#pragma once

#include <stdint.h>

typedef enum {
  SET_SSID = 0x10,
  SET_KEY = 0x11,
  SET_SOFTAP = 0x12,

  WIFI_CONNECT = 0x20,
  
  STATUS_WIFI_CONNECTED = 0x31,
  STATUS_CLIENT_CONNECTED = 0x32
} __attribute__((packed)) WiFiCTRLType;

typedef struct {
  WiFiCTRLType cmd;
  uint8_t data[50];
} __attribute__((packed)) WiFiCTRLPacket_t;