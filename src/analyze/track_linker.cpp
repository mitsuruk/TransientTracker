/**
 * @file track_linker.cpp
 * @brief 軌跡連結処理を実装する.
 */

#include "TransientTracker/analyze/track_linker.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <vector>

namespace transient_tracker::analyze {

namespace {

/**
 * @brief 点間距離を返す.
 * @param x0 X0
 * @param y0 Y0
 * @param x1 X1
 * @param y1 Y1
 * @return ユークリッド距離
 */
double ComputeDistance(double x0, double y0, double x1, double y1) {
    const double dx = x1 - x0;
    const double dy = y1 - y0;
    return std::sqrt(dx * dx + dy * dy);
}

/**
 * @brief 軌跡の次フレーム予測位置を返す.
 * @param track 入力軌跡
 * @param predicted_x 出力X
 * @param predicted_y 出力Y
 */
void PredictNextPosition(const Track& track, double* predicted_x, double* predicted_y) {
    const std::size_t length = track.detections.size();
    const Detection& last = track.detections.back();
    if (length < 2) {
        *predicted_x = last.x;
        *predicted_y = last.y;
        return;
    }

    const Detection& previous = track.detections[length - 2];
    *predicted_x = last.x + (last.x - previous.x);
    *predicted_y = last.y + (last.y - previous.y);
}

/**
 * @brief 軌跡統計量を計算する.
 * @param track 入出力軌跡
 */
void FinalizeTrack(Track* track) {
    if (track == nullptr || track->detections.empty()) {
        return;
    }

    if (track->detections.size() == 1) {
        track->mean_vx = 0.0;
        track->mean_vy = 0.0;
        track->fit_error = 0.0;
        track->score = track->detections.front().score;
        return;
    }

    double sum_scores = 0.0;
    for (const Detection& detection : track->detections) {
        sum_scores += detection.score;
    }

    const Detection& first = track->detections.front();
    const Detection& last = track->detections.back();
    const double frame_span = static_cast<double>(last.frame_index - first.frame_index);
    track->mean_vx = (last.x - first.x) / std::max(1.0, frame_span);
    track->mean_vy = (last.y - first.y) / std::max(1.0, frame_span);

    double fit_error = 0.0;
    for (const Detection& detection : track->detections) {
        const double dt = static_cast<double>(detection.frame_index - first.frame_index);
        const double expected_x = first.x + dt * track->mean_vx;
        const double expected_y = first.y + dt * track->mean_vy;
        fit_error += ComputeDistance(detection.x, detection.y, expected_x, expected_y);
    }
    fit_error /= static_cast<double>(track->detections.size());
    track->fit_error = fit_error;
    track->score = static_cast<double>(track->detections.size()) * 1000.0 + sum_scores - 100.0 * fit_error;
}

}  // namespace

std::vector<Track> LinkTracksGreedy(
    const std::vector<std::vector<Detection>>& detections_by_frame,
    double max_velocity_per_frame,
    std::size_t min_track_length) {
    std::vector<Track> finished_tracks;
    std::vector<Track> active_tracks;

    for (std::size_t frame_index = 0; frame_index < detections_by_frame.size(); ++frame_index) {
        const std::vector<Detection>& detections = detections_by_frame[frame_index];
        std::vector<bool> used(detections.size(), false);
        std::vector<Track> next_active_tracks;

        std::sort(
            active_tracks.begin(),
            active_tracks.end(),
            [](const Track& lhs, const Track& rhs) {
                if (lhs.detections.size() != rhs.detections.size()) {
                    return lhs.detections.size() > rhs.detections.size();
                }
                return lhs.score > rhs.score;
            });

        for (Track& track : active_tracks) {
            double predicted_x = 0.0;
            double predicted_y = 0.0;
            PredictNextPosition(track, &predicted_x, &predicted_y);

            double best_distance = std::numeric_limits<double>::max();
            int best_index = -1;
            for (std::size_t detection_index = 0; detection_index < detections.size(); ++detection_index) {
                if (used[detection_index]) {
                    continue;
                }
                const double distance = ComputeDistance(
                    predicted_x,
                    predicted_y,
                    detections[detection_index].x,
                    detections[detection_index].y);
                if (distance <= max_velocity_per_frame && distance < best_distance) {
                    best_distance = distance;
                    best_index = static_cast<int>(detection_index);
                }
            }

            if (best_index >= 0) {
                used[static_cast<std::size_t>(best_index)] = true;
                track.detections.push_back(detections[static_cast<std::size_t>(best_index)]);
                FinalizeTrack(&track);
                next_active_tracks.push_back(track);
            } else {
                FinalizeTrack(&track);
                finished_tracks.push_back(track);
            }
        }

        for (std::size_t detection_index = 0; detection_index < detections.size(); ++detection_index) {
            if (used[detection_index]) {
                continue;
            }
            Track new_track;
            new_track.detections.push_back(detections[detection_index]);
            FinalizeTrack(&new_track);
            next_active_tracks.push_back(new_track);
        }

        active_tracks = std::move(next_active_tracks);
    }

    for (Track& track : active_tracks) {
        FinalizeTrack(&track);
        finished_tracks.push_back(track);
    }

    std::vector<Track> filtered_tracks;
    for (Track& track : finished_tracks) {
        if (track.detections.size() >= min_track_length) {
            filtered_tracks.push_back(track);
        }
    }

    std::sort(
        filtered_tracks.begin(),
        filtered_tracks.end(),
        [](const Track& lhs, const Track& rhs) {
            if (lhs.detections.size() != rhs.detections.size()) {
                return lhs.detections.size() > rhs.detections.size();
            }
            return lhs.score > rhs.score;
        });

    for (std::size_t track_index = 0; track_index < filtered_tracks.size(); ++track_index) {
        filtered_tracks[track_index].track_id = track_index;
    }
    return filtered_tracks;
}

}  // namespace transient_tracker::analyze
