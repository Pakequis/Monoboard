#include "crypto_client.h"
#include "config.h"
#include "debug.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

bool fetchCryptoRates(float* outBtcUsd, float* outBtcBrl, float* outEthUsd, float* outEthBrl)
{
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(CRYPTO_HTTP_TIMEOUT_MS);

  if (!http.begin(client, CRYPTO_API_URL))
  {
    DEBUG_PRINTLN("fetchCryptoRates: http.begin() failed");
    return false;
  }

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK)
  {
    DEBUG_PRINT("fetchCryptoRates: GET failed, code=");
    DEBUG_PRINTLN(httpCode);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  StaticJsonDocument<384> doc;
  DeserializationError err = deserializeJson(doc, payload);

  if (err)
  {
    DEBUG_PRINT("fetchCryptoRates: JSON parse failed: ");
    DEBUG_PRINTLN(err.c_str());
    return false;
  }

  JsonVariant btcUsd = doc["bitcoin"]["usd"];
  JsonVariant btcBrl = doc["bitcoin"]["brl"];
  JsonVariant ethUsd = doc["ethereum"]["usd"];
  JsonVariant ethBrl = doc["ethereum"]["brl"];

  if (btcUsd.isNull() || btcBrl.isNull() || ethUsd.isNull() || ethBrl.isNull())
  {
    DEBUG_PRINTLN("fetchCryptoRates: missing price fields");
    return false;
  }

  *outBtcUsd = btcUsd.as<float>();
  *outBtcBrl = btcBrl.as<float>();
  *outEthUsd = ethUsd.as<float>();
  *outEthBrl = ethBrl.as<float>();
  return true;
}
