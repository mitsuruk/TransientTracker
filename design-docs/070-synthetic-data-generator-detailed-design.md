# TransientTracker サンプルデータ生成器 詳細設計書

## 略語

- CLI: Command Line Interface
- CSV: Comma-Separated Values
- JSON: JavaScript Object Notation
- PNG: Portable Network Graphics
- PoC: Proof of Concept
- PSF: Point Spread Function
- RNG: Random Number Generator

## 目的

`050-synthetic-data-generation.md` と `060-synthetic-data-generator-development-specification.md` を実装可能な粒度へ分解し, サンプルデータ生成器の詳細設計を定義する.

## 設計方針

- 生成器は `設定読込`, `真値生成`, `画像レンダリング`, `保存` を明確に分離する.
- 画像生成の内部表現は `xt::xtensor<float, 2>` に統一する.
- 真値ラベルは画像生成より先に確定し, 後段はその真値を消費する.
- ノイズや可視化は差し替え可能な独立モジュールにする.
- 初期PoCでは並進ずれのみを扱い, 回転や歪みは扱わない.
- 生成器と解析器は別プログラムとし, 接点はデータセットディレクトリのみとする.
- コンパイル条件は `C++20` とし, `xtensor` の利用もこの前提で統一する.

## ビルド前提

- コンパイラは `C++20` を解釈できること.
- `CMake` から `OpenCV` と `xtensor` を解決できること.
- 実ビルド確認の結果として, Homebrew 版 `xtensor` は `C++20` を要求するため, 本詳細設計でも `C++20` を前提とする.

## ディレクトリ構成

```text
TransientTracker/
├── apps/
│   ├── analyze/
│   │   └── main.cpp
│   └── generate/
│       └── main.cpp
├── include/
│   └── TransientTracker/
│       ├── synthetic/
│       │   ├── synthetic_types.hpp
│       │   ├── scenario_config.hpp
│       │   ├── scenario_repository.hpp
│       │   ├── star_field_generator.hpp
│       │   ├── moving_object_generator.hpp
│       │   ├── frame_shift_generator.hpp
│       │   ├── noise_model.hpp
│       │   ├── frame_renderer.hpp
│       │   ├── label_writer.hpp
│       │   ├── preview_writer.hpp
│       │   └── synthetic_data_app.hpp
├── src/
│   └── synthetic/
│       ├── scenario_config.cpp
│       ├── scenario_repository.cpp
│       ├── star_field_generator.cpp
│       ├── moving_object_generator.cpp
│       ├── frame_shift_generator.cpp
│       ├── noise_model.cpp
│       ├── frame_renderer.cpp
│       ├── label_writer.cpp
│       ├── preview_writer.cpp
│       └── synthetic_data_app.cpp
└── tests/
    ├── synthetic_config_test.cpp
    ├── synthetic_renderer_test.cpp
    ├── synthetic_truth_test.cpp
    └── synthetic_app_test.cpp
```

## 主要データ構造

### `ImageSize`

```cpp
struct ImageSize {
    std::size_t width;
    std::size_t height;
};
```

- 全画像の共通サイズを表す.
- `width > 0`, `height > 0` を必須条件とする.

### `StarSource`

```cpp
struct StarSource {
    double x;
    double y;
    double flux;
    double sigma;
};
```

- 静止星1個の真値を表す.
- 座標は理想座標系上の実数値とする.

### `MovingObjectSpec`

```cpp
struct MovingObjectSpec {
    std::size_t object_id;
    double initial_x;
    double initial_y;
    double velocity_x;
    double velocity_y;
    double flux;
    double sigma_x;
    double sigma_y;
    double angle_rad;
};
```

- 移動天体の全フレーム共通パラメータを表す.
- `sigma_x == sigma_y` なら点状または等方ガウスとみなす.

### `FrameShift`

```cpp
struct FrameShift {
    std::size_t frame_index;
    double dx;
    double dy;
};
```

- 各フレームへ注入する真の並進ずれを表す.

### `FrameTruth`

```cpp
struct FrameTruth {
    std::size_t frame_index;
    std::size_t object_id;
    double shift_dx;
    double shift_dy;
    double object_x;
    double object_y;
    double object_flux;
    bool object_visible;
};
```

- 検証で必要な最小真値を1行で表す.
- `labels.csv` の1レコードに対応する.

### `RenderedFrame`

```cpp
struct RenderedFrame {
    std::size_t frame_index;
    xt::xtensor<float, 2> image;
    std::vector<FrameTruth> truths;
};
```

- 保存前の1フレーム分データを表す.
- 複数移動天体へ拡張しやすいよう, 真値は配列で保持する.

### `DatasetManifest`

