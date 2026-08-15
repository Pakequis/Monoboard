#ifndef TEXT_LAYOUT_H
#define TEXT_LAYOUT_H

#include <cstddef>
#include <cstdint>

// Scratch-buffer capacity this module uses internally for candidate/
// ellipsis strings -- exposed so callers with a known maximum input
// length can assert their input can never exceed it.
constexpr size_t TEXT_LAYOUT_SCRATCH_LEN = 128;

// Measures the pixel width a piece of text would render at, in whatever
// font the caller has already bound to this callback (e.g. a captureless
// lambda closing over a specific GFXfont). Lets this module's word-wrap
// logic stay free of any display/font dependency.
typedef int16_t (*TextWidthMeasurer)(const char* text);

// Shortens text in place, measuring real pixel width via measureWidth(),
// appending "..." if it doesn't fit maxWidth. textCapacity is text's real
// buffer size (not just its current content length) since the "..." can
// make the result a couple of bytes longer than the truncated prefix.
void truncateToFitWidth(char* text, size_t textCapacity, TextWidthMeasurer measureWidth, int16_t maxWidth);

// Splits text across line1/line2 at the last word boundary that still
// fits maxWidth on line1 (word-wrap, not a mid-word cut). If text already
// fits on one line, line2 comes back empty. If even a single word is too
// wide to fit, falls back to a hard truncateToFitWidth() cut on line1.
// If what's left over for line2 still doesn't fit, it's truncated the
// same way. line1Size/line2Size are each buffer's real capacity.
void wrapToTwoLines(const char* text, char* line1, size_t line1Size,
                     char* line2, size_t line2Size,
                     TextWidthMeasurer measureWidth, int16_t maxWidth);

#endif // TEXT_LAYOUT_H
