/**
 * @file frame_renderer.hpp
 * @brief フレーム描画関数を宣言する.
 */

#pragma once

#include <random>
#include <vector>

#include "TransientTracker/synthetic/scenario_config.hpp"

namespace transient_tracker::synthetic {

/**
 * @brief 1フレーム分の画像を描画する.
 * @param config シナリオ設定
 * @param stars 星群真値
 * @param shift 当該フレームのずれ
 * @param truths_for_frame 当該フレームの天体真値
 * @param rng 乱数生成器
 * @return 描画済み画像
 */
xt::xtensor<float, 2> RenderFrame(
    const ScenarioConfig& config,
    const std::vector<StarSource>& stars,
    const FrameShift& shift,
    const std::vector<FrameTruth>& truths_for_frame,
    std::mt19937_64* rng);

}  // namespace transient_tracker::synthetic
