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
//      "hostname": "smartfarm",
//      "apSsid":   "SmartFarm-Mine",
//      "apPass":   "12345678"
//    }
//
//  hostname: default "smartfarm" ถ้าไม่มี
//  apSsid:   default "SmartFarm-Setup" ถ้าไม่มี
//  apPass:   default "" (open AP) ถ้าไม่มี
// ============================================================

struct WifiConfig
{
   String ssid;
   String password;
   String hostname;
   String apSsid;
   String apPass;

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