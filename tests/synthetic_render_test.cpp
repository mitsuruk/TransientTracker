/**
 * @file synthetic_render_test.cpp
 * @brief レンダリング挙動の回帰テスト.
 */

#include <cmath>
#include <cstddef>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include <xtensor/containers/xtensor.hpp>

#include "TransientTracker/synthetic/frame_renderer.hpp"
#include "TransientTracker/synthetic/scenario_config.hpp"

namespace {

/**
 * @brief 条件を検証する.
 * @param condition 条件
 * @param message エラー文言
 * @return 失敗時の終了コード
 */
int Expect(bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << message << '\n';
    return 1;
  }
  return 0;
}

/**
 * @brief テスト用の基本設定を返す.
 * @return 基本設定
 */
transient_tracker::synthetic::ScenarioConfig MakeBaseConfig() {
  using transient_tracker::synthetic::MovingObjectSpec;
  using transient_tracker::synthetic::ScenarioConfig;

  ScenarioConfig config;
  config.scenario_name = "render-test";
  config.image_size = {129, 129};
  config.num_frames = 1;
  config.num_stars = 1;
  config.background_level = 0.0;
  config.background_gradient_x = 0.0;
  config.background_gradient_y = 0.0;
  config.read_noise_sigma = 0.0;
  config.use_poisson_noise = false;
  config.shift_sigma_x = 0.0;
  config.shift_sigma_y = 0.0;
  config.star_sigma = 1.2;
  MovingObjectSpec moving_object;
  moving_object.object_id = 0;
  moving_object.initial_x = 0.0;
  moving_object.initial_y = 0.0;
  moving_object.velocity_x = 0.0;
  moving_object.velocity_y = 0.0;
  moving_object.flux = 1.0;
  moving_object.sigma_x = 1.2;
  moving_object.sigma_y = 1.2;
  config.moving_objects = {moving_object};
  config.hot_pixel_count = 0;
  config.hot_pixel_peak = 0.0;
  config.write_all_preview = false;
  config.seed = 1;
  return config;
}

/**
 * @brief 画像の総和を計算する.
 * @param image 入力画像
 * @return 総和
 */
double SumImage(const xt::xtensor<float, 2> &image) {
  double sum = 0.0;
  for (std::size_t y = 0; y < image.shape()[0]; ++y) {
    for (std::size_t x = 0; x < image.shape()[1]; ++x) {
      sum += static_cast<double>(image(y, x));
    }
  }
  return sum;
}

/**
 * @brief 非整数画素の有無を判定する.
 * @param image 入力画像
 * @return 非整数画素があれば真
 */
bool HasNonIntegerPixel(const xt::xtensor<float, 2> &image) {
  for (std::size_t y = 0; y < image.shape()[0]; ++y) {
    for (std::size_t x = 0; x < image.shape()[1]; ++x) {
      const double value = static_cast<double>(image(y, x));
      if (std::fabs(value - std::round(value)) > 1.0e-6) {
        return true;
      }
    }
  }
  return false;
}

/**
 * @brief 画像の完全一致を判定する.
 * @param lhs 左辺
 * @param rhs 右辺
 * @return 一致なら真
 */
bool ImagesEqual(const xt::xtensor<float, 2> &lhs,
                 const xt::xtensor<float, 2> &rhs) {
  if (lhs.shape() != rhs.shape()) {
    return false;
  }
  for (std::size_t y = 0; y < lhs.shape()[0]; ++y) {
    for (std::size_t x = 0; x < lhs.shape()[1]; ++x) {
      if (lhs(y, x) != rhs(y, x)) {
        return false;
      }
    }
  }
  return true;
}

/**
 * @brief 単一星像を描画した画像総和を返す.
 * @param sigma 星像幅
 * @param flux 総フラックス
 * @return 画像総和
 */
double RenderSingleStarSum(double sigma, double flux) {
  using transient_tracker::synthetic::FrameShift;
  using transient_tracker::synthetic::RenderFrame;
  using transient_tracker::synthetic::StarSource;

  transient_tracker::synthetic::ScenarioConfig config = MakeBaseConfig();
  std::vector<StarSource> stars(1);
  stars[0].x = 64.0;
  stars[0].y = 64.0;
  stars[0].flux = flux;
  stars[0].sigma = sigma;

  FrameShift shift;
  std::mt19937_64 rng(7);
  const xt::xtensor<float, 2> image =
      RenderFrame(config, stars, shift, {}, &rng);
  return SumImage(image);
}

} // namespace

/**
 * @brief テストを実行する.
 * @return 終了コード
 */
int main() {
  using transient_tracker::synthetic::FrameShift;
  using transient_tracker::synthetic::RenderFrame;
  using transient_tracker::synthetic::ScenarioConfig;

  const double narrow_sum = RenderSingleStarSum(1.2, 500.0);
  const double wide_sum = RenderSingleStarSum(2.4, 500.0);
  const double ratio = wide_sum / narrow_sum;
  if (int result = Expect(std::fabs(ratio - 1.0) < 0.05,
                          "総フラックスが sigma に依存しています.");
      result != 0) {
    return result;
  }

  ScenarioConfig noise_config = MakeBaseConfig();
  noise_config.image_size = {32, 32};
  noise_config.num_stars = 0;
  noise_config.use_poisson_noise = true;
  noise_config.read_noise_sigma = 1.5;
  std::mt19937_64 noise_rng(11);
  const xt::xtensor<float, 2> noisy_image =
      RenderFrame(noise_config, {}, FrameShift{}, {}, &noise_rng);
  if (int result = Expect(HasNonIntegerPixel(noisy_image),
                          "Poisson 後の Gaussian ノイズが反映されていません.");
      result != 0) {
    return result;
  }

  ScenarioConfig hot_pixel_config = MakeBaseConfig();
  hot_pixel_config.image_size = {32, 32};
  hot_pixel_config.num_stars = 0;
  hot_pixel_config.hot_pixel_count = 4;
  hot_pixel_config.hot_pixel_peak = 25.0;
  hot_pixel_config.moving_objects.front().flux = 10.0;

  std::mt19937_64 hot_pixel_rng_a(29);
  const xt::xtensor<float, 2> hot_pixel_image_a =
      RenderFrame(hot_pixel_config, {}, FrameShift{}, {}, &hot_pixel_rng_a);

  hot_pixel_config.moving_objects.front().flux = 1000.0;
  std::mt19937_64 hot_pixel_rng_b(29);
  const xt::xtensor<float, 2> hot_pixel_image_b =
      RenderFrame(hot_pixel_config, {}, FrameShift{}, {}, &hot_pixel_rng_b);

  if (int result = Expect(
          ImagesEqual(hot_pixel_image_a, hot_pixel_image_b),
          "ホットピクセル強度が moving_objects.front().flux に依存しています.");
      result != 0) {
    return result;
  }

  return 0;
}
