/**
 * @file analysis_writer.hpp
 * @brief 解析結果保存関数を宣言する.
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <xtensor/containers/xtensor.hpp>

#include "TransientTracker/analyze/analysis_types.hpp"

namespace transient_tracker::analyze {

/**
 * @brief 解析出力先の配置を表す.
 */
struct AnalysisOutputPaths {
  /** @brief 出力ルート. */
  std::filesystem::path output_root;
  /** @brief サマリJSON. */
  std::filesystem::path summary_json_path;
  /** @brief 候補CSV. */
  std::filesystem::path detections_csv_path;
  /** @brief 軌跡CSV. */
  std::filesystem::path tracks_csv_path;
  /** @brief 評価JSON. */
  std::filesystem::path evaluation_json_path;
  /** @brief 解析結果画像ディレクトリ. */
  std::filesystem::path results_dir;
  /** @brief 軌跡要約画像. */
  std::filesystem::path trajectory_result_image_path;
  /** @brief デバッグ出力ディレクトリ. */
  std::filesystem::path debug_dir;
  /** @brief 基準画像パス. */
  std::filesystem::path reference_image_path;
  /** @brief 整列画像ディレクトリ. */
  std::filesystem::path aligned_dir;
  /** @brief 残差画像ディレクトリ. */
  std::filesystem::path residual_dir;
  /** @brief マスク画像ディレクトリ. */
  std::filesystem::path mask_dir;
  /** @brief 重畳画像ディレクトリ. */
  std::filesystem::path overlay_dir;
};

/**
 * @brief 解析出力先を構築する.
 * @param output_root 出力ルート
 * @return 出力先
 */
AnalysisOutputPaths
BuildAnalysisOutputPaths(const std::filesystem::path &output_root);

/**
 * @brief 解析出力用ディレクトリを作成する.
 * @param paths 出力先
 * @param write_debug_images デバッグ画像出力フラグ
 * @param message エラー内容
 * @return 成功なら真
 */
bool CreateAnalysisOutputDirectories(const AnalysisOutputPaths &paths,
                                     bool write_debug_images,
                                     std::string *message);

/**
 * @brief 候補CSVを書き出す.
 * @param output_path 出力先
 * @param detections 候補一覧
 * @param message エラー内容
 * @return 成功なら真
 */
bool WriteDetectionsCsv(const std::filesystem::path &output_path,
                        const std::vector<Detection> &detections,
                        std::string *message);

/**
 * @brief 軌跡CSVを書き出す.
 * @param output_path 出力先
 * @param tracks 軌跡一覧
 * @param message エラー内容
 * @return 成功なら真
 */
bool WriteTracksCsv(const std::filesystem::path &output_path,
                    const std::vector<Track> &tracks, std::string *message);

/**
 * @brief 評価JSONを書き出す.
 * @param output_path 出力先
 * @param evaluation 評価サマリ
 * @param message エラー内容
 * @return 成功なら真
 */
bool WriteEvaluationJson(const std::filesystem::path &output_path,
                         const EvaluationSummary &evaluation,
                         std::string *message);

/**
 * @brief 解析サマリJSONを書き出す.
 * @param output_path 出力先
 * @param summary サマリ
 * @param message エラー内容
 * @return 成功なら真
 */
bool WriteSummaryJson(const std::filesystem::path &output_path,
                      const AnalysisSummary &summary, std::string *message);

/**
 * @brief 浮動小数画像をデバッグPNGとして保存する.
 * @param output_path 出力先
 * @param image 入力画像
 * @param message エラー内容
 * @return 成功なら真
 */
bool WriteDebugFloatImage(const std::filesystem::path &output_path,
                          const xt::xtensor<float, 2> &image,
                          std::string *message);

/**
 * @brief マスク画像をデバッグPNGとして保存する.
 * @param output_path 出力先
 * @param mask 入力マスク
 * @param message エラー内容
 * @return 成功なら真
 */
bool WriteDebugMaskImage(const std::filesystem::path &output_path,
                         const xt::xtensor<std::uint8_t, 2> &mask,
                         std::string *message);

/**
 * @brief 候補重畳画像を保存する.
 * @param output_path 出力先
 * @param frame 入力画像
 * @param detections 候補一覧
 * @param message エラー内容
 * @return 成功なら真
 */
bool WriteDebugOverlayImage(const std::filesystem::path &output_path,
                            const xt::xtensor<float, 2> &frame,
                            const std::vector<Detection> &detections,
                            std::string *message);

/**
 * @brief 解析結果画像を保存する.
 * @param output_path 出力先
 * @param frame 入力画像
 * @param tracks_to_draw 描画対象軌跡一覧
 * @param frame_index 出力対象フレーム番号
 * @param message エラー内容
 * @return 成功なら真
 */
bool WriteResultImage(const std::filesystem::path &output_path,
                      const xt::xtensor<float, 2> &frame,
                      const std::vector<Track> &tracks_to_draw,
                      std::size_t frame_index, std::string *message);

/**
 * @brief 先頭星図上に軌跡一覧を重畳した要約画像を保存する.
 * @param output_path 出力先
 * @param frame 背景画像
 * @param tracks_to_draw 描画対象軌跡一覧
 * @param message エラー内容
 * @return 成功なら真
 */
bool WriteTrajectoryResultImage(const std::filesystem::path &output_path,
                                const xt::xtensor<float, 2> &frame,
                                const std::vector<Track> &tracks_to_draw,
                                std::string *message);

} // namespace transient_tracker::analyze
