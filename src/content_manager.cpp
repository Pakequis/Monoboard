#include "content_manager.h"
#include "config.h"
#include "debug.h"
#include "weather_client.h"
#include "news_client.h"
#include "crypto_client.h"
#include "time_manager.h"
#include "strings.h"
#include "weather_icon_map.h"
#include <math.h>
#include <string.h>

struct WeatherForecastCache {
  int weatherCode[WEATHER_FORECAST_HOURS];
  float temperatureC[WEATHER_FORECAST_HOURS];
  int isDay[WEATHER_FORECAST_HOURS]; // Open-Meteo convention: 1=day, 0=night
  char hourLabel[WEATHER_FORECAST_HOURS][4]; // Open-Meteo's exact local interval
  bool valid;
};

struct NewsCache {
  char headlines[NEWS_TOTAL_HEADLINE_COUNT][NEWS_HEADLINE_LEN];
  bool valid;
};

struct CryptoCache {
  float btcUsd;
  float btcBrl;
  float ethUsd;
  float ethBrl;
  bool valid;
};

static RTC_DATA_ATTR WeatherForecastCache weatherForecastCache = { {0}, {0.0f}, {0}, {{0}}, false };
static RTC_DATA_ATTR NewsCache newsCache = { {{0}}, false };
static RTC_DATA_ATTR CryptoCache cryptoCache = { 0.0f, 0.0f, 0.0f, 0.0f, false };
// Which of the NEWS_CAROUSEL_BUFFER_COUNT buffers is currently on screen.
// Advances once per wake (advanceNewsCarousel()), independent of sync
// windows -- see config.h's news-carousel comment for the full picture.
static RTC_DATA_ATTR uint8_t newsCarouselIndex = 0;

void fetchNetworkContent()
{
#if FEATURE_WEATHER_ENABLED
  if (fetchWeatherForecast(weatherForecastCache.weatherCode, weatherForecastCache.temperatureC,
                           weatherForecastCache.isDay, weatherForecastCache.hourLabel,
                           WEATHER_FORECAST_HOURS))
  {
    weatherForecastCache.valid = true;
    DEBUG_PRINTLN("fetchNetworkContent: weather forecast updated ->");
    for (size_t i = 0; i < WEATHER_FORECAST_HOURS; i++)
    {
      DEBUG_PRINT("  hour +");
      DEBUG_PRINT(i);
      DEBUG_PRINT(": code=");
      DEBUG_PRINT(weatherForecastCache.weatherCode[i]);
      DEBUG_PRINT(", temp=");
      DEBUG_PRINT(weatherForecastCache.temperatureC[i]);
      DEBUG_PRINT(" C, isDay=");
      DEBUG_PRINT(weatherForecastCache.isDay[i]);
      DEBUG_PRINT(", time=");
      DEBUG_PRINTLN(weatherForecastCache.hourLabel[i]);
    }
  }
  else
  {
    // Reliability rule: never show a value that wasn't just correctly
    // received. Drop the cache instead of displaying a stale forecast.
    weatherForecastCache.valid = false;
    DEBUG_PRINTLN("fetchNetworkContent: weather forecast fetch failed, clearing cache");
  }
#endif

#if FEATURE_NEWS_ENABLED
  if (fetchTopHeadlines(&newsCache.headlines[0][0], NEWS_TOTAL_HEADLINE_COUNT, NEWS_HEADLINE_LEN))
  {
    newsCache.valid = true;
    DEBUG_PRINTLN("fetchNetworkContent: news headlines updated ->");
    for (size_t i = 0; i < NEWS_TOTAL_HEADLINE_COUNT; i++)
    {
      DEBUG_PRINT("  ");
      DEBUG_PRINTLN(newsCache.headlines[i]);
    }
  }
  else
  {
    // Same reliability rule as the weather forecast above: never show
    // stale or partially-fetched headlines.
    newsCache.valid = false;
    DEBUG_PRINTLN("fetchNetworkContent: news headlines fetch failed, clearing cache");
  }
#endif

#if FEATURE_CRYPTO_ENABLED
  if (fetchCryptoRates(&cryptoCache.btcUsd, &cryptoCache.btcBrl, &cryptoCache.ethUsd, &cryptoCache.ethBrl))
  {
    cryptoCache.valid = true;
    DEBUG_PRINT("fetchNetworkContent: crypto rates updated -> BTC $");
    DEBUG_PRINT(cryptoCache.btcUsd);
    DEBUG_PRINT(" / R$");
    DEBUG_PRINT(cryptoCache.btcBrl);
    DEBUG_PRINT(", ETH $");
    DEBUG_PRINT(cryptoCache.ethUsd);
    DEBUG_PRINT(" / R$");
    DEBUG_PRINTLN(cryptoCache.ethBrl);
  }
  else
  {
    // Same reliability rule as the weather forecast/news above: never show
    // stale or partially-fetched prices.
    cryptoCache.valid = false;
    DEBUG_PRINTLN("fetchNetworkContent: crypto rates fetch failed, clearing cache");
  }
#endif
}

