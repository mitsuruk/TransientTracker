/**
 * @file analyze_dataset_reader_test.cpp
 * @brief 解析器のデータセット読込テスト.
 */

#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

#include <xtensor/containers/xtensor.hpp>

#include "TransientTracker/analyze/analysis_types.hpp"
#include "TransientTracker/analyze/dataset_reader.hpp"
#include "TransientTracker/analyze/frame_loader.hpp"
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
  options.scenario_name = "S-01";
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

} // namespace

/**
 * @brief テストを実行する.
 * @return 終了コード
 */
int main() {
  using transient_tracker::analyze::BuildDatasetPaths;
  using transient_tracker::analyze::DatasetMetadata;
  using transient_tracker::analyze::DatasetPaths;
  using transient_tracker::analyze::GroundTruthRecord;
  using transient_tracker::analyze::ListFramePaths;
  using transient_tracker::analyze::LoadFrames;
  using transient_tracker::analyze::ReadDatasetMetadata;
  using transient_tracker::analyze::ReadGroundTruthCsv;
  using transient_tracker::analyze::ValidateDatasetStructure;

  const std::filesystem::path dataset_root =
      std::filesystem::temp_directory_path() /
      "transient_tracker_analyze_dataset_reader";
  std::error_code error;
  std::filesystem::remove_all(dataset_root, error);

  if (int result = Expect(GenerateDataset(dataset_root),
                          "テスト用データセットの生成に失敗しました.");
      result != 0) {
    return result;
  }

  const DatasetPaths paths = BuildDatasetPaths(dataset_root);
  std::string message;
  if (int result = Expect(ValidateDatasetStructure(paths, &message), message);
      result != 0) {
    return result;
  }

  DatasetMetadata metadata;
  if (int result = Expect(
          ReadDatasetMetadata(paths.metadata_json_path, &metadata, &message),
          message);
      result != 0) {
    return result;
  }
  if (int result = Expect(metadata.num_frames == 8,
                          "metadata.json の num_frames が不正です.");
      result != 0) {
    return result;
  }
  if (int result = Expect(metadata.num_objects == 3,
                          "metadata.json の num_objects が不正です.");
      result != 0) {
    return result;
  }

  std::vector<GroundTruthRecord> truths;
  if (int result =
          Expect(ReadGroundTruthCsv(paths.labels_csv_path, &truths, &message),
                 message);
      result != 0) {
    return result;
  }
  if (int result =
          Expect(truths.size() == metadata.num_frames * metadata.num_objects,
                 "labels.csv の行数が不正です.");
      result != 0) {
    return result;
  }

  std::vector<std::filesystem::path> frame_paths;
  if (int result =
          Expect(ListFramePaths(paths, &frame_paths, &message), message);
      result != 0) {
    return result;
  }
  if (int result = Expect(frame_paths.size() == metadata.num_frames,
                          "フレーム数が metadata と一致しません.");
      result != 0) {
    return result;
  }

  std::vector<xt::xtensor<float, 2>> frames;
  if (int result = Expect(LoadFrames(frame_paths, &frames, &message), message);
      result != 0) {
    return result;
  }
  if (int result = Expect(frames.front().shape()[0] == metadata.height &&
                              frames.front().shape()[1] == metadata.width,
                          "読込フレームサイズが metadata と一致しません.");
      result != 0) {
    return result;
  }

  std::filesystem::remove_all(dataset_root, error);
  return 0;
}
