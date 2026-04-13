/**
 * @file analysis_writer.cpp
 * @brief 解析結果保存処理を実装する.
 */

#include "TransientTracker/analyze/analysis_writer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace transient_tracker::analyze {

namespace {

/**
 * @brief メッセージを設定する.
 * @param message 出力先
 * @param text 設定内容
 */
void SetMessage(std::string *message, const std::string &text) {
  if (message != nullptr) {
    *message = text;
  }
}

/**
 * @brief JSON文字列用に最小限のエスケープを行う.
 * @param text 入力文字列
 * @return エスケープ後文字列
 */
std::string EscapeJson(const std::string &text) {
  std::string escaped;
  escaped.reserve(text.size());
  for (char ch : text) {
    if (ch == '\\' || ch == '"') {
      escaped.push_back('\\');
    }
    escaped.push_back(ch);
  }
  return escaped;
}

/**
 * @brief 浮動小数画像を8bit可視化画像へ変換する.
 * @param image 入力画像
 * @return 8bit画像
 */
cv::Mat MakePreviewMat(const xt::xtensor<float, 2> &image) {
  const std::size_t height = image.shape()[0];
  const std::size_t width = image.shape()[1];
  float min_value = std::numeric_limits<float>::max();
  float max_value = std::numeric_limits<float>::lowest();
  for (std::size_t y = 0; y < height; ++y) {
    for (std::size_t x = 0; x < width; ++x) {
      min_value = std::min(min_value, image(y, x));
      max_value = std::max(max_value, image(y, x));
    }
  }

  const float range = std::max(1.0e-6F, max_value - min_value);
  cv::Mat mat(static_cast<int>(height), static_cast<int>(width), CV_8UC1);
  for (std::size_t y = 0; y < height; ++y) {
    for (std::size_t x = 0; x < width; ++x) {
      const float normalized = (image(y, x) - min_value) / range;
      const float scaled = std::clamp(normalized * 255.0F, 0.0F, 255.0F);
      mat.at<std::uint8_t>(static_cast<int>(y), static_cast<int>(x)) =
          static_cast<std::uint8_t>(scaled);
    }
  }
  return mat;
}

/**
 * @brief 浮動小数画像をBGR可視化画像へ変換する.
 * @param image 入力画像
 * @return BGR画像
 */
cv::Mat MakeColorPreviewMat(const xt::xtensor<float, 2> &image) {
  const cv::Mat grayscale = MakePreviewMat(image);
  cv::Mat color;
  cv::cvtColor(grayscale, color, cv::COLOR_GRAY2BGR);
  return color;
}

/**
 * @brief マスクを8bit画像へ変換する.
 * @param mask 入力マスク
 * @return 8bit画像
 */
cv::Mat MakeMaskPreviewMat(const xt::xtensor<std::uint8_t, 2> &mask) {
  const std::size_t height = mask.shape()[0];
  const std::size_t width = mask.shape()[1];
  cv::Mat mat(static_cast<int>(height), static_cast<int>(width), CV_8UC1);
  for (std::size_t y = 0; y < height; ++y) {
    for (std::size_t x = 0; x < width; ++x) {
      mat.at<std::uint8_t>(static_cast<int>(y), static_cast<int>(x)) =
          mask(y, x);
    }
  }
  return mat;
}

/**
 * @brief 検出点をOpenCV座標へ変換する.
 * @param detection 候補点
 * @return 整数座標
 */
cv::Point MakePoint(const Detection &detection) {
  return cv::Point(static_cast<int>(std::lround(detection.x)),
                   static_cast<int>(std::lround(detection.y)));
}

/**
 * @brief 軌跡描画色を返す.
 * @param track 軌跡
 * @return BGR色
 */
cv::Scalar MakeTrackColor(const Track &track) {
  static const std::vector<cv::Scalar> kPalette = {
      cv::Scalar(0, 255, 255), cv::Scalar(255, 192, 0),
      cv::Scalar(255, 0, 255), cv::Scalar(0, 200, 0),
      cv::Scalar(0, 128, 255), cv::Scalar(255, 128, 128),
  };

  const int palette_index_source = track.matched_object_id >= 0
                                       ? track.matched_object_id
                                       : static_cast<int>(track.track_id);
  const std::size_t palette_index =
      static_cast<std::size_t>(std::abs(palette_index_source)) %
      kPalette.size();
  return kPalette[palette_index];
}

/**
 * @brief 軌跡ラベル文字列を返す.
 * @param track 軌跡
 * @return ラベル
 */
std::string BuildTrackLabel(const Track &track) {
  if (track.matched_object_id >= 0) {
    return "obj " + std::to_string(track.matched_object_id) + " / track " +
           std::to_string(track.track_id);
  }
  return "track " + std::to_string(track.track_id);
}

/**
 * @brief 指定時刻までの軌跡点列を集める.
 * @param track 軌跡
 * @param max_frame_index 上限フレーム番号
 * @return 点列
 */
std::vector<cv::Point> CollectTrackPoints(const Track &track,
                                          std::size_t max_frame_index) {
  std::vector<cv::Point> points;
  for (const Detection &detection : track.detections) {
    if (detection.frame_index > max_frame_index) {
      continue;
    }
    points.push_back(MakePoint(detection));
  }
  return points;
}

/**
 * @brief 指定フレームに属する軌跡点を返す.
 * @param track 軌跡
 * @param frame_index フレーム番号
 * @return 検出点へのポインタ. 見つからなければ nullptr
 */
const Detection *FindDetectionAtFrame(const Track &track,
                                      std::size_t frame_index) {
  for (const Detection &detection : track.detections) {
    if (detection.frame_index == frame_index) {
      return &detection;
    }
  }
  return nullptr;
}

/**
 * @brief 軌跡を描画する.
 * @param image 描画先
 * @param track 軌跡
 * @param max_frame_index 描画対象の最終フレーム
 * @param highlight_current 現在フレーム点を強調するなら真
 */
void DrawTrackUntilFrame(cv::Mat *image, const Track &track,
                         std::size_t max_frame_index, bool highlight_current,
                         const cv::Scalar &color) {
  if (image == nullptr) {
    return;
  }

  const std::vector<cv::Point> points =
      CollectTrackPoints(track, max_frame_index);
  if (points.empty()) {
    return;
  }

  if (points.size() >= 2) {
    const std::vector<std::vector<cv::Point>> polyline = {points};
    cv::polylines(*image, polyline, false, color, 1, cv::LINE_AA);
  }

  cv::circle(*image, points.front(), 5, color, 1, cv::LINE_AA);
  cv::circle(*image, points.back(), 5, color, 1, cv::LINE_AA);

  if (!highlight_current) {
    return;
  }

  const Detection *current_detection =
      FindDetectionAtFrame(track, max_frame_index);
  if (current_detection == nullptr) {
    return;
  }

  const cv::Point current_point = MakePoint(*current_detection);
  cv::circle(*image, current_point, 3, cv::Scalar(255, 255, 0), -1,
             cv::LINE_AA);
  cv::putText(*image, BuildTrackLabel(track),
              cv::Point(current_point.x + 8, current_point.y - 8),
              cv::FONT_HERSHEY_SIMPLEX, 0.35, color, 1, cv::LINE_AA);
}

/**
 * @brief 軌跡全体を描画する.
 * @param image 描画先
 * @param track 軌跡
 */
void DrawWholeTrack(cv::Mat *image, const Track &track,
                    const cv::Scalar &color) {
  if (image == nullptr || track.detections.empty()) {
    return;
  }

  const std::vector<cv::Point> points =
      CollectTrackPoints(track, track.detections.back().frame_index);
  if (points.size() >= 2) {
    const std::vector<std::vector<cv::Point>> polyline = {points};
    cv::polylines(*image, polyline, false, color, 1, cv::LINE_AA);
  }

  const cv::Point start_point = MakePoint(track.detections.front());
  const cv::Point end_point = MakePoint(track.detections.back());
  cv::circle(*image, start_point, 5, color, 1, cv::LINE_AA);
  cv::circle(*image, end_point, 5, color, -1, cv::LINE_AA);
  cv::putText(*image, "start", cv::Point(start_point.x + 8, start_point.y - 8),
              cv::FONT_HERSHEY_SIMPLEX, 0.4, color, 1, cv::LINE_AA);
  cv::putText(*image, "end", cv::Point(end_point.x + 8, end_point.y - 8),
              cv::FONT_HERSHEY_SIMPLEX, 0.4, color, 1, cv::LINE_AA);
  cv::putText(*image, BuildTrackLabel(track),
              cv::Point(end_point.x + 8, end_point.y + 16),
              cv::FONT_HERSHEY_SIMPLEX, 0.4, color, 1, cv::LINE_AA);
}

} // namespace

