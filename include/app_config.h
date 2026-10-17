#pragma once

#include <cstring>

#include "secrets.h"

namespace app_config {
static constexpr char TIMEZONE[] = "KST-9";
static constexpr char NTP_SERVER_1[] = "time.google.com";
static constexpr char NTP_SERVER_2[] = "pool.ntp.org";
static constexpr char NTP_SERVER_3[] = "time.cloudflare.com";

static constexpr unsigned long WIFI_TIMEOUT_MS = 15000;
static constexpr unsigned long NTP_TIMEOUT_MS = 15000;
static constexpr unsigned long HTTP_TIMEOUT_MS = 15000;
static constexpr long RETRY_INTERVAL_SECONDS = 6L * 60L * 60L;
static constexpr int LOW_BATTERY_PERCENT = 10;

inline bool hasSecrets() {
  return std::strlen(secrets::WIFI_SSID) > 0 &&
         std::strlen(secrets::GNEWS_API_KEY) > 0 &&
         std::strlen(secrets::DEEPL_API_KEY) > 0;
}
}  // namespace app_config
