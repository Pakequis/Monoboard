#include "text_cleanup.h"
#include <cstring>

void stripSourceSuffix(char* text)
{
  size_t len = strlen(text);
  if (len < 3) return;
  for (size_t i = len - 3; ; i--)
  {
    if (text[i] == ' ' && text[i + 1] == '-' && text[i + 2] == ' ')
    {
      text[i] = '\0';
      return;
    }
    if (i == 0) break;
  }
}

void decodeEntities(const char* in, char* out, size_t outSize)
{
  if (outSize == 0) return;
  size_t o = 0;
  for (size_t i = 0; in[i] != '\0' && o < outSize - 1; )
  {
    if (in[i] == '&')
    {
      struct Entity { const char* code; char value; };
      static const Entity kEntities[] = {
        { "&amp;", '&' }, { "&lt;", '<' }, { "&gt;", '>' },
        { "&quot;", '"' }, { "&#39;", '\'' }, { "&apos;", '\'' },
      };
      bool matched = false;
      for (const Entity& e : kEntities)
      {
        size_t len = strlen(e.code);
        if (strncmp(&in[i], e.code, len) == 0)
        {
          out[o++] = e.value;
          i += len;
          matched = true;
          break;
        }
      }
      if (matched) continue;
    }
    out[o++] = in[i++];
  }
  out[o] = '\0';
}

void transliterate(const char* in, char* out, size_t outSize)
{
  if (outSize == 0) return;
  // Latin-1 Supplement (U+00C0-U+00FF), always encoded as 0xC3 0x80-0xBF.
  static const char kLatin1Supplement[64] = {
    'A','A','A','A','A','A','A','C','E','E','E','E','I','I','I','I', // C0-CF
    'D','N','O','O','O','O','O','x','O','U','U','U','U','Y','P','s', // D0-DF
    'a','a','a','a','a','a','a','c','e','e','e','e','i','i','i','i', // E0-EF
    'd','n','o','o','o','o','o','/','o','u','u','u','u','y','p','y', // F0-FF
  };

  size_t o = 0;
  for (size_t i = 0; in[i] != '\0' && o < outSize - 1; )
  {
    unsigned char c = static_cast<unsigned char>(in[i]);
    if (c < 0x80)
    {
      out[o++] = static_cast<char>(c);
      i += 1;
    }
    else if (c == 0xC3 && in[i + 1] != '\0')
    {
      unsigned char c2 = static_cast<unsigned char>(in[i + 1]);
      out[o++] = kLatin1Supplement[c2 & 0x3F];
      i += 2;
    }
    else if (c == 0xE2 && in[i + 1] == '\x80' && in[i + 2] != '\0')
    {
      // General Punctuation block (U+2010-U+2026): smart quotes/dashes.
      unsigned char c3 = static_cast<unsigned char>(in[i + 2]);
      switch (c3)
      {
        case 0x93: case 0x94: out[o++] = '-'; break;             // – —
        case 0x98: case 0x99: out[o++] = '\''; break;            // ' '
        case 0x9C: case 0x9D: out[o++] = '"'; break;             // " "
        case 0xA6:                                              // …
          if (o + 3 < outSize) { out[o++]='.'; out[o++]='.'; out[o++]='.'; }
          break;
        default: break; // drop unrecognized punctuation
      }
      i += 3;
    }
    else
    {
      // Unrecognized multi-byte sequence -- skip just the lead byte
      // rather than risk mis-syncing on the rest of the string.
      i += 1;
    }
  }
  out[o] = '\0';
}
