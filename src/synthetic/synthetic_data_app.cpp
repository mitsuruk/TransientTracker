/**
 * @file synthetic_data_app.cpp
 * @brief 生成器アプリケーション本体を実装する.
 */

#include "TransientTracker/synthetic/synthetic_data_app.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "TransientTracker/synthetic/dataset_layout.hpp"
#include "TransientTracker/synthetic/frame_renderer.hpp"
#include "TransientTracker/synthetic/frame_shift_generator.hpp"
#include "TransientTracker/synthetic/label_writer.hpp"
#include "TransientTracker/synthetic/moving_object_generator.hpp"
#include "TransientTracker/synthetic/preview_writer.hpp"
#include "TransientTracker/synthetic/scenario_config.hpp"
#include "TransientTracker/synthetic/scenario_repository.hpp"
#include "TransientTracker/synthetic/star_field_generator.hpp"

namespace transient_tracker::synthetic {

namespace {

/**
 * @brief 文字列を符号なし整数へ変換する.
 * @param text 入力文字列
 * @param value 出力先
 * @return 成功なら真
 */
bool ParseUnsigned(const std::string& text, std::size_t* value) {
    std::istringstream stream(text);
    std::size_t parsed = 0;
    stream >> parsed;
    if (!stream || !stream.eof()) {
        return false;
    }
    *value = parsed;
    return true;
}

/**
 * @brief 文字列を符号なし64bit整数へ変換する.
 * @param text 入力文字列
 * @param value 出力先
 * @return 成功なら真
 */
bool ParseUnsigned64(const std::string& text, std::uint64_t* value) {
    std::istringstream stream(text);
    std::uint64_t parsed = 0;
    stream >> parsed;
    if (!stream || !stream.eof()) {
        return false;
    }
    *value = parsed;
    return true;
}

/**
 * @brief 文字列を浮動小数へ変換する.
 * @param text 入力文字列
 * @param value 出力先
 * @return 成功なら真
 */
bool ParseDouble(const std::string& text, double* value) {
    std::istringstream stream(text);
    double parsed = 0.0;
    stream >> parsed;
    if (!stream || !stream.eof()) {
        return false;
    }
    *value = parsed;
    return true;
}

/**
 * @brief 引数値を取り出す.
 * @param argc 引数数
 * @param argv 引数配列
 * @param index 現在位置
 * @param value 出力先
 * @param message エラー内容
 * @return 成功なら真
 */
bool ConsumeValue(int argc, char** argv, int* index, std::string* value, std::string* message) {
    if (*index + 1 >= argc) {
        if (message != nullptr) {
            *message = "引数値が不足しています.";
        }
        return false;
    }
    *value = argv[*index + 1];
    *index += 1;
    return true;
}

/**
 * @brief フレーム真値へシフト値を反映する.
 * @param base_truths 基本真値
 * @param shifts フレームずれ
 * @return 更新後真値
 */
std::vector<FrameTruth> MergeTruthsWithShifts(
    const std::vector<FrameTruth>& base_truths,
    const std::vector<FrameShift>& shifts) {
    std::vector<FrameTruth> merged_truths = base_truths;
    for (FrameTruth& truth : merged_truths) {
        if (truth.frame_index >= shifts.size()) {
            continue;
        }
        truth.shift_dx = shifts[truth.frame_index].dx;
        truth.shift_dy = shifts[truth.frame_index].dy;
    }
    return merged_truths;
}

/**
 * @brief 当該フレームの真値一覧を抽出する.
 * @param truths 全真値
 * @param frame_index フレーム番号
 * @return 当該フレーム真値
 */
std::vector<FrameTruth> CollectTruthsForFrame(
    const std::vector<FrameTruth>& truths,
    std::size_t frame_index) {
    std::vector<FrameTruth> frame_truths;
    for (const FrameTruth& truth : truths) {
        if (truth.frame_index == frame_index) {
            frame_truths.push_back(truth);
        }
    }
    return frame_truths;
}

/**
 * @brief プレビュー対象フレームかを判定する.
 * @param config シナリオ設定
 * @param frame_index フレーム番号
 * @return 出力するなら真
 */
bool ShouldWritePreview(const ScenarioConfig& config, std::size_t frame_index) {
    if (config.write_all_preview || config.num_frames == 1) {
        return true;
    }
    const std::size_t middle_index = config.num_frames / 2;
    return frame_index == 0 || frame_index == middle_index || frame_index + 1 == config.num_frames;
}

/**
 * @brief フレームファイル名を返す.
 * @param prefix 接頭辞
 * @param frame_index フレーム番号
 * @return ファイル名
 */
std::string BuildFrameFilename(const std::string& prefix, std::size_t frame_index) {
    std::ostringstream stream;
    stream << prefix;
    stream.width(4);
    stream.fill('0');
    stream << frame_index << ".png";
    return stream.str();
}

/**
 * @brief 未指定時のシードを返す.
 * @return シード値
 */
std::uint64_t MakeDefaultSeed() {
    const auto now = std::chrono::system_clock::now().time_since_epoch().count();
    return static_cast<std::uint64_t>(now);
}

}  // namespace

bool ParseCliOptions(int argc, char** argv, CliOptions* options, std::string* message) {
    if (options == nullptr) {
        if (message != nullptr) {
            *message = "options が null です.";
        }
        return false;
    }

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--help" || argument == "-h") {
            options->show_help = true;
            return true;
        }
        if (argument == "--write-all-preview") {
            options->write_all_preview = true;
            continue;
        }

