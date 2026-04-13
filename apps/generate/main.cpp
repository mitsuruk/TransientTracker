/**
 * @file main.cpp
 * @brief サンプルデータ生成器のエントリーポイント.
 */

#include <iostream>
#include <string>

#include "TransientTracker/synthetic/synthetic_data_app.hpp"

/**
 * @brief アプリケーションを起動する.
 * @param argc 引数数
 * @param argv 引数配列
 * @return 終了コード
 */
int main(int argc, char** argv) {
    transient_tracker::synthetic::CliOptions options;
    std::string message;
    if (!transient_tracker::synthetic::ParseCliOptions(argc, argv, &options, &message)) {
        std::cerr << message << '\n';
        return 1;
    }

    if (options.show_help) {
        std::cout << transient_tracker::synthetic::BuildHelpText() << '\n';
        return 0;
    }

    return transient_tracker::synthetic::RunSyntheticDataApp(options);
}
