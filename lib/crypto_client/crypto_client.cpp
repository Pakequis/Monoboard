#include "crypto_client.h"
#include "config.h"
#include "debug.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <string.h>

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

  // Response is an array of {"symbol":"BTCUSDT","price":"84066.48"} objects;
  // Binance returns prices as strings.
  StaticJsonDocument<768> doc;
  DeserializationError err = deserializeJson(doc, payload);

  if (err)
  {
    DEBUG_PRINT("fetchCryptoRates: JSON parse failed: ");
    DEBUG_PRINTLN(err.c_str());
    return false;
  }

  float btcUsd = 0, btcBrl = 0, ethUsd = 0, ethBrl = 0;
  for (JsonObject item : doc.as<JsonArray>())
  {
    const char* symbol = item["symbol"] | "";
    float price = item["price"].as<float>();
    if (strcmp(symbol, "BTCUSDT") == 0) btcUsd = price;
    else if (strcmp(symbol, "BTCBRL") == 0) btcBrl = price;
    else if (strcmp(symbol, "ETHUSDT") == 0) ethUsd = price;
    else if (strcmp(symbol, "ETHBRL") == 0) ethBrl = price;
  }

  if (btcUsd <= 0 || btcBrl <= 0 || ethUsd <= 0 || ethBrl <= 0)
  {
    DEBUG_PRINTLN("fetchCryptoRates: missing price fields");
    return false;
  }

  *outBtcUsd = btcUsd;
  *outBtcBrl = btcBrl;
  *outEthUsd = ethUsd;
  *outEthBrl = ethBrl;
  return true;
}