```cpp
struct DatasetManifest {
    std::filesystem::path dataset_root;
    std::filesystem::path frames_dir;
    std::filesystem::path labels_csv_path;
    std::filesystem::path metadata_json_path;
    std::filesystem::path preview_dir;
};
```

- 生成器が書き出すデータセットの物理配置を表す.
- 解析器はこの配置契約だけを前提に読込を行う.

### `ScenarioConfig`

```cpp
struct ScenarioConfig {
    std::string scenario_name;
    ImageSize image_size;
    std::size_t num_frames;
    std::size_t num_stars;
    double background_level;
    double background_gradient_x;
    double background_gradient_y;
    double read_noise_sigma;
    bool use_poisson_noise;
    double shift_sigma_x;
    double shift_sigma_y;
    double star_sigma;
    MovingObjectSpec moving_object;
    std::vector<std::size_t> drop_frame_indices;
    std::size_t hot_pixel_count;
    bool write_all_preview;
    std::uint64_t seed;
};
```

- 実行単位の設定を集約する.
- 将来の複数移動天体対応では `MovingObjectSpec` を `std::vector` へ置換可能な形に保つ.

## モジュール詳細

### 1. `ScenarioRepository`

責務:
- `S-01` から `S-05` の既定設定を返す.
- 未知シナリオを拒否する.

主要関数:

```cpp
std::optional<ScenarioConfig> FindScenario(const std::string& scenario_name);
std::vector<std::string> ListScenarioNames();
```

設計詳細:
- シナリオはコード内の静的表で持つ.
- 初期段階では外部 `JSON` 読込にしない.
- 将来, 設定ファイル読込へ差し替えられるよう, 呼び出し側は `ScenarioRepository` のみを見る.

### 2. `ScenarioConfig`

責務:
- `CLI` 上書き値を既定設定へ反映する.
- 値域検証を行う.

主要関数:

```cpp
bool ValidateScenarioConfig(const ScenarioConfig& config, std::string* message);
ScenarioConfig ApplyOverrides(
    const ScenarioConfig& base_config,
    const CliOptions& cli_options);
```

検証規則:
- `num_frames >= 1`
- `image_size.width >= 16`
- `image_size.height >= 16`
- `num_stars >= 1`
- `read_noise_sigma >= 0`
- `moving_object.flux > 0`
- `moving_object.sigma_x > 0`
- `moving_object.sigma_y > 0`

### 3. `StarFieldGenerator`

責務:
- 静止星群の真値を生成する.

主要関数:

```cpp
std::vector<StarSource> GenerateStars(
    const ScenarioConfig& config,
    std::mt19937_64* rng);
```

アルゴリズム:
1. `num_stars` 回ループする.
2. `x`, `y` を画像範囲内で一様乱数生成する.
3. `flux` を対数一様分布または指数分布に近い形で生成する.
4. `sigma` は `config.star_sigma` を使う.

設計判断:
- 初期PoCでは星の重なりを許容する.
- 星位置の最小間隔制約は導入しない.

### 4. `MovingObjectGenerator`

責務:
- 各フレームでの移動天体真値座標を生成する.

主要関数:

```cpp
std::vector<FrameTruth> GenerateObjectTruths(const ScenarioConfig& config);
```

アルゴリズム:

`x_t = initial_x + frame_index * velocity_x`

`y_t = initial_y + frame_index * velocity_y`

可視判定:
- `0 <= x_t < width`
- `0 <= y_t < height`
- `frame_index` が `drop_frame_indices` に含まれない

設計判断:
- `object_visible == false` の場合もレコードは残す.
- これにより, 欠損フレームの意図を後から検証できる.

### 5. `FrameShiftGenerator`

責務:
- 各フレームへ注入する並進ずれを生成する.

主要関数:

```cpp
std::vector<FrameShift> GenerateFrameShifts(
    const ScenarioConfig& config,
    std::mt19937_64* rng);
```

アルゴリズム:
- `dx_t ~ Normal(0, shift_sigma_x)`
- `dy_t ~ Normal(0, shift_sigma_y)`

設計判断:
- 初期フレームを必ず `0, 0` に固定する必要はない.
- ただし比較を簡単にするなら `frame_0000` を基準として `0, 0` に固定してもよい.
- 初期実装では `frame_0000` を `0, 0` に固定する.

### 6. `FrameRenderer`

責務:
- 真値と設定から1フレーム画像を生成する.

主要関数:

```cpp
xt::xtensor<float, 2> RenderFrame(
    const ScenarioConfig& config,
    const std::vector<StarSource>& stars,
    const FrameShift& shift,
    const std::vector<FrameTruth>& truths_for_frame,
    std::mt19937_64* rng);
```

