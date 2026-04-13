/**
 * @file reference_builder.hpp
 * @brief 基準画像生成関数を宣言する.
 */

#pragma once

#include <vector>

#include <xtensor/containers/xtensor.hpp>

namespace transient_tracker::analyze {

/**
 * @brief 時系列中央値から基準画像を構築する.
 * @param frames 入力フレーム列
 * @return 基準画像
 */
xt::xtensor<float, 2> BuildMedianReference(const std::vector<xt::xtensor<float, 2>>& frames);

/**
 * @brief 許容誤差付き最大包絡画像を構築する.
 * @param reference 基準画像
 * @param tolerance_radius 許容誤差半径
 * @return 最大包絡画像
 */
xt::xtensor<float, 2> BuildMaxEnvelopeReference(
    const xt::xtensor<float, 2>& reference,
    int tolerance_radius);

}  // namespace transient_tracker::analyze
