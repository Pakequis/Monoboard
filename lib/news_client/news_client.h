#ifndef NEWS_CLIENT_H
#define NEWS_CLIENT_H

#include <Arduino.h>

// Fetches the top news headlines from the configured RSS feed (see
// NEWS_API_URL, config.h), cleaned up for this project's display font:
// the feed's own "- Source Name" suffix is stripped, HTML entities are
// decoded, and accented/smart-punctuation characters are transliterated
// to plain ASCII (FreeMonoBold9pt7b only has glyphs for 0x20-0x7E).
//
// WiFi must already be connected. outHeadlines must have room for
// headlineCount * headlineLen bytes, laid out as headlineCount
// consecutive fixed-size C strings (i.e. a char[headlineCount][headlineLen]
// array decays correctly into this parameter). On success, fills every
// slot and returns true. On any failure (HTTP error, malformed feed,
// fewer than headlineCount items found), returns false and leaves
// outHeadlines untouched.
//
// Reads the HTTP response as it arrives instead of buffering it whole --
// the feed itself runs well over 100KB (each item carries a multi-KB
// "related articles" description blob this project never uses), too big
// to hold in RAM at once on this hardware.
bool fetchTopHeadlines(char* outHeadlines, size_t headlineCount, size_t headlineLen);

#endif // NEWS_CLIENT_H