処理順:
1. 背景だけの `height x width` 配列を確保する.
2. 背景定数と勾配を加える.
3. 静止星群を `shift` を考慮して描画する.
4. `object_visible == true` の移動天体を描画する.
5. ホットピクセルを注入する.
6. ガウス雑音を加える.
7. 必要ならポアソン雑音を加える.

背景勾配:

`background(x, y) = background_level + background_gradient_x * x + background_gradient_y * y`

描画範囲:
- ガウス描画は中心の周囲 `ceil(3 * sigma)` までに限定する.
- これにより, 全画面走査を避ける.

### 7. `NoiseModel`

責務:
- ノイズとホットピクセルを付与する.

主要関数:

```cpp
void ApplyGaussianNoise(
    xt::xtensor<float, 2>* image,
    double sigma,
    std::mt19937_64* rng);

void ApplyPoissonNoise(
    xt::xtensor<float, 2>* image,
    std::mt19937_64* rng);

void InjectHotPixels(
    xt::xtensor<float, 2>* image,
    std::size_t hot_pixel_count,
    std::mt19937_64* rng);
```

設計判断:
- ノイズ順序は `ホットピクセル -> ガウス -> ポアソン` ではなく, `ホットピクセル -> ガウス -> 必要時ポアソン` とする.
- ただしポアソン雑音は本来, 光子計数に近い段階で適用した方が自然なので, 将来再検討余地を残す.
- 初期PoCでは検出器評価用途を優先し, 実装単純性を取る.

### 8. `LabelWriter`

責務:
- `labels.csv` と `metadata.json` を保存する.

主要関数:

```cpp
bool WriteLabelsCsv(
    const std::filesystem::path& output_path,
    const std::vector<FrameTruth>& truths);

bool WriteMetadataJson(
    const std::filesystem::path& output_path,
    const ScenarioConfig& config);
```

`labels.csv` ヘッダ:

```text
frame_index,object_id,shift_dx,shift_dy,object_x,object_y,object_flux,object_visible
```

### 9. `PreviewWriter`

責務:
- 可視化用フレームを保存する.

主要関数:

```cpp
bool WritePreviewImage(
    const std::filesystem::path& output_path,
    const xt::xtensor<float, 2>& image,
    const std::vector<FrameTruth>& truths_for_frame);
```

描画仕様:
- 元画像を `0-255` へ線形正規化する.
- `object_visible == true` の真値位置に十字マーカを描く.
- マーカ色は初期実装では白または最大輝度値とする.

### 10. `SyntheticDataApp`

責務:
- 生成器全体の制御を行う.

主要関数:

```cpp
int RunSyntheticDataApp(const CliOptions& options);
```

処理手順:
1. シナリオ読込
2. `CLI` 上書き適用
3. 設定検証
4. データセット構成決定
5. 出力先作成
6. 星群生成
7. 移動天体真値生成
8. フレームずれ生成
9. フレーム描画
10. 保存
11. 実行サマリ表示

### 11. `DatasetLayout`

責務:
- 生成器と解析器で共有するデータセット配置を定義する.
- 出力先パスを一元的に解決する.

主要関数:

```cpp
DatasetManifest BuildDatasetManifest(const std::filesystem::path& dataset_root);
bool CreateDatasetDirectories(const DatasetManifest& manifest);
```

設計判断:
- 生成器は `DatasetLayout` を通じてのみ出力先を解決する.
- 解析器も同じ契約に従うが, 生成器内部の描画モジュールには依存しない.

## 主要アルゴリズム詳細

### 星像描画

描画式:

`value(x, y) = flux * exp(-0.5 * (((x - cx) / sigma_x)^2 + ((y - cy) / sigma_y)^2))`

回転なしの初期実装では `sigma_x == sigma_y` を標準とする.

処理詳細:
- 中心座標 `cx`, `cy` に `FrameShift` を加算する.
- `int x_min = floor(cx - 3 * sigma_x)`
- `int x_max = ceil(cx + 3 * sigma_x)`
- `int y_min = floor(cy - 3 * sigma_y)`
- `int y_max = ceil(cy + 3 * sigma_y)`
- 範囲を画像境界で切り詰める.

### 彗星風天体像描画

初期実装では次の2方式のどちらかを採る.

- 等方ガウスで `sigma_object > sigma_star`
- 異方ガウスで `sigma_x != sigma_y`

詳細設計では後者にも備えて `MovingObjectSpec` に `sigma_x`, `sigma_y`, `angle_rad` を持たせる.
ただし `angle_rad` は初期実装では未使用でもよい.

### ホットピクセル生成

