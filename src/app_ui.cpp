#include "app_ui.h"

namespace {
void printParagraph(M5Canvas& canvas, const String& text, int x, int y,
                    int width, int height, const lgfx::IFont* font,
                    float size = 1.0f) {
  canvas.setClipRect(x, y, width, height);
  canvas.setFont(font);
  canvas.setTextSize(size);
  canvas.setTextColor(TFT_BLACK, TFT_WHITE);
  canvas.setTextDatum(lgfx::textdatum_t::top_left);
  canvas.setTextWrap(true, false);
  canvas.setCursor(x, y);
  canvas.print(text);
  canvas.clearClipRect();
}
}  // namespace

void AppUi::draw(const ArticleData& article, int batteryPercent,
                 const String& status) {
  // CoreInk can cut panel power between wakes, so differential refresh state
  // cannot be assumed to survive. A quality refresh is reliable on all panels.
  M5.Display.setEpdMode(m5gfx::epd_quality);

  M5Canvas frame(&M5.Display);
  frame.setColorDepth(1);
  if (!frame.createSprite(200, 200)) return;
  frame.fillSprite(TFT_WHITE);

  frame.setTextColor(TFT_BLACK, TFT_WHITE);
  frame.setFont(&fonts::DejaVu12);
  frame.setTextSize(1);
  frame.setTextDatum(lgfx::textdatum_t::top_left);
  frame.drawString("AI / US", 5, 3);
  frame.setFont(&fonts::DejaVu9);
  frame.drawString(shortDate(article.dateKey), 66, 6);
  drawBattery(frame, batteryPercent);
  frame.drawFastHLine(5, 22, 190, TFT_BLACK);

  const String english = article.english.length() > 0
                             ? article.english
                             : "Add Wi-Fi and API keys in include/secrets.h.";
  const lgfx::IFont* englishFont =
      english.length() > 110 ? static_cast<const lgfx::IFont*>(&fonts::DejaVu9)
                             : static_cast<const lgfx::IFont*>(&fonts::DejaVu12);
  printParagraph(frame, english, 6, 29, 188, 60, englishFont);

  frame.drawFastHLine(52, 95, 143, TFT_BLACK);
  frame.fillRect(6, 91, 38, 10, TFT_BLACK);
  frame.setFont(&fonts::DejaVu9);
  frame.setTextColor(TFT_WHITE, TFT_BLACK);
  frame.drawString("KOR", 14, 92);

  const String korean = article.korean.length() > 0
                            ? article.korean
                            : String(u8"설정 파일에 접속 정보를 입력하세요.");
  const lgfx::IFont* koreanFont =
      korean.length() > 150
          ? static_cast<const lgfx::IFont*>(&fonts::efontKR_14)
          : static_cast<const lgfx::IFont*>(&fonts::efontKR_16);
  printParagraph(frame, korean, 6, 104, 188, 74, koreanFont);

  frame.drawFastHLine(5, 182, 190, TFT_BLACK);
  frame.setFont(&fonts::DejaVu9);
  frame.setTextSize(1);
  frame.setTextColor(TFT_BLACK, TFT_WHITE);
  frame.setTextDatum(lgfx::textdatum_t::top_left);
  frame.drawString(shortSource(article.source), 5, 187);
  if (status.length() > 0) {
    frame.setTextDatum(lgfx::textdatum_t::top_right);
    frame.drawString(status, 195, 187);
  }

  frame.pushSprite(0, 0);
  M5.Display.waitDisplay();
  frame.deleteSprite();
}

String AppUi::shortDate(const String& dateKey) {
  if (dateKey.length() != 10) return "SETUP";
  return dateKey.substring(5, 7) + "." + dateKey.substring(8, 10);
}

String AppUi::shortSource(const String& source) {
  if (source.isEmpty()) return "TODAY'S AI SENTENCE";
  if (source.length() <= 16) return source;
  return source.substring(0, 14) + "..";
}

void AppUi::drawBattery(M5Canvas& canvas, int percent) {
  const int clamped = std::max(0, std::min(100, percent));
  canvas.drawRect(175, 5, 18, 11, TFT_BLACK);
  canvas.fillRect(193, 8, 2, 5, TFT_BLACK);
  const int fillWidth = (clamped * 14) / 100;
  if (fillWidth > 0) canvas.fillRect(177, 7, fillWidth, 7, TFT_BLACK);
  canvas.setFont(&fonts::DejaVu9);
  canvas.setTextSize(1);
  canvas.setTextColor(TFT_BLACK, TFT_WHITE);
  canvas.setTextDatum(lgfx::textdatum_t::top_right);
  canvas.drawString(String(clamped) + "%", 170, 6);
}
