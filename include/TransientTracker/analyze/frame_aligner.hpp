/**
 * @file frame_aligner.hpp
 * @brief フレーム再配置関数を宣言する.
 */

#pragma once

#include <vector>

#include <xtensor/containers/xtensor.hpp>

#include "TransientTracker/analyze/analysis_types.hpp"

namespace transient_tracker::analyze {

/**
 * @brief 推定シフトでフレームを整列する.
 * @param frames 入力フレーム列
 * @param shifts 推定シフト列
 * @return 整列後フレーム列. シフト数が一致しない場合は空
 */
std::vector<xt::xtensor<float, 2>> AlignFramesByTranslation(
    const std::vector<xt::xtensor<float, 2>>& frames,
    const std::vector<Shift2D>& shifts);

}  // namespace transient_tracker::analyze
