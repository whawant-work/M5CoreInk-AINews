#pragma once

#include <Arduino.h>

struct ArticleData {
  String dateKey;
  String english;
  String korean;
  String source;
  String publishedAt;

  bool isValid() const {
    return dateKey.length() == 10 && english.length() > 0 &&
           korean.length() > 0 && source.length() > 0;
  }
};
