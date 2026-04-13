/**
 * @file analyze_app.cpp
 * @brief 解析器アプリケーション本体を実装する.
 */

#include "TransientTracker/analyze/analyze_app.hpp"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <xtensor/containers/xtensor.hpp>

#include "TransientTracker/analyze/analysis_types.hpp"
#include "TransientTracker/analyze/analysis_writer.hpp"
#include "TransientTracker/analyze/candidate_extractor.hpp"
#include "TransientTracker/analyze/dataset_reader.hpp"
#include "TransientTracker/analyze/frame_aligner.hpp"
#include "TransientTracker/analyze/frame_loader.hpp"
#include "TransientTracker/analyze/frame_normalizer.hpp"
#include "TransientTracker/analyze/reference_builder.hpp"
#include "TransientTracker/analyze/registration_estimator.hpp"
#include "TransientTracker/analyze/track_linker.hpp"
#include "TransientTracker/analyze/truth_evaluator.hpp"

namespace transient_tracker::analyze {

namespace {

/**
 * @brief フレーム名を構築する.
 * @param prefix 接頭辞
 * @param frame_index フレーム番号
 * @return ファイル名
 */
std::string BuildFrameFilename(const std::string &prefix,
                               std::size_t frame_index) {
  std::ostringstream stream;
  stream << prefix;
  stream.width(4);
  stream.fill('0');
  stream << frame_index << ".png";
  return stream.str();
}

/**
 * @brief ゼロシフト列を構築する.
 * @param num_frames フレーム数
 * @return ゼロシフト列
 */
std::vector<Shift2D> BuildIdentityShifts(std::size_t num_frames) {
  std::vector<Shift2D> shifts(num_frames);
  for (std::size_t frame_index = 0; frame_index < num_frames; ++frame_index) {
    shifts[frame_index].frame_index = frame_index;
    shifts[frame_index].dx = 0.0;
    shifts[frame_index].dy = 0.0;
    shifts[frame_index].response = 1.0;
  }
  return shifts;
}

/**
 * @brief 全フレームサイズが metadata と一致するか検証する.
 * @param frames 入力フレーム列
 * @param metadata メタデータ
 * @param message エラー内容
 * @return 一致するなら真
 */
bool ValidateFrameSizes(const std::vector<xt::xtensor<float, 2>> &frames,
                        const DatasetMetadata &metadata, std::string *message) {
  for (std::size_t frame_index = 0; frame_index < frames.size();
       ++frame_index) {
    const std::size_t width = frames[frame_index].shape()[1];
    const std::size_t height = frames[frame_index].shape()[0];
    if (width == metadata.width && height == metadata.height) {
      continue;
    }

    if (message != nullptr) {
      std::ostringstream stream;
      stream << "フレームサイズが metadata.json と一致しません. frame_index="
             << frame_index << ", expected=(" << metadata.width << ", "
             << metadata.height << ")"
             << ", actual=(" << width << ", " << height << ")";
      *message = stream.str();
    }
    return false;
  }
  return true;
}

} // namespace

int RunAnalyzeApp(const AnalysisConfig &config) {
  std::string message;
  if (!ValidateAnalysisConfig(config, &message)) {
    std::cerr << message << '\n';
    return 1;
  }

  const DatasetPaths dataset_paths = BuildDatasetPaths(config.dataset_root);
  if (!ValidateDatasetStructure(dataset_paths, &message)) {
    std::cerr << message << '\n';
    return 1;
  }

  DatasetMetadata metadata;
  if (!ReadDatasetMetadata(dataset_paths.metadata_json_path, &metadata,
                           &message)) {
    std::cerr << message << '\n';
    return 1;
  }

  std::vector<GroundTruthRecord> truths;
  if (!ReadGroundTruthCsv(dataset_paths.labels_csv_path, &truths, &message)) {
    std::cerr << message << '\n';
    return 1;
  }

  std::vector<std::filesystem::path> frame_paths;
  if (!ListFramePaths(dataset_paths, &frame_paths, &message)) {
    std::cerr << message << '\n';
    return 1;
  }
  if (frame_paths.size() != metadata.num_frames) {
    std::cerr << "frames 数が metadata.json と一致しません." << '\n';
    return 1;
  }

  std::vector<xt::xtensor<float, 2>> frames;
  if (!LoadFrames(frame_paths, &frames, &message)) {
    std::cerr << message << '\n';
    return 1;
  }
  if (frames.empty()) {
    std::cerr << "フレーム画像がありません." << '\n';
    return 1;
  }
  if (!ValidateFrameSizes(frames, metadata, &message)) {
    std::cerr << message << '\n';
    return 1;
  }

  const bool registration_enabled = !config.disable_registration;
  const std::vector<xt::xtensor<float, 2>> normalized_frames =
      NormalizeFramesByMedian(frames);
  const std::vector<Shift2D> shifts =
      registration_enabled
          ? EstimateTranslationsPhaseCorrelation(normalized_frames)
          : BuildIdentityShifts(normalized_frames.size());
  const std::vector<xt::xtensor<float, 2>> aligned_frames =
      registration_enabled ? AlignFramesByTranslation(frames, shifts) : frames;
  const std::vector<xt::xtensor<float, 2>> aligned_normalized_frames =
      registration_enabled ? AlignFramesByTranslation(normalized_frames, shifts)
                           : normalized_frames;
  if (aligned_frames.size() != frames.size() ||
      aligned_normalized_frames.size() != normalized_frames.size()) {
    std::cerr << "位置合わせ結果のフレーム数が一致しません." << '\n';
    return 1;
  }
  const xt::xtensor<float, 2> reference =
      BuildMedianReference(aligned_normalized_frames);
  const xt::xtensor<float, 2> reference_envelope =
      BuildMaxEnvelopeReference(reference, config.tolerance_radius);

  const AnalysisOutputPaths output_paths =
      BuildAnalysisOutputPaths(config.output_root);
  if (!CreateAnalysisOutputDirectories(output_paths, config.write_debug_images,
                                       &message)) {
    std::cerr << message << '\n';
    return 1;
  }

  if (config.write_debug_images &&
      !WriteDebugFloatImage(output_paths.reference_image_path,
                            reference_envelope, &message)) {
    std::cerr << message << '\n';
    return 1;
  }

  std::vector<std::vector<Detection>> detections_by_frame(
      aligned_normalized_frames.size());
  std::vector<Detection> all_detections;
  std::size_t next_detection_id = 0;
  std::size_t frames_with_detections = 0;
  for (std::size_t frame_index = 0;
       frame_index < aligned_normalized_frames.size(); ++frame_index) {
    const xt::xtensor<float, 2> residual = BuildResidualImage(
        aligned_normalized_frames[frame_index], reference_envelope);
    const double sigma = EstimateNoiseSigmaMad(residual);
    const xt::xtensor<std::uint8_t, 2> mask =
        BuildThresholdMask(residual, sigma, config.threshold_scale);
    std::vector<Detection> detections = ExtractDetections(
        frame_index, residual, mask, sigma, config.area_min, config.area_max);

    if (!detections.empty()) {
      ++frames_with_detections;
    }
    for (Detection &detection : detections) {
      detection.detection_id = next_detection_id;
      ++next_detection_id;
      all_detections.push_back(detection);
    }
    detections_by_frame[frame_index] = detections;

    if (!config.write_debug_images) {
      continue;
    }

    if (!WriteDebugFloatImage(output_paths.aligned_dir /
                                  BuildFrameFilename("aligned_", frame_index),
                              aligned_frames[frame_index], &message)) {
      std::cerr << message << '\n';
      return 1;
    }

    if (!WriteDebugFloatImage(output_paths.residual_dir /
                                  BuildFrameFilename("residual_", frame_index),
                              residual, &message) ||
        !WriteDebugMaskImage(output_paths.mask_dir /
                                 BuildFrameFilename("mask_", frame_index),
                             mask, &message) ||
        !WriteDebugOverlayImage(output_paths.overlay_dir /
                                    BuildFrameFilename("overlay_", frame_index),
                                aligned_normalized_frames[frame_index],
                                detections, &message)) {
      std::cerr << message << '\n';
      return 1;
    }
  }

  std::vector<Track> tracks =
      LinkTracksGreedy(detections_by_frame, config.max_velocity_per_frame,
                       config.min_track_length);
  const EvaluationSummary evaluation = EvaluateTracksAgainstTruth(
      truths, &tracks, registration_enabled, config.truth_match_distance);
  std::vector<Track> tracks_to_draw = SelectRepresentativeMatchedTracks(tracks);
  const Track *primary_track = SelectPrimaryTrack(tracks);
  if (tracks_to_draw.empty() && primary_track != nullptr) {
    tracks_to_draw.push_back(*primary_track);
  }

  for (std::size_t frame_index = 0; frame_index < aligned_frames.size();
       ++frame_index) {
    if (!WriteResultImage(output_paths.results_dir /
                              BuildFrameFilename("result_", frame_index),
                          aligned_frames[frame_index], tracks_to_draw,
                          frame_index, &message)) {
      std::cerr << message << '\n';
      return 1;
    }
  }

  if (!WriteTrajectoryResultImage(output_paths.trajectory_result_image_path,
                                  aligned_frames.front(), tracks_to_draw,
                                  &message)) {
    std::cerr << message << '\n';
    return 1;
  }

  AnalysisSummary summary;
  summary.dataset_root = config.dataset_root;
  summary.output_root = config.output_root;
  summary.scenario_name = metadata.scenario_name;
  summary.num_frames = metadata.num_frames;
  summary.num_truth_rows = truths.size();
  summary.num_detections = all_detections.size();
  summary.num_frames_with_detections = frames_with_detections;
  summary.num_tracks = tracks.size();
  summary.num_matched_objects = evaluation.num_matched_objects;
  summary.primary_track_id =
      primary_track != nullptr ? static_cast<int>(primary_track->track_id) : -1;
  summary.threshold_scale = config.threshold_scale;
  summary.tolerance_radius = config.tolerance_radius;
  summary.area_min = config.area_min;
  summary.area_max = config.area_max;
  summary.max_velocity_per_frame = config.max_velocity_per_frame;
  summary.min_track_length = config.min_track_length;
  summary.write_debug_images = config.write_debug_images;
  summary.registration_enabled = registration_enabled;
  summary.status = "ok";

  if (!WriteDetectionsCsv(output_paths.detections_csv_path, all_detections,
                          &message) ||
      !WriteTracksCsv(output_paths.tracks_csv_path, tracks, &message) ||
      !WriteEvaluationJson(output_paths.evaluation_json_path, evaluation,
                           &message) ||
      !WriteSummaryJson(output_paths.summary_json_path, summary, &message)) {
    std::cerr << message << '\n';
    return 1;
  }

  std::cout << "scenario_name: " << metadata.scenario_name << '\n';
  std::cout << "dataset_root: " << config.dataset_root << '\n';
  std::cout << "output_root: " << config.output_root << '\n';
  std::cout << "num_frames: " << metadata.num_frames << '\n';
  std::cout << "num_detections: " << all_detections.size() << '\n';
  std::cout << "num_tracks: " << tracks.size() << '\n';
  std::cout << "num_matched_objects: " << evaluation.num_matched_objects
            << '\n';
  std::cout << "primary_track_id: "
            << (primary_track != nullptr
                    ? static_cast<int>(primary_track->track_id)
                    : -1)
            << '\n';
  return 0;
}

} // namespace transient_tracker::analyze
