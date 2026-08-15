# Internationalization (PT-BR / EN)

Every string rendered on the e-paper display is chosen at compile time
between Portuguese (Brazil) and English by a single switch. There is no
runtime language selection.

## Mechanism

`include/strings.h` defines the switch and every translatable string:

```cpp
#define LANG_PT_BR 0
#define LANG_EN    1

#ifndef APP_LANGUAGE
#define APP_LANGUAGE LANG_PT_BR
#endif

#if APP_LANGUAGE == LANG_PT_BR
  #define STR_DASHBOARD_TITLE "Monoboard"
  // ...one #define/array per key, PT-BR text...
#elif APP_LANGUAGE == LANG_EN
  #define STR_DASHBOARD_TITLE "Monoboard"
  // ...the same keys, EN text...
#endif
```

Both language blocks define the exact same set of `STR_*` keys. If a key
is added to one block and forgotten in the other, any file that uses it
under the missing language fails to build with an "undeclared identifier"
error — this is the only parity check that exists, and it is enforced by
the compiler, not by a manual review step.

Default language is `LANG_PT_BR`. Override without editing the file via
a build flag:

```
-D APP_LANGUAGE=LANG_EN
```

## Current keys

| Key | Used in |
|---|---|
| `STR_DASHBOARD_TITLE` | `dashboard_manager.cpp` (header title) |
| `STR_WIFI_OK` / `STR_WIFI_ERROR` | `dashboard_manager.cpp` (header status) |
| `STR_WEEKDAYS[]` | `time_manager.cpp` (full weekday names) |
| `STR_WEEKDAYS_SHORT[]` | `dashboard_manager.cpp` (single-letter calendar header row, indexed 0=Sunday..6=Saturday to match `struct tm::tm_wday`) |
| `STR_MONTHS[]` | `time_manager.cpp`, `dashboard_manager.cpp` (calendar title) |
| `STR_VALUE_PLACEHOLDER` | `local_sensors.cpp`, `content_manager.cpp` (generic "no data yet" value, `"--"`) |
| `STR_NEWS_UNAVAILABLE` | `content_manager.cpp` (shown when the news feed hasn't been fetched yet) |
| `STR_NEWS_LOCALE_QUERY` | `config.h` (`hl`/`gl`/`ceid` query params baked into `NEWS_API_URL`, selects the Google News RSS edition matching the active language) |
| `STR_LABEL_LIGHTNING` | `dashboard_manager.cpp` (lightning-strike panel label) |

## What stays outside the language switch

- **`NEWS_SOURCE_TAG`** (`config.h`, `"[GN]"`) — a fixed source
  abbreviation, language-independent, not translated text.
- **Debug/serial logging** (`DEBUG_PRINT`/`DEBUG_PRINTLN` calls) — always
  in English regardless of `APP_LANGUAGE`. These are developer-facing
  only and never reach the display.
- **Purely numeric/unit format strings** (e.g. `"%.1fC"`, `"%.0f%%"`,
  `"%dkm"`) — no words, nothing to translate.

## Constraint: no accented characters in Portuguese strings

The project's display font (`FreeMonoBold9pt7b`) only covers ASCII
0x20–0x7E — no diacritics. Any PT-BR string in `strings.h` must be
written without accents (e.g. `"Marco"`, not `"Março"`; `"Sabado"`, not
`"Sábado"`) or the missing glyph will render garbled or blank.

## Adding a new translatable string

1. Add a `#define STR_YOUR_KEY "..."` (or a `static const char* const
   STR_YOUR_ARRAY[] = {...}` for a list) to **both** blocks in
   `include/strings.h` — same key name, one line per language.
2. If the PT-BR text has an accented character, write it without the
   accent.
3. Replace the hardcoded literal in the source file with the new key,
   and make sure that file `#include`s `strings.h`.
4. Build once with the default language and once with
   `-D APP_LANGUAGE=LANG_EN` to confirm both compile.

## Adding a third language

Not supported today — `strings.h` is a two-way `#if`/`#elif`/`#error`
switch, and nothing else in the mechanism assumes exactly two languages.
Adding a third would mean: a new `LANG_*` value, a third block with the
same key set, and changing the trailing `#else` / `#error` to an
`#elif`/`#error` pair so an unrecognized value still fails the build
instead of silently compiling with no strings defined.

Two languages are also the reason this is a compile-time switch instead
of a runtime lookup table: keeping every language's strings resident in
flash/RAM at once, plus an indirection layer to select among them, only
pays for itself once more than a small, fixed number of languages must
coexist in the same binary — which is not the case here.
