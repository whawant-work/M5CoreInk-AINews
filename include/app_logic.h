#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace app_logic {
std::string cleanNewsText(const std::string& input);
std::string extractFirstSentence(const std::string& input,
                                 std::size_t maxLength = 140);
std::string sanitizeKoreanText(const std::string& input);
int sentenceScore(const std::string& sentence, std::size_t resultIndex);
int batteryPercentFromMillivolts(std::int32_t millivolts);
}  // namespace app_logic
