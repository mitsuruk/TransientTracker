/**
 * @file frame_renderer.cpp
 * @brief フレーム描画処理を実装する.
 */

#include "TransientTracker/synthetic/frame_renderer.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

#include <xtensor/generators/xbuilder.hpp>

#include "TransientTracker/synthetic/noise_model.hpp"

namespace transient_tracker::synthetic {

namespace {

/**
 * @brief 2次元ガウス像を描画する.
 * @param image 描画先画像
 * @param center_x 中心X座標
 * @param center_y 中心Y座標
 * @param flux フラックス
 * @param sigma_x X方向像幅
 * @param sigma_y Y方向像幅
 */
void AddGaussianSource(
    xt::xtensor<float, 2>* image,
    double center_x,
    double center_y,
    double flux,
    double sigma_x,
    double sigma_y) {
    if (image == nullptr || flux <= 0.0 || sigma_x <= 0.0 || sigma_y <= 0.0) {
        return;
    }

    const std::size_t height = image->shape()[0];
    const std::size_t width = image->shape()[1];

    const int x_min = std::max(0, static_cast<int>(std::floor(center_x - 3.0 * sigma_x)));
    const int x_max = std::min(
        static_cast<int>(width) - 1, static_cast<int>(std::ceil(center_x + 3.0 * sigma_x)));
    const int y_min = std::max(0, static_cast<int>(std::floor(center_y - 3.0 * sigma_y)));
    const int y_max = std::min(
        static_cast<int>(height) - 1, static_cast<int>(std::ceil(center_y + 3.0 * sigma_y)));
    const double normalization = flux / (2.0 * std::numbers::pi_v<double> * sigma_x * sigma_y);

    for (int y = y_min; y <= y_max; ++y) {
        for (int x = x_min; x <= x_max; ++x) {
            const double dx = (static_cast<double>(x) - center_x) / sigma_x;
            const double dy = (static_cast<double>(y) - center_y) / sigma_y;
            const double value = normalization * std::exp(-0.5 * (dx * dx + dy * dy));
            (*image)(static_cast<std::size_t>(y), static_cast<std::size_t>(x)) +=
                static_cast<float>(value);
        }
    }
}

/**
 * @brief 背景と勾配を加える.
 * @param config シナリオ設定
 * @param image 画像
 */
void AddBackground(const ScenarioConfig& config, xt::xtensor<float, 2>* image) {
    const std::size_t height = image->shape()[0];
    const std::size_t width = image->shape()[1];
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            (*image)(y, x) = static_cast<float>(
                config.background_level + config.background_gradient_x * static_cast<double>(x) +
                config.background_gradient_y * static_cast<double>(y));
        }
    }
}

}  // namespace

xt::xtensor<float, 2> RenderFrame(
    const ScenarioConfig& config,
    const std::vector<StarSource>& stars,
    const FrameShift& shift,
    const std::vector<FrameTruth>& truths_for_frame,
    std::mt19937_64* rng) {
    xt::xtensor<float, 2> image =
        xt::zeros<float>({config.image_size.height, config.image_size.width});

    AddBackground(config, &image);

    for (const StarSource& star : stars) {
        AddGaussianSource(
            &image,
            star.x + shift.dx,
            star.y + shift.dy,
            star.flux,
            star.sigma,
            star.sigma);
    }

    for (const FrameTruth& truth : truths_for_frame) {
        if (!truth.object_visible) {
            continue;
        }
        AddGaussianSource(
            &image,
            truth.object_x + shift.dx,
            truth.object_y + shift.dy,
            truth.object_flux,
            truth.sigma_x,
            truth.sigma_y);
    }

    InjectHotPixels(&image, config.hot_pixel_count, rng, static_cast<float>(config.hot_pixel_peak));
    if (config.use_poisson_noise) {
        ApplyPoissonNoise(&image, rng);
    }
    ApplyGaussianNoise(&image, config.read_noise_sigma, rng);

    return image;
}

}  // namespace transient_tracker::synthetic
