/**
 * @file dataset_layout.cpp
 * @brief データセット配置管理を実装する.
 */

#include "TransientTracker/synthetic/dataset_layout.hpp"

#include <system_error>

namespace transient_tracker::synthetic {

DatasetManifest BuildDatasetManifest(const std::filesystem::path& dataset_root) {
    DatasetManifest manifest;
    manifest.dataset_root = dataset_root;
    manifest.frames_dir = dataset_root / "frames";
    manifest.labels_csv_path = dataset_root / "labels.csv";
    manifest.metadata_json_path = dataset_root / "metadata.json";
    manifest.preview_dir = dataset_root / "preview";
    return manifest;
}

bool CreateDatasetDirectories(const DatasetManifest& manifest, std::string* message) {
    std::error_code error;
    std::filesystem::create_directories(manifest.dataset_root, error);
    if (error) {
        if (message != nullptr) {
            *message = "dataset_root を作成できませんでした: " + manifest.dataset_root.string();
        }
        return false;
    }

    std::filesystem::create_directories(manifest.frames_dir, error);
    if (error) {
        if (message != nullptr) {
            *message = "frames ディレクトリを作成できませんでした: " + manifest.frames_dir.string();
        }
        return false;
    }

    std::filesystem::create_directories(manifest.preview_dir, error);
    if (error) {
        if (message != nullptr) {
            *message = "preview ディレクトリを作成できませんでした: " + manifest.preview_dir.string();
        }
        return false;
    }

    return true;
}

}  // namespace transient_tracker::synthetic