char getWeatherForecastIconChar(size_t hourIndex)
{
  if (!weatherForecastCache.valid || hourIndex >= WEATHER_FORECAST_HOURS)
  {
    return ':';
  }
  return weatherCodeToIconChar(weatherForecastCache.weatherCode[hourIndex], weatherForecastCache.isDay[hourIndex]);
}

void getWeatherForecastHourLabel(size_t hourIndex, char* outLabel, size_t outLabelSize)
{
  if (!weatherForecastCache.valid || hourIndex >= WEATHER_FORECAST_HOURS)
  {
    snprintf(outLabel, outLabelSize, "--");
    return;
  }
  snprintf(outLabel, outLabelSize, "%s", weatherForecastCache.hourLabel[hourIndex]);
}

void getWeatherForecastTemperatureNumberLabel(size_t hourIndex, char* outLabel, size_t outLabelSize)
{
  if (!weatherForecastCache.valid || hourIndex >= WEATHER_FORECAST_HOURS)
  {
    snprintf(outLabel, outLabelSize, "%s", STR_VALUE_PLACEHOLDER);
    return;
  }
  snprintf(outLabel, outLabelSize, "%.0f", weatherForecastCache.temperatureC[hourIndex]);
}

void advanceNewsCarousel()
{
  newsCarouselIndex = (newsCarouselIndex + 1) % NEWS_CAROUSEL_BUFFER_COUNT;
}

bool getNewsHeadline(size_t index, char* outText, size_t outTextSize)
{
  if (!newsCache.valid)
  {
    snprintf(outText, outTextSize, "%s", (index == 0) ? STR_NEWS_UNAVAILABLE : "");
    return false;
  }
  if (index >= NEWS_HEADLINES_PER_BUFFER)
  {
    snprintf(outText, outTextSize, "%s", "");
    return false;
  }
  size_t realIndex = newsCarouselIndex * NEWS_HEADLINES_PER_BUFFER + index;
  snprintf(outText, outTextSize, "%s", newsCache.headlines[realIndex]);
  return true;
}

void getCryptoBitcoinText(char* outText, size_t outTextSize)
{
  if (!cryptoCache.valid)
  {
    snprintf(outText, outTextSize, "%s %s", CRYPTO_BTC_LABEL, STR_VALUE_PLACEHOLDER);
    return;
  }
#if STR_CRYPTO_QUOTE_CURRENCY_IS_BRL
  snprintf(outText, outTextSize, "%s %s%.0f", CRYPTO_BTC_LABEL, STR_CRYPTO_CURRENCY_SYMBOL, cryptoCache.btcBrl);
#else
  snprintf(outText, outTextSize, "%s %s%.0f", CRYPTO_BTC_LABEL, STR_CRYPTO_CURRENCY_SYMBOL, cryptoCache.btcUsd);
#endif
}

void getCryptoEthereumText(char* outText, size_t outTextSize)
{
  if (!cryptoCache.valid)
  {
    snprintf(outText, outTextSize, "%s %s", CRYPTO_ETH_LABEL, STR_VALUE_PLACEHOLDER);
    return;
  }
#if STR_CRYPTO_QUOTE_CURRENCY_IS_BRL
  snprintf(outText, outTextSize, "%s %s%.0f", CRYPTO_ETH_LABEL, STR_CRYPTO_CURRENCY_SYMBOL, cryptoCache.ethBrl);
#else
  snprintf(outText, outTextSize, "%s %s%.0f", CRYPTO_ETH_LABEL, STR_CRYPTO_CURRENCY_SYMBOL, cryptoCache.ethUsd);
#endif
}

void getCryptoFxText(char* outText, size_t outTextSize)
{
  // Derived from the same two BTC prices rather than a separate fetch --
  // either coin's BRL price divided by its own USD price is the USD/BRL
  // rate, since both express that coin's value in each currency.
  if (!cryptoCache.valid || cryptoCache.btcUsd <= 0.0f || cryptoCache.btcBrl <= 0.0f)
  {
    snprintf(outText, outTextSize, "%s %s", STR_CRYPTO_FX_LABEL, STR_VALUE_PLACEHOLDER);
    return;
  }
#if STR_CRYPTO_QUOTE_CURRENCY_IS_BRL
  float rate = cryptoCache.btcBrl / cryptoCache.btcUsd; // reais per dollar
#else
  float rate = cryptoCache.btcUsd / cryptoCache.btcBrl; // dollars per real
#endif
  snprintf(outText, outTextSize, "%s %s%.2f", STR_CRYPTO_FX_LABEL, STR_CRYPTO_CURRENCY_SYMBOL, rate);
}
