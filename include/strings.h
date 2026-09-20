#ifndef STRINGS_H
#define STRINGS_H

// ===== Language Selection =====
#define LANG_PT_BR 0
#define LANG_EN    1

// Change this one line to switch language. Guarded with #ifndef so it can
// also be overridden without editing this file, e.g. -D APP_LANGUAGE=LANG_EN
// -- same convention config.h already uses for APP_DEBUG_SERIAL.
#ifndef APP_LANGUAGE
#define APP_LANGUAGE LANG_PT_BR
#endif

#if APP_LANGUAGE == LANG_PT_BR

// ===== Header Chrome =====
#define STR_DASHBOARD_TITLE "Monoboard by Pakequis"
#define STR_WIFI_OK         "WiFi: OK"
#define STR_WIFI_ERROR      "WiFi: ERROR"

static const char* const STR_WEEKDAYS[] = {
  "Domingo", "Segunda-feira", "Terca-feira", "Quarta-feira",
  "Quinta-feira", "Sexta-feira", "Sabado"
};

// Single-letter weekday headers for the calendar box (0=Sunday..6=Saturday,
// matching struct tm's tm_wday). Repeated letters (S for Segunda/Sexta/
// Sabado) match how Brazilian calendars conventionally abbreviate this row.
static const char* const STR_WEEKDAYS_SHORT[] = {
  "D", "S", "T", "Q", "Q", "S", "S"
};

static const char* const STR_MONTHS[] = {
  "Janeiro", "Fevereiro", "Marco", "Abril",
  "Maio", "Junho", "Julho", "Agosto",
  "Setembro", "Outubro", "Novembro", "Dezembro"
};

// ===== Content Panel =====
#define STR_VALUE_PLACEHOLDER  "--"
#define STR_NEWS_UNAVAILABLE   "Sem noticias"

// Google News RSS locale query params -- picks the Brazilian Portuguese
// edition to match this language block. ceid's second half is a bare
// language code (no region), unlike hl/gl.
#define STR_NEWS_LOCALE_QUERY  "hl=pt-BR&gl=BR&ceid=BR:pt-419"

// ===== Local Sensors =====
#define STR_LABEL_LIGHTNING     "Raios"

// ===== Crypto/FX Panel =====
// BTC/ETH are quoted in BRL, matching this language's own currency; the
// third line is the USD/BRL rate itself (1 dollar's worth of reais).
#define STR_CRYPTO_QUOTE_CURRENCY_IS_BRL 1
#define STR_CRYPTO_CURRENCY_SYMBOL       "R$"
#define STR_CRYPTO_FX_LABEL              "USD"

#elif APP_LANGUAGE == LANG_EN

// ===== Header Chrome =====
#define STR_DASHBOARD_TITLE "Monoboard by Pakequis"
#define STR_WIFI_OK         "WiFi: OK"
#define STR_WIFI_ERROR      "WiFi: ERROR"

static const char* const STR_WEEKDAYS[] = {
  "Sunday", "Monday", "Tuesday", "Wednesday",
  "Thursday", "Friday", "Saturday"
};

// Single-letter weekday headers for the calendar box (0=Sunday..6=Saturday,
// matching struct tm's tm_wday).
static const char* const STR_WEEKDAYS_SHORT[] = {
  "S", "M", "T", "W", "T", "F", "S"
};

static const char* const STR_MONTHS[] = {
  "January", "February", "March", "April",
  "May", "June", "July", "August",
  "September", "October", "November", "December"
};

// ===== Content Panel =====
#define STR_VALUE_PLACEHOLDER  "--"
#define STR_NEWS_UNAVAILABLE   "No news"

// Google News RSS locale query params -- picks the US English edition to
// match this language block.
#define STR_NEWS_LOCALE_QUERY  "hl=en-US&gl=US&ceid=US:en"

// ===== Local Sensors =====
#define STR_LABEL_LIGHTNING     "Strikes"

// ===== Crypto/FX Panel =====
// BTC/ETH are quoted in USD, matching this language's own currency; the
// third line inverts to the BRL/USD rate (1 real's worth of dollars) --
// "1 dollar in reais" would be meaningless once the base currency is
// already the dollar.
#define STR_CRYPTO_QUOTE_CURRENCY_IS_BRL 0
#define STR_CRYPTO_CURRENCY_SYMBOL       "$"
#define STR_CRYPTO_FX_LABEL              "BRL"

#else
#error "Unsupported APP_LANGUAGE value - must be LANG_PT_BR or LANG_EN"
#endif

#endif // STRINGS_H
