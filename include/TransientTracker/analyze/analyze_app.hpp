/**
 * @file analyze_app.hpp
 * @brief 解析器アプリケーション本体を宣言する.
 */

#pragma once

#include "TransientTracker/analyze/analysis_config.hpp"

namespace transient_tracker::analyze {

/**
 * @brief 解析器を実行する.
 * @param config 実行設定
 * @return 終了コード
 */
int RunAnalyzeApp(const AnalysisConfig& config);

}  // namespace transient_tracker::analyze
