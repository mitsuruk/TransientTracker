/**
 * @file synthetic_app_smoke_test.cpp
 * @brief 生成器のスモークテスト.
 */

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <system_error>

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

} // namespace

/**
 * @brief テストを実行する.
 * @return 終了コード
 */
int main() {
  using transient_tracker::synthetic::CliOptions;
  using transient_tracker::synthetic::RunSyntheticDataApp;

  const std::filesystem::path output_dir =
      std::filesystem::temp_directory_path() /
      "transient_tracker_generate_smoke";
  const std::filesystem::path multi_output_dir =
      std::filesystem::temp_directory_path() /
      "transient_tracker_generate_multi_smoke";

  std::error_code error;
  std::filesystem::remove_all(output_dir, error);
  std::filesystem::remove_all(multi_output_dir, error);

  CliOptions options;
  options.scenario_name = "S-01";
  options.output_dir = output_dir;
  options.seed = 42;
  options.has_seed = true;

  const int exit_code = RunSyntheticDataApp(options);
  if (int result = Expect(exit_code == 0, "生成器の実行に失敗しました.");
      result != 0) {
    return result;
  }
  if (int result = Expect(
          std::filesystem::exists(output_dir / "frames" / "frame_0000.png"),
          "frame_0000.png が生成されていません.");
      result != 0) {
    return result;
  }
  if (int result = Expect(std::filesystem::exists(output_dir / "labels.csv"),
                          "labels.csv が生成されていません.");
      result != 0) {
    return result;
  }
  if (int result = Expect(std::filesystem::exists(output_dir / "metadata.json"),
                          "metadata.json が生成されていません.");
      result != 0) {
    return result;
  }

  std::ifstream single_metadata_stream(output_dir / "metadata.json");
  const std::string single_metadata_text{
      std::istreambuf_iterator<char>(single_metadata_stream),
      std::istreambuf_iterator<char>()};
  if (int result = Expect(
          single_metadata_text.find("\"num_objects\": 3") != std::string::npos,
          "S-01 の metadata.json に 3 移動天体が出力されていません.");
      result != 0) {
    return result;
  }

  CliOptions multi_options;
  multi_options.scenario_name = "S-06";
  multi_options.output_dir = multi_output_dir;
  multi_options.seed = 99;
  multi_options.has_seed = true;

  const int multi_exit_code = RunSyntheticDataApp(multi_options);
  if (int result = Expect(multi_exit_code == 0,
                          "複数移動天体シナリオの生成に失敗しました.");
      result != 0) {
    return result;
  }

  std::ifstream metadata_stream(multi_output_dir / "metadata.json");
  const std::string metadata_text{
      std::istreambuf_iterator<char>(metadata_stream),
      std::istreambuf_iterator<char>()};
  if (int result =
          Expect(metadata_text.find("\"num_objects\": 2") != std::string::npos,
                 "metadata.json に複数移動天体数が出力されていません.");
      result != 0) {
    return result;
  }

  std::ifstream labels_stream(multi_output_dir / "labels.csv");
  const std::string labels_text{std::istreambuf_iterator<char>(labels_stream),
                                std::istreambuf_iterator<char>()};
  if (int result = Expect(labels_text.find(",0,") != std::string::npos &&
                              labels_text.find(",1,") != std::string::npos,
                          "labels.csv に複数 object_id が出力されていません.");
      result != 0) {
    return result;
  }

  std::filesystem::remove_all(output_dir, error);
  std::filesystem::remove_all(multi_output_dir, error);
  return 0;
}
