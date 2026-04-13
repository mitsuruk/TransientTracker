/**
 * @file dataset_layout.hpp
 * @brief データセット出力配置の定義関数を宣言する.
 */

#pragma once

#include <filesystem>

#include "TransientTracker/synthetic/synthetic_types.hpp"

namespace transient_tracker::synthetic {

/**
 * @brief データセット配置を構築する.
 * @param dataset_root データセットルート
 * @return 生成した配置情報
 */
DatasetManifest BuildDatasetManifest(const std::filesystem::path& dataset_root);

/**
 * @brief データセット出力用ディレクトリを作成する.
 * @param manifest データセット配置
 * @param message エラー内容の出力先
 * @return 成功なら真
 */
bool CreateDatasetDirectories(const DatasetManifest& manifest, std::string* message);

}  // namespace transient_tracker::synthetic
