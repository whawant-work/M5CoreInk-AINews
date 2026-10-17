#include "news_service.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include <string>
#include <ctime>

#include "app_config.h"
#include "app_logic.h"
#include "certificates.h"
#include "secrets.h"

namespace {
constexpr char GNEWS_ENDPOINT[] = "https://gnews.io/api/v4/search";
constexpr char DEEPL_ENDPOINT[] = "https://api-free.deepl.com/v2/translate";
constexpr char GNEWS_QUERY[] =
    "(\"artificial intelligence\" OR \"generative AI\" OR OpenAI OR "
    "Anthropic OR ChatGPT OR Gemini OR \"machine learning\" OR "
    "\"large language model\")";

String httpError(const char* service, int status) {
  return String(service) + " HTTP " + String(status);
}

bool containsAiTerm(const std::string& value) {
  std::string lower = value;
  std::transform(lower.begin(), lower.end(), lower.begin(), [](char character) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
  });
  static const char* terms[] = {
      "artificial intelligence", "generative ai", "openai", "anthropic",
      "chatgpt", "gemini", "machine learning", "language model"};
  for (const char* term : terms) {
    if (lower.find(term) != std::string::npos) return true;
  }
  return false;
}
}  // namespace

bool NewsService::fetchToday(const String& dateKey, ArticleData& article,
                             String& error) {
  ArticleData candidate;
  if (!fetchEnglish(candidate, error)) return false;
  if (!translateToKorean(candidate.english, candidate.korean, error)) return false;
  candidate.dateKey = dateKey;
  if (!candidate.isValid()) {
    error = "Incomplete article";
    return false;
  }
  article = candidate;
  return true;
}

bool NewsService::fetchEnglish(ArticleData& article, String& error) {
  const std::time_t fromTime = std::time(nullptr) - 72L * 60L * 60L;
  std::tm fromUtc{};
  gmtime_r(&fromTime, &fromUtc);
  char fromBuffer[21];
  std::strftime(fromBuffer, sizeof(fromBuffer), "%Y-%m-%dT%H:%M:%SZ", &fromUtc);
  String url = String(GNEWS_ENDPOINT) + "?q=" + urlEncode(GNEWS_QUERY) +
               "&lang=en&country=us&in=title%2Cdescription" +
               "&from=" + urlEncode(fromBuffer) +
               "&sortby=publishedAt&max=10&page=1&truncate=content";

  WiFiClientSecure client;
  client.setCACert(ISRG_ROOT_X1);
  HTTPClient http;
  http.useHTTP10(true);
  http.setTimeout(app_config::HTTP_TIMEOUT_MS);
  if (!http.begin(client, url)) {
    error = "GNews connection failed";
    return false;
  }
  http.addHeader("Accept", "application/json");
  http.addHeader("X-Api-Key", secrets::GNEWS_API_KEY);
  const int status = http.GET();
  if (status != HTTP_CODE_OK) {
    error = httpError("GNews", status);
    http.end();
    return false;
  }
  if (http.getSize() > 64000) {
    error = "GNews response too large";
    http.end();
    return false;
  }

  StaticJsonDocument<320> filter;
  filter["articles"][0]["title"] = true;
  filter["articles"][0]["description"] = true;
  filter["articles"][0]["publishedAt"] = true;
  filter["articles"][0]["source"]["name"] = true;
  filter["articles"][0]["source"]["country"] = true;
  DynamicJsonDocument document(24576);
  const auto jsonError = deserializeJson(
      document, http.getStream(), DeserializationOption::Filter(filter));
  http.end();
  if (jsonError) {
    error = String("GNews JSON: ") + jsonError.c_str();
    return false;
  }

  int bestScore = -1;
  std::size_t index = 0;
  for (JsonObject item : document["articles"].as<JsonArray>()) {
    const char* description = item["description"] | "";
    const char* title = item["title"] | "";
    const char* country = item["source"]["country"] | "us";
    if (description[0] == '\0' || strcmp(country, "us") != 0) {
      ++index;
      continue;
    }

    const std::string sentence = app_logic::extractFirstSentence(description);
    if (!containsAiTerm(sentence) && !containsAiTerm(title)) {
      ++index;
      continue;
    }
    const int score = app_logic::sentenceScore(sentence, index);
    if (score > bestScore) {
      bestScore = score;
      article.english = sentence.c_str();
      article.source = app_logic::cleanNewsText(
                           item["source"]["name"] | "Unknown source")
                           .c_str();
      article.publishedAt = item["publishedAt"] | "";
    }
    ++index;
  }

  if (bestScore < 0) {
    error = "No suitable sentence";
    return false;
  }
  return true;
}

bool NewsService::translateToKorean(const String& english, String& korean,
                                    String& error) {
  StaticJsonDocument<768> requestDocument;
  requestDocument.createNestedArray("text").add(english);
  requestDocument["source_lang"] = "EN";
  requestDocument["target_lang"] = "KO";
  String requestBody;
  serializeJson(requestDocument, requestBody);

  WiFiClientSecure client;
  client.setCACert(ISRG_ROOT_X1);
  HTTPClient http;
  http.useHTTP10(true);
  http.setTimeout(app_config::HTTP_TIMEOUT_MS);
  if (!http.begin(client, DEEPL_ENDPOINT)) {
    error = "DeepL connection failed";
    return false;
  }
  http.addHeader("Accept", "application/json");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", String("DeepL-Auth-Key ") +
                                      secrets::DEEPL_API_KEY);
  const int status = http.POST(requestBody);
  if (status != HTTP_CODE_OK) {
    error = httpError("DeepL", status);
    http.end();
    return false;
  }
  const String response = http.getString();
  http.end();
  StaticJsonDocument<2048> document;
  const auto jsonError = deserializeJson(document, response);
  if (jsonError) {
    error = String("DeepL JSON: ") + jsonError.c_str();
    return false;
  }
  const char* translated = document["translations"][0]["text"] | "";
  const std::string safeText = app_logic::sanitizeKoreanText(translated);
  if (safeText.empty()) {
    error = "DeepL returned no text";
    return false;
  }
  korean = safeText.c_str();
  return true;
}

String NewsService::urlEncode(const char* value) {
  static constexpr char HEX_DIGITS[] = "0123456789ABCDEF";
  String encoded;
  encoded.reserve(strlen(value) * 2);
  for (const unsigned char* current =
           reinterpret_cast<const unsigned char*>(value);
       *current; ++current) {
    const unsigned char character = *current;
    if (std::isalnum(character) || character == '-' || character == '_' ||
        character == '.' || character == '~') {
      encoded += static_cast<char>(character);
    } else {
      encoded += '%';
      encoded += HEX_DIGITS[character >> 4];
      encoded += HEX_DIGITS[character & 0x0F];
    }
  }
  return encoded;
}
