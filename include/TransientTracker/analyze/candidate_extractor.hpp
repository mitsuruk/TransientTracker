/**
 * @file candidate_extractor.hpp
 * @brief 候補抽出関数を宣言する.
 */

#pragma once

#include <cstdint>
#include <vector>

#include <xtensor/containers/xtensor.hpp>

#include "TransientTracker/analyze/analysis_types.hpp"

namespace transient_tracker::analyze {

/**
 * @brief 残差画像を構築する.
 * @param frame 入力フレーム
 * @param reference 基準画像
 * @return 残差画像
 */
xt::xtensor<float, 2> BuildResidualImage(
    const xt::xtensor<float, 2>& frame,
    const xt::xtensor<float, 2>& reference);

/**
 * @brief MADベースでノイズ標準偏差を推定する.
 * @param image 入力画像
 * @return 推定標準偏差
 */
double EstimateNoiseSigmaMad(const xt::xtensor<float, 2>& image);

/**
 * @brief 残差画像から2値マスクを構築する.
 * @param residual 残差画像
 * @param sigma 推定ノイズ標準偏差
 * @param threshold_scale しきい値倍率
 * @return 候補マスク
 */
xt::xtensor<std::uint8_t, 2> BuildThresholdMask(
    const xt::xtensor<float, 2>& residual,
    double sigma,
    double threshold_scale);

/**
 * @brief 候補マスクから候補一覧を抽出する.
 * @param frame_index フレーム番号
 * @param residual 残差画像
 * @param mask 候補マスク
 * @param sigma 推定ノイズ標準偏差
 * @param area_min 最小面積
 * @param area_max 最大面積
 * @return 候補一覧
 */
std::vector<Detection> ExtractDetections(
    std::size_t frame_index,
    const xt::xtensor<float, 2>& residual,
    const xt::xtensor<std::uint8_t, 2>& mask,
    double sigma,
    std::size_t area_min,
    std::size_t area_max);

}  // namespace transient_tracker::analyze
