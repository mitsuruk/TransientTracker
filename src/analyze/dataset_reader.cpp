/**
 * @file dataset_reader.cpp
 * @brief データセット契約の読込を実装する.
 */

#include "TransientTracker/analyze/dataset_reader.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace transient_tracker::analyze {

namespace {

/**
 * @brief メッセージを設定する.
 * @param message 出力先
 * @param text 設定内容
 */
void SetMessage(std::string *message, const std::string &text) {
  if (message != nullptr) {
    *message = text;
  }
}

/**
 * @brief 文字列前後の空白を除去する.
 * @param text 入力文字列
 * @return 除去後文字列
 */
std::string Trim(const std::string &text) {
  std::size_t begin = 0;
  while (begin < text.size() &&
         std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
    ++begin;
  }

  std::size_t end = text.size();
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
    --end;
  }
  return text.substr(begin, end - begin);
}

/**
 * @brief テキストファイルを読み込む.
 * @param path 入力パス
 * @param text 出力先
 * @param message エラー内容
 * @return 成功なら真
 */
bool ReadTextFile(const std::filesystem::path &path, std::string *text,
                  std::string *message) {
  std::ifstream stream(path);
  if (!stream.is_open()) {
    SetMessage(message, "ファイルを開けませんでした: " + path.string());
    return false;
  }

  std::ostringstream buffer;
  buffer << stream.rdbuf();
  stream.close();
  if (!stream) {
    SetMessage(message, "ファイル読込に失敗しました: " + path.string());
    return false;
  }
  *text = buffer.str();
  return true;
}

/**
 * @brief JSON内の値開始位置を探す.
 * @param text JSON文字列
 * @param key キー
 * @param value_start 値開始位置
 * @return 見つかれば真
 */
bool FindJsonValueStart(const std::string &text, const std::string &key,
                        std::size_t *value_start) {
  const std::string pattern = "\"" + key + "\"";
  const std::size_t key_pos = text.find(pattern);
  if (key_pos == std::string::npos) {
    return false;
  }
  const std::size_t colon_pos = text.find(':', key_pos + pattern.size());
  if (colon_pos == std::string::npos) {
    return false;
  }

  std::size_t pos = colon_pos + 1;
  while (pos < text.size() &&
         std::isspace(static_cast<unsigned char>(text[pos])) != 0) {
    ++pos;
  }
  if (pos >= text.size()) {
    return false;
  }

  *value_start = pos;
  return true;
}

/**
 * @brief JSONの値トークンを取り出す.
 * @param text JSON文字列
 * @param key キー
 * @param token 出力先
 * @return 成功なら真
 */
bool ExtractJsonToken(const std::string &text, const std::string &key,
                      std::string *token) {
  std::size_t value_start = 0;
  if (!FindJsonValueStart(text, key, &value_start)) {
    return false;
  }

  if (text[value_start] == '"') {
    const std::size_t end = text.find('"', value_start + 1);
    if (end == std::string::npos) {
      return false;
    }
    *token = text.substr(value_start + 1, end - value_start - 1);
    return true;
  }

  std::size_t end = value_start;
  while (end < text.size() && text[end] != ',' && text[end] != '\n' &&
         text[end] != '\r' && text[end] != '}') {
    ++end;
  }
  *token = Trim(text.substr(value_start, end - value_start));
  return !token->empty();
}

/**
 * @brief JSONトークンをサイズ値へ変換する.
 * @param token 入力トークン
 * @param value 出力先
 * @return 成功なら真
 */
bool ParseSizeToken(const std::string &token, std::size_t *value) {
  std::istringstream stream(token);
  std::size_t parsed = 0;
  stream >> parsed;
  if (!stream || !stream.eof()) {
    return false;
  }
  *value = parsed;
  return true;
}

/**
 * @brief JSONトークンを64bit符号なし整数へ変換する.
 * @param token 入力トークン
 * @param value 出力先
 * @return 成功なら真
 */
bool ParseUnsigned64Token(const std::string &token, std::uint64_t *value) {
  std::istringstream stream(token);
  std::uint64_t parsed = 0;
  stream >> parsed;
  if (!stream || !stream.eof()) {
    return false;
  }
  *value = parsed;
  return true;
}

/**
 * @brief JSONトークンを浮動小数へ変換する.
 * @param token 入力トークン
 * @param value 出力先
 * @return 成功なら真
 */
bool ParseDoubleToken(const std::string &token, double *value) {
  std::istringstream stream(token);
  double parsed = 0.0;
  stream >> parsed;
  if (!stream || !stream.eof()) {
    return false;
  }
  *value = parsed;
  return true;
}

/**
 * @brief JSONトークンを真偽値へ変換する.
 * @param token 入力トークン
 * @param value 出力先
 * @return 成功なら真
 */
bool ParseBoolToken(const std::string &token, bool *value) {
  if (token == "true") {
    *value = true;
    return true;
  }
  if (token == "false") {
    *value = false;
    return true;
  }
  return false;
}

