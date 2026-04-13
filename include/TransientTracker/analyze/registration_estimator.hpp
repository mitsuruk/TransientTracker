/**
 * @file registration_estimator.hpp
 * @brief 並進位置合わせ推定関数を宣言する.
 */

#pragma once

#include <vector>

#include <xtensor/containers/xtensor.hpp>

#include "TransientTracker/analyze/analysis_types.hpp"

namespace transient_tracker::analyze {

/**
 * @brief フレーム列に対する並進シフトを推定する.
 * @param frames 入力フレーム列
 * @return 推定シフト列
 */
std::vector<Shift2D> EstimateTranslationsPhaseCorrelation(
    const std::vector<xt::xtensor<float, 2>>& frames);

}  // namespace transient_tracker::analyze
