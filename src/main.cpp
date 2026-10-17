#include <Arduino.h>
#include <HTTPClient.h>
#include <M5Unified.h>
#include <WiFi.h>

#include <algorithm>
#include <ctime>
#include <sys/time.h>
#include <esp_sntp.h>

#include "app_config.h"
#include "app_logic.h"
#include "app_ui.h"
#include "article_store.h"
#include "news_service.h"
#include "secrets.h"

namespace {
constexpr std::time_t MIN_VALID_EPOCH = 1704067200;  // 2024-01-01 UTC

ArticleStore store;
NewsService newsService;
AppUi ui;

bool systemClockIsValid() {
  return std::time(nullptr) >= MIN_VALID_EPOCH;
}

String koreanDateKey(std::time_t timestamp) {
  std::tm local{};
  localtime_r(&timestamp, &local);
  char buffer[11];
  std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", local.tm_year + 1900,
                local.tm_mon + 1, local.tm_mday);
  return buffer;
}

bool connectWifi() {
  Serial.println("Wi-Fi: connecting");
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.begin(secrets::WIFI_SSID, secrets::WIFI_PASSWORD);
  const unsigned long started = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - started < app_config::WIFI_TIMEOUT_MS) {
    delay(200);
  }
  const bool connected = WiFi.status() == WL_CONNECTED;
  Serial.println(connected ? "Wi-Fi: connected" : "Wi-Fi: failed");
  return connected;
}

bool syncClock() {
  Serial.println("NTP: synchronizing");
  configTzTime(app_config::TIMEZONE, app_config::NTP_SERVER_1,
               app_config::NTP_SERVER_2, app_config::NTP_SERVER_3);
  const unsigned long started = millis();
  while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED &&
         millis() - started < app_config::NTP_TIMEOUT_MS) {
    delay(250);
  }
  if (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED ||
      !systemClockIsValid()) {
    Serial.println("NTP: failed, trying HTTP date");
    HTTPClient http;
    http.setTimeout(app_config::HTTP_TIMEOUT_MS);
    const char* headerNames[] = {"Date"};
    http.collectHeaders(headerNames, 1);
    if (!http.begin("http://www.google.com/generate_204")) {
      Serial.println("HTTP time: connection failed");
      return false;
    }
    const int status = http.GET();
    const String dateHeader = http.header("Date");
    http.end();

    std::tm utc{};
    const char* parsed = strptime(dateHeader.c_str(),
                                  "%a, %d %b %Y %H:%M:%S GMT", &utc);
    if (status < 200 || status >= 400 || parsed == nullptr) {
      Serial.printf("HTTP time: failed (%d)\n", status);
      return false;
    }
    setenv("TZ", "UTC0", 1);
    tzset();
    const std::time_t timestamp = std::mktime(&utc);
    setenv("TZ", app_config::TIMEZONE, 1);
    tzset();
    const timeval synchronizedTime{timestamp, 0};
    settimeofday(&synchronizedTime, nullptr);
    Serial.println("HTTP time: synchronized");
  }
  const std::time_t now = std::time(nullptr);
  M5.Rtc.setDateTime(gmtime(&now));
  Serial.println("Clock: synchronized");
  return true;
}

void stopWifi() {
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
}

int readBatteryPercent() {
  int samples[5];
  for (int& sample : samples) {
    sample = M5.Power.getBatteryVoltage();
    delay(15);
  }
  std::sort(std::begin(samples), std::end(samples));
  return app_logic::batteryPercentFromMillivolts(samples[2]);
}

void initializeSystemClockFromRtc() {
  if (!M5.Rtc.isEnabled() || M5.Rtc.getVoltLow()) {
    const timeval invalidTime{0, 0};
    settimeofday(&invalidTime, nullptr);
    return;
  }
  M5.Rtc.setSystemTimeFromRtc();
  setenv("TZ", app_config::TIMEZONE, 1);
  tzset();
}

void sleepUntilNextHour() {
  std::time_t now = std::time(nullptr);
  int seconds = 3600;
  if (now >= MIN_VALID_EPOCH) {
    seconds = 3630 - static_cast<int>(now % 3600);
  }
  Serial.printf("Sleeping for %d seconds\n", seconds);
  Serial.flush();
  m5::rtc_datetime_t rtcNow;
  if (M5.Rtc.isEnabled() && M5.Rtc.getDateTime(&rtcNow)) {
    M5.Power.timerSleep(seconds);
  }
  M5.Power.deepSleep(static_cast<uint64_t>(seconds) * 1000000ULL, true);
}
}  // namespace

void setup() {
  auto config = M5.config();
  config.serial_baudrate = 115200;
  config.clear_display = false;
  config.internal_imu = false;
  config.internal_mic = false;
  config.internal_spk = false;
  config.external_display_value = 0;
  config.internal_rtc = true;
  config.fallback_board = m5::board_t::board_M5StackCoreInk;
  M5.begin(config);
  M5.Power.setLed(0);

  setenv("TZ", app_config::TIMEZONE, 1);
  tzset();
  initializeSystemClockFromRtc();

  const int batteryPercent = readBatteryPercent();
  Serial.printf("Configuration: %s\n",
                app_config::hasSecrets() ? "ready" : "missing");
  Serial.printf("Battery: %d%%\n", batteryPercent);
  ArticleData article;
  const bool hadCachedArticle = store.load(article);
  String status;

  std::time_t now = std::time(nullptr);
  String today = systemClockIsValid() ? koreanDateKey(now) : "";
  bool stale = !hadCachedArticle || today.isEmpty() || article.dateKey != today;
  const std::time_t lastAttempt = store.lastAttempt();
  const String lastAttemptDate = store.lastAttemptDate();
  const bool retryDue = !hadCachedArticle || !systemClockIsValid() ||
                        lastAttempt == 0 ||
                        lastAttemptDate != today || now <= lastAttempt ||
                        now - lastAttempt >= app_config::RETRY_INTERVAL_SECONDS;

  if (!app_config::hasSecrets()) {
    status = "CONFIG";
  } else if (batteryPercent < app_config::LOW_BATTERY_PERCENT) {
    status = "LOW BAT";
  } else if (stale && retryDue) {
    if (systemClockIsValid()) store.saveLastAttempt(now, today);
    if (!connectWifi()) {
      status = "NO WIFI";
    } else if (!syncClock()) {
      status = "NO TIME";
    } else {
      now = std::time(nullptr);
      today = koreanDateKey(now);
      store.saveLastAttempt(now, today);
      ArticleData downloaded;
      String error;
      if (newsService.fetchToday(today, downloaded, error) &&
          store.save(downloaded)) {
        article = downloaded;
        status = "NEW";
      } else {
        status = "OFFLINE";
        Serial.println(error);
      }
    }
    stopWifi();
  }

  if (!article.isValid()) {
    article.dateKey = today;
    if (status.isEmpty()) status = "WAIT";
  } else if (status.isEmpty()) {
    status = "CACHED";
  }
  Serial.printf("Result: %s, article: %s\n", status.c_str(),
                article.isValid() ? "ready" : "missing");
  ui.draw(article, batteryPercent, status);
  sleepUntilNextHour();
}

void loop() {
  delay(1000);
}
