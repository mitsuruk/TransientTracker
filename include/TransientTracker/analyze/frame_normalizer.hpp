/**
 * @file frame_normalizer.hpp
 * @brief フレーム正規化関数を宣言する.
 */

#pragma once

#include <vector>

#include <xtensor/containers/xtensor.hpp>

namespace transient_tracker::analyze {

/**
 * @brief 各フレームから中央値を引いて正規化する.
 * @param frames 入力フレーム列
 * @return 正規化済みフレーム列
 */
std::vector<xt::xtensor<float, 2>> NormalizeFramesByMedian(
    const std::vector<xt::xtensor<float, 2>>& frames);

}  // namespace transient_tracker::analyze
