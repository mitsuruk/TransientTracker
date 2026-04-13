/**
 * @file truth_evaluator.hpp
 * @brief 真値評価関数を宣言する.
 */

#pragma once

#include <vector>

#include "TransientTracker/analyze/analysis_types.hpp"

namespace transient_tracker::analyze {

/**
 * @brief 軌跡一覧を真値と照合して評価する.
 * @param truths 真値一覧
 * @param tracks 軌跡一覧
 * @param registration_enabled 位置合わせ有効フラグ
 * @param truth_match_distance 一致距離
 * @return 評価サマリ
 */
EvaluationSummary EvaluateTracksAgainstTruth(
    const std::vector<GroundTruthRecord> &truths, std::vector<Track> *tracks,
    bool registration_enabled, double truth_match_distance);

/**
 * @brief 真値天体ごとの代表一致軌跡を選択する.
 * @param tracks 軌跡一覧
 * @return 真値天体ごとの代表軌跡一覧
 */
std::vector<Track>
SelectRepresentativeMatchedTracks(const std::vector<Track> &tracks);

/**
 * @brief 主軌跡を選択する.
 * @param tracks 軌跡一覧
 * @return 主軌跡へのポインタ. 見つからなければ nullptr
 */
const Track *SelectPrimaryTrack(const std::vector<Track> &tracks);

} // namespace transient_tracker::analyze
