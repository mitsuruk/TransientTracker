/**
 * @file frame_loader.hpp
 * @brief フレーム画像の読込関数を宣言する.
 */

#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <xtensor/containers/xtensor.hpp>

#include "TransientTracker/analyze/analysis_types.hpp"

namespace transient_tracker::analyze {

/**
 * @brief フレーム画像パス一覧を列挙する.
 * @param paths データセット入力パス群
 * @param frame_paths 出力先
 * @param message エラー内容
 * @return 成功なら真
 */
bool ListFramePaths(
    const DatasetPaths& paths,
    std::vector<std::filesystem::path>* frame_paths,
    std::string* message);

/**
 * @brief フレーム画像を読み込む.
 * @param frame_paths フレーム画像パス一覧
 * @param frames 出力先
 * @param message エラー内容
 * @return 成功なら真
 */
bool LoadFrames(
    const std::vector<std::filesystem::path>& frame_paths,
    std::vector<xt::xtensor<float, 2>>* frames,
    std::string* message);

}  // namespace transient_tracker::analyze
