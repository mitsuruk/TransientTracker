/**
 * @file preview_writer.hpp
 * @brief PNG保存関数を宣言する.
 */

#pragma once

#include <string>
#include <vector>

#include "TransientTracker/synthetic/synthetic_types.hpp"

namespace transient_tracker::synthetic {

/**
 * @brief 生フレーム画像を16bit PNGとして保存する.
 * @param output_path 出力パス
 * @param image 画像
 * @param message エラー内容の出力先
 * @return 成功なら真
 */
bool WriteFrameImage(
    const std::filesystem::path& output_path,
    const xt::xtensor<float, 2>& image,
    std::string* message);

/**
 * @brief プレビュー画像を保存する.
 * @param output_path 出力パス
 * @param image 画像
 * @param truths_for_frame 当該フレームの真値
 * @param shift フレームずれ
 * @param message エラー内容の出力先
 * @return 成功なら真
 */
bool WritePreviewImage(
    const std::filesystem::path& output_path,
    const xt::xtensor<float, 2>& image,
    const std::vector<FrameTruth>& truths_for_frame,
    const FrameShift& shift,
    std::string* message);

}  // namespace transient_tracker::synthetic
