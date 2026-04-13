/**
 * @file frame_normalizer.cpp
 * @brief フレーム正規化処理を実装する.
 */

#include "TransientTracker/analyze/frame_normalizer.hpp"

#include <vector>

#include "TransientTracker/analyze/math_utils.hpp"

namespace transient_tracker::analyze {

std::vector<xt::xtensor<float, 2>> NormalizeFramesByMedian(
    const std::vector<xt::xtensor<float, 2>>& frames) {
    std::vector<xt::xtensor<float, 2>> normalized_frames;
    normalized_frames.reserve(frames.size());
    for (const xt::xtensor<float, 2>& frame : frames) {
        std::vector<float> values;
        values.reserve(frame.shape()[0] * frame.shape()[1]);
        for (std::size_t y = 0; y < frame.shape()[0]; ++y) {
            for (std::size_t x = 0; x < frame.shape()[1]; ++x) {
                values.push_back(frame(y, x));
            }
        }

        const float median = ComputeMedian(&values);
        xt::xtensor<float, 2> normalized = frame;
        for (std::size_t y = 0; y < normalized.shape()[0]; ++y) {
            for (std::size_t x = 0; x < normalized.shape()[1]; ++x) {
                normalized(y, x) -= median;
            }
        }
        normalized_frames.push_back(std::move(normalized));
    }
    return normalized_frames;
}

}  // namespace transient_tracker::analyze
