#include "article_store.h"

#include <ArduinoJson.h>
#include <Preferences.h>

namespace {
constexpr char PREFERENCES_NAMESPACE[] = "today-ai";
constexpr char ARTICLE_KEY[] = "article";
constexpr char LAST_ATTEMPT_KEY[] = "attempt";
constexpr char LAST_ATTEMPT_DATE_KEY[] = "attemptDate";
}  // namespace

bool ArticleStore::load(ArticleData& article) const {
  Preferences preferences;
  if (!preferences.begin(PREFERENCES_NAMESPACE, false)) return false;
  if (!preferences.isKey(ARTICLE_KEY)) {
    preferences.end();
    return false;
  }
  const String json = preferences.getString(ARTICLE_KEY, "");
  preferences.end();
  if (json.isEmpty()) return false;

  StaticJsonDocument<1536> document;
  if (deserializeJson(document, json) != DeserializationError::Ok) return false;
  article.dateKey = document["date"] | "";
  article.english = document["english"] | "";
  article.korean = document["korean"] | "";
  article.source = document["source"] | "";
  article.publishedAt = document["published"] | "";
  return article.isValid();
}

bool ArticleStore::save(const ArticleData& article) const {
  if (!article.isValid()) return false;
  StaticJsonDocument<1536> document;
  document["date"] = article.dateKey;
  document["english"] = article.english;
  document["korean"] = article.korean;
  document["source"] = article.source;
  document["published"] = article.publishedAt;

  String json;
  serializeJson(document, json);
  Preferences preferences;
  if (!preferences.begin(PREFERENCES_NAMESPACE, false)) return false;
  const std::size_t written = preferences.putString(ARTICLE_KEY, json);
  preferences.end();
  return written == json.length();
}

std::time_t ArticleStore::lastAttempt() const {
  Preferences preferences;
  if (!preferences.begin(PREFERENCES_NAMESPACE, false)) return 0;
  if (!preferences.isKey(LAST_ATTEMPT_KEY)) {
    preferences.end();
    return 0;
  }
  const std::time_t result = static_cast<std::time_t>(
      preferences.getULong64(LAST_ATTEMPT_KEY, 0));
  preferences.end();
  return result;
}

String ArticleStore::lastAttemptDate() const {
  Preferences preferences;
  if (!preferences.begin(PREFERENCES_NAMESPACE, false)) return "";
  if (!preferences.isKey(LAST_ATTEMPT_DATE_KEY)) {
    preferences.end();
    return "";
  }
  const String result = preferences.getString(LAST_ATTEMPT_DATE_KEY, "");
  preferences.end();
  return result;
}

void ArticleStore::saveLastAttempt(std::time_t timestamp,
                                   const String& dateKey) const {
  Preferences preferences;
  if (!preferences.begin(PREFERENCES_NAMESPACE, false)) return;
  preferences.putULong64(LAST_ATTEMPT_KEY, static_cast<uint64_t>(timestamp));
  preferences.putString(LAST_ATTEMPT_DATE_KEY, dateKey);
  preferences.end();
}
