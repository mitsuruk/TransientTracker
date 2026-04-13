/**
 * @file noise_model.cpp
 * @brief ノイズ付与処理を実装する.
 */

#include "TransientTracker/synthetic/noise_model.hpp"

#include <algorithm>
#include <cmath>

namespace transient_tracker::synthetic {

void ApplyGaussianNoise(xt::xtensor<float, 2>* image, double sigma, std::mt19937_64* rng) {
    if (image == nullptr || sigma <= 0.0) {
        return;
    }

    std::normal_distribution<float> distribution(0.0F, static_cast<float>(sigma));
    const std::size_t height = image->shape()[0];
    const std::size_t width = image->shape()[1];
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            (*image)(y, x) += distribution(*rng);
        }
    }
}

void ApplyPoissonNoise(xt::xtensor<float, 2>* image, std::mt19937_64* rng) {
    if (image == nullptr) {
        return;
    }

    const std::size_t height = image->shape()[0];
    const std::size_t width = image->shape()[1];
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            const double lambda = std::max(0.0, static_cast<double>((*image)(y, x)));
            std::poisson_distribution<int> distribution(lambda);
            (*image)(y, x) = static_cast<float>(distribution(*rng));
        }
    }
}

void InjectHotPixels(
    xt::xtensor<float, 2>* image,
    std::size_t hot_pixel_count,
    std::mt19937_64* rng,
    float peak_value) {
    if (image == nullptr || hot_pixel_count == 0) {
        return;
    }

    const std::size_t height = image->shape()[0];
    const std::size_t width = image->shape()[1];
    std::uniform_int_distribution<std::size_t> y_distribution(0, height - 1);
    std::uniform_int_distribution<std::size_t> x_distribution(0, width - 1);
    std::uniform_real_distribution<float> scale_distribution(0.8F, 1.2F);

    for (std::size_t index = 0; index < hot_pixel_count; ++index) {
        const std::size_t y = y_distribution(*rng);
        const std::size_t x = x_distribution(*rng);
        (*image)(y, x) += peak_value * scale_distribution(*rng);
    }
}

}  // namespace transient_tracker::synthetic
