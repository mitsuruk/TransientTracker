/**
 * @file analyze_track_linker_test.cpp
 * @brief 軌跡連結と真値照合を検証するテスト.
 */

#include <iostream>
#include <string>
#include <vector>

#include "TransientTracker/analyze/analysis_types.hpp"
#include "TransientTracker/analyze/track_linker.hpp"
#include "TransientTracker/analyze/truth_evaluator.hpp"

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
 * @brief 検出点を作る.
 * @param frame_index フレーム番号
 * @param detection_id 候補ID
 * @param x X座標
 * @param y Y座標
 * @param score スコア
 * @return 検出点
 */
transient_tracker::analyze::Detection MakeDetection(std::size_t frame_index,
                                                    std::size_t detection_id,
                                                    double x, double y,
                                                    double score) {
  transient_tracker::analyze::Detection detection;
  detection.frame_index = frame_index;
  detection.detection_id = detection_id;
  detection.x = x;
  detection.y = y;
  detection.peak_value = score;
  detection.sum_value = score;
  detection.area = 1;
  detection.sigma_estimate = 1.0;
  detection.score = score;
  return detection;
}

/**
 * @brief 真値を作る.
 * @param frame_index フレーム番号
 * @param x X座標
 * @param y Y座標
 * @return 真値
 */
transient_tracker::analyze::GroundTruthRecord
MakeTruth(std::size_t frame_index, std::size_t object_id, double x, double y) {
  transient_tracker::analyze::GroundTruthRecord truth;
  truth.frame_index = frame_index;
  truth.object_id = object_id;
  truth.object_x = x;
  truth.object_y = y;
  truth.object_visible = true;
  return truth;
}

} // namespace

/**
 * @brief テストを実行する.
 * @return 終了コード
 */
int main() {
  std::vector<std::vector<transient_tracker::analyze::Detection>>
      detections_by_frame(4);
  detections_by_frame[0].push_back(MakeDetection(0, 0, 10.0, 20.0, 8.0));
  detections_by_frame[0].push_back(MakeDetection(0, 1, 80.0, 90.0, 2.0));
  detections_by_frame[0].push_back(MakeDetection(0, 2, 40.0, 30.0, 7.0));
  detections_by_frame[1].push_back(MakeDetection(1, 3, 11.1, 20.4, 7.5));
  detections_by_frame[1].push_back(MakeDetection(1, 4, 39.2, 31.2, 6.8));
  detections_by_frame[1].push_back(MakeDetection(1, 5, 78.5, 88.0, 2.0));
  detections_by_frame[2].push_back(MakeDetection(2, 6, 12.2, 21.0, 7.0));
  detections_by_frame[2].push_back(MakeDetection(2, 7, 38.4, 32.4, 6.5));
  detections_by_frame[3].push_back(MakeDetection(3, 8, 13.3, 21.5, 6.8));
  detections_by_frame[3].push_back(MakeDetection(3, 9, 37.6, 33.6, 6.2));

  std::vector<transient_tracker::analyze::Track> tracks =
      transient_tracker::analyze::LinkTracksGreedy(detections_by_frame, 2.0, 3);
  if (int result =
          Expect(tracks.size() == 2, "期待した軌跡数になっていません.");
      result != 0) {
    return result;
  }
  if (int result = Expect(tracks.front().detections.size() == 4,
                          "第1軌跡の長さが不正です.");
      result != 0) {
    return result;
  }
  if (int result =
          Expect(tracks[1].detections.size() == 4, "第2軌跡の長さが不正です.");
      result != 0) {
    return result;
  }

  const std::vector<transient_tracker::analyze::GroundTruthRecord> truths = {
      MakeTruth(0, 0, 10.0, 20.0), MakeTruth(1, 0, 11.1, 20.4),
      MakeTruth(2, 0, 12.2, 21.0), MakeTruth(3, 0, 13.3, 21.5),
      MakeTruth(0, 1, 40.0, 30.0), MakeTruth(1, 1, 39.2, 31.2),
      MakeTruth(2, 1, 38.4, 32.4), MakeTruth(3, 1, 37.6, 33.6),
  };
  const transient_tracker::analyze::EvaluationSummary evaluation =
      transient_tracker::analyze::EvaluateTracksAgainstTruth(truths, &tracks,
                                                             true, 0.75);
  const std::vector<transient_tracker::analyze::Track> representative_tracks =
      transient_tracker::analyze::SelectRepresentativeMatchedTracks(tracks);
  const transient_tracker::analyze::Track *primary_track =
      transient_tracker::analyze::SelectPrimaryTrack(tracks);

  if (int result = Expect(evaluation.num_visible_objects == 2,
                          "可視真値天体数が不正です.");
      result != 0) {
    return result;
  }
  if (int result = Expect(evaluation.num_matched_tracks == 2,
                          "真値一致軌跡数が不正です.");
      result != 0) {
    return result;
  }
  if (int result = Expect(evaluation.num_matched_objects == 2,
                          "真値一致天体数が不正です.");
      result != 0) {
    return result;
  }
  if (int result = Expect(representative_tracks.size() == 2,
                          "代表一致軌跡数が不正です.");
      result != 0) {
    return result;
  }
  if (int result =
          Expect(primary_track != nullptr, "主軌跡が選択されていません.");
      result != 0) {
    return result;
  }
  if (int result = Expect(primary_track->matched_object_id >= 0,
                          "主軌跡が真値へ一致していません.");
      result != 0) {
    return result;
  }
  if (int result = Expect(primary_track->matched_frame_count == 4,
                          "主軌跡の一致フレーム数が不正です.");
      result != 0) {
    return result;
  }
  if (int result = Expect(representative_tracks[0].matched_object_id == 0 &&
                              representative_tracks[1].matched_object_id == 1,
                          "代表一致軌跡の object_id が不正です.");
      result != 0) {
    return result;
  }
  if (int result = Expect(evaluation.matched_objects.size() == 2 &&
                              evaluation.matched_objects[0].object_id == 0 &&
                              evaluation.matched_objects[1].object_id == 1,
                          "評価サマリの matched_objects が不正です.");
      result != 0) {
    return result;
  }

  return 0;
}
