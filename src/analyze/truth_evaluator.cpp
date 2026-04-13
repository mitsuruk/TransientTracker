/**
 * @file truth_evaluator.cpp
 * @brief 真値評価処理を実装する.
 */

#include "TransientTracker/analyze/truth_evaluator.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>
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
 * @brief 真値座標を返す.
 * @param truth 真値
 * @param registration_enabled 位置合わせ有効フラグ
 * @param truth_x 出力X座標
 * @param truth_y 出力Y座標
 */
void GetTruthPosition(const GroundTruthRecord &truth, bool registration_enabled,
                      double *truth_x, double *truth_y) {
  *truth_x =
      registration_enabled ? truth.object_x : truth.object_x + truth.shift_dx;
  *truth_y =
      registration_enabled ? truth.object_y : truth.object_y + truth.shift_dy;
}

/**
 * @brief 代表軌跡の優先度比較を返す.
 * @param lhs 左辺
 * @param rhs 右辺
 * @return lhs を優先するなら真
 */
bool IsBetterRepresentativeTrack(const Track &lhs, const Track &rhs) {
  if (lhs.matched_frame_count != rhs.matched_frame_count) {
    return lhs.matched_frame_count > rhs.matched_frame_count;
  }
  if (lhs.detections.size() != rhs.detections.size()) {
    return lhs.detections.size() > rhs.detections.size();
  }
  if (lhs.score != rhs.score) {
    return lhs.score > rhs.score;
  }
  return lhs.track_id < rhs.track_id;
}

} // namespace

EvaluationSummary EvaluateTracksAgainstTruth(
    const std::vector<GroundTruthRecord> &truths, std::vector<Track> *tracks,
    bool registration_enabled, double truth_match_distance) {
  EvaluationSummary summary;
  if (tracks == nullptr) {
    return summary;
  }

  std::set<int> visible_object_ids;
  for (const GroundTruthRecord &truth : truths) {
    if (truth.object_visible) {
      ++summary.num_visible_truth_frames;
      visible_object_ids.insert(static_cast<int>(truth.object_id));
    }
  }
  summary.num_visible_objects = visible_object_ids.size();

  for (Track &track : *tracks) {
    std::map<int, std::size_t> object_match_counts;

    for (const Detection &detection : track.detections) {
      double best_distance = std::numeric_limits<double>::max();
      int best_object_id = -1;
      for (const GroundTruthRecord &truth : truths) {
        if (!truth.object_visible ||
            truth.frame_index != detection.frame_index) {
          continue;
        }
        double truth_x = 0.0;
        double truth_y = 0.0;
        GetTruthPosition(truth, registration_enabled, &truth_x, &truth_y);
        const double distance =
            ComputeDistance(detection.x, detection.y, truth_x, truth_y);
        if (distance < best_distance) {
          best_distance = distance;
          best_object_id = static_cast<int>(truth.object_id);
        }
      }

      if (best_object_id >= 0 && best_distance <= truth_match_distance) {
        ++object_match_counts[best_object_id];
      }
    }

    int matched_object_id = -1;
    std::size_t matched_frame_count = 0;
    for (const auto &[object_id, count] : object_match_counts) {
      if (count > matched_frame_count) {
        matched_object_id = object_id;
        matched_frame_count = count;
      }
    }

    track.matched_frame_count = matched_frame_count;
    if (matched_frame_count * 2 >= track.detections.size() &&
        matched_object_id >= 0) {
      track.matched_object_id = matched_object_id;
      ++summary.num_matched_tracks;
    } else {
      track.matched_object_id = -1;
    }
  }

  const std::vector<Track> representative_tracks =
      SelectRepresentativeMatchedTracks(*tracks);
  summary.num_matched_objects = representative_tracks.size();
  summary.matched_objects.reserve(representative_tracks.size());
  for (const Track &track : representative_tracks) {
    MatchedObjectSummary matched_object;
    matched_object.object_id = track.matched_object_id;
    matched_object.track_id = static_cast<int>(track.track_id);
    matched_object.matched_frame_count = track.matched_frame_count;
    matched_object.track_length = track.detections.size();
    summary.matched_objects.push_back(matched_object);
  }

  const Track *primary_track = SelectPrimaryTrack(*tracks);
  if (primary_track != nullptr) {
    summary.primary_track_id = static_cast<int>(primary_track->track_id);
    summary.primary_track_matched_object_id = primary_track->matched_object_id;
    summary.primary_track_match_count = primary_track->matched_frame_count;
    summary.primary_track_length = primary_track->detections.size();
  }
  return summary;
}

std::vector<Track>
SelectRepresentativeMatchedTracks(const std::vector<Track> &tracks) {
  std::map<int, Track> best_tracks_by_object_id;
  for (const Track &track : tracks) {
    if (track.matched_object_id < 0) {
      continue;
    }

    const auto [iterator, inserted] =
        best_tracks_by_object_id.emplace(track.matched_object_id, track);
    if (!inserted && IsBetterRepresentativeTrack(track, iterator->second)) {
      iterator->second = track;
    }
  }

  std::vector<Track> representative_tracks;
  representative_tracks.reserve(best_tracks_by_object_id.size());
  for (const auto &[object_id, track] : best_tracks_by_object_id) {
    static_cast<void>(object_id);
    representative_tracks.push_back(track);
  }
  return representative_tracks;
}

const Track *SelectPrimaryTrack(const std::vector<Track> &tracks) {
  if (tracks.empty()) {
    return nullptr;
  }
  const Track *best_track = &tracks.front();
  for (const Track &track : tracks) {
    if (track.matched_object_id >= 0 && best_track->matched_object_id < 0) {
      best_track = &track;
      continue;
    }
    if (track.matched_object_id < 0 && best_track->matched_object_id >= 0) {
      continue;
    }
    if (track.detections.size() > best_track->detections.size()) {
      best_track = &track;
      continue;
    }
    if (track.detections.size() == best_track->detections.size() &&
        track.score > best_track->score) {
      best_track = &track;
    }
  }
  return best_track;
}

} // namespace transient_tracker::analyze
