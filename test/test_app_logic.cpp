#include <unity.h>

#include "app_logic.h"

void test_extracts_first_complete_sentence() {
  TEST_ASSERT_EQUAL_STRING(
      "OpenAI released a new model for developers.",
      app_logic::extractFirstSentence(
          "OpenAI released a new model for developers. More details followed.")
          .c_str());
}

void test_does_not_split_us_abbreviation() {
  TEST_ASSERT_EQUAL_STRING(
      "A U.S. company released a new artificial intelligence model today.",
      app_logic::extractFirstSentence(
          "A U.S. company released a new artificial intelligence model today. "
          "It is available now.")
          .c_str());
}

void test_rejects_sentence_that_is_too_long() {
  const std::string text(141, 'a');
  TEST_ASSERT_TRUE(app_logic::extractFirstSentence(text + ".").empty());
}

void test_handles_sentence_final_abbreviation() {
  TEST_ASSERT_EQUAL_STRING(
      "The model is now available in the U.S.",
      app_logic::extractFirstSentence("The model is now available in the U.S.")
          .c_str());
}

void test_handles_curly_quote_and_parenthesis_endings() {
  TEST_ASSERT_EQUAL_STRING(
      "OpenAI called it \"a major breakthrough.\"",
      app_logic::extractFirstSentence(
          u8"OpenAI called it “a major breakthrough.” Another sentence follows.")
          .c_str());
  TEST_ASSERT_EQUAL_STRING(
      "The company released an AI model.)",
      app_logic::extractFirstSentence(
          "The company released an AI model.) More news followed.")
          .c_str());
}

void test_cleans_html_and_entities() {
  TEST_ASSERT_EQUAL_STRING(
      "AI & chips are \"important\".",
      app_logic::cleanNewsText("<b>AI</b> &amp; chips are &quot;important&quot;.")
          .c_str());
}

void test_replaces_unsupported_korean_glyphs() {
  TEST_ASSERT_EQUAL_STRING(
      u8"한글? 문장.", app_logic::sanitizeKoreanText(u8"한글😀 문장.").c_str());
}

void test_maps_battery_curve() {
  TEST_ASSERT_EQUAL_INT(0, app_logic::batteryPercentFromMillivolts(3300));
  TEST_ASSERT_EQUAL_INT(35, app_logic::batteryPercentFromMillivolts(3750));
  TEST_ASSERT_EQUAL_INT(100, app_logic::batteryPercentFromMillivolts(4300));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_extracts_first_complete_sentence);
  RUN_TEST(test_does_not_split_us_abbreviation);
  RUN_TEST(test_rejects_sentence_that_is_too_long);
  RUN_TEST(test_handles_sentence_final_abbreviation);
  RUN_TEST(test_handles_curly_quote_and_parenthesis_endings);
  RUN_TEST(test_cleans_html_and_entities);
  RUN_TEST(test_replaces_unsupported_korean_glyphs);
  RUN_TEST(test_maps_battery_curve);
  return UNITY_END();
}
