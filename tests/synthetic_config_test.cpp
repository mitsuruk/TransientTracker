/**
 * @file synthetic_config_test.cpp
 * @brief 設定検証の単体テスト.
 */

#include <iostream>
#include <optional>
#include <string>

#include "TransientTracker/synthetic/scenario_config.hpp"
#include "TransientTracker/synthetic/scenario_repository.hpp"

namespace {

/**
 * @brief 条件を検証する.
 * @param condition 条件
 * @param message エラー文言
 * @return 失敗時の終了コード
 */
int Expect(bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << message << '\n';
    return 1;
  }
  return 0;
}

} // namespace

/**
 * @brief テストを実行する.
 * @return 終了コード
 */
int main() {
  using transient_tracker::synthetic::FindScenario;
  using transient_tracker::synthetic::ScenarioConfig;
  using transient_tracker::synthetic::ValidateScenarioConfig;

  const std::optional<ScenarioConfig> maybe_config = FindScenario("S-01");
  if (int result =
          Expect(maybe_config.has_value(), "S-01 の取得に失敗しました.");
      result != 0) {
    return result;
  }
  const std::optional<ScenarioConfig> maybe_multi_config = FindScenario("S-06");
  if (int result =
          Expect(maybe_multi_config.has_value(), "S-06 の取得に失敗しました.");
      result != 0) {
    return result;
  }

  std::string message;
  if (int result =
          Expect(ValidateScenarioConfig(*maybe_config, &message), message);
      result != 0) {
    return result;
  }
  if (int result = Expect(ValidateScenarioConfig(*maybe_multi_config, &message),
                          message);
      result != 0) {
    return result;
  }

  ScenarioConfig invalid_config = *maybe_config;
  invalid_config.num_frames = 0;
  if (int result = Expect(!ValidateScenarioConfig(invalid_config, &message),
                          "不正設定が通ってしまいました.");
      result != 0) {
    return result;
  }

  invalid_config = *maybe_config;
  invalid_config.hot_pixel_count = 1;
  invalid_config.hot_pixel_peak = 0.0;
  if (int result =
          Expect(!ValidateScenarioConfig(invalid_config, &message),
                 "hot_pixel_peak 未設定の不正設定が通ってしまいました.");
      result != 0) {
    return result;
  }

  invalid_config = *maybe_multi_config;
  invalid_config.moving_objects[1].object_id =
      invalid_config.moving_objects[0].object_id;
  if (int result = Expect(!ValidateScenarioConfig(invalid_config, &message),
                          "重複 object_id を持つ不正設定が通ってしまいました.");
      result != 0) {
    return result;
  }

  return 0;
}
