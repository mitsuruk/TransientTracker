/**
 * @file label_writer.cpp
 * @brief 真値ラベル書き出しを実装する.
 */

#include "TransientTracker/synthetic/label_writer.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace transient_tracker::synthetic {

namespace {

/**
 * @brief JSON文字列用に最小限のエスケープを行う.
 * @param text 入力文字列
 * @return エスケープ後文字列
 */
std::string EscapeJson(const std::string& text) {
    std::string escaped;
    escaped.reserve(text.size());
    for (char ch : text) {
        if (ch == '\\' || ch == '"') {
            escaped.push_back('\\');
        }
        escaped.push_back(ch);
    }
    return escaped;
}

/**
 * @brief 真値一覧を並べ替える.
 * @param truths 真値一覧
 */
void SortTruths(std::vector<FrameTruth>* truths) {
    std::sort(
        truths->begin(),
        truths->end(),
        [](const FrameTruth& lhs, const FrameTruth& rhs) {
            if (lhs.frame_index != rhs.frame_index) {
                return lhs.frame_index < rhs.frame_index;
            }
            return lhs.object_id < rhs.object_id;
        });
}

}  // namespace

bool WriteLabelsCsv(
    const std::filesystem::path& output_path,
    const std::vector<FrameTruth>& truths,
    std::string* message) {
    std::vector<FrameTruth> sorted_truths = truths;
    SortTruths(&sorted_truths);

    std::ofstream stream(output_path);
    if (!stream.is_open()) {
        if (message != nullptr) {
            *message = "labels.csv を開けませんでした: " + output_path.string();
        }
        return false;
    }

    stream << "frame_index,object_id,shift_dx,shift_dy,object_x,object_y,object_flux,object_visible\n";
    for (const FrameTruth& truth : sorted_truths) {
        stream << truth.frame_index << ','
               << truth.object_id << ','
               << truth.shift_dx << ','
               << truth.shift_dy << ','
               << truth.object_x << ','
               << truth.object_y << ','
               << truth.object_flux << ','
               << (truth.object_visible ? "true" : "false") << '\n';
    }

    stream.close();
    if (!stream) {
        if (message != nullptr) {
            *message = "labels.csv の書き出しに失敗しました: " + output_path.string();
        }
        return false;
    }
    return true;
}

bool WriteMetadataJson(
    const std::filesystem::path& output_path,
    const ScenarioConfig& config,
    const DatasetManifest& manifest,
    std::string* message) {
    std::ofstream stream(output_path);
    if (!stream.is_open()) {
        if (message != nullptr) {
            *message = "metadata.json を開けませんでした: " + output_path.string();
        }
        return false;
    }

    stream << "{\n";
    stream << "  \"dataset_format_version\": 1,\n";
    stream << "  \"scenario_name\": \"" << EscapeJson(config.scenario_name) << "\",\n";
    stream << "  \"seed\": " << config.seed << ",\n";
    stream << "  \"width\": " << config.image_size.width << ",\n";
    stream << "  \"height\": " << config.image_size.height << ",\n";
    stream << "  \"num_frames\": " << config.num_frames << ",\n";
    stream << "  \"num_stars\": " << config.num_stars << ",\n";
    stream << "  \"num_objects\": " << config.moving_objects.size() << ",\n";
    stream << "  \"background_level\": " << config.background_level << ",\n";
    stream << "  \"background_gradient_x\": " << config.background_gradient_x << ",\n";
    stream << "  \"background_gradient_y\": " << config.background_gradient_y << ",\n";
    stream << "  \"read_noise_sigma\": " << config.read_noise_sigma << ",\n";
    stream << "  \"use_poisson_noise\": " << (config.use_poisson_noise ? "true" : "false") << ",\n";
    stream << "  \"shift_sigma_x\": " << config.shift_sigma_x << ",\n";
    stream << "  \"shift_sigma_y\": " << config.shift_sigma_y << ",\n";
    stream << "  \"hot_pixel_count\": " << config.hot_pixel_count << ",\n";
    stream << "  \"hot_pixel_peak\": " << config.hot_pixel_peak << ",\n";
    stream << "  \"frame_bit_depth\": 16,\n";
    stream << "  \"frames_dir\": \"" << EscapeJson(manifest.frames_dir.filename().string()) << "\",\n";
    stream << "  \"labels_csv\": \"" << EscapeJson(manifest.labels_csv_path.filename().string()) << "\",\n";
    stream << "  \"preview_dir\": \"" << EscapeJson(manifest.preview_dir.filename().string()) << "\"\n";
    stream << "}\n";

    stream.close();
    if (!stream) {
        if (message != nullptr) {
            *message = "metadata.json の書き出しに失敗しました: " + output_path.string();
        }
        return false;
    }
    return true;
}

}  // namespace transient_tracker::synthetic