        std::string value;
        if (!ConsumeValue(argc, argv, &index, &value, message)) {
            return false;
        }

        if (argument == "--scenario") {
            options->scenario_name = value;
        } else if (argument == "--output") {
            options->output_dir = value;
        } else if (argument == "--seed") {
            options->has_seed = ParseUnsigned64(value, &options->seed);
            if (!options->has_seed) {
                *message = "seed の解析に失敗しました.";
                return false;
            }
        } else if (argument == "--width") {
            options->has_width = ParseUnsigned(value, &options->width);
            if (!options->has_width) {
                *message = "width の解析に失敗しました.";
                return false;
            }
        } else if (argument == "--height") {
            options->has_height = ParseUnsigned(value, &options->height);
            if (!options->has_height) {
                *message = "height の解析に失敗しました.";
                return false;
            }
        } else if (argument == "--num-frames") {
            options->has_num_frames = ParseUnsigned(value, &options->num_frames);
            if (!options->has_num_frames) {
                *message = "num-frames の解析に失敗しました.";
                return false;
            }
        } else if (argument == "--num-stars") {
            options->has_num_stars = ParseUnsigned(value, &options->num_stars);
            if (!options->has_num_stars) {
                *message = "num-stars の解析に失敗しました.";
                return false;
            }
        } else if (argument == "--object-flux") {
            options->has_object_flux = ParseDouble(value, &options->object_flux);
            if (!options->has_object_flux) {
                *message = "object-flux の解析に失敗しました.";
                return false;
            }
        } else if (argument == "--object-velocity-x") {
            options->has_object_velocity_x = ParseDouble(value, &options->object_velocity_x);
            if (!options->has_object_velocity_x) {
                *message = "object-velocity-x の解析に失敗しました.";
                return false;
            }
        } else if (argument == "--object-velocity-y") {
            options->has_object_velocity_y = ParseDouble(value, &options->object_velocity_y);
            if (!options->has_object_velocity_y) {
                *message = "object-velocity-y の解析に失敗しました.";
                return false;
            }
        } else {
            *message = "未知の引数です: " + argument;
            return false;
        }
    }

    if (options->scenario_name.empty()) {
        *message = "--scenario は必須です.";
        return false;
    }
    if (options->output_dir.empty()) {
        *message = "--output は必須です.";
        return false;
    }
    return true;
}

std::string BuildHelpText() {
    return
        "Usage:\n"
        "  transient_tracker_generate --scenario S-01 --output ./output/S-01 [options]\n"
        "\n"
        "Options:\n"
        "  --scenario <name>\n"
        "  --output <dir>\n"
        "  --seed <value>\n"
        "  --width <value>\n"
        "  --height <value>\n"
        "  --num-frames <value>\n"
        "  --num-stars <value>\n"
        "  --object-flux <value>\n"
        "  --object-velocity-x <value>\n"
        "  --object-velocity-y <value>\n"
        "  --write-all-preview\n"
        "  --help\n";
}

int RunSyntheticDataApp(const CliOptions& options) {
    const std::optional<ScenarioConfig> maybe_base_config = FindScenario(options.scenario_name);
    if (!maybe_base_config.has_value()) {
        std::cerr << "未知のシナリオです: " << options.scenario_name << '\n';
        std::cerr << "利用可能: ";
        const std::vector<std::string> names = ListScenarioNames();
        for (std::size_t index = 0; index < names.size(); ++index) {
            std::cerr << names[index];
            if (index + 1 != names.size()) {
                std::cerr << ", ";
            }
        }
        std::cerr << '\n';
        return 1;
    }

    ScenarioConfig config = ApplyOverrides(*maybe_base_config, options);
    if (!options.has_seed) {
        config.seed = MakeDefaultSeed();
    }

    std::string message;
    if (!ValidateScenarioConfig(config, &message)) {
        std::cerr << message << '\n';
        return 1;
    }

    const DatasetManifest manifest = BuildDatasetManifest(options.output_dir);
    if (!CreateDatasetDirectories(manifest, &message)) {
        std::cerr << message << '\n';
        return 1;
    }

    std::mt19937_64 rng(config.seed);
    const std::vector<StarSource> stars = GenerateStars(config, &rng);
    const std::vector<FrameTruth> base_truths = GenerateObjectTruths(config);
    const std::vector<FrameShift> shifts = GenerateFrameShifts(config, &rng);
    const std::vector<FrameTruth> truths = MergeTruthsWithShifts(base_truths, shifts);

    for (std::size_t frame_index = 0; frame_index < config.num_frames; ++frame_index) {
        const std::vector<FrameTruth> frame_truths = CollectTruthsForFrame(truths, frame_index);
        const xt::xtensor<float, 2> image =
            RenderFrame(config, stars, shifts[frame_index], frame_truths, &rng);

        const std::filesystem::path frame_path =
            manifest.frames_dir / BuildFrameFilename("frame_", frame_index);
        if (!WriteFrameImage(frame_path, image, &message)) {
            std::cerr << message << '\n';
            return 1;
        }

        if (ShouldWritePreview(config, frame_index)) {
            const std::filesystem::path preview_path =
                manifest.preview_dir / BuildFrameFilename("preview_frame_", frame_index);
            if (!WritePreviewImage(preview_path, image, frame_truths, shifts[frame_index], &message)) {
                std::cerr << message << '\n';
                return 1;
            }
        }
    }

    if (!WriteLabelsCsv(manifest.labels_csv_path, truths, &message)) {
        std::cerr << message << '\n';
        return 1;
    }
    if (!WriteMetadataJson(manifest.metadata_json_path, config, manifest, &message)) {
        std::cerr << message << '\n';
        return 1;
    }

    std::cout << "scenario_name: " << config.scenario_name << '\n';
    std::cout << "dataset_root: " << manifest.dataset_root << '\n';
    std::cout << "seed: " << config.seed << '\n';
    std::cout << "num_frames: " << config.num_frames << '\n';
    std::cout << "num_stars: " << config.num_stars << '\n';
    std::cout << "num_objects: " << config.moving_objects.size() << '\n';
    for (const MovingObjectSpec& moving_object : config.moving_objects) {
        std::cout << "object[" << moving_object.object_id << "].initial: ("
                  << moving_object.initial_x << ", " << moving_object.initial_y << ")\n";
        std::cout << "object[" << moving_object.object_id << "].velocity: ("
                  << moving_object.velocity_x << ", " << moving_object.velocity_y << ")\n";
    }
    return 0;
}

}  // namespace transient_tracker::synthetic