/**
 * @brief CSV1行を分割する.
 * @param line 入力行
 * @return 列一覧
 */
std::vector<std::string> SplitCsvLine(const std::string &line) {
  std::vector<std::string> columns;
  std::stringstream stream(line);
  std::string column;
  while (std::getline(stream, column, ',')) {
    columns.push_back(column);
  }
  return columns;
}

/**
 * @brief 列名の位置を返す.
 * @param columns 列名一覧
 * @param name 探す列名
 * @return 見つからない場合は -1
 */
int FindColumnIndex(const std::vector<std::string> &columns,
                    const std::string &name) {
  for (std::size_t index = 0; index < columns.size(); ++index) {
    if (columns[index] == name) {
      return static_cast<int>(index);
    }
  }
  return -1;
}

/**
 * @brief CSV真偽値を解析する.
 * @param text 入力文字列
 * @param value 出力先
 * @return 成功なら真
 */
bool ParseCsvBool(const std::string &text, bool *value) {
  if (text == "true") {
    *value = true;
    return true;
  }
  if (text == "false") {
    *value = false;
    return true;
  }
  return false;
}

} // namespace

DatasetPaths BuildDatasetPaths(const std::filesystem::path &dataset_root) {
  DatasetPaths paths;
  paths.dataset_root = dataset_root;
  paths.frames_dir = dataset_root / "frames";
  paths.labels_csv_path = dataset_root / "labels.csv";
  paths.metadata_json_path = dataset_root / "metadata.json";
  return paths;
}

bool ValidateDatasetStructure(const DatasetPaths &paths, std::string *message) {
  if (!std::filesystem::exists(paths.dataset_root)) {
    SetMessage(message,
               "dataset_root が存在しません: " + paths.dataset_root.string());
    return false;
  }
  if (!std::filesystem::is_directory(paths.dataset_root)) {
    SetMessage(message, "dataset_root がディレクトリではありません: " +
                            paths.dataset_root.string());
    return false;
  }
  if (!std::filesystem::exists(paths.frames_dir) ||
      !std::filesystem::is_directory(paths.frames_dir)) {
    SetMessage(message, "frames ディレクトリが存在しません: " +
                            paths.frames_dir.string());
    return false;
  }
  if (!std::filesystem::exists(paths.labels_csv_path) ||
      !std::filesystem::is_regular_file(paths.labels_csv_path)) {
    SetMessage(message,
               "labels.csv が存在しません: " + paths.labels_csv_path.string());
    return false;
  }
  if (!std::filesystem::exists(paths.metadata_json_path) ||
      !std::filesystem::is_regular_file(paths.metadata_json_path)) {
    SetMessage(message, "metadata.json が存在しません: " +
                            paths.metadata_json_path.string());
    return false;
  }
  return true;
}

bool ReadDatasetMetadata(const std::filesystem::path &metadata_json_path,
                         DatasetMetadata *metadata, std::string *message) {
  if (metadata == nullptr) {
    SetMessage(message, "metadata が null です.");
    return false;
  }

  std::string text;
  if (!ReadTextFile(metadata_json_path, &text, message)) {
    return false;
  }

  std::string token;
  if (!ExtractJsonToken(text, "dataset_format_version", &token) ||
      !ParseSizeToken(token, &metadata->dataset_format_version)) {
    SetMessage(message, "dataset_format_version の読込に失敗しました.");
    return false;
  }
  if (!ExtractJsonToken(text, "scenario_name", &metadata->scenario_name)) {
    SetMessage(message, "scenario_name の読込に失敗しました.");
    return false;
  }
  if (!ExtractJsonToken(text, "seed", &token) ||
      !ParseUnsigned64Token(token, &metadata->seed)) {
    SetMessage(message, "seed の読込に失敗しました.");
    return false;
  }
  if (!ExtractJsonToken(text, "width", &token) ||
      !ParseSizeToken(token, &metadata->width)) {
    SetMessage(message, "width の読込に失敗しました.");
    return false;
  }
  if (!ExtractJsonToken(text, "height", &token) ||
      !ParseSizeToken(token, &metadata->height)) {
    SetMessage(message, "height の読込に失敗しました.");
    return false;
  }
  if (!ExtractJsonToken(text, "num_frames", &token) ||
      !ParseSizeToken(token, &metadata->num_frames)) {
    SetMessage(message, "num_frames の読込に失敗しました.");
    return false;
  }
  if (!ExtractJsonToken(text, "num_stars", &token) ||
      !ParseSizeToken(token, &metadata->num_stars)) {
    SetMessage(message, "num_stars の読込に失敗しました.");
    return false;
  }
  metadata->num_objects = 1;
  if (ExtractJsonToken(text, "num_objects", &token) &&
      !ParseSizeToken(token, &metadata->num_objects)) {
    SetMessage(message, "num_objects の読込に失敗しました.");
    return false;
  }
  if (!ExtractJsonToken(text, "background_level", &token) ||
      !ParseDoubleToken(token, &metadata->background_level)) {
    SetMessage(message, "background_level の読込に失敗しました.");
    return false;
  }
  if (!ExtractJsonToken(text, "read_noise_sigma", &token) ||
      !ParseDoubleToken(token, &metadata->read_noise_sigma)) {
    SetMessage(message, "read_noise_sigma の読込に失敗しました.");
    return false;
  }
  if (!ExtractJsonToken(text, "use_poisson_noise", &token) ||
      !ParseBoolToken(token, &metadata->use_poisson_noise)) {
    SetMessage(message, "use_poisson_noise の読込に失敗しました.");
    return false;
  }
  if (!ExtractJsonToken(text, "shift_sigma_x", &token) ||
      !ParseDoubleToken(token, &metadata->shift_sigma_x)) {
    SetMessage(message, "shift_sigma_x の読込に失敗しました.");
    return false;
  }
  if (!ExtractJsonToken(text, "shift_sigma_y", &token) ||
      !ParseDoubleToken(token, &metadata->shift_sigma_y)) {
    SetMessage(message, "shift_sigma_y の読込に失敗しました.");
    return false;
  }
  metadata->hot_pixel_count = 0;
  if (ExtractJsonToken(text, "hot_pixel_count", &token) &&
      !ParseSizeToken(token, &metadata->hot_pixel_count)) {
    SetMessage(message, "hot_pixel_count の読込に失敗しました.");
    return false;
  }

  metadata->hot_pixel_peak = 0.0;
  if (ExtractJsonToken(text, "hot_pixel_peak", &token) &&
      !ParseDoubleToken(token, &metadata->hot_pixel_peak)) {
    SetMessage(message, "hot_pixel_peak の読込に失敗しました.");
    return false;
  }
  return true;
}

