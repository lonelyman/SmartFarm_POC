// src/infrastructure/WifiConfigStore.cpp
#include "infrastructure/WifiConfigStore.h"

#include <LittleFS.h>
#include <ArduinoJson.h>

// ============================================================
//  Helpers
// ============================================================

static String getStringOrEmpty(JsonVariantConst v)
{
   if (v.is<const char *>())
      return String(v.as<const char *>());
   return String();
}

static bool isMounted()
{
   if (LittleFS.exists("/"))
      return true;
   Serial.println("⚠️ [WiFiCFG] LittleFS not mounted — call LittleFS.begin() in AppBoot");
   return false;
}

// ============================================================
//  load
// ============================================================

bool WifiConfigStore::load(WifiConfig &out)
{
   if (!isMounted())
      return false;

   if (!LittleFS.exists(_path))
   {
      Serial.printf("⚠️ [WiFiCFG] not found: %s\n", _path);
      return false;
   }

   File f = LittleFS.open(_path, "r");
   if (!f)
   {
      Serial.printf("⚠️ [WiFiCFG] open failed: %s\n", _path);
      return false;
   }

   JsonDocument doc;
   DeserializationError err = deserializeJson(doc, f);
   f.close();

   if (err)
   {
      Serial.printf("⚠️ [WiFiCFG] JSON parse failed: %s\n", err.c_str());
      return false;
   }

   out.ssid = getStringOrEmpty(doc["ssid"]);
   out.password = getStringOrEmpty(doc["password"]);
   out.hostname = getStringOrEmpty(doc["hostname"]);
   out.apSsid = getStringOrEmpty(doc["apSsid"]);
   out.apPass = getStringOrEmpty(doc["apPass"]);

   if (out.hostname.length() == 0)
      out.hostname = "smartfarm";

   if (out.apSsid.length() == 0)
      out.apSsid = "SmartFarm-Setup";

   Serial.printf("✅ [WiFiCFG] loaded: ssid='%s' hostname='%s' apSsid='%s'\n",
                 out.ssid.c_str(), out.hostname.c_str(), out.apSsid.c_str());
   return true;
}

// ============================================================
//  save
// ============================================================

bool WifiConfigStore::save(const WifiConfig &cfg)
{
   if (!isMounted())
      return false;

   File f = LittleFS.open(_path, "w");
   if (!f)
   {
      Serial.printf("⚠️ [WiFiCFG] open for write failed: %s\n", _path);
      return false;
   }

   JsonDocument doc;
   doc["ssid"] = cfg.ssid;
   doc["password"] = cfg.password;
   doc["hostname"] = cfg.hostname.length() > 0 ? cfg.hostname : "smartfarm";
   doc["apSsid"] = cfg.apSsid.length() > 0 ? cfg.apSsid : "SmartFarm-Setup";
   doc["apPass"] = cfg.apPass;

   if (serializeJsonPretty(doc, f) == 0)
   {
      f.close();
      Serial.println("⚠️ [WiFiCFG] write failed");
      return false;
   }

   f.close();
   Serial.printf("✅ [WiFiCFG] saved: %s\n", _path);
   return true;
}