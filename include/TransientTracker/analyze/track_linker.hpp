/**
 * @file track_linker.hpp
 * @brief 軌跡連結関数を宣言する.
 */

#pragma once

#include <vector>

#include "TransientTracker/analyze/analysis_types.hpp"

namespace transient_tracker::analyze {

/**
 * @brief 候補点列を軌跡へ連結する.
 * @param detections_by_frame フレームごとの候補一覧
 * @param max_velocity_per_frame 隣接フレーム間の最大速度
 * @param min_track_length 最小軌跡長
 * @return 軌跡一覧
 */
std::vector<Track> LinkTracksGreedy(
    const std::vector<std::vector<Detection>>& detections_by_frame,
    double max_velocity_per_frame,
    std::size_t min_track_length);

}  // namespace transient_tracker::analyze
