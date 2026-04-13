/**
 * @file reference_builder.cpp
 * @brief 基準画像生成を実装する.
 */

#include "TransientTracker/analyze/reference_builder.hpp"

#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <xtensor/generators/xbuilder.hpp>

#include "TransientTracker/analyze/math_utils.hpp"

namespace transient_tracker::analyze {

xt::xtensor<float, 2> BuildMedianReference(const std::vector<xt::xtensor<float, 2>>& frames) {
    if (frames.empty()) {
        return xt::xtensor<float, 2>();
    }

    const std::size_t height = frames.front().shape()[0];
    const std::size_t width = frames.front().shape()[1];
    xt::xtensor<float, 2> reference = xt::zeros<float>({height, width});

    std::vector<float> samples;
    samples.reserve(frames.size());
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            samples.clear();
            for (const xt::xtensor<float, 2>& frame : frames) {
                samples.push_back(frame(y, x));
            }
            reference(y, x) = ComputeMedian(&samples);
        }
    }
    return reference;
}

xt::xtensor<float, 2> BuildMaxEnvelopeReference(
    const xt::xtensor<float, 2>& reference,
    int tolerance_radius) {
    if (reference.dimension() != 2 || tolerance_radius <= 0) {
        return reference;
    }

    const std::size_t height = reference.shape()[0];
    const std::size_t width = reference.shape()[1];
    cv::Mat reference_mat(static_cast<int>(height), static_cast<int>(width), CV_32FC1);
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            reference_mat.at<float>(static_cast<int>(y), static_cast<int>(x)) = reference(y, x);
        }
    }

    const int kernel_size = 2 * tolerance_radius + 1;
    const cv::Mat kernel = cv::getStructuringElement(
        cv::MORPH_ELLIPSE, cv::Size(kernel_size, kernel_size));
    cv::Mat dilated_mat;
    cv::dilate(reference_mat, dilated_mat, kernel);

    xt::xtensor<float, 2> envelope = xt::zeros<float>({height, width});
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            envelope(y, x) = dilated_mat.at<float>(static_cast<int>(y), static_cast<int>(x));
        }
    }
    return envelope;
}

}  // namespace transient_tracker::analyze
