#pragma once

#include <M5Unified.h>

#include "article.h"

class AppUi {
 public:
  void draw(const ArticleData& article, int batteryPercent, const String& status);

 private:
  static String shortDate(const String& dateKey);
  static String shortSource(const String& source);
  static void drawBattery(M5Canvas& canvas, int percent);
};
