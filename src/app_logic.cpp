#include "app_logic.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <vector>

namespace {
bool isCommonAbbreviation(const std::string& text, std::size_t period) {
  std::size_t start = period;
  while (start > 0 && !std::isspace(static_cast<unsigned char>(text[start - 1]))) {
    --start;
  }
  std::string token = text.substr(start, period - start);
  while (!token.empty() && (token.front() == '(' || token.front() == '"')) {
    token.erase(token.begin());
  }

  static const char* abbreviations[] = {
      "Mr", "Mrs", "Ms", "Dr", "Prof", "Sr", "Jr", "Inc", "Ltd",
      "Co", "Corp", "No", "vs", "U.S", "U.K", "e.g", "i.e"};
  for (const char* abbreviation : abbreviations) {
    if (token == abbreviation) return true;
  }

  if (token.size() == 1 && std::isalpha(static_cast<unsigned char>(token[0]))) {
    return true;
  }
  if (token.find('.') != std::string::npos) {
    const bool initialism = std::all_of(token.begin(), token.end(), [](char value) {
      return value == '.' || std::isalpha(static_cast<unsigned char>(value));
    });
    if (initialism) return true;
  }
  if (period > 0 && period + 1 < text.size() &&
      std::isdigit(static_cast<unsigned char>(text[period - 1])) &&
      std::isdigit(static_cast<unsigned char>(text[period + 1]))) {
    return true;
  }
  return false;
}

void replaceAll(std::string& value, const std::string& from,
                const std::string& to) {
  std::size_t position = 0;
  while ((position = value.find(from, position)) != std::string::npos) {
    value.replace(position, from.length(), to);
    position += to.length();
  }
}

bool decodeUtf8(const std::string& text, std::size_t& position,
                std::uint32_t& codepoint, std::size_t& byteCount) {
  const auto first = static_cast<unsigned char>(text[position]);
  if (first < 0x80) {
    codepoint = first;
    byteCount = 1;
    return true;
  }

  if ((first & 0xE0) == 0xC0) byteCount = 2;
  else if ((first & 0xF0) == 0xE0) byteCount = 3;
  else if ((first & 0xF8) == 0xF0) byteCount = 4;
  else return false;

  if (position + byteCount > text.size()) return false;
  codepoint = first & (0x7F >> byteCount);
  for (std::size_t i = 1; i < byteCount; ++i) {
    const auto next = static_cast<unsigned char>(text[position + i]);
    if ((next & 0xC0) != 0x80) return false;
    codepoint = (codepoint << 6) | (next & 0x3F);
  }
  return true;
}

bool isDisplayableKoreanCodepoint(std::uint32_t codepoint) {
  return (codepoint >= 0x20 && codepoint <= 0x7E) || codepoint == 0x00B7 ||
         (codepoint >= 0x2013 && codepoint <= 0x2015) ||
         (codepoint >= 0x2018 && codepoint <= 0x201D) ||
         codepoint == 0x2022 || codepoint == 0x2026 || codepoint == 0x20A9 ||
         (codepoint >= 0x3000 && codepoint <= 0x303F) ||
         (codepoint >= 0x3131 && codepoint <= 0x318E) ||
         (codepoint >= 0xAC00 && codepoint <= 0xD7A3);
}
}  // namespace

