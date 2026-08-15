#ifndef TEXT_CLEANUP_H
#define TEXT_CLEANUP_H

#include <cstddef>

// Cuts a feed's trailing " - Source Name" suffix (RSS feeds like Google
// News append one to every title), keeping the character budget for the
// actual headline. Modifies text in place.
void stripSourceSuffix(char* text);

// Decodes the handful of HTML entities that actually show up in RSS
// titles (&amp; &lt; &gt; &quot; &#39; &apos;). Unrecognized "&...;"
// sequences are copied through as-is. outSize is out's real buffer size.
void decodeEntities(const char* in, char* out, size_t outSize);

// Maps UTF-8 bytes outside the display font's 0x20-0x7E range to a plain-
// ASCII equivalent (Portuguese accents, smart quotes/dashes). Anything not
// covered here is dropped rather than risking a garbled glyph. outSize is
// out's real buffer size.
void transliterate(const char* in, char* out, size_t outSize);

#endif // TEXT_CLEANUP_H
