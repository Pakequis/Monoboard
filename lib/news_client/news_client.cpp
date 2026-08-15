#include "news_client.h"
#include "config.h"
#include "debug.h"
#include "text_cleanup.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <string.h>

namespace {

// Longest raw <title>...</title> text this will buffer before giving up
// on capturing more of it (real headlines are well under this).
constexpr size_t RAW_TITLE_CAP = 220;

// Streams the (already-dechunked, courtesy of HTTPClient::writeToStream())
// RSS body through a tiny state machine that captures just the <title>
// text of each <item>, ignoring everything else -- crucially, the multi-KB
// <description> blob Google News embeds per item. Only a couple hundred
// bytes are ever held at once, regardless of how large the feed is.
class HeadlineExtractor : public Stream
{
public:
  HeadlineExtractor(char* outHeadlines, size_t headlineCount, size_t headlineLen)
    : outHeadlines_(outHeadlines), headlineCount_(headlineCount), headlineLen_(headlineLen)
  {
  }

  size_t write(const uint8_t* buffer, size_t size) override
  {
    for (size_t i = 0; i < size; i++)
    {
      feed(static_cast<char>(buffer[i]));
    }
    return size;
  }

  size_t write(uint8_t b) override
  {
    feed(static_cast<char>(b));
    return 1;
  }

  int available() override { return 0; }
  int read() override { return -1; }
  int peek() override { return -1; }

  size_t headlinesFound() const { return found_; }

private:
  enum State { SEEK_ITEM, SEEK_TITLE, IN_TITLE, DONE };

  void feed(char c)
  {
    if (state_ == DONE) return;

    if (state_ == IN_TITLE && titleLen_ < RAW_TITLE_CAP - 1)
    {
      rawTitle_[titleLen_++] = c;
    }

    const char* pattern = (state_ == SEEK_ITEM) ? "<item>"
                         : (state_ == SEEK_TITLE) ? "<title>"
                         : "</title>";
    size_t patternLen = strlen(pattern);

    // Naive streaming substring matcher. Safe (no need for KMP-style
    // overlap handling) because none of these three literal tags contains
    // a repeated prefix/suffix that would make a naive restart incorrect.
    while (true)
    {
      if (c == pattern[matchPos_])
      {
        matchPos_++;
        break;
      }
      else if (matchPos_ == 0)
      {
        break;
      }
      else
      {
        matchPos_ = 0;
      }
    }

    if (matchPos_ != patternLen) return;
    matchPos_ = 0;

    switch (state_)
    {
      case SEEK_ITEM:
        state_ = SEEK_TITLE;
        break;
      case SEEK_TITLE:
        state_ = IN_TITLE;
        titleLen_ = 0;
        break;
      case IN_TITLE:
      {
        // The just-matched "</title>" itself was appended above as it was
        // being tentatively matched -- trim those trailing bytes back off.
        size_t trim = (titleLen_ >= patternLen) ? patternLen : titleLen_;
        titleLen_ -= trim;
        rawTitle_[titleLen_] = '\0';
        finishHeadline();
        state_ = (found_ < headlineCount_) ? SEEK_ITEM : DONE;
        break;
      }
      default:
        break;
    }
  }

  void finishHeadline()
  {
    if (found_ >= headlineCount_) return;

    stripSourceSuffix(rawTitle_);

    char decoded[RAW_TITLE_CAP];
    decodeEntities(rawTitle_, decoded, sizeof(decoded));

    char clean[RAW_TITLE_CAP];
    transliterate(decoded, clean, sizeof(clean));

    char* slot = outHeadlines_ + found_ * headlineLen_;
    snprintf(slot, headlineLen_, "%s", clean);
    found_++;
  }

  char* outHeadlines_;
  size_t headlineCount_;
  size_t headlineLen_;

  State state_ = SEEK_ITEM;
  size_t matchPos_ = 0;
  char rawTitle_[RAW_TITLE_CAP] = { 0 };
  size_t titleLen_ = 0;
  size_t found_ = 0;
};

} // namespace

bool fetchTopHeadlines(char* outHeadlines, size_t headlineCount, size_t headlineLen)
{
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(NEWS_HTTP_TIMEOUT_MS);

  if (!http.begin(client, NEWS_API_URL))
  {
    DEBUG_PRINTLN("fetchTopHeadlines: http.begin() failed");
    return false;
  }

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK)
  {
    DEBUG_PRINT("fetchTopHeadlines: GET failed, code=");
    DEBUG_PRINTLN(httpCode);
    http.end();
    return false;
  }

  HeadlineExtractor extractor(outHeadlines, headlineCount, headlineLen);
  // writeToStream() dechunks correctly (same mechanism getString() uses
  // internally) and streams into our extractor instead of a giant String --
  // this is what keeps memory use bounded regardless of feed size.
  http.writeToStream(&extractor);
  http.end();

  if (extractor.headlinesFound() < headlineCount)
  {
    DEBUG_PRINT("fetchTopHeadlines: only found ");
    DEBUG_PRINT(extractor.headlinesFound());
    DEBUG_PRINT(" of ");
    DEBUG_PRINTLN(headlineCount);
    return false;
  }

  return true;
}
