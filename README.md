# Today AI News for M5Stack Core Ink

미국 AI 뉴스에서 하루 한 문장을 골라 영어 원문과 한국어 번역을 함께
표시하는 M5Stack Core Ink 앱입니다. 매시간 RTC로 깨어 배터리를 갱신하고,
한국 날짜가 바뀐 경우에만 Wi-Fi와 API를 사용합니다.

## 준비

1. [GNews](https://gnews.io/) API 키를 발급합니다.
2. [DeepL API Free](https://www.deepl.com/pro-api) 키를 발급합니다.
3. `include/secrets.example.h`를 참고해 `include/secrets.h`를 작성합니다.

```cpp
#pragma once
namespace secrets {
static constexpr char WIFI_SSID[] = "wifi-name";
static constexpr char WIFI_PASSWORD[] = "wifi-password";
static constexpr char GNEWS_API_KEY[] = "gnews-key";
static constexpr char DEEPL_API_KEY[] = "deepl-key";
}
```

`include/secrets.h`는 Git에서 제외됩니다. 키는 펌웨어에 포함되므로 개인용
기기에서만 사용하고 펌웨어 파일을 공개하지 마세요.

## 빌드 및 업로드

```bash
~/.platformio/penv/bin/platformio run -e m5stack-coreink
~/.platformio/penv/bin/platformio run -e m5stack-coreink -t upload
```

## 동작

- RTC와 시스템 시계에는 UTC를 사용하고 표시 날짜는 `Asia/Seoul` 기준입니다.
- 날짜가 바뀌면 GNews에서 미국 발행처의 최신 AI 기사 10건을 조회합니다.
- 화면에 맞는 완결된 문장을 선택하고 DeepL로 한국어 번역합니다.
- 정상 결과만 NVS에 저장하며 네트워크 장애 시 이전 문장을 유지합니다.
- 실패한 일일 갱신은 최대 6시간 간격으로 재시도합니다.
- 배터리 10% 미만에서는 네트워크를 사용하지 않습니다.
- 완전 전원 종료 뒤에도 모든 Core Ink 패널에서 정확히 표시되도록 매시간
  품질 모드로 한 번만 갱신합니다.

Core Ink는 USB가 연결된 동안 RTC 알람에 의한 완전한 전원 종료가 제한될 수
있습니다. 배터리 사용 시간과 시간별 기동은 USB를 분리한 상태에서 확인하세요.

## 테스트

```bash
~/.platformio/penv/bin/platformio test -e native
```
