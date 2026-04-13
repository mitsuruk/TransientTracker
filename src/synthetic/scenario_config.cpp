/**
 * @file scenario_config.cpp
 * @brief シナリオ設定の上書きと検証を実装する.
 */

#include "TransientTracker/synthetic/scenario_config.hpp"

#include <algorithm>
#include <sstream>
#include <vector>

namespace transient_tracker::synthetic {

namespace {

/**
 * @brief エラーメッセージを設定する.
 * @param message 出力先
 * @param text 設定内容
 */
void SetMessage(std::string* message, const std::string& text) {
    if (message != nullptr) {
        *message = text;
    }
}

}  // namespace

ScenarioConfig ApplyOverrides(const ScenarioConfig& base_config, const CliOptions& cli_options) {
    ScenarioConfig config = base_config;
    if (cli_options.has_seed) {
        config.seed = cli_options.seed;
    }
    if (cli_options.has_width) {
        config.image_size.width = cli_options.width;
    }
    if (cli_options.has_height) {
        config.image_size.height = cli_options.height;
    }
    if (cli_options.has_num_frames) {
        config.num_frames = cli_options.num_frames;
    }
    if (cli_options.has_num_stars) {
        config.num_stars = cli_options.num_stars;
    }
    if (!config.moving_objects.empty()) {
        if (cli_options.has_object_flux) {
            config.moving_objects.front().flux = cli_options.object_flux;
        }
        if (cli_options.has_object_velocity_x) {
            config.moving_objects.front().velocity_x = cli_options.object_velocity_x;
        }
        if (cli_options.has_object_velocity_y) {
            config.moving_objects.front().velocity_y = cli_options.object_velocity_y;
        }
    }
    if (cli_options.write_all_preview) {
        config.write_all_preview = true;
    }
    return config;
}

bool ValidateScenarioConfig(const ScenarioConfig& config, std::string* message) {
    if (config.scenario_name.empty()) {
        SetMessage(message, "scenario_name は必須です.");
        return false;
    }
    if (config.image_size.width < 16 || config.image_size.height < 16) {
        SetMessage(message, "width と height は 16 以上が必要です.");
        return false;
    }
    if (config.num_frames < 1) {
        SetMessage(message, "num_frames は 1 以上が必要です.");
        return false;
    }
    if (config.num_stars < 1) {
        SetMessage(message, "num_stars は 1 以上が必要です.");
        return false;
    }
    if (config.background_level < 0.0) {
        SetMessage(message, "background_level は 0 以上が必要です.");
        return false;
    }
    if (config.read_noise_sigma < 0.0) {
        SetMessage(message, "read_noise_sigma は 0 以上が必要です.");
        return false;
    }
    if (config.shift_sigma_x < 0.0 || config.shift_sigma_y < 0.0) {
        SetMessage(message, "shift_sigma_x と shift_sigma_y は 0 以上が必要です.");
        return false;
    }
    if (config.star_sigma <= 0.0) {
        SetMessage(message, "star_sigma は正である必要があります.");
        return false;
    }
    if (config.moving_objects.empty()) {
        SetMessage(message, "moving_objects は 1 件以上が必要です.");
        return false;
    }
    std::vector<std::size_t> object_ids;
    object_ids.reserve(config.moving_objects.size());
    for (const MovingObjectSpec& moving_object : config.moving_objects) {
        if (moving_object.flux <= 0.0) {
            SetMessage(message, "moving_objects[*].flux は正である必要があります.");
            return false;
        }
        if (moving_object.sigma_x <= 0.0 || moving_object.sigma_y <= 0.0) {
            SetMessage(message, "moving_objects[*] の sigma は正である必要があります.");
            return false;
        }
        if (std::find(object_ids.begin(), object_ids.end(), moving_object.object_id) != object_ids.end()) {
            SetMessage(message, "moving_objects の object_id は一意である必要があります.");
            return false;
        }
        object_ids.push_back(moving_object.object_id);
    }
    if (config.hot_pixel_count > 0 && config.hot_pixel_peak <= 0.0) {
        SetMessage(message, "hot_pixel_peak は hot_pixel_count > 0 のとき正である必要があります.");
        return false;
    }

    for (std::size_t frame_index : config.drop_frame_indices) {
        if (frame_index >= config.num_frames) {
            std::ostringstream stream;
            stream << "drop_frame_indices に範囲外の値があります: " << frame_index;
            SetMessage(message, stream.str());
            return false;
        }
    }

    return true;
}

}  // namespace transient_tracker::synthetic
