/**
 * @file synthetic_truth_test.cpp
 * @brief 真値生成の単体テスト.
 */

#include <iostream>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include "TransientTracker/synthetic/frame_shift_generator.hpp"
#include "TransientTracker/synthetic/moving_object_generator.hpp"
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

/**
 * @brief 指定フレームと指定天体IDの真値を探す.
 * @param truths 真値一覧
 * @param frame_index フレーム番号
 * @param object_id 天体ID
 * @return 見つかった真値. 見つからなければ nullptr
 */
const transient_tracker::synthetic::FrameTruth *
FindTruth(const std::vector<transient_tracker::synthetic::FrameTruth> &truths,
          std::size_t frame_index, std::size_t object_id) {
  for (const transient_tracker::synthetic::FrameTruth &truth : truths) {
    if (truth.frame_index == frame_index && truth.object_id == object_id) {
      return &truth;
    }
  }
  return nullptr;
}

} // namespace

/**
 * @brief テストを実行する.
 * @return 終了コード
 */
int main() {
  using transient_tracker::synthetic::FindScenario;
  using transient_tracker::synthetic::FrameShift;
  using transient_tracker::synthetic::FrameTruth;
  using transient_tracker::synthetic::GenerateFrameShifts;
  using transient_tracker::synthetic::GenerateObjectTruths;
  using transient_tracker::synthetic::ScenarioConfig;

  const std::optional<ScenarioConfig> maybe_config = FindScenario("S-01");
  if (int result =
          Expect(maybe_config.has_value(), "S-01 の取得に失敗しました.");
      result != 0) {
    return result;
  }

  const ScenarioConfig config = *maybe_config;
  const std::vector<FrameTruth> truths = GenerateObjectTruths(config);
  if (int result = Expect(truths.size() ==
                              config.num_frames * config.moving_objects.size(),
                          "真値数がフレーム数と移動天体数の積と一致しません.");
      result != 0) {
    return result;
  }
  if (int result = Expect(config.moving_objects.size() == 3,
                          "S-01 が 3 移動天体になっていません.");
      result != 0) {
    return result;
  }

  const transient_tracker::synthetic::MovingObjectSpec &object =
      config.moving_objects.front();
  const double expected_x = object.initial_x + object.velocity_x;
  const double expected_y = object.initial_y + object.velocity_y;
  const transient_tracker::synthetic::FrameTruth *frame1_object0 =
      FindTruth(truths, 1, object.object_id);
  if (int result = Expect(frame1_object0 != nullptr,
                          "frame 1, object 0 の真値が見つかりません.");
      result != 0) {
    return result;
  }
  if (int result = Expect(frame1_object0->object_x == expected_x,
                          "X座標更新が期待値と一致しません.");
      result != 0) {
    return result;
  }
  if (int result = Expect(frame1_object0->object_y == expected_y,
                          "Y座標更新が期待値と一致しません.");
      result != 0) {
    return result;
  }

  const std::optional<ScenarioConfig> maybe_multi_config = FindScenario("S-06");
  if (int result =
          Expect(maybe_multi_config.has_value(), "S-06 の取得に失敗しました.");
      result != 0) {
    return result;
  }

  const ScenarioConfig multi_config = *maybe_multi_config;
  const std::vector<FrameTruth> multi_truths =
      GenerateObjectTruths(multi_config);
  if (int result =
          Expect(multi_truths.size() == multi_config.num_frames *
                                            multi_config.moving_objects.size(),
                 "複数移動天体シナリオの真値数が不正です.");
      result != 0) {
    return result;
  }
  if (int result = Expect(multi_truths[0].object_id == 0 &&
                              multi_truths[1].object_id == 1,
                          "複数移動天体シナリオの object_id 並びが不正です.");
      result != 0) {
    return result;
  }

  std::mt19937_64 rng(config.seed);
  const std::vector<FrameShift> shifts = GenerateFrameShifts(config, &rng);
  if (int result = Expect(shifts.size() == config.num_frames,
                          "シフト数がフレーム数と一致しません.");
      result != 0) {
    return result;
  }
  if (int result = Expect(shifts.front().dx == 0.0 && shifts.front().dy == 0.0,
                          "先頭フレームが 0,0 ではありません.");
      result != 0) {
    return result;
  }

  return 0;
}
