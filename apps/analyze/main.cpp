/**
 * @file main.cpp
 * @brief 移動天体解析器のエントリーポイント.
 */

#include <iostream>
#include <string>

#include "TransientTracker/analyze/analyze_app.hpp"
#include "TransientTracker/analyze/analysis_config.hpp"

/**
 * @brief アプリケーションを起動する.
 * @param argc 引数数
 * @param argv 引数配列
 * @return 終了コード
 */
int main(int argc, char** argv) {
    transient_tracker::analyze::AnalysisConfig config;
    std::string message;
    if (!transient_tracker::analyze::ParseAnalysisCliOptions(argc, argv, &config, &message)) {
        std::cerr << message << '\n';
        return 1;
    }

    if (config.show_help) {
        std::cout << transient_tracker::analyze::BuildAnalysisHelpText() << '\n';
        return 0;
    }

    return transient_tracker::analyze::RunAnalyzeApp(config);
}
