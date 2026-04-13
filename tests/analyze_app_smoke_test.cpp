/**
 * @file analyze_app_smoke_test.cpp
 * @brief 解析器アプリケーションのスモークテスト.
 */

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <system_error>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

#include "TransientTracker/analyze/analysis_config.hpp"
#include "TransientTracker/analyze/analyze_app.hpp"
#include "TransientTracker/synthetic/synthetic_data_app.hpp"

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
 * @brief 一時データセットを生成する.
 * @param output_dir 出力先
 * @return 成功なら真
 */
bool GenerateDataset(const std::filesystem::path &output_dir) {
  transient_tracker::synthetic::CliOptions options;
  options.scenario_name = "S-02";
  options.output_dir = output_dir;
  options.seed = 42;
  options.has_seed = true;
  options.width = 220;
  options.has_width = true;
  options.height = 280;
  options.has_height = true;
  options.num_frames = 8;
  options.has_num_frames = true;
  options.num_stars = 30;
  options.has_num_stars = true;
  return transient_tracker::synthetic::RunSyntheticDataApp(options) == 0;
}

/**
 * @brief 複数移動天体データセットを生成する.
 * @param output_dir 出力先
 * @return 成功なら真
 */
bool GenerateMultiDataset(const std::filesystem::path &output_dir) {
  transient_tracker::synthetic::CliOptions options;
  options.scenario_name = "S-06";
  options.output_dir = output_dir;
  options.seed = 24;
  options.has_seed = true;
  options.width = 220;
  options.has_width = true;
  options.height = 280;
  options.has_height = true;
  options.num_frames = 8;
  options.has_num_frames = true;
  options.num_stars = 30;
  options.has_num_stars = true;
  return transient_tracker::synthetic::RunSyntheticDataApp(options) == 0;
}

/**
 * @brief テキストファイルを読む.
 * @param path 入力パス
 * @return 読み込んだ文字列
 */
std::string ReadTextFile(const std::filesystem::path &path) {
  std::ifstream stream(path);
  return std::string(std::istreambuf_iterator<char>(stream),
                     std::istreambuf_iterator<char>());
}

} // namespace

/**
 * @brief テストを実行する.
 * @return 終了コード
 */
