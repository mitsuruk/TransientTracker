/**
 * @file analysis_config.cpp
 * @brief 解析器の設定とCLI解析を実装する.
 */

#include "TransientTracker/analyze/analysis_config.hpp"

#include <sstream>
#include <string>

namespace transient_tracker::analyze {

namespace {

/**
 * @brief メッセージを設定する.
 * @param message 出力先
 * @param text 設定内容
 */
void SetMessage(std::string* message, const std::string& text) {
    if (message != nullptr) {
        *message = text;
    }
}

/**
 * @brief 次の引数値を取り出す.
 * @param argc 引数数
 * @param argv 引数配列
 * @param index 現在位置
 * @param value 出力先
 * @param message エラー内容
 * @return 成功なら真
 */
bool ConsumeValue(int argc, char** argv, int* index, std::string* value, std::string* message) {
    if (*index + 1 >= argc) {
        SetMessage(message, "引数値が不足しています.");
        return false;
    }
    *value = argv[*index + 1];
    *index += 1;
    return true;
}

/**
 * @brief 文字列をサイズ値へ変換する.
 * @param text 入力文字列
 * @param value 出力先
 * @return 成功なら真
 */
bool ParseSizeValue(const std::string& text, std::size_t* value) {
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
 * @brief 文字列を浮動小数へ変換する.
 * @param text 入力文字列
 * @param value 出力先
 * @return 成功なら真
 */
bool ParseDoubleValue(const std::string& text, double* value) {
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
 * @brief 文字列を整数へ変換する.
 * @param text 入力文字列
 * @param value 出力先
 * @return 成功なら真
 */
bool ParseIntValue(const std::string& text, int* value) {
    std::istringstream stream(text);
    int parsed = 0;
    stream >> parsed;
    if (!stream || !stream.eof()) {
        return false;
    }
    *value = parsed;
    return true;
}

}  // namespace

bool ParseAnalysisCliOptions(int argc, char** argv, AnalysisConfig* config, std::string* message) {
    if (config == nullptr) {
        SetMessage(message, "config が null です.");
        return false;
    }

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--help" || argument == "-h") {
            config->show_help = true;
            return true;
        }
        if (argument == "--write-debug-images") {
            config->write_debug_images = true;
            continue;
        }
        if (argument == "--disable-registration") {
            config->disable_registration = true;
            continue;
        }

        std::string value;
        if (!ConsumeValue(argc, argv, &index, &value, message)) {
            return false;
        }

        if (argument == "--dataset") {
            config->dataset_root = value;
        } else if (argument == "--output") {
            config->output_root = value;
        } else if (argument == "--tolerance-radius") {
            if (!ParseIntValue(value, &config->tolerance_radius)) {
                SetMessage(message, "tolerance-radius の解析に失敗しました.");
                return false;
            }
        } else if (argument == "--threshold-scale") {
            if (!ParseDoubleValue(value, &config->threshold_scale)) {
                SetMessage(message, "threshold-scale の解析に失敗しました.");
                return false;
            }
        } else if (argument == "--area-min") {
            if (!ParseSizeValue(value, &config->area_min)) {
                SetMessage(message, "area-min の解析に失敗しました.");
                return false;
            }
        } else if (argument == "--area-max") {
            if (!ParseSizeValue(value, &config->area_max)) {
                SetMessage(message, "area-max の解析に失敗しました.");
                return false;
            }
        } else if (argument == "--max-velocity") {
            if (!ParseDoubleValue(value, &config->max_velocity_per_frame)) {
                SetMessage(message, "max-velocity の解析に失敗しました.");
                return false;
            }
        } else if (argument == "--min-track-length") {
            if (!ParseSizeValue(value, &config->min_track_length)) {
                SetMessage(message, "min-track-length の解析に失敗しました.");
                return false;
            }
        } else if (argument == "--truth-match-distance") {
            if (!ParseDoubleValue(value, &config->truth_match_distance)) {
                SetMessage(message, "truth-match-distance の解析に失敗しました.");
                return false;
            }
        } else {
            SetMessage(message, "未知の引数です: " + argument);
            return false;
        }
    }

    return ValidateAnalysisConfig(*config, message);
}

bool ValidateAnalysisConfig(const AnalysisConfig& config, std::string* message) {
    if (config.show_help) {
        return true;
    }
    if (config.dataset_root.empty()) {
        SetMessage(message, "--dataset は必須です.");
        return false;
    }
    if (config.output_root.empty()) {
        SetMessage(message, "--output は必須です.");
        return false;
    }
    if (config.threshold_scale <= 0.0) {
        SetMessage(message, "threshold-scale は正である必要があります.");
        return false;
    }
    if (config.tolerance_radius < 0) {
        SetMessage(message, "tolerance-radius は 0 以上が必要です.");
        return false;
    }
    if (config.area_min == 0) {
        SetMessage(message, "area-min は 1 以上が必要です.");
        return false;
    }
    if (config.area_min > config.area_max) {
        SetMessage(message, "area-min は area-max 以下である必要があります.");
        return false;
    }
    if (config.max_velocity_per_frame <= 0.0) {
        SetMessage(message, "max-velocity は正である必要があります.");
        return false;
    }
    if (config.min_track_length < 2) {
        SetMessage(message, "min-track-length は 2 以上が必要です.");
        return false;
    }
    if (config.truth_match_distance <= 0.0) {
        SetMessage(message, "truth-match-distance は正である必要があります.");
        return false;
    }
    return true;
}

std::string BuildAnalysisHelpText() {
    return
        "Usage:\n"
        "  transient_tracker_analyze --dataset ./output/S-01 --output ./analysis/S-01 [options]\n"
        "\n"
        "Options:\n"
        "  --dataset <dir>\n"
        "  --output <dir>\n"
        "  --tolerance-radius <value>\n"
        "  --threshold-scale <value>\n"
        "  --area-min <value>\n"
        "  --area-max <value>\n"
        "  --max-velocity <value>\n"
        "  --min-track-length <value>\n"
        "  --truth-match-distance <value>\n"
        "  --write-debug-images\n"
        "  --disable-registration\n"
        "  --help\n";
}

}  // namespace transient_tracker::analyze
