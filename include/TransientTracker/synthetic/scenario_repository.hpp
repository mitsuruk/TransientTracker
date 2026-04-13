/**
 * @file scenario_repository.hpp
 * @brief 既定シナリオの取得関数を宣言する.
 */

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "TransientTracker/synthetic/scenario_config.hpp"

namespace transient_tracker::synthetic {

/**
 * @brief シナリオ名から既定設定を取得する.
 * @param scenario_name シナリオ名
 * @return 設定. 未知の名前なら空
 */
std::optional<ScenarioConfig> FindScenario(const std::string& scenario_name);

/**
 * @brief 利用可能なシナリオ名一覧を返す.
 * @return シナリオ名一覧
 */
std::vector<std::string> ListScenarioNames();

}  // namespace transient_tracker::synthetic
