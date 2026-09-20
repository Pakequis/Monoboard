#ifndef CRYPTO_CLIENT_H
#define CRYPTO_CLIENT_H

#include <Arduino.h>

// Fetches BTC and ETH spot prices in both USD and BRL from CoinGecko's
// simple/price endpoint (one request covers both coins and both
// currencies -- see CRYPTO_API_URL, config.h). There's no separate
// USD/BRL exchange-rate fetch: either coin's BRL price divided by its own
// USD price already is that rate (both express the same coin's value, just
// in different currencies), so the caller derives it from these four
// numbers instead. WiFi must already be connected.
bool fetchCryptoRates(float* outBtcUsd, float* outBtcBrl, float* outEthUsd, float* outEthBrl);

#endif // CRYPTO_CLIENT_H
