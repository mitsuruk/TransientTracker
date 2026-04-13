/**
 * @file noise_model.hpp
 * @brief ノイズ付与関数を宣言する.
 */

#pragma once

#include <random>

#include <xtensor/containers/xtensor.hpp>

namespace transient_tracker::synthetic {

/**
 * @brief ガウス雑音を画像へ加える.
 * @param image 画像
 * @param sigma 標準偏差
 * @param rng 乱数生成器
 */
void ApplyGaussianNoise(xt::xtensor<float, 2>* image, double sigma, std::mt19937_64* rng);

/**
 * @brief ポアソン雑音を画像へ加える.
 * @param image 画像
 * @param rng 乱数生成器
 */
void ApplyPoissonNoise(xt::xtensor<float, 2>* image, std::mt19937_64* rng);

/**
 * @brief ホットピクセルを注入する.
 * @param image 画像
 * @param hot_pixel_count 注入数
 * @param rng 乱数生成器
 * @param peak_value 注入強度の基準値
 */
void InjectHotPixels(
    xt::xtensor<float, 2>* image,
    std::size_t hot_pixel_count,
    std::mt19937_64* rng,
    float peak_value);

}  // namespace transient_tracker::synthetic
