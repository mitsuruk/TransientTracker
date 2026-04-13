/**
 * @file frame_loader.cpp
 * @brief フレーム画像の読込を実装する.
 */

#include "TransientTracker/analyze/frame_loader.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <xtensor/generators/xbuilder.hpp>

namespace transient_tracker::analyze {

namespace {

/**
 * @brief メッセージを設定する.
 * @param message 出力先
 * @param text 設定内容
 */
void SetMessage(std::string* message, const std::string& text) {
    if (message != nullptr) {
        *message = text;
    }
}

/**
 * @brief フレーム名かを判定する.
 * @param path 入力パス
 * @return フレーム画像なら真
 */
bool IsFramePng(const std::filesystem::path& path) {
    const std::string filename = path.filename().string();
    return path.extension() == ".png" && filename.rfind("frame_", 0) == 0;
}

/**
 * @brief OpenCV画像を浮動小数テンソルへ変換する.
 * @param image 入力画像
 * @return 変換後テンソル
 */
xt::xtensor<float, 2> ConvertMatToTensor(const cv::Mat& image) {
    cv::Mat grayscale;
    if (image.channels() == 1) {
        grayscale = image;
    } else {
        cv::cvtColor(image, grayscale, cv::COLOR_BGR2GRAY);
    }

    const std::size_t height = static_cast<std::size_t>(grayscale.rows);
    const std::size_t width = static_cast<std::size_t>(grayscale.cols);
    xt::xtensor<float, 2> frame = xt::zeros<float>({height, width});

    if (grayscale.type() == CV_16UC1) {
        for (std::size_t y = 0; y < height; ++y) {
            for (std::size_t x = 0; x < width; ++x) {
                frame(y, x) =
                    static_cast<float>(grayscale.at<std::uint16_t>(static_cast<int>(y), static_cast<int>(x)));
            }
        }
        return frame;
    }

    if (grayscale.type() == CV_8UC1) {
        for (std::size_t y = 0; y < height; ++y) {
            for (std::size_t x = 0; x < width; ++x) {
                frame(y, x) =
                    static_cast<float>(grayscale.at<std::uint8_t>(static_cast<int>(y), static_cast<int>(x)));
            }
        }
        return frame;
    }

    cv::Mat float_image;
    grayscale.convertTo(float_image, CV_32FC1);
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            frame(y, x) = float_image.at<float>(static_cast<int>(y), static_cast<int>(x));
        }
    }
    return frame;
}

}  // namespace

bool ListFramePaths(
    const DatasetPaths& paths,
    std::vector<std::filesystem::path>* frame_paths,
    std::string* message) {
    if (frame_paths == nullptr) {
        SetMessage(message, "frame_paths が null です.");
        return false;
    }

    frame_paths->clear();
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(paths.frames_dir)) {
        if (entry.is_regular_file() && IsFramePng(entry.path())) {
            frame_paths->push_back(entry.path());
        }
    }

    std::sort(frame_paths->begin(), frame_paths->end());
    if (frame_paths->empty()) {
        SetMessage(message, "frames ディレクトリに frame_*.png が見つかりません.");
        return false;
    }
    return true;
}

bool LoadFrames(
    const std::vector<std::filesystem::path>& frame_paths,
    std::vector<xt::xtensor<float, 2>>* frames,
    std::string* message) {
    if (frames == nullptr) {
        SetMessage(message, "frames が null です.");
        return false;
    }

    frames->clear();
    frames->reserve(frame_paths.size());
    for (const std::filesystem::path& frame_path : frame_paths) {
        const cv::Mat image = cv::imread(frame_path.string(), cv::IMREAD_UNCHANGED);
        if (image.empty()) {
            SetMessage(message, "フレーム画像の読込に失敗しました: " + frame_path.string());
            return false;
        }
        frames->push_back(ConvertMatToTensor(image));
    }
    return true;
}

}  // namespace transient_tracker::analyze
