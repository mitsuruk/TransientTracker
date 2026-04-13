/**
 * @file preview_writer.cpp
 * @brief 画像保存処理を実装する.
 */

#include "TransientTracker/synthetic/preview_writer.hpp"

#include <algorithm>
#include <limits>
#include <string>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace transient_tracker::synthetic {

namespace {

/**
 * @brief 画像を16bit Matへ変換する.
 * @param image 入力画像
 * @return 16bit Mat
 */
cv::Mat MakeFrameMat(const xt::xtensor<float, 2>& image) {
    const std::size_t height = image.shape()[0];
    const std::size_t width = image.shape()[1];
    cv::Mat mat(static_cast<int>(height), static_cast<int>(width), CV_16UC1);
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            const float value = std::clamp(image(y, x), 0.0F, 65535.0F);
            mat.at<std::uint16_t>(static_cast<int>(y), static_cast<int>(x)) =
                static_cast<std::uint16_t>(value);
        }
    }
    return mat;
}

/**
 * @brief 画像を8bit可視化用 Matへ変換する.
 * @param image 入力画像
 * @return 8bit Mat
 */
cv::Mat MakePreviewMat(const xt::xtensor<float, 2>& image) {
    const std::size_t height = image.shape()[0];
    const std::size_t width = image.shape()[1];
    float min_value = std::numeric_limits<float>::max();
    float max_value = std::numeric_limits<float>::lowest();
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            min_value = std::min(min_value, image(y, x));
            max_value = std::max(max_value, image(y, x));
        }
    }

    const float range = std::max(1.0e-6F, max_value - min_value);
    cv::Mat mat(static_cast<int>(height), static_cast<int>(width), CV_8UC1);
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            const float normalized = (image(y, x) - min_value) / range;
            const float scaled = std::clamp(normalized * 255.0F, 0.0F, 255.0F);
            mat.at<std::uint8_t>(static_cast<int>(y), static_cast<int>(x)) =
                static_cast<std::uint8_t>(scaled);
        }
    }
    return mat;
}

/**
 * @brief 十字マーカを描画する.
 * @param image 描画先
 * @param x 中心X座標
 * @param y 中心Y座標
 */
void DrawCrossMarker(cv::Mat* image, int x, int y) {
    const cv::Scalar color(255);
    cv::line(*image, cv::Point(x - 5, y), cv::Point(x + 5, y), color, 1, cv::LINE_AA);
    cv::line(*image, cv::Point(x, y - 5), cv::Point(x, y + 5), color, 1, cv::LINE_AA);
}

/**
 * @brief 物体IDラベルを描画する.
 * @param image 描画先
 * @param x 基準X
 * @param y 基準Y
 * @param object_id 物体ID
 */
void DrawObjectLabel(cv::Mat* image, int x, int y, std::size_t object_id) {
    cv::putText(
        *image,
        std::to_string(object_id),
        cv::Point(x + 6, y - 6),
        cv::FONT_HERSHEY_SIMPLEX,
        0.35,
        cv::Scalar(255),
        1,
        cv::LINE_AA);
}

}  // namespace

bool WriteFrameImage(
    const std::filesystem::path& output_path,
    const xt::xtensor<float, 2>& image,
    std::string* message) {
    const cv::Mat frame_mat = MakeFrameMat(image);
    if (!cv::imwrite(output_path.string(), frame_mat)) {
        if (message != nullptr) {
            *message = "フレーム画像の保存に失敗しました: " + output_path.string();
        }
        return false;
    }
    return true;
}

bool WritePreviewImage(
    const std::filesystem::path& output_path,
    const xt::xtensor<float, 2>& image,
    const std::vector<FrameTruth>& truths_for_frame,
    const FrameShift& shift,
    std::string* message) {
    cv::Mat preview = MakePreviewMat(image);
    for (const FrameTruth& truth : truths_for_frame) {
        if (!truth.object_visible) {
            continue;
        }
        const int marker_x = static_cast<int>(std::lround(truth.object_x + shift.dx));
        const int marker_y = static_cast<int>(std::lround(truth.object_y + shift.dy));
        if (marker_x >= 0 && marker_x < preview.cols && marker_y >= 0 && marker_y < preview.rows) {
            DrawCrossMarker(&preview, marker_x, marker_y);
            DrawObjectLabel(&preview, marker_x, marker_y, truth.object_id);
        }
    }

    if (!cv::imwrite(output_path.string(), preview)) {
        if (message != nullptr) {
            *message = "プレビュー画像の保存に失敗しました: " + output_path.string();
        }
        return false;
    }
    return true;
}

}  // namespace transient_tracker::synthetic