AnalysisOutputPaths
BuildAnalysisOutputPaths(const std::filesystem::path &output_root) {
  AnalysisOutputPaths paths;
  paths.output_root = output_root;
  paths.summary_json_path = output_root / "summary.json";
  paths.detections_csv_path = output_root / "detections.csv";
  paths.tracks_csv_path = output_root / "tracks.csv";
  paths.evaluation_json_path = output_root / "evaluation.json";
  paths.results_dir = output_root / "results";
  paths.trajectory_result_image_path =
      paths.results_dir / "trajectory_result.png";
  paths.debug_dir = output_root / "debug";
  paths.reference_image_path = paths.debug_dir / "reference.png";
  paths.aligned_dir = paths.debug_dir / "aligned";
  paths.residual_dir = paths.debug_dir / "residual";
  paths.mask_dir = paths.debug_dir / "mask";
  paths.overlay_dir = paths.debug_dir / "overlay";
  return paths;
}

bool CreateAnalysisOutputDirectories(const AnalysisOutputPaths &paths,
                                     bool write_debug_images,
                                     std::string *message) {
  std::error_code error;
  std::filesystem::create_directories(paths.output_root, error);
  if (error) {
    SetMessage(message, "出力ディレクトリを作成できませんでした: " +
                            paths.output_root.string());
    return false;
  }

  error.clear();
  std::filesystem::create_directories(paths.results_dir, error);
  if (error) {
    SetMessage(message, "results ディレクトリを作成できませんでした: " +
                            paths.results_dir.string());
    return false;
  }

  if (!write_debug_images) {
    return true;
  }

  error.clear();
  std::filesystem::create_directories(paths.debug_dir, error);
  if (error) {
    SetMessage(message, "debug ディレクトリを作成できませんでした: " +
                            paths.debug_dir.string());
    return false;
  }

  error.clear();
  std::filesystem::create_directories(paths.aligned_dir, error);
  if (error) {
    SetMessage(message, "aligned ディレクトリを作成できませんでした: " +
                            paths.aligned_dir.string());
    return false;
  }

  error.clear();
  std::filesystem::create_directories(paths.residual_dir, error);
  if (error) {
    SetMessage(message, "residual ディレクトリを作成できませんでした: " +
                            paths.residual_dir.string());
    return false;
  }

  error.clear();
  std::filesystem::create_directories(paths.mask_dir, error);
  if (error) {
    SetMessage(message, "mask ディレクトリを作成できませんでした: " +
                            paths.mask_dir.string());
    return false;
  }

  error.clear();
  std::filesystem::create_directories(paths.overlay_dir, error);
  if (error) {
    SetMessage(message, "overlay ディレクトリを作成できませんでした: " +
                            paths.overlay_dir.string());
    return false;
  }
  return true;
}

