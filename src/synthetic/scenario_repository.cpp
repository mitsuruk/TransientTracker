/**
 * @file scenario_repository.cpp
 * @brief 既定シナリオ定義を実装する.
 */

#include "TransientTracker/synthetic/scenario_repository.hpp"

namespace transient_tracker::synthetic {

namespace {

/**
 * @brief 移動天体仕様を作る.
 * @param object_id 天体ID
 * @param initial_x 初期X
 * @param initial_y 初期Y
 * @param velocity_x X速度
 * @param velocity_y Y速度
 * @param flux フラックス
 * @param sigma_x X像幅
 * @param sigma_y Y像幅
 * @return 移動天体仕様
 */
MovingObjectSpec MakeMovingObject(std::size_t object_id, double initial_x,
                                  double initial_y, double velocity_x,
                                  double velocity_y, double flux,
                                  double sigma_x, double sigma_y) {
  MovingObjectSpec moving_object;
  moving_object.object_id = object_id;
  moving_object.initial_x = initial_x;
  moving_object.initial_y = initial_y;
  moving_object.velocity_x = velocity_x;
  moving_object.velocity_y = velocity_y;
  moving_object.flux = flux;
  moving_object.sigma_x = sigma_x;
  moving_object.sigma_y = sigma_y;
  moving_object.angle_rad = 0.0;
  return moving_object;
}

/**
 * @brief 基本設定を作る.
 * @return 基本設定
 */
ScenarioConfig MakeBaseConfig() {
  ScenarioConfig config;
  config.scenario_name = "S-01";
  config.image_size = {512, 512};
  config.num_frames = 16;
  config.num_stars = 120;
  config.background_level = 100.0;
  config.background_gradient_x = 0.0;
  config.background_gradient_y = 0.0;
  config.read_noise_sigma = 2.0;
  config.use_poisson_noise = false;
  config.shift_sigma_x = 0.25;
  config.shift_sigma_y = 0.25;
  config.star_sigma = 1.2;
  config.moving_objects = {
      MakeMovingObject(0, 120.0, 250.0, 1.2, 0.5, 300.0, 1.2, 1.2),
  };
  config.hot_pixel_count = 0;
  config.hot_pixel_peak = 450.0;
  config.write_all_preview = false;
  config.seed = 12345;
  return config;
}

} // namespace

std::optional<ScenarioConfig> FindScenario(const std::string &scenario_name) {
  ScenarioConfig config = MakeBaseConfig();
  if (scenario_name == "S-01") {
    config.scenario_name = "S-01";
    config.moving_objects = {
        MakeMovingObject(0, 120.0, 250.0, 1.2, 0.5, 300.0, 1.2, 1.2),
        MakeMovingObject(1, 170.0, 90.0, -0.9, 0.7, 260.0, 1.4, 1.4),
        MakeMovingObject(2, 320.0, 160.0, 0.6, -1.1, 240.0, 1.3, 1.3),
    };
    return config;
  }
  if (scenario_name == "S-02") {
    config.scenario_name = "S-02";
    config.shift_sigma_x = 0.8;
    config.shift_sigma_y = 0.8;
    return config;
  }
  if (scenario_name == "S-03") {
    config.scenario_name = "S-03";
    config.read_noise_sigma = 6.0;
    config.use_poisson_noise = true;
    config.moving_objects.front().flux = 80.0;
    return config;
  }
  if (scenario_name == "S-04") {
    config.scenario_name = "S-04";
    config.moving_objects.front().flux = 220.0;
    config.moving_objects.front().sigma_x = 2.8;
    config.moving_objects.front().sigma_y = 1.6;
    return config;
  }
  if (scenario_name == "S-05") {
    config.scenario_name = "S-05";
    config.num_stars = 220;
    config.read_noise_sigma = 5.0;
    config.shift_sigma_x = 1.2;
    config.shift_sigma_y = 1.2;
    config.hot_pixel_count = 48;
    return config;
  }
  if (scenario_name == "S-06") {
    config.scenario_name = "S-06";
    config.moving_objects = {
        MakeMovingObject(0, 120.0, 250.0, 1.2, 0.5, 300.0, 1.2, 1.2),
        MakeMovingObject(1, 170.0, 90.0, -0.9, 0.7, 260.0, 1.4, 1.4),
    };
    return config;
  }
  return std::nullopt;
}

std::vector<std::string> ListScenarioNames() {
  return {"S-01", "S-02", "S-03", "S-04", "S-05", "S-06"};
}

} // namespace transient_tracker::synthetic
