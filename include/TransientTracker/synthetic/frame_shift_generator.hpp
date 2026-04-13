/**
 * @file frame_shift_generator.hpp
 * @brief フレームずれ生成関数を宣言する.
 */

#pragma once

#include <random>
#include <vector>

#include "TransientTracker/synthetic/scenario_config.hpp"

namespace transient_tracker::synthetic {

/**
 * @brief 各フレームの真の並進ずれを生成する.
 * @param config シナリオ設定
 * @param rng 乱数生成器
 * @return フレームずれ一覧
 */
std::vector<FrameShift> GenerateFrameShifts(const ScenarioConfig& config, std::mt19937_64* rng);

}  // namespace transient_tracker::synthetic