bool WriteDetectionsCsv(const std::filesystem::path &output_path,
                        const std::vector<Detection> &detections,
                        std::string *message) {
  std::ofstream stream(output_path);
  if (!stream.is_open()) {
    SetMessage(message,
               "detections.csv を開けませんでした: " + output_path.string());
    return false;
  }

  stream << "frame_index,detection_id,x,y,peak_value,sum_value,area,sigma_"
            "estimate,score\n";
  for (const Detection &detection : detections) {
    stream << detection.frame_index << ',' << detection.detection_id << ','
           << detection.x << ',' << detection.y << ',' << detection.peak_value
           << ',' << detection.sum_value << ',' << detection.area << ','
           << detection.sigma_estimate << ',' << detection.score << '\n';
  }

  stream.close();
  if (!stream) {
    SetMessage(message, "detections.csv の書き出しに失敗しました: " +
                            output_path.string());
    return false;
  }
  return true;
}

bool WriteTracksCsv(const std::filesystem::path &output_path,
                    const std::vector<Track> &tracks, std::string *message) {
  std::ofstream stream(output_path);
  if (!stream.is_open()) {
    SetMessage(message,
               "tracks.csv を開けませんでした: " + output_path.string());
    return false;
  }

  stream << "track_id,frame_index,detection_id,x,y,peak_value,sum_value,area,"
            "sigma_estimate,score,"
            "track_length,track_score,mean_vx,mean_vy,fit_error,matched_object_"
            "id,matched_frame_count\n";
  for (const Track &track : tracks) {
    for (const Detection &detection : track.detections) {
      stream << track.track_id << ',' << detection.frame_index << ','
             << detection.detection_id << ',' << detection.x << ','
             << detection.y << ',' << detection.peak_value << ','
             << detection.sum_value << ',' << detection.area << ','
             << detection.sigma_estimate << ',' << detection.score << ','
             << track.detections.size() << ',' << track.score << ','
             << track.mean_vx << ',' << track.mean_vy << ',' << track.fit_error
             << ',' << track.matched_object_id << ','
             << track.matched_frame_count << '\n';
    }
  }

  stream.close();
  if (!stream) {
    SetMessage(message,
               "tracks.csv の書き出しに失敗しました: " + output_path.string());
    return false;
  }
  return true;
}

