/**
 * @file star_field_generator.cpp
 * @brief 静止星群生成を実装する.
 */

#include "TransientTracker/synthetic/star_field_generator.hpp"

#include <cmath>

namespace transient_tracker::synthetic {

std::vector<StarSource> GenerateStars(const ScenarioConfig& config, std::mt19937_64* rng) {
    std::uniform_real_distribution<double> x_distribution(
        0.0, static_cast<double>(config.image_size.width - 1));
    std::uniform_real_distribution<double> y_distribution(
        0.0, static_cast<double>(config.image_size.height - 1));
    std::uniform_real_distribution<double> flux_log_distribution(std::log(80.0), std::log(1200.0));

    std::vector<StarSource> stars;
    stars.reserve(config.num_stars);
    for (std::size_t index = 0; index < config.num_stars; ++index) {
        StarSource star;
        star.x = x_distribution(*rng);
        star.y = y_distribution(*rng);
        star.flux = std::exp(flux_log_distribution(*rng));
        star.sigma = config.star_sigma;
        stars.push_back(star);
    }
    return stars;
}

}  // namespace transient_tracker::synthetic