bool ReadGroundTruthCsv(const std::filesystem::path &labels_csv_path,
                        std::vector<GroundTruthRecord> *truths,
                        std::string *message) {
  if (truths == nullptr) {
    SetMessage(message, "truths が null です.");
    return false;
  }

  std::ifstream stream(labels_csv_path);
  if (!stream.is_open()) {
    SetMessage(message,
               "labels.csv を開けませんでした: " + labels_csv_path.string());
    return false;
  }

  std::string line;
  if (!std::getline(stream, line)) {
    SetMessage(message, "labels.csv のヘッダ読込に失敗しました.");
    return false;
  }

  const std::vector<std::string> header = SplitCsvLine(line);
  const int frame_index_column = FindColumnIndex(header, "frame_index");
  const int object_id_column = FindColumnIndex(header, "object_id");
  const int shift_dx_column = FindColumnIndex(header, "shift_dx");
  const int shift_dy_column = FindColumnIndex(header, "shift_dy");
  const int object_x_column = FindColumnIndex(header, "object_x");
  const int object_y_column = FindColumnIndex(header, "object_y");
  const int object_flux_column = FindColumnIndex(header, "object_flux");
  const int object_visible_column = FindColumnIndex(header, "object_visible");

  if (frame_index_column < 0 || object_id_column < 0 || shift_dx_column < 0 ||
      shift_dy_column < 0 || object_x_column < 0 || object_y_column < 0 ||
      object_flux_column < 0 || object_visible_column < 0) {
    SetMessage(message, "labels.csv の必須列が不足しています.");
    return false;
  }

  truths->clear();
  while (std::getline(stream, line)) {
    if (line.empty()) {
      continue;
    }

    const std::vector<std::string> columns = SplitCsvLine(line);
    if (columns.size() != header.size()) {
      SetMessage(message, "labels.csv の列数が不正です.");
      return false;
    }

    GroundTruthRecord record;
    if (!ParseSizeToken(columns[static_cast<std::size_t>(frame_index_column)],
                        &record.frame_index) ||
        !ParseSizeToken(columns[static_cast<std::size_t>(object_id_column)],
                        &record.object_id) ||
        !ParseDoubleToken(columns[static_cast<std::size_t>(shift_dx_column)],
                          &record.shift_dx) ||
        !ParseDoubleToken(columns[static_cast<std::size_t>(shift_dy_column)],
                          &record.shift_dy) ||
        !ParseDoubleToken(columns[static_cast<std::size_t>(object_x_column)],
                          &record.object_x) ||
        !ParseDoubleToken(columns[static_cast<std::size_t>(object_y_column)],
                          &record.object_y) ||
        !ParseDoubleToken(columns[static_cast<std::size_t>(object_flux_column)],
                          &record.object_flux) ||
        !ParseCsvBool(columns[static_cast<std::size_t>(object_visible_column)],
                      &record.object_visible)) {
      SetMessage(message, "labels.csv の値解析に失敗しました.");
      return false;
    }
    truths->push_back(record);
  }

  if (stream.bad()) {
    SetMessage(message,
               "labels.csv の読込に失敗しました: " + labels_csv_path.string());
    return false;
  }
  stream.close();
  return true;
}

} // namespace transient_tracker::analyze