bool WriteEvaluationJson(const std::filesystem::path &output_path,
                         const EvaluationSummary &evaluation,
                         std::string *message) {
  std::ofstream stream(output_path);
  if (!stream.is_open()) {
    SetMessage(message,
               "evaluation.json を開けませんでした: " + output_path.string());
    return false;
  }

  stream << "{\n";
  stream << "  \"num_visible_truth_frames\": "
         << evaluation.num_visible_truth_frames << ",\n";
  stream << "  \"num_visible_objects\": " << evaluation.num_visible_objects
         << ",\n";
  stream << "  \"num_matched_tracks\": " << evaluation.num_matched_tracks
         << ",\n";
  stream << "  \"num_matched_objects\": " << evaluation.num_matched_objects
         << ",\n";
  stream << "  \"primary_track_id\": " << evaluation.primary_track_id << ",\n";
  stream << "  \"primary_track_matched_object_id\": "
         << evaluation.primary_track_matched_object_id << ",\n";
  stream << "  \"primary_track_match_count\": "
         << evaluation.primary_track_match_count << ",\n";
  stream << "  \"primary_track_length\": " << evaluation.primary_track_length
         << ",\n";
  stream << "  \"matched_objects\": [\n";
  for (std::size_t index = 0; index < evaluation.matched_objects.size();
       ++index) {
    const MatchedObjectSummary &matched_object =
        evaluation.matched_objects[index];
    stream << "    {\n";
    stream << "      \"object_id\": " << matched_object.object_id << ",\n";
    stream << "      \"track_id\": " << matched_object.track_id << ",\n";
    stream << "      \"matched_frame_count\": "
           << matched_object.matched_frame_count << ",\n";
    stream << "      \"track_length\": " << matched_object.track_length << "\n";
    stream << "    }";
    if (index + 1 != evaluation.matched_objects.size()) {
      stream << ",";
    }
    stream << "\n";
  }
  stream << "  ]\n";
  stream << "}\n";

  stream.close();
  if (!stream) {
    SetMessage(message, "evaluation.json の書き出しに失敗しました: " +
                            output_path.string());
    return false;
  }
  return true;
}

bool WriteSummaryJson(const std::filesystem::path &output_path,
                      const AnalysisSummary &summary, std::string *message) {
  std::ofstream stream(output_path);
  if (!stream.is_open()) {
    SetMessage(message,
               "summary.json を開けませんでした: " + output_path.string());
    return false;
  }

  stream << "{\n";
  stream << "  \"dataset_root\": \""
         << EscapeJson(summary.dataset_root.string()) << "\",\n";
  stream << "  \"output_root\": \"" << EscapeJson(summary.output_root.string())
         << "\",\n";
  stream << "  \"scenario_name\": \"" << EscapeJson(summary.scenario_name)
         << "\",\n";
  stream << "  \"num_frames\": " << summary.num_frames << ",\n";
  stream << "  \"num_truth_rows\": " << summary.num_truth_rows << ",\n";
  stream << "  \"num_detections\": " << summary.num_detections << ",\n";
  stream << "  \"num_frames_with_detections\": "
         << summary.num_frames_with_detections << ",\n";
  stream << "  \"num_tracks\": " << summary.num_tracks << ",\n";
  stream << "  \"num_matched_objects\": " << summary.num_matched_objects
         << ",\n";
  stream << "  \"primary_track_id\": " << summary.primary_track_id << ",\n";
  stream << "  \"threshold_scale\": " << summary.threshold_scale << ",\n";
  stream << "  \"tolerance_radius\": " << summary.tolerance_radius << ",\n";
  stream << "  \"area_min\": " << summary.area_min << ",\n";
  stream << "  \"area_max\": " << summary.area_max << ",\n";
  stream << "  \"max_velocity_per_frame\": " << summary.max_velocity_per_frame
         << ",\n";
  stream << "  \"min_track_length\": " << summary.min_track_length << ",\n";
  stream << "  \"write_debug_images\": "
         << (summary.write_debug_images ? "true" : "false") << ",\n";
  stream << "  \"registration_enabled\": "
         << (summary.registration_enabled ? "true" : "false") << ",\n";
  stream << "  \"status\": \"" << EscapeJson(summary.status) << "\"\n";
  stream << "}\n";

  stream.close();
  if (!stream) {
    SetMessage(message, "summary.json の書き出しに失敗しました: " +
                            output_path.string());
    return false;
  }
  return true;
}

