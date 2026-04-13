/**
 * @file synthetic_data_app.hpp
 * @brief 生成器アプリケーションの入口関数を宣言する.
 */

#pragma once

#include <string>

#include "TransientTracker/synthetic/synthetic_types.hpp"

namespace transient_tracker::synthetic {

/**
 * @brief コマンドライン引数を解析する.
 * @param argc 引数数
 * @param argv 引数配列
 * @param options 解析結果の出力先
 * @param message エラー内容の出力先
 * @return 成功なら真
 */
bool ParseCliOptions(int argc, char** argv, CliOptions* options, std::string* message);

/**
 * @brief ヘルプ文字列を返す.
 * @return ヘルプ文字列
 */
std::string BuildHelpText();

/**
 * @brief 生成器アプリケーションを実行する.
 * @param options コマンドライン引数
 * @return 終了コード
 */
int RunSyntheticDataApp(const CliOptions& options);

}  // namespace transient_tracker::synthetic
