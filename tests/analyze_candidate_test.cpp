/**
 * @file analyze_candidate_test.cpp
 * @brief 解析器の候補抽出テスト.
 */

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

#include <xtensor/containers/xtensor.hpp>

#include "TransientTracker/analyze/analysis_types.hpp"
#include "TransientTracker/analyze/candidate_extractor.hpp"
#include "TransientTracker/analyze/dataset_reader.hpp"
#include "TransientTracker/analyze/frame_aligner.hpp"
#include "TransientTracker/analyze/frame_loader.hpp"
#include "TransientTracker/analyze/frame_normalizer.hpp"
#include "TransientTracker/analyze/registration_estimator.hpp"
#include "TransientTracker/analyze/reference_builder.hpp"
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

}  // namespace

/**
 * @brief テストを実行する.
 * @return 終了コード
 */
int main() {
    using transient_tracker::analyze::BuildDatasetPaths;
    using transient_tracker::analyze::BuildResidualImage;
    using transient_tracker::analyze::BuildThresholdMask;
    using transient_tracker::analyze::DatasetMetadata;
    using transient_tracker::analyze::ExtractDetections;
    using transient_tracker::analyze::GroundTruthRecord;
    using transient_tracker::analyze::ListFramePaths;
    using transient_tracker::analyze::LoadFrames;
    using transient_tracker::analyze::NormalizeFramesByMedian;
    using transient_tracker::analyze::ReadDatasetMetadata;
    using transient_tracker::analyze::ReadGroundTruthCsv;
    using transient_tracker::analyze::AlignFramesByTranslation;
    using transient_tracker::analyze::BuildMedianReference;
    using transient_tracker::analyze::BuildMaxEnvelopeReference;
    using transient_tracker::analyze::EstimateNoiseSigmaMad;
    using transient_tracker::analyze::EstimateTranslationsPhaseCorrelation;

    const std::filesystem::path dataset_root =
        std::filesystem::temp_directory_path() / "transient_tracker_analyze_candidate";
    std::error_code error;
    std::filesystem::remove_all(dataset_root, error);

    if (int result = Expect(GenerateDataset(dataset_root), "テスト用データセットの生成に失敗しました."); result != 0) {
        return result;
    }

    const transient_tracker::analyze::DatasetPaths paths = BuildDatasetPaths(dataset_root);
    std::string message;
    DatasetMetadata metadata;
    std::vector<GroundTruthRecord> truths;
    std::vector<std::filesystem::path> frame_paths;
    std::vector<xt::xtensor<float, 2>> frames;

    if (int result = Expect(ReadDatasetMetadata(paths.metadata_json_path, &metadata, &message), message); result != 0) {
        return result;
    }
    if (int result = Expect(ReadGroundTruthCsv(paths.labels_csv_path, &truths, &message), message); result != 0) {
        return result;
    }
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
    const xt::xtensor<float, 2> reference = BuildMedianReference(aligned_frames);
    const xt::xtensor<float, 2> reference_envelope = BuildMaxEnvelopeReference(reference, 1);

    std::size_t matched_frames = 0;
    for (std::size_t frame_index = 0; frame_index < aligned_frames.size(); ++frame_index) {
        const xt::xtensor<float, 2> residual = BuildResidualImage(aligned_frames[frame_index], reference_envelope);
        const double sigma = EstimateNoiseSigmaMad(residual);
        const xt::xtensor<std::uint8_t, 2> mask = BuildThresholdMask(residual, sigma, 3.5);
        const std::vector<transient_tracker::analyze::Detection> detections =
            ExtractDetections(frame_index, residual, mask, sigma, 1, 64);

        for (const GroundTruthRecord& truth : truths) {
            if (truth.frame_index != frame_index || !truth.object_visible) {
                continue;
            }

            const double actual_x = truth.object_x;
            const double actual_y = truth.object_y;
            bool matched = false;
            for (const transient_tracker::analyze::Detection& detection : detections) {
                const double dx = detection.x - actual_x;
                const double dy = detection.y - actual_y;
                if (std::sqrt(dx * dx + dy * dy) <= 2.5) {
                    matched = true;
                    break;
                }
            }
            if (matched) {
                ++matched_frames;
            }
        }
    }

    if (int result = Expect(matched_frames >= 3, "位置合わせ後も真値近傍の候補が十分に見つかりませんでした.");
        result != 0) {
        return result;
    }

    std::filesystem::remove_all(dataset_root, error);
    return 0;
}