int main() {
  const std::filesystem::path dataset_root =
      std::filesystem::temp_directory_path() /
      "transient_tracker_analyze_smoke_dataset";
  const std::filesystem::path output_root =
      std::filesystem::temp_directory_path() /
      "transient_tracker_analyze_smoke_output";
  const std::filesystem::path multi_dataset_root =
      std::filesystem::temp_directory_path() /
      "transient_tracker_analyze_multi_dataset";
  const std::filesystem::path multi_output_root =
      std::filesystem::temp_directory_path() /
      "transient_tracker_analyze_multi_output";
  const std::filesystem::path invalid_dataset_root =
      std::filesystem::temp_directory_path() /
      "transient_tracker_analyze_invalid_dataset";
  const std::filesystem::path invalid_output_root =
      std::filesystem::temp_directory_path() /
      "transient_tracker_analyze_invalid_output";

  std::error_code error;
  std::filesystem::remove_all(dataset_root, error);
  std::filesystem::remove_all(output_root, error);
  std::filesystem::remove_all(multi_dataset_root, error);
  std::filesystem::remove_all(multi_output_root, error);
  std::filesystem::remove_all(invalid_dataset_root, error);
  std::filesystem::remove_all(invalid_output_root, error);

  if (int result = Expect(GenerateDataset(dataset_root),
                          "テスト用データセットの生成に失敗しました.");
      result != 0) {
    return result;
  }

  transient_tracker::analyze::AnalysisConfig config;
  config.dataset_root = dataset_root;
  config.output_root = output_root;
  config.threshold_scale = 3.5;
  config.tolerance_radius = 1;
  config.area_min = 1;
  config.area_max = 64;
  config.max_velocity_per_frame = 3.0;
  config.min_track_length = 3;
  config.truth_match_distance = 2.0;
  config.write_debug_images = true;

  const int exit_code = transient_tracker::analyze::RunAnalyzeApp(config);
  if (int result = Expect(exit_code == 0, "解析器の実行に失敗しました.");
      result != 0) {
    return result;
  }
  if (int result = Expect(std::filesystem::exists(output_root / "summary.json"),
                          "summary.json が生成されていません.");
      result != 0) {
    return result;
  }
  if (int result =
          Expect(std::filesystem::exists(output_root / "detections.csv"),
                 "detections.csv が生成されていません.");
      result != 0) {
    return result;
  }
  if (int result = Expect(std::filesystem::exists(output_root / "tracks.csv"),
                          "tracks.csv が生成されていません.");
      result != 0) {
    return result;
  }
  if (int result =
          Expect(std::filesystem::exists(output_root / "evaluation.json"),
                 "evaluation.json が生成されていません.");
      result != 0) {
    return result;
  }
  if (int result = Expect(
          std::filesystem::exists(output_root / "results" / "result_0000.png"),
          "result_0000.png が生成されていません.");
      result != 0) {
    return result;
  }
  if (int result = Expect(std::filesystem::exists(output_root / "results" /
                                                  "trajectory_result.png"),
                          "trajectory_result.png が生成されていません.");
      result != 0) {
    return result;
  }
  if (int result = Expect(
          std::filesystem::exists(output_root / "debug" / "reference.png"),
          "reference.png が生成されていません.");
      result != 0) {
    return result;
  }
  if (int result =
          Expect(std::filesystem::exists(output_root / "debug" / "aligned" /
                                         "aligned_0000.png"),
                 "aligned_0000.png が生成されていません.");
      result != 0) {
    return result;
  }
  if (int result =
          Expect(std::filesystem::exists(output_root / "debug" / "residual" /
                                         "residual_0000.png"),
                 "residual_0000.png が生成されていません.");
      result != 0) {
    return result;
  }

  const std::string summary_text = ReadTextFile(output_root / "summary.json");
  const std::string evaluation_text =
      ReadTextFile(output_root / "evaluation.json");
  if (int result =
          Expect(summary_text.find("\"num_tracks\": 0") == std::string::npos,
                 "summary.json 上で軌跡数が 0 件です.");
      result != 0) {
    return result;
  }
  if (int result = Expect(summary_text.find("\"primary_track_id\": -1") ==
                              std::string::npos,
                          "summary.json 上で主軌跡が未選択です.");
      result != 0) {
    return result;
  }
  if (int result = Expect(
          evaluation_text.find("\"primary_track_matched_object_id\": -1") ==
              std::string::npos,
          "evaluation.json 上で主軌跡が真値へ一致していません.");
      result != 0) {
    return result;
  }

  if (int result = Expect(GenerateMultiDataset(multi_dataset_root),
                          "複数移動天体データセットの生成に失敗しました.");
      result != 0) {
    return result;
  }

  transient_tracker::analyze::AnalysisConfig multi_config = config;
  multi_config.dataset_root = multi_dataset_root;
  multi_config.output_root = multi_output_root;
  const int multi_exit_code =
      transient_tracker::analyze::RunAnalyzeApp(multi_config);
  if (int result = Expect(multi_exit_code == 0,
                          "複数移動天体データセットの解析に失敗しました.");
      result != 0) {
    return result;
  }

  const std::string multi_evaluation_text =
      ReadTextFile(multi_output_root / "evaluation.json");
  const std::string multi_summary_text =
      ReadTextFile(multi_output_root / "summary.json");
  if (int result =
          Expect(multi_evaluation_text.find("\"num_matched_objects\": 2") !=
                     std::string::npos,
                 "evaluation.json 上で複数移動天体が一致していません.");
      result != 0) {
    return result;
  }
  if (int result =
          Expect(multi_summary_text.find("\"num_matched_objects\": 2") !=
                     std::string::npos,
                 "summary.json 上で複数移動天体数が反映されていません.");
      result != 0) {
    return result;
  }

  if (int result = Expect(GenerateDataset(invalid_dataset_root),
                          "不正データセットの生成に失敗しました.");
      result != 0) {
    return result;
  }
  const cv::Mat corrupted_frame(40, 50, CV_8UC1, cv::Scalar(0));
  if (!cv::imwrite(
          (invalid_dataset_root / "frames" / "frame_0007.png").string(),
          corrupted_frame)) {
    std::cerr << "不正フレームの書き換えに失敗しました." << '\n';
    return 1;
  }

  transient_tracker::analyze::AnalysisConfig invalid_config = config;
  invalid_config.dataset_root = invalid_dataset_root;
  invalid_config.output_root = invalid_output_root;
  const int invalid_exit_code =
      transient_tracker::analyze::RunAnalyzeApp(invalid_config);
  if (int result = Expect(invalid_exit_code != 0,
                          "異常サイズフレームを検知できていません.");
      result != 0) {
    return result;
  }

  std::filesystem::remove_all(dataset_root, error);
  std::filesystem::remove_all(output_root, error);
  std::filesystem::remove_all(multi_dataset_root, error);
  std::filesystem::remove_all(multi_output_root, error);
  std::filesystem::remove_all(invalid_dataset_root, error);
  std::filesystem::remove_all(invalid_output_root, error);
  return 0;
}
