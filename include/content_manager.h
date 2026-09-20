#ifndef CONTENT_MANAGER_H
#define CONTENT_MANAGER_H

#include <Arduino.h>

// Fetches all enabled network content types and updates their RTC caches.
// WiFi must already be connected. Call once per sync window.
void fetchNetworkContent();

// Advances the news carousel to its next buffer (wrapping back to the
// first after NEWS_CAROUSEL_BUFFER_COUNT-1). Call once per wake, before
// drawing -- unlike fetchNetworkContent(), this doesn't need WiFi and
// isn't tied to a sync window, since the carousel rotates on every screen
// redraw so all NEWS_TOTAL_HEADLINE_COUNT cached headlines get shown a
// few at a time instead of only ever the first NEWS_HEADLINES_PER_BUFFER.
void advanceNewsCarousel();

// Fills outText with the news headline at the given index within the
// *current carousel buffer* (0 = that buffer's top headline, up to
// NEWS_HEADLINES_PER_BUFFER-1 -- not an index into the full
// NEWS_TOTAL_HEADLINE_COUNT cache), already cleaned up for display (see
// news_client.h), and returns true. If the last fetch failed or hasn't
// happened yet, index 0 fills STR_NEWS_UNAVAILABLE and every other index
// fills an empty string (so the placeholder isn't repeated on every
// line); also fills an empty string if index is out of range. Both cases
// return false, telling the caller this isn't a real headline (e.g. so
// it skips adding a "source" prefix to it). Buffer must be at least
// NEWS_HEADLINE_LEN bytes.
bool getNewsHeadline(size_t index, char* outText, size_t outTextSize);

// Returns the Weather Icons glyph character (see lib/weather_icons_font)
// for the given forecast hour (0 = the next complete hour, up to
// WEATHER_FORECAST_HOURS-1). If the forecast hasn't been correctly
// fetched, or hourIndex is out of range, returns ':' (a blank glyph)
// rather than showing stale data.
char getWeatherForecastIconChar(size_t hourIndex);

// Fills outLabel with the local hour label ("HHh", e.g. "14h") for the given
// forecast hour, as captured from Open-Meteo at fetch time. If the forecast
// wasn't correctly fetched, fills "--" (a placeholder) instead.
void getWeatherForecastHourLabel(size_t hourIndex, char* outLabel, size_t outLabelSize);

// Fills outLabel with just the rounded forecast temperature number (no
// decimal point, no unit -- e.g. "18" or "-5") for the given forecast hour,
// mirroring getTemperatureNumberText()'s split: the caller measures this
// text's ink width to position a drawn degree-mark circle before appending
// "C" (FreeMonoBold9pt7b has no "°" glyph, same as the sensor strip). If
// the forecast hasn't been correctly fetched, fills a placeholder instead.
void getWeatherForecastTemperatureNumberLabel(size_t hourIndex, char* outLabel, size_t outLabelSize);

// Fills outText with the Bitcoin price line, quoted in whichever currency
// matches APP_LANGUAGE (see strings.h's STR_CRYPTO_* macros): e.g.
// "BTC R$304521" in Portuguese, "BTC $60521" in English. Uses the cache
// from the last successful fetchNetworkContent() sync; if that fetch
// failed or hasn't happened yet, fills a placeholder instead.
void getCryptoBitcoinText(char* outText, size_t outTextSize);

// Same as getCryptoBitcoinText(), for Ethereum.
void getCryptoEthereumText(char* outText, size_t outTextSize);

// Fills outText with the USD/BRL exchange rate line, derived from the
// cached BTC prices in both currencies (see crypto_client.h) rather than
// a separate fetch: "USD R$5.23" (1 dollar in reais) in Portuguese, or
// "BRL $0.19" (1 real in dollars) in English. Same placeholder fallback
// as getCryptoBitcoinText().
void getCryptoFxText(char* outText, size_t outTextSize);

#endif // CONTENT_MANAGER_H
