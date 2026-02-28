#include "infrastructure/Esp32WiFiNetwork.h"

#include <Arduino.h>
#include <WiFi.h>

Esp32WiFiNetwork::Esp32WiFiNetwork(const char *ssid, const char *pass)
    : _ssid(ssid), _pass(pass) {}

void Esp32WiFiNetwork::begin()
{
   WiFi.mode(WIFI_STA);
   WiFi.setAutoReconnect(true);
   WiFi.persistent(false);
}

bool Esp32WiFiNetwork::isConnected() const
{
   return WiFi.status() == WL_CONNECTED;
}

bool Esp32WiFiNetwork::ensureConnected(uint32_t timeoutMs)
{
   if (isConnected())
      return true;

   if (_ssid == nullptr || _ssid[0] == '\0')
   {
      Serial.println("[NET] SSID not set");
      return false;
   }

   Serial.printf("[NET] Connecting to SSID: %s\n", _ssid);
   WiFi.begin(_ssid, _pass);

   const uint32_t start = millis();
   while (!isConnected() && (millis() - start) < timeoutMs)
   {
      vTaskDelay(pdMS_TO_TICKS(250));
   }

   if (isConnected())
   {
      Serial.printf("[NET] Connected, IP=%s\n", WiFi.localIP().toString().c_str());
      return true;
   }

   Serial.println("[NET] Connect timeout");
   return false;
}

void Esp32WiFiNetwork::disconnect()
{
   if (isConnected())
   {
      Serial.println("[NET] Disconnecting...");
      WiFi.disconnect(true);
   }
}