/**
 * @file moving_object_generator.cpp
 * @brief 移動天体真値生成を実装する.
 */

#include "TransientTracker/synthetic/moving_object_generator.hpp"

#include <algorithm>

namespace transient_tracker::synthetic {

namespace {

/**
 * @brief 欠損フレームかを判定する.
 * @param config シナリオ設定
 * @param frame_index フレーム番号
 * @return 欠損なら真
 */
bool IsDroppedFrame(const ScenarioConfig& config, std::size_t frame_index) {
    return std::find(config.drop_frame_indices.begin(), config.drop_frame_indices.end(), frame_index) !=
           config.drop_frame_indices.end();
}

}  // namespace

std::vector<FrameTruth> GenerateObjectTruths(const ScenarioConfig& config) {
    std::vector<FrameTruth> truths;
    truths.reserve(config.num_frames * config.moving_objects.size());
    for (std::size_t frame_index = 0; frame_index < config.num_frames; ++frame_index) {
        for (const MovingObjectSpec& moving_object : config.moving_objects) {
            FrameTruth truth;
            truth.frame_index = frame_index;
            truth.object_id = moving_object.object_id;
            truth.shift_dx = 0.0;
            truth.shift_dy = 0.0;
            truth.object_x = moving_object.initial_x +
                             static_cast<double>(frame_index) * moving_object.velocity_x;
            truth.object_y = moving_object.initial_y +
                             static_cast<double>(frame_index) * moving_object.velocity_y;
            truth.object_flux = moving_object.flux;
            truth.sigma_x = moving_object.sigma_x;
            truth.sigma_y = moving_object.sigma_y;

            const bool in_x = truth.object_x >= 0.0 &&
                              truth.object_x < static_cast<double>(config.image_size.width);
            const bool in_y = truth.object_y >= 0.0 &&
                              truth.object_y < static_cast<double>(config.image_size.height);
            truth.object_visible = in_x && in_y && !IsDroppedFrame(config, frame_index);
            truths.push_back(truth);
        }
    }
    return truths;
}

}  // namespace transient_tracker::synthetic
