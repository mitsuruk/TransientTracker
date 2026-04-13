/**
 * @file analysis_config.hpp
 * @brief 解析器の設定とCLI解析関数を宣言する.
 */

#pragma once

#include <cstddef>
#include <filesystem>
#include <string>

namespace transient_tracker::analyze {

/**
 * @brief 解析器の実行設定を表す.
 */
struct AnalysisConfig {
    /** @brief 入力データセット. */
    std::filesystem::path dataset_root;
    /** @brief 出力先ディレクトリ. */
    std::filesystem::path output_root;
    /** @brief 許容誤差半径. */
    int tolerance_radius = 1;
    /** @brief 残差しきい値倍率. */
    double threshold_scale = 5.0;
    /** @brief 最小候補面積. */
    std::size_t area_min = 1;
    /** @brief 最大候補面積. */
    std::size_t area_max = 64;
    /** @brief 最大速度. */
    double max_velocity_per_frame = 3.0;
    /** @brief 最小軌跡長. */
    std::size_t min_track_length = 3;
    /** @brief 真値一致距離. */
    double truth_match_distance = 2.0;
    /** @brief デバッグ画像出力フラグ. */
    bool write_debug_images = false;
    /** @brief 位置合わせ無効フラグ. */
    bool disable_registration = false;
    /** @brief ヘルプ表示フラグ. */
    bool show_help = false;
};

/**
 * @brief CLI引数を解析する.
 * @param argc 引数数
 * @param argv 引数配列
 * @param config 出力先設定
 * @param message エラー内容
 * @return 成功なら真
 */
bool ParseAnalysisCliOptions(int argc, char** argv, AnalysisConfig* config, std::string* message);

/**
 * @brief 設定妥当性を検証する.
 * @param config 設定
 * @param message エラー内容
 * @return 妥当なら真
 */
bool ValidateAnalysisConfig(const AnalysisConfig& config, std::string* message);

/**
 * @brief ヘルプ文字列を構築する.
 * @return ヘルプ文字列
 */
std::string BuildAnalysisHelpText();

}  // namespace transient_tracker::analyze