bool WriteDebugFloatImage(const std::filesystem::path &output_path,
                          const xt::xtensor<float, 2> &image,
                          std::string *message) {
  const cv::Mat preview = MakePreviewMat(image);
  if (!cv::imwrite(output_path.string(), preview)) {
    SetMessage(message,
               "デバッグ画像の保存に失敗しました: " + output_path.string());
    return false;
  }
  return true;
}

bool WriteDebugMaskImage(const std::filesystem::path &output_path,
                         const xt::xtensor<std::uint8_t, 2> &mask,
                         std::string *message) {
  const cv::Mat preview = MakeMaskPreviewMat(mask);
  if (!cv::imwrite(output_path.string(), preview)) {
    SetMessage(message,
               "マスク画像の保存に失敗しました: " + output_path.string());
    return false;
  }
  return true;
}

bool WriteDebugOverlayImage(const std::filesystem::path &output_path,
                            const xt::xtensor<float, 2> &frame,
                            const std::vector<Detection> &detections,
                            std::string *message) {
  cv::Mat overlay = MakePreviewMat(frame);
  for (const Detection &detection : detections) {
    const cv::Point center(static_cast<int>(std::lround(detection.x)),
                           static_cast<int>(std::lround(detection.y)));
    cv::circle(overlay, center, 5, cv::Scalar(255), 1, cv::LINE_AA);
  }

  if (!cv::imwrite(output_path.string(), overlay)) {
    SetMessage(message,
               "重畳画像の保存に失敗しました: " + output_path.string());
    return false;
  }
  return true;
}

bool WriteResultImage(const std::filesystem::path &output_path,
                      const xt::xtensor<float, 2> &frame,
                      const std::vector<Track> &tracks_to_draw,
                      std::size_t frame_index, std::string *message) {
  cv::Mat result_bgr = MakeColorPreviewMat(frame);
  if (!tracks_to_draw.empty()) {
    for (const Track &track : tracks_to_draw) {
      DrawTrackUntilFrame(&result_bgr, track, frame_index, true,
                          MakeTrackColor(track));
    }
  } else {
    cv::putText(result_bgr, "no matched tracks", cv::Point(16, 24),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 128, 255), 1,
                cv::LINE_AA);
  }

  if (!cv::imwrite(output_path.string(), result_bgr)) {
    SetMessage(message,
               "解析結果画像の保存に失敗しました: " + output_path.string());
    return false;
  }
  return true;
}

bool WriteTrajectoryResultImage(const std::filesystem::path &output_path,
                                const xt::xtensor<float, 2> &frame,
                                const std::vector<Track> &tracks_to_draw,
                                std::string *message) {
  cv::Mat result_bgr = MakeColorPreviewMat(frame);
  if (!tracks_to_draw.empty()) {
    for (const Track &track : tracks_to_draw) {
      DrawWholeTrack(&result_bgr, track, MakeTrackColor(track));
    }
  } else {
    cv::putText(result_bgr, "no matched tracks", cv::Point(16, 24),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 128, 255), 1,
                cv::LINE_AA);
  }

  if (!cv::imwrite(output_path.string(), result_bgr)) {
    SetMessage(message,
               "軌跡要約画像の保存に失敗しました: " + output_path.string());
    return false;
  }
  return true;
}

} // namespace transient_tracker::analyze
