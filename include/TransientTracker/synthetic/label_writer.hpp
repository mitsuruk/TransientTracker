/**
 * @file label_writer.hpp
 * @brief 真値ラベル保存関数を宣言する.
 */

#pragma once

#include <string>
#include <vector>

#include "TransientTracker/synthetic/scenario_config.hpp"

namespace transient_tracker::synthetic {

/**
 * @brief 真値CSVを書き出す.
 * @param output_path 出力パス
 * @param truths 真値一覧
 * @param message エラー内容の出力先
 * @return 成功なら真
 */
bool WriteLabelsCsv(
    const std::filesystem::path& output_path,
    const std::vector<FrameTruth>& truths,
    std::string* message);

/**
 * @brief メタデータJSONを書き出す.
 * @param output_path 出力パス
 * @param config シナリオ設定
 * @param manifest データセット配置
 * @param message エラー内容の出力先
 * @return 成功なら真
 */
bool WriteMetadataJson(
    const std::filesystem::path& output_path,
    const ScenarioConfig& config,
    const DatasetManifest& manifest,
    std::string* message);

}  // namespace transient_tracker::synthetic
