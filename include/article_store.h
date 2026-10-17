#pragma once

#include <ctime>

#include "article.h"

class ArticleStore {
 public:
  bool load(ArticleData& article) const;
  bool save(const ArticleData& article) const;
  std::time_t lastAttempt() const;
  String lastAttemptDate() const;
  void saveLastAttempt(std::time_t timestamp, const String& dateKey) const;
};
