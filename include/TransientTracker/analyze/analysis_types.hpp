/**
 * @file analysis_types.hpp
 * @brief 解析器で共有する基本型を定義する.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace transient_tracker::analyze {

/**
 * @brief データセット入力パス群を表す.
 */
struct DatasetPaths {
  /** @brief データセットルート. */
  std::filesystem::path dataset_root;
  /** @brief フレーム画像ディレクトリ. */
  std::filesystem::path frames_dir;
  /** @brief 真値CSVのパス. */
  std::filesystem::path labels_csv_path;
  /** @brief メタデータJSONのパス. */
  std::filesystem::path metadata_json_path;
};

/**
 * @brief データセットのメタデータを表す.
 */
struct DatasetMetadata {
  /** @brief フォーマット版数. */
  std::size_t dataset_format_version = 0;
  /** @brief シナリオ名. */
  std::string scenario_name;
  /** @brief 乱数シード. */
  std::uint64_t seed = 0;
  /** @brief 画像幅. */
  std::size_t width = 0;
  /** @brief 画像高さ. */
  std::size_t height = 0;
  /** @brief フレーム数. */
  std::size_t num_frames = 0;
  /** @brief 星数. */
  std::size_t num_stars = 0;
  /** @brief 移動天体数. */
  std::size_t num_objects = 0;
  /** @brief 背景レベル. */
  double background_level = 0.0;
  /** @brief 読み出しノイズ標準偏差. */
  double read_noise_sigma = 0.0;
  /** @brief ポアソンノイズ利用フラグ. */
  bool use_poisson_noise = false;
  /** @brief X方向シフト標準偏差. */
  double shift_sigma_x = 0.0;
  /** @brief Y方向シフト標準偏差. */
  double shift_sigma_y = 0.0;
  /** @brief ホットピクセル数. */
  std::size_t hot_pixel_count = 0;
  /** @brief ホットピクセル強度. */
  double hot_pixel_peak = 0.0;
};

/**
 * @brief 真値ラベル1行分を表す.
 */
struct GroundTruthRecord {
  /** @brief フレーム番号. */
  std::size_t frame_index = 0;
  /** @brief 天体識別子. */
  std::size_t object_id = 0;
  /** @brief X方向シフト真値. */
  double shift_dx = 0.0;
  /** @brief Y方向シフト真値. */
  double shift_dy = 0.0;
  /** @brief 理想座標系のX位置. */
  double object_x = 0.0;
  /** @brief 理想座標系のY位置. */
  double object_y = 0.0;
  /** @brief 天体フラックス. */
  double object_flux = 0.0;
  /** @brief 可視フラグ. */
  bool object_visible = false;
};

/**
 * @brief 候補点1件を表す.
 */
struct Detection {
  /** @brief フレーム番号. */
  std::size_t frame_index = 0;
  /** @brief 候補識別子. */
  std::size_t detection_id = 0;
  /** @brief 重心X座標. */
  double x = 0.0;
  /** @brief 重心Y座標. */
  double y = 0.0;
  /** @brief 最大残差値. */
  double peak_value = 0.0;
  /** @brief 総残差値. */
  double sum_value = 0.0;
  /** @brief 領域面積. */
  std::size_t area = 0;
  /** @brief 推定ノイズ標準偏差. */
  double sigma_estimate = 0.0;
  /** @brief 候補スコア. */
  double score = 0.0;
};

/**
 * @brief 並進シフト推定結果を表す.
 */
struct Shift2D {
  /** @brief フレーム番号. */
  std::size_t frame_index = 0;
  /** @brief X方向シフト量. */
  double dx = 0.0;
  /** @brief Y方向シフト量. */
  double dy = 0.0;
  /** @brief 推定信頼度. */
  double response = 0.0;
};

/**
 * @brief 軌跡を表す.
 */
struct Track {
  /** @brief 軌跡識別子. */
  std::size_t track_id = 0;
  /** @brief 候補列. */
  std::vector<Detection> detections;
  /** @brief 平均X速度. */
  double mean_vx = 0.0;
  /** @brief 平均Y速度. */
  double mean_vy = 0.0;
  /** @brief 直線適合誤差. */
  double fit_error = 0.0;
  /** @brief 軌跡スコア. */
  double score = 0.0;
  /** @brief 一致した真値 object_id. 一致なしは -1. */
  int matched_object_id = -1;
  /** @brief 真値一致フレーム数. */
  std::size_t matched_frame_count = 0;
};

/**
 * @brief 真値天体ごとの代表一致結果を表す.
 */
struct MatchedObjectSummary {
  /** @brief 真値 object_id. */
  int object_id = -1;
  /** @brief 代表軌跡ID. */
  int track_id = -1;
  /** @brief 一致フレーム数. */
  std::size_t matched_frame_count = 0;
  /** @brief 軌跡長. */
  std::size_t track_length = 0;
};

/**
 * @brief 評価サマリを表す.
 */
struct EvaluationSummary {
  /** @brief 可視真値フレーム数. */
  std::size_t num_visible_truth_frames = 0;
  /** @brief 可視真値天体数. */
  std::size_t num_visible_objects = 0;
  /** @brief 一致軌跡数. */
  std::size_t num_matched_tracks = 0;
  /** @brief 代表一致できた真値天体数. */
  std::size_t num_matched_objects = 0;
  /** @brief 主軌跡ID. 軌跡なしは -1. */
  int primary_track_id = -1;
  /** @brief 主軌跡の一致 object_id. */
  int primary_track_matched_object_id = -1;
  /** @brief 主軌跡の一致フレーム数. */
  std::size_t primary_track_match_count = 0;
  /** @brief 主軌跡長. */
  std::size_t primary_track_length = 0;
  /** @brief 真値天体ごとの代表一致結果. */
  std::vector<MatchedObjectSummary> matched_objects;
};

/**
 * @brief 解析サマリを表す.
 */
struct AnalysisSummary {
  /** @brief データセットルート. */
  std::filesystem::path dataset_root;
  /** @brief 出力先ルート. */
  std::filesystem::path output_root;
  /** @brief シナリオ名. */
  std::string scenario_name;
  /** @brief フレーム数. */
  std::size_t num_frames = 0;
  /** @brief 真値行数. */
  std::size_t num_truth_rows = 0;
  /** @brief 検出数. */
  std::size_t num_detections = 0;
  /** @brief 候補が1件以上あるフレーム数. */
  std::size_t num_frames_with_detections = 0;
  /** @brief 軌跡数. */
  std::size_t num_tracks = 0;
  /** @brief 真値に一致した代表天体数. */
  std::size_t num_matched_objects = 0;
  /** @brief 主軌跡ID. */
  int primary_track_id = -1;
  /** @brief しきい値倍率. */
  double threshold_scale = 0.0;
  /** @brief 許容誤差半径. */
  int tolerance_radius = 0;
  /** @brief 最小面積. */
  std::size_t area_min = 0;
  /** @brief 最大面積. */
  std::size_t area_max = 0;
  /** @brief 最大速度. */
  double max_velocity_per_frame = 0.0;
  /** @brief 最小軌跡長. */
  std::size_t min_track_length = 0;
  /** @brief デバッグ画像出力フラグ. */
  bool write_debug_images = false;
  /** @brief 位置合わせ有効フラグ. */
  bool registration_enabled = false;
  /** @brief 実行状態. */
  std::string status;
};

} // namespace transient_tracker::analyze
