/**
 * @file synthetic_types.hpp
 * @brief サンプルデータ生成器で共有する基本型を定義する.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <xtensor/containers/xtensor.hpp>

namespace transient_tracker::synthetic {

/**
 * @brief 画像サイズを表す.
 */
struct ImageSize {
    /** @brief 画像幅. */
    std::size_t width = 0;
    /** @brief 画像高さ. */
    std::size_t height = 0;
};

/**
 * @brief 静止星1個の真値を表す.
 */
struct StarSource {
    /** @brief 理想座標系でのX座標. */
    double x = 0.0;
    /** @brief 理想座標系でのY座標. */
    double y = 0.0;
    /** @brief 星の総フラックス. */
    double flux = 0.0;
    /** @brief 星像の標準偏差. */
    double sigma = 1.0;
};

/**
 * @brief 移動天体の共通仕様を表す.
 */
struct MovingObjectSpec {
    /** @brief 天体識別子. */
    std::size_t object_id = 0;
    /** @brief 初期X座標. */
    double initial_x = 0.0;
    /** @brief 初期Y座標. */
    double initial_y = 0.0;
    /** @brief フレームごとのX方向速度. */
    double velocity_x = 0.0;
    /** @brief フレームごとのY方向速度. */
    double velocity_y = 0.0;
    /** @brief 天体の総フラックス. */
    double flux = 0.0;
    /** @brief X方向の像幅. */
    double sigma_x = 1.0;
    /** @brief Y方向の像幅. */
    double sigma_y = 1.0;
    /** @brief 回転角度. 初期実装では未使用. */
    double angle_rad = 0.0;
};

/**
 * @brief フレームごとの真の並進ずれを表す.
 */
struct FrameShift {
    /** @brief フレーム番号. */
    std::size_t frame_index = 0;
    /** @brief X方向のずれ量. */
    double dx = 0.0;
    /** @brief Y方向のずれ量. */
    double dy = 0.0;
};

/**
 * @brief フレーム単位の真値情報を表す.
 */
struct FrameTruth {
    /** @brief フレーム番号. */
    std::size_t frame_index = 0;
    /** @brief 天体識別子. */
    std::size_t object_id = 0;
    /** @brief 真のX方向ずれ. */
    double shift_dx = 0.0;
    /** @brief 真のY方向ずれ. */
    double shift_dy = 0.0;
    /** @brief 理想座標系での天体X座標. */
    double object_x = 0.0;
    /** @brief 理想座標系での天体Y座標. */
    double object_y = 0.0;
    /** @brief 天体フラックス. */
    double object_flux = 0.0;
    /** @brief 天体のX方向像幅. */
    double sigma_x = 1.0;
    /** @brief 天体のY方向像幅. */
    double sigma_y = 1.0;
    /** @brief フレーム内で可視なら真. */
    bool object_visible = false;
};

/**
 * @brief レンダリング済みフレームを表す.
 */
struct RenderedFrame {
    /** @brief フレーム番号. */
    std::size_t frame_index = 0;
    /** @brief レンダリング済み画像. */
    xt::xtensor<float, 2> image;
    /** @brief 当該フレームの真値一覧. */
    std::vector<FrameTruth> truths;
};

/**
 * @brief データセット出力先の配置を表す.
 */
struct DatasetManifest {
    /** @brief データセットルート. */
    std::filesystem::path dataset_root;
    /** @brief フレーム画像ディレクトリ. */
    std::filesystem::path frames_dir;
    /** @brief 真値CSVのパス. */
    std::filesystem::path labels_csv_path;
    /** @brief メタデータJSONのパス. */
    std::filesystem::path metadata_json_path;
    /** @brief プレビュー画像ディレクトリ. */
    std::filesystem::path preview_dir;
};

/**
 * @brief コマンドライン引数を保持する.
 */
struct CliOptions {
    /** @brief シナリオ名. */
    std::string scenario_name;
    /** @brief 出力ディレクトリ. */
    std::filesystem::path output_dir;
    /** @brief 任意の乱数シード. */
    std::uint64_t seed = 0;
    /** @brief シード指定の有無. */
    bool has_seed = false;
    /** @brief 上書き幅. */
    std::size_t width = 0;
    /** @brief 幅指定の有無. */
    bool has_width = false;
    /** @brief 上書き高さ. */
    std::size_t height = 0;
    /** @brief 高さ指定の有無. */
    bool has_height = false;
    /** @brief 上書きフレーム数. */
    std::size_t num_frames = 0;
    /** @brief フレーム数指定の有無. */
    bool has_num_frames = false;
    /** @brief 上書き星数. */
    std::size_t num_stars = 0;
    /** @brief 星数指定の有無. */
    bool has_num_stars = false;
    /** @brief 上書きフラックス. */
    double object_flux = 0.0;
    /** @brief フラックス指定の有無. */
    bool has_object_flux = false;
    /** @brief 上書きX速度. */
    double object_velocity_x = 0.0;
    /** @brief X速度指定の有無. */
    bool has_object_velocity_x = false;
    /** @brief 上書きY速度. */
    double object_velocity_y = 0.0;
    /** @brief Y速度指定の有無. */
    bool has_object_velocity_y = false;
    /** @brief 全プレビュー出力フラグ. */
    bool write_all_preview = false;
    /** @brief ヘルプ表示フラグ. */
    bool show_help = false;
};

}  // namespace transient_tracker::synthetic
