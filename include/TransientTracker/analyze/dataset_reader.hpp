/**
 * @file dataset_reader.hpp
 * @brief データセット契約の読込関数を宣言する.
 */

#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "TransientTracker/analyze/analysis_types.hpp"

namespace transient_tracker::analyze {

/**
 * @brief データセット入力パス群を構築する.
 * @param dataset_root データセットルート
 * @return 入力パス群
 */
DatasetPaths BuildDatasetPaths(const std::filesystem::path& dataset_root);

/**
 * @brief データセット構造を検証する.
 * @param paths 入力パス群
 * @param message エラー内容
 * @return 妥当なら真
 */
bool ValidateDatasetStructure(const DatasetPaths& paths, std::string* message);

/**
 * @brief メタデータJSONを読み込む.
 * @param metadata_json_path JSONパス
 * @param metadata 出力先
 * @param message エラー内容
 * @return 成功なら真
 */
bool ReadDatasetMetadata(
    const std::filesystem::path& metadata_json_path,
    DatasetMetadata* metadata,
    std::string* message);

/**
 * @brief 真値CSVを読み込む.
 * @param labels_csv_path CSVパス
 * @param truths 出力先
 * @param message エラー内容
 * @return 成功なら真
 */
bool ReadGroundTruthCsv(
    const std::filesystem::path& labels_csv_path,
    std::vector<GroundTruthRecord>* truths,
    std::string* message);

}  // namespace transient_tracker::analyze
