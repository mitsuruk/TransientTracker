/**
 * @file registration_estimator.cpp
 * @brief 並進位置合わせ推定を実装する.
 */

#include "TransientTracker/analyze/registration_estimator.hpp"

#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

namespace transient_tracker::analyze {

namespace {

/**
 * @brief テンソルをOpenCV浮動小数画像へ変換する.
 * @param frame 入力フレーム
 * @return OpenCV画像
 */
cv::Mat MakeFloatMat(const xt::xtensor<float, 2>& frame) {
    const std::size_t height = frame.shape()[0];
    const std::size_t width = frame.shape()[1];
    cv::Mat mat(static_cast<int>(height), static_cast<int>(width), CV_32FC1);
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            mat.at<float>(static_cast<int>(y), static_cast<int>(x)) = frame(y, x);
        }
    }
    return mat;
}

}  // namespace

std::vector<Shift2D> EstimateTranslationsPhaseCorrelation(
    const std::vector<xt::xtensor<float, 2>>& frames) {
    std::vector<Shift2D> shifts;
    shifts.reserve(frames.size());
    if (frames.empty()) {
        return shifts;
    }

    const cv::Mat reference = MakeFloatMat(frames.front());
    cv::Mat hann_window;
    cv::createHanningWindow(
        hann_window, cv::Size(reference.cols, reference.rows), CV_32FC1);

    Shift2D reference_shift;
    reference_shift.frame_index = 0;
    reference_shift.dx = 0.0;
    reference_shift.dy = 0.0;
    reference_shift.response = 1.0;
    shifts.push_back(reference_shift);

    for (std::size_t frame_index = 1; frame_index < frames.size(); ++frame_index) {
        const cv::Mat current = MakeFloatMat(frames[frame_index]);
        double response = 0.0;
        const cv::Point2d shift = cv::phaseCorrelate(reference, current, hann_window, &response);

        Shift2D estimated_shift;
        estimated_shift.frame_index = frame_index;
        estimated_shift.dx = shift.x;
        estimated_shift.dy = shift.y;
        estimated_shift.response = response;
        shifts.push_back(estimated_shift);
    }

    return shifts;
}

}  // namespace transient_tracker::analyze
