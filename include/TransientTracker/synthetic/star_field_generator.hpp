/**
 * @file star_field_generator.hpp
 * @brief 静止星群生成関数を宣言する.
 */

#pragma once

#include <random>
#include <vector>

#include "TransientTracker/synthetic/scenario_config.hpp"

namespace transient_tracker::synthetic {

/**
 * @brief 静止星群の真値を生成する.
 * @param config シナリオ設定
 * @param rng 乱数生成器
 * @return 星源一覧
 */
std::vector<StarSource> GenerateStars(const ScenarioConfig& config, std::mt19937_64* rng);

}  // namespace transient_tracker::synthetic