namespace app_logic {
std::string cleanNewsText(const std::string& input) {
  std::string output;
  output.reserve(input.size());
  bool insideTag = false;
  bool pendingSpace = false;

  for (char character : input) {
    if (character == '<') {
      insideTag = true;
      pendingSpace = !output.empty();
      continue;
    }
    if (character == '>') {
      insideTag = false;
      continue;
    }
    if (insideTag) continue;

    if (std::isspace(static_cast<unsigned char>(character))) {
      pendingSpace = !output.empty();
      continue;
    }
    if (pendingSpace) {
      output.push_back(' ');
      pendingSpace = false;
    }
    output.push_back(character);
  }

  replaceAll(output, "&quot;", "\"");
  replaceAll(output, "&#39;", "'");
  replaceAll(output, "&apos;", "'");
  replaceAll(output, "&amp;", "&");
  replaceAll(output, "&lt;", "<");
  replaceAll(output, "&gt;", ">");
  replaceAll(output, "&nbsp;", " ");
  replaceAll(output, "\xC2\xA0", " ");
  replaceAll(output, "\xE2\x80\x98", "'");
  replaceAll(output, "\xE2\x80\x99", "'");
  replaceAll(output, "\xE2\x80\x9C", "\"");
  replaceAll(output, "\xE2\x80\x9D", "\"");
  replaceAll(output, "\xE2\x80\x93", "-");
  replaceAll(output, "\xE2\x80\x94", "-");
  replaceAll(output, "\xE2\x80\xA6", "...");
  return output;
}

std::string extractFirstSentence(const std::string& input,
                                 std::size_t maxLength) {
  const std::string text = cleanNewsText(input);
  for (std::size_t i = 0; i < text.size(); ++i) {
    const char character = text[i];
    if (character != '.' && character != '?' && character != '!') continue;
    std::size_t end = i + 1;
    while (end < text.size() &&
           (text[end] == '"' || text[end] == '\'' || text[end] == ')' ||
            text[end] == ']')) {
      ++end;
    }
    if (end < text.size() &&
        !std::isspace(static_cast<unsigned char>(text[end]))) {
      continue;
    }

    if (character == '.' && isCommonAbbreviation(text, i) && end < text.size()) {
      std::size_t next = end;
      while (next < text.size() &&
             std::isspace(static_cast<unsigned char>(text[next]))) {
        ++next;
      }
      if (next < text.size() &&
          std::islower(static_cast<unsigned char>(text[next]))) {
        continue;
      }
    }
    if (end > maxLength) return {};
    return text.substr(0, end);
  }
  return {};
}

std::string sanitizeKoreanText(const std::string& input) {
  const std::string clean = cleanNewsText(input);
  std::string output;
  output.reserve(clean.size());
  for (std::size_t position = 0; position < clean.size();) {
    std::uint32_t codepoint = 0;
    std::size_t byteCount = 0;
    if (!decodeUtf8(clean, position, codepoint, byteCount)) {
      output.push_back('?');
      ++position;
      continue;
    }
    if (isDisplayableKoreanCodepoint(codepoint)) {
      output.append(clean, position, byteCount);
    } else {
      output.push_back('?');
    }
    position += byteCount;
  }
  return output;
}

int sentenceScore(const std::string& sentence, std::size_t resultIndex) {
  if (sentence.size() < 45 || sentence.size() > 140) return -1;
  if (sentence.find("...") != std::string::npos ||
      sentence.find("[Removed]") != std::string::npos) {
    return -1;
  }
  const int lengthPenalty = std::abs(static_cast<int>(sentence.size()) - 90);
  return 1000 - lengthPenalty * 3 - static_cast<int>(resultIndex) * 12;
}

int batteryPercentFromMillivolts(std::int32_t millivolts) {
  struct Point {
    int millivolts;
    int percent;
  };
  static constexpr Point curve[] = {
      {3400, 0}, {3500, 5}, {3600, 10}, {3700, 25},
      {3800, 45}, {3900, 65}, {4000, 80}, {4100, 90}, {4200, 100}};

  if (millivolts <= curve[0].millivolts) return 0;
  if (millivolts >= curve[8].millivolts) return 100;
  for (std::size_t i = 1; i < sizeof(curve) / sizeof(curve[0]); ++i) {
    if (millivolts <= curve[i].millivolts) {
      const Point& low = curve[i - 1];
      const Point& high = curve[i];
      return low.percent + (millivolts - low.millivolts) *
                               (high.percent - low.percent) /
                               (high.millivolts - low.millivolts);
    }
  }
  return 0;
}
}  // namespace app_logic
