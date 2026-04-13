# TransientTracker Proof of Concept(PoC) C++実装メモ

## 略語

- CLI: Command Line Interface
- PoC: Proof of Concept

## 実装方針

- C++20とCMakeを前提にする.
- 初期PoCは, `生成器` と `解析器` の2つの実行ファイルに分けて構成する.
- 両者の接点は, 生成器が書き出すデータセットディレクトリとする.
- 保存形式には依存せず, 画像読み込み層と検出ロジック層を分離する.
- 数値計算の中心は `xtensor` を使い, 画像I/O(Input/Output) と幾何変換は必要に応じて外部ライブラリを使う.

実環境でのビルド確認の結果として, Homebrew 版 `xtensor` は `C++20` 前提で扱う必要がある.
そのため, このプロジェクトの実装とビルド設定は `C++20` に統一する.

## 推奨ライブラリ

- `xtensor`: 3次元配列, 統計量, 要素ごとの演算.
- `OpenCV`: 画像読込, ワープ, 位相相関, 連結成分解析.
- `fmt` または標準ライブラリ: ログ出力.

`OpenCV` を使わない場合でも成立するが, 位置合わせとラベリング実装の手間が増える.

## データ表現

- 画像スタック: `xt::xtensor<float, 3>`
- 単フレーム画像: `xt::xtensor<float, 2>`
- 候補マスク: `xt::xtensor<std::uint8_t, 2>`
- フレームごとの並進ずれ: `std::vector<Shift2D>`
- 候補一覧: `std::vector<Detection>`
- 軌跡一覧: `std::vector<Track>`

```cpp
struct Shift2D {
    double dx;
    double dy;
    double error;
};

struct Detection {
    std::size_t frame_index;
    double x;
    double y;
    double peak_value;
    double sum_value;
    std::size_t area;
};

struct Track {
    std::vector<Detection> detections;
    double mean_vx;
    double mean_vy;
    double fit_error;
    double score;
};
```

## モジュール分割案

### 1. 入出力層

- `ImageLoader`
- 責務は, ファイル列を読み込み, 単フレーム配列へ変換すること.
- この層だけが入力形式を知る.

### 2. 前処理層

- `FrameNormalizer`
- `RegistrationEstimator`
- `FrameWarper`
- 責務は, 明るさ正規化, 位置ずれ推定, 位置合わせ済み画像生成.

### 3. 検出層

- `ReferenceBuilder`
- `ResidualSuppressor`
- `CandidateExtractor`
- 責務は, 基準画像生成, `R_min` と `R_max` の計算, 候補点抽出.

### 4. 追跡層

- `TrackLinker`
- `TrackScorer`
- 責務は, フレーム間連結, 速度推定, スコア計算.

### 5. 出力層

- `ResultWriter`
- 責務は, 候補一覧とデバッグ画像の保存.

## ディレクトリ案

```text
TransientTracker/
├── CMakeLists.txt
├── include/
│   └── TransientTracker/
│       ├── detection_types.hpp
│       ├── image_loader.hpp
│       ├── preprocessing.hpp
│       ├── detection_pipeline.hpp
│       └── result_writer.hpp
├── src/
│   ├── main.cpp
│   ├── image_loader.cpp
│   ├── preprocessing.cpp
│   ├── detection_pipeline.cpp
│   └── result_writer.cpp
└── tests/
    ├── synthetic_stack_test.cpp
    └── track_linker_test.cpp
```

## 実装順序

1. 人工データ生成を先に用意する.
2. 位置合わせなしの差分検出でパイプライン全体を通す.
3. 並進位置合わせを追加する.
4. 許容誤差 `e` による包絡除去を追加する.
5. 軌跡連結とスコアリングを追加する.
6. 実画像でパラメータ調整する.

この順序にすると, 失敗時の切り分けがしやすい.

## 主要関数のイメージ

```cpp
std::vector<xt::xtensor<float, 2>> LoadFrames(const std::vector<std::string>& paths);
std::vector<Shift2D> EstimateShifts(const std::vector<xt::xtensor<float, 2>>& frames);
xt::xtensor<float, 3> AlignFrames(
    const std::vector<xt::xtensor<float, 2>>& frames,
    const std::vector<Shift2D>& shifts);
xt::xtensor<float, 2> BuildReferenceMedian(const xt::xtensor<float, 3>& aligned_frames);
xt::xtensor<std::uint8_t, 2> ExtractCandidates(
    const xt::xtensor<float, 2>& frame,
    const xt::xtensor<float, 2>& reference_min,
    const xt::xtensor<float, 2>& reference_max,
    double sigma,
    double threshold_scale);
std::vector<Track> LinkTracks(const std::vector<std::vector<Detection>>& detections_by_frame);
```

## パラメータ管理

- 実行時パラメータは `DetectionConfig` に集約する.
- 初期PoCで外に出す値は, `e`, `k`, `v_max`, `area_min`, `area_max`, `track_len_min` に絞る.
- `CLI` から上書きできるようにすると比較実験しやすい.

```cpp
struct DetectionConfig {
    int tolerance_radius;
    double threshold_scale;
    double max_velocity_per_frame;
    std::size_t area_min;
    std::size_t area_max;
    std::size_t min_track_length;
};
```

## テスト方針

- まず人工画像で, 静止星群と既知速度の移動点を生成する.
- 位置合わせ誤差を人工的に注入し, `e` による除去効果を検証する.
- 軌跡連結は, 小さな座標列で単体テストする.
- 実画像テストでは, 中間画像を保存して目視確認できるようにする.

## 実装上の注意

- 位置合わせとリサンプリングで端部が欠けるため, 有効領域マスクを持つとよい.
- `xtensor` の遅延評価は便利だが, 再利用する式は `xt::eval` で実体化した方が追跡しやすい.
- 例外を多用せず, 失敗は戻り値やステータス構造体で返す方針が扱いやすい.
- PoC段階では最適化よりも, 中間結果の見える化を優先する.
