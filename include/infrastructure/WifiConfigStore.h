#pragma once

#include <Arduino.h>

struct WifiConfig
{
   String ssid;
   String password;
   String hostname;

   bool isValid() const
   {
      return ssid.length() > 0;
   }
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