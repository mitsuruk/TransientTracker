/**
 * @file scenario_config.hpp
 * @brief シナリオ設定の定義と検証関数を宣言する.
 */

#pragma once

#include <string>

#include "TransientTracker/synthetic/synthetic_types.hpp"

namespace transient_tracker::synthetic {

/**
 * @brief サンプルデータ生成器のシナリオ設定を表す.
 */
struct ScenarioConfig {
    /** @brief シナリオ名. */
    std::string scenario_name;
    /** @brief 画像サイズ. */
    ImageSize image_size;
    /** @brief フレーム数. */
    std::size_t num_frames = 0;
    /** @brief 星数. */
    std::size_t num_stars = 0;
    /** @brief 一様背景レベル. */
    double background_level = 0.0;
    /** @brief X方向の背景勾配. */
    double background_gradient_x = 0.0;
    /** @brief Y方向の背景勾配. */
    double background_gradient_y = 0.0;
    /** @brief 読み出しノイズの標準偏差. */
    double read_noise_sigma = 0.0;
    /** @brief ポアソン雑音を使うなら真. */
    bool use_poisson_noise = false;
    /** @brief フレームずれX標準偏差. */
    double shift_sigma_x = 0.0;
    /** @brief フレームずれY標準偏差. */
    double shift_sigma_y = 0.0;
    /** @brief 星像幅. */
    double star_sigma = 1.0;
    /** @brief 移動天体仕様一覧. */
    std::vector<MovingObjectSpec> moving_objects;
    /** @brief 信号を落とすフレーム番号一覧. */
    std::vector<std::size_t> drop_frame_indices;
    /** @brief ホットピクセル数. */
    std::size_t hot_pixel_count = 0;
    /** @brief ホットピクセル強度の基準値. */
    double hot_pixel_peak = 0.0;
    /** @brief 全プレビューを出力するなら真. */
    bool write_all_preview = false;
    /** @brief 実行シード. */
    std::uint64_t seed = 0;
};

/**
 * @brief CLI(Command Line Interface)上書きを設定へ反映する.
 * @param base_config 基本設定
 * @param cli_options コマンドライン引数
 * @return 上書き反映後の設定
 */
ScenarioConfig ApplyOverrides(const ScenarioConfig& base_config, const CliOptions& cli_options);

/**
 * @brief 設定値の妥当性を検証する.
 * @param config 検証対象設定
 * @param message エラー内容の出力先
 * @return 妥当なら真
 */
bool ValidateScenarioConfig(const ScenarioConfig& config, std::string* message);

}  // namespace transient_tracker::synthetic
