/**
 * @file frame_shift_generator.cpp
 * @brief フレームずれ生成を実装する.
 */

#include "TransientTracker/synthetic/frame_shift_generator.hpp"

namespace transient_tracker::synthetic {

std::vector<FrameShift> GenerateFrameShifts(const ScenarioConfig& config, std::mt19937_64* rng) {
    std::normal_distribution<double> dx_distribution(0.0, config.shift_sigma_x);
    std::normal_distribution<double> dy_distribution(0.0, config.shift_sigma_y);

    std::vector<FrameShift> shifts;
    shifts.reserve(config.num_frames);
    for (std::size_t frame_index = 0; frame_index < config.num_frames; ++frame_index) {
        FrameShift shift;
        shift.frame_index = frame_index;
        if (frame_index == 0) {
            shift.dx = 0.0;
            shift.dy = 0.0;
        } else {
            shift.dx = dx_distribution(*rng);
            shift.dy = dy_distribution(*rng);
        }
        shifts.push_back(shift);
    }
    return shifts;
}

}  // namespace transient_tracker::synthetic
