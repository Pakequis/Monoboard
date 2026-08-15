#include "text_layout.h"
#include <cstdio>
#include <cstring>

void truncateToFitWidth(char* text, size_t textCapacity, TextWidthMeasurer measureWidth, int16_t maxWidth)
{
  if (measureWidth(text) <= maxWidth) return;

  size_t len = strlen(text);
  while (len > 0)
  {
    len--;
    text[len] = '\0';
    char withEllipsis[TEXT_LAYOUT_SCRATCH_LEN];
    snprintf(withEllipsis, sizeof(withEllipsis), "%s...", text);
    if (measureWidth(withEllipsis) <= maxWidth)
    {
      snprintf(text, textCapacity, "%s", withEllipsis);
      return;
    }
  }
  text[0] = '\0';
}

void wrapToTwoLines(const char* text, char* line1, size_t line1Size,
                     char* line2, size_t line2Size,
                     TextWidthMeasurer measureWidth, int16_t maxWidth)
{
  if (measureWidth(text) <= maxWidth)
  {
    snprintf(line1, line1Size, "%s", text);
    line2[0] = '\0';
    return;
  }

  size_t splitAt = 0;
  size_t len = strlen(text);
  for (size_t i = 0; i < len; i++)
  {
    if (text[i] != ' ') continue;

    char candidate[TEXT_LAYOUT_SCRATCH_LEN];
    snprintf(candidate, sizeof(candidate), "%.*s", (int)i, text);
    if (measureWidth(candidate) > maxWidth) break;
    splitAt = i;
  }

  if (splitAt == 0)
  {
    snprintf(line1, line1Size, "%s", text);
    truncateToFitWidth(line1, line1Size, measureWidth, maxWidth);
    line2[0] = '\0';
    return;
  }

  snprintf(line1, line1Size, "%.*s", (int)splitAt, text);
  snprintf(line2, line2Size, "%s", text + splitAt + 1); // skip the space itself
  truncateToFitWidth(line2, line2Size, measureWidth, maxWidth);
}
