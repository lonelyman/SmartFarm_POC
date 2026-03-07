// include/infrastructure/WifiConfigStore.h
#pragma once

#include <Arduino.h>

// ============================================================
//  WifiConfigStore — โหลด/บันทึก WiFi config จาก LittleFS
//
//  ⚠️  LittleFS.begin() ต้องถูกเรียกก่อนแล้ว (AppBoot จัดการ)
//
//  Format JSON (/wifi.json):
//    {
//      "ssid":     "MyNetwork",
//      "password": "secret",
//      "hostname": "smartfarm"
//    }
//
//  hostname: ถ้าไม่มีใน JSON จะ default เป็น "smartfarm"
// ============================================================

struct WifiConfig
{
   String ssid;
   String password;
   String hostname;

   bool isValid() const { return ssid.length() > 0; }
};

class WifiConfigStore
{
public:
   explicit WifiConfigStore(const char *path = "/wifi.json")
       : _path(path) {}

   bool load(WifiConfig &out);
   bool save(const WifiConfig &cfg);

   const char *path() const { return _path; }

private:
   const char *_path;
};