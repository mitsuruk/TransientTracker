/**
 * @file frame_aligner.cpp
 * @brief フレーム再配置処理を実装する.
 */

#include "TransientTracker/analyze/frame_aligner.hpp"

#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <xtensor/generators/xbuilder.hpp>

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

/**
 * @brief OpenCV浮動小数画像をテンソルへ変換する.
 * @param image 入力画像
 * @return テンソル
 */
xt::xtensor<float, 2> MakeTensor(const cv::Mat& image) {
    const std::size_t height = static_cast<std::size_t>(image.rows);
    const std::size_t width = static_cast<std::size_t>(image.cols);
    xt::xtensor<float, 2> frame = xt::zeros<float>({height, width});
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            frame(y, x) = image.at<float>(static_cast<int>(y), static_cast<int>(x));
        }
    }
    return frame;
}

}  // namespace

std::vector<xt::xtensor<float, 2>> AlignFramesByTranslation(
    const std::vector<xt::xtensor<float, 2>>& frames,
    const std::vector<Shift2D>& shifts) {
    if (frames.size() != shifts.size()) {
        return {};
    }

    std::vector<xt::xtensor<float, 2>> aligned_frames;
    aligned_frames.reserve(frames.size());
    for (std::size_t frame_index = 0; frame_index < frames.size(); ++frame_index) {
        const cv::Mat frame_mat = MakeFloatMat(frames[frame_index]);
        cv::Mat aligned_mat;
        const cv::Mat transform = (cv::Mat_<double>(2, 3)
            << 1.0, 0.0, -shifts[frame_index].dx,
               0.0, 1.0, -shifts[frame_index].dy);
        cv::warpAffine(
            frame_mat,
            aligned_mat,
            transform,
            frame_mat.size(),
            cv::INTER_LINEAR,
            cv::BORDER_CONSTANT,
            cv::Scalar(0.0));
        aligned_frames.push_back(MakeTensor(aligned_mat));
    }
    return aligned_frames;
}

}  // namespace transient_tracker::analyze
