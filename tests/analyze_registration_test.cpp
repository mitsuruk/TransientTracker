/**
 * @file analyze_registration_test.cpp
 * @brief 並進位置合わせの改善効果を検証するテスト.
 */

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

#include <xtensor/containers/xtensor.hpp>

#include "TransientTracker/analyze/dataset_reader.hpp"
#include "TransientTracker/analyze/frame_aligner.hpp"
#include "TransientTracker/analyze/frame_loader.hpp"
#include "TransientTracker/analyze/frame_normalizer.hpp"
#include "TransientTracker/analyze/registration_estimator.hpp"
#include "TransientTracker/synthetic/synthetic_data_app.hpp"

namespace {

/**
 * @brief 条件を検証する.
 * @param condition 条件
 * @param message エラー文言
 * @return 失敗時の終了コード
 */
int Expect(bool condition, const std::string& message) {
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
bool GenerateDataset(const std::filesystem::path& output_dir) {
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
 * @brief 2画像の平均絶対差を返す.
 * @param lhs 左画像
 * @param rhs 右画像
 * @return 平均絶対差
 */
double ComputeMeanAbsoluteDifference(
    const xt::xtensor<float, 2>& lhs,
    const xt::xtensor<float, 2>& rhs) {
    const std::size_t height = lhs.shape()[0];
    const std::size_t width = lhs.shape()[1];
    double sum = 0.0;
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            sum += std::fabs(static_cast<double>(lhs(y, x) - rhs(y, x)));
        }
    }
    return sum / static_cast<double>(height * width);
}

}  // namespace

/**
 * @brief テストを実行する.
 * @return 終了コード
 */
int main() {
    using transient_tracker::analyze::AlignFramesByTranslation;
    using transient_tracker::analyze::BuildDatasetPaths;
    using transient_tracker::analyze::DatasetPaths;
    using transient_tracker::analyze::EstimateTranslationsPhaseCorrelation;
    using transient_tracker::analyze::ListFramePaths;
    using transient_tracker::analyze::LoadFrames;
    using transient_tracker::analyze::NormalizeFramesByMedian;

    const std::filesystem::path dataset_root =
        std::filesystem::temp_directory_path() / "transient_tracker_analyze_registration";
    std::error_code error;
    std::filesystem::remove_all(dataset_root, error);

    if (int result = Expect(GenerateDataset(dataset_root), "テスト用データセットの生成に失敗しました."); result != 0) {
        return result;
    }

    const DatasetPaths paths = BuildDatasetPaths(dataset_root);
    std::string message;
    std::vector<std::filesystem::path> frame_paths;
    std::vector<xt::xtensor<float, 2>> frames;
    if (int result = Expect(ListFramePaths(paths, &frame_paths, &message), message); result != 0) {
        return result;
    }
    if (int result = Expect(LoadFrames(frame_paths, &frames, &message), message); result != 0) {
        return result;
    }

    const std::vector<xt::xtensor<float, 2>> normalized_frames = NormalizeFramesByMedian(frames);
    const std::vector<transient_tracker::analyze::Shift2D> shifts =
        EstimateTranslationsPhaseCorrelation(normalized_frames);
    const std::vector<xt::xtensor<float, 2>> aligned_frames =
        AlignFramesByTranslation(normalized_frames, shifts);

    const xt::xtensor<float, 2>& reference = normalized_frames.front();
    double before = 0.0;
    double after = 0.0;
    for (std::size_t frame_index = 1; frame_index < normalized_frames.size(); ++frame_index) {
        before += ComputeMeanAbsoluteDifference(reference, normalized_frames[frame_index]);
        after += ComputeMeanAbsoluteDifference(reference, aligned_frames[frame_index]);
    }
    before /= static_cast<double>(normalized_frames.size() - 1);
    after /= static_cast<double>(normalized_frames.size() - 1);

    if (int result = Expect(after < before, "位置合わせ後の平均差分が改善していません."); result != 0) {
        return result;
    }

    std::vector<transient_tracker::analyze::Shift2D> invalid_shifts = shifts;
    invalid_shifts.pop_back();
    const std::vector<xt::xtensor<float, 2>> invalid_result =
        AlignFramesByTranslation(normalized_frames, invalid_shifts);
    if (int result = Expect(invalid_result.empty(), "シフト数不一致時に空結果が返っていません."); result != 0) {
        return result;
    }

    std::filesystem::remove_all(dataset_root, error);
    return 0;
}
