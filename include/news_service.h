#pragma once

#include <Arduino.h>

#include "article.h"

class NewsService {
 public:
  bool fetchToday(const String& dateKey, ArticleData& article, String& error);

 private:
  bool fetchEnglish(ArticleData& article, String& error);
  bool translateToKorean(const String& english, String& korean, String& error);
  static String urlEncode(const char* value);
};
