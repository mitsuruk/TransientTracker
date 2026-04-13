/**
 * @file candidate_extractor.cpp
 * @brief 候補抽出処理を実装する.
 */

#include "TransientTracker/analyze/candidate_extractor.hpp"

#include <cmath>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <xtensor/generators/xbuilder.hpp>

#include "TransientTracker/analyze/math_utils.hpp"

namespace transient_tracker::analyze {

namespace {

/**
 * @brief 2値マスクをOpenCV画像へ変換する.
 * @param mask 入力マスク
 * @return OpenCV画像
 */
cv::Mat MakeMaskMat(const xt::xtensor<std::uint8_t, 2>& mask) {
    const std::size_t height = mask.shape()[0];
    const std::size_t width = mask.shape()[1];
    cv::Mat mat(static_cast<int>(height), static_cast<int>(width), CV_8UC1);
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            mat.at<std::uint8_t>(static_cast<int>(y), static_cast<int>(x)) = mask(y, x);
        }
    }
    return mat;
}

}  // namespace

xt::xtensor<float, 2> BuildResidualImage(
    const xt::xtensor<float, 2>& frame,
    const xt::xtensor<float, 2>& reference) {
    const std::size_t height = frame.shape()[0];
    const std::size_t width = frame.shape()[1];
    xt::xtensor<float, 2> residual = xt::zeros<float>({height, width});
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            residual(y, x) = frame(y, x) - reference(y, x);
        }
    }
    return residual;
}

double EstimateNoiseSigmaMad(const xt::xtensor<float, 2>& image) {
    std::vector<double> values;
    values.reserve(image.shape()[0] * image.shape()[1]);
    for (std::size_t y = 0; y < image.shape()[0]; ++y) {
        for (std::size_t x = 0; x < image.shape()[1]; ++x) {
            values.push_back(static_cast<double>(image(y, x)));
        }
    }

    const double median = ComputeMedian(&values);
    std::vector<double> absolute_deviations;
    absolute_deviations.reserve(values.size());
    for (double value : values) {
        absolute_deviations.push_back(std::fabs(value - median));
    }

    const double mad = ComputeMedian(&absolute_deviations);
    return std::max(1.0e-6, 1.4826 * mad);
}

xt::xtensor<std::uint8_t, 2> BuildThresholdMask(
    const xt::xtensor<float, 2>& residual,
    double sigma,
    double threshold_scale) {
    const std::size_t height = residual.shape()[0];
    const std::size_t width = residual.shape()[1];
    xt::xtensor<std::uint8_t, 2> mask = xt::zeros<std::uint8_t>({height, width});

    const double threshold = sigma * threshold_scale;
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            if (static_cast<double>(residual(y, x)) > threshold) {
                mask(y, x) = 255;
            }
        }
    }
    return mask;
}

std::vector<Detection> ExtractDetections(
    std::size_t frame_index,
    const xt::xtensor<float, 2>& residual,
    const xt::xtensor<std::uint8_t, 2>& mask,
    double sigma,
    std::size_t area_min,
    std::size_t area_max) {
    const cv::Mat mask_mat = MakeMaskMat(mask);
    cv::Mat labels;
    cv::Mat stats;
    cv::Mat centroids;
    const int num_labels = cv::connectedComponentsWithStats(mask_mat, labels, stats, centroids, 8, CV_32S);

    std::vector<Detection> detections;
    for (int label = 1; label < num_labels; ++label) {
        const int area = stats.at<int>(label, cv::CC_STAT_AREA);
        const std::size_t area_size = static_cast<std::size_t>(area);
        if (area_size < area_min || area_size > area_max) {
            continue;
        }

        Detection detection;
        detection.frame_index = frame_index;
        detection.x = centroids.at<double>(label, 0);
        detection.y = centroids.at<double>(label, 1);
        detection.area = area_size;
        detection.sigma_estimate = sigma;

        double peak_value = 0.0;
        double sum_value = 0.0;
        for (int y = 0; y < labels.rows; ++y) {
            for (int x = 0; x < labels.cols; ++x) {
                if (labels.at<int>(y, x) != label) {
                    continue;
                }
                const double value = std::max(0.0, static_cast<double>(residual(
                                                      static_cast<std::size_t>(y), static_cast<std::size_t>(x))));
                peak_value = std::max(peak_value, value);
                sum_value += value;
            }
        }

        detection.peak_value = peak_value;
        detection.sum_value = sum_value;
        detection.score = peak_value / std::max(1.0e-6, sigma);
        detections.push_back(detection);
    }

    return detections;
}

}  // namespace transient_tracker::analyze