アルゴリズム:
1. `hot_pixel_count` 個の位置を一様乱数で選ぶ.
2. 各位置へ大きな固定値または乱数値を加算する.

設計判断:
- 同じ位置へ複数回入ってもよい.
- 厳密な重複排除は初期実装で行わない.

## シーケンス設計

### 実行シーケンス

```text
apps/generate/main.cpp
  -> SyntheticDataApp
  -> ScenarioRepository
  -> ScenarioConfig
  -> DatasetLayout
  -> StarFieldGenerator
  -> MovingObjectGenerator
  -> FrameShiftGenerator
  -> FrameRenderer
  -> LabelWriter
  -> PreviewWriter
```

### フレーム生成シーケンス

```text
for each frame_index:
  FrameRenderer::RenderFrame
    -> 背景生成
    -> 星描画
    -> 移動天体描画
    -> ホットピクセル注入
    -> ガウス雑音付与
    -> ポアソン雑音付与
    -> 画像返却
```

### 解析シーケンス(参考)

```text
apps/analyze/main.cpp
  -> dataset_root を受け取る
  -> DatasetLayout の契約で frames/, labels.csv, metadata.json を参照する
  -> 生成済み画像列を読込
  -> 解析を実行
```

## `CLI` 詳細設計

### `CliOptions`

```cpp
struct CliOptions {
    std::string scenario_name;
    std::filesystem::path output_dir;
    std::optional<std::uint64_t> seed;
    std::optional<std::size_t> width;
    std::optional<std::size_t> height;
    std::optional<std::size_t> num_frames;
    std::optional<std::size_t> num_stars;
    std::optional<double> object_flux;
    std::optional<double> object_velocity_x;
    std::optional<double> object_velocity_y;
    bool write_all_preview;
};
```

### 引数解釈規則

- 指定がない項目はシナリオ既定値を使う.
- `--seed` 未指定時は乱数で決め, 実行ログへ出す.
- `--write-all-preview` は真偽値フラグとする.

## 保存形式詳細

### `metadata.json` 例

```json
{
  "scenario_name": "S-01",
  "seed": 12345,
  "width": 512,
  "height": 512,
  "num_frames": 16,
  "num_stars": 120,
  "background_level": 100.0,
  "read_noise_sigma": 2.0,
  "use_poisson_noise": false
}
```

`metadata.json` には, 将来的に `dataset_format_version` を追加できる余地を残す.

### `labels.csv` 例

```text
frame_index,object_id,shift_dx,shift_dy,object_x,object_y,object_flux,object_visible
0,0,0.0,0.0,120.0,250.0,300.0,true
1,0,0.3,-0.1,121.2,250.5,300.0,true
```

## エラー処理設計

- 設定検証失敗時は, 何が不正かを明示して即時終了する.
- 出力先作成失敗時は, 対象パスを含むエラーメッセージを返す.
- `PNG` 保存失敗時は, 失敗ファイル名を返す.
- `labels.csv` または `metadata.json` 保存失敗時は, 途中生成物を消さずに終了する.

## ログ設計

最低限, 次を標準出力へ出す.

- シナリオ名
- 出力先
- 乱数シード
- フレーム数
- 星数
- 移動天体初期位置
- 移動天体速度
- 出力データセットルート

## テスト詳細

### 単体テスト

- `ScenarioRepository` が `S-01` から `S-05` を返すこと.
- `ValidateScenarioConfig` が不正値を拒否すること.
- `GenerateObjectTruths` が速度式通りの座標を返すこと.
- `GenerateFrameShifts` が `frame_0000` で `0, 0` を返すこと.

### 結合テスト

- `RunSyntheticDataApp` が最小シナリオで全出力を生成すること.
- 同一シードで2回実行したとき, `labels.csv` が一致すること.
- `drop_frame_indices` 指定時に `object_visible == false` となること.

### 目視確認項目

- プレビューで真値マーカが移動して見えること.
- `S-04` で星より広がった天体像が見えること.
- `S-05` で誤認しやすいホットピクセルが含まれること.

## 実装上の注意

- `xtensor` の式を再利用する箇所は `xt::eval` で実体化する.
- 画素値は負値になり得るため, `PNG` 保存前にクリップする.
- ラベル書出し順序は `frame_index`, `object_id` 昇順で固定する.
- 将来の複数天体対応を考え, 単一天体専用の前提を局所化する.
- 解析器が生成器内部コードへ依存しないよう, 共有事項はファイル配置と列定義に限定する.

## 関連文書

- `010-moving-object-detection-poc.md`
- `020-requirements-definition.md`
- `030-algorithm-specification.md`
- `040-cpp-implementation-notes.md`
- `050-synthetic-data-generation.md`
- `060-synthetic-data-generator-development-specification.md`
