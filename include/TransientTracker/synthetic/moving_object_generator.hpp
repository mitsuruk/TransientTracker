/**
 * @file moving_object_generator.hpp
 * @brief 移動天体真値生成関数を宣言する.
 */

#pragma once

#include <vector>

#include "TransientTracker/synthetic/scenario_config.hpp"

namespace transient_tracker::synthetic {

/**
 * @brief 全フレームの移動天体真値を生成する.
 * @param config シナリオ設定
 * @return フレーム真値一覧
 */
std::vector<FrameTruth> GenerateObjectTruths(const ScenarioConfig& config);

}  // namespace transient_tracker::synthetic
