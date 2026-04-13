# TransientTracker 移動天体解析器 開発仕様書

## 略語

- CLI: Command Line Interface
- CSV: Comma-Separated Values
- JSON: JavaScript Object Notation
- MAD: Median Absolute Deviation
- PNG: Portable Network Graphics
- PoC: Proof of Concept

## 目的

生成器が出力したデータセットを入力として, 移動天体候補の抽出, 軌跡生成, 真値評価を行う解析器の仕様を定義する.

## 実装前提

- 実装言語は `C++20` とする.
- ビルドシステムは `CMake` とする.
- 画像I/O(Input/Output), ワープ, 位相相関, 連結成分解析には `OpenCV` を使う.
- 数値配列と統計量計算には `xtensor` を使う.
- 解析器は `transient_tracker_analyze` という独立実行ファイルとして実装する.

## 対象範囲

- データセット構造の検証
- `frames/` の読込とフレーム順整列
- `labels.csv`, `metadata.json` の読込
- 並進位置合わせ
- 基準画像生成
- 許容誤差付き包絡画像生成
- 残差ベースの候補抽出
- フレーム間の軌跡連結
- 真値との比較評価
- 中間生成物と結果の書き出し

対象外は次とする.

- 回転, 拡大縮小を含む高次位置合わせ
- 実画像フォーマットの多様化対応
- 軌道決定
- 学習ベース検出器
- `shift-and-stack` の本実装

## 入力仕様

### 1. データセットルート

解析器は `dataset_root` を受け取り, 次の構造を前提にする.

```text
dataset_root/
├── metadata.json
├── labels.csv
├── frames/
│   ├── frame_0000.png
│   ├── frame_0001.png
│   └── ...
└── preview/
```

`preview/` は任意とする. 解析に必須なのは `metadata.json`, `labels.csv`, `frames/` の3要素である.

### 2. `metadata.json`

少なくとも次の値を読めること.

- `dataset_format_version`
- `scenario_name`
- `seed`
- `width`
- `height`
- `num_frames`
- `num_stars`
- `background_level`
- `read_noise_sigma`
- `use_poisson_noise`
- `shift_sigma_x`
- `shift_sigma_y`
- `hot_pixel_count`
- `hot_pixel_peak`

未知キーは無視してよい.

### 3. `labels.csv`

少なくとも次の列を読めること.

- `frame_index`
- `object_id`
- `shift_dx`
- `shift_dy`
- `object_x`
- `object_y`
- `object_flux`
- `object_visible`

初期実装では `object_id = 0` を主対象とするが, 列としては複数天体拡張を許容する.

## 出力仕様

解析器は `output_root` 配下へ次を出力する.

```text
analysis/
├── summary.json
├── detections.csv
├── tracks.csv
├── evaluation.json
├── results/
│   ├── result_0000.png
│   ├── result_0001.png
│   └── trajectory_result.png
└── debug/
    ├── aligned/
    ├── residual/
    ├── mask/
    └── overlay/
```

### 1. `summary.json`

- 入力データセット名
- 実行パラメータ
- フレーム数
- 候補数
- 軌跡数
- 主軌跡ID
- 実行成否

### 2. `detections.csv`

列は少なくとも次を含む.

- `frame_index`
- `detection_id`
- `x`
- `y`
- `peak_value`
- `sum_value`
- `area`
- `sigma_estimate`
- `score`

### 3. `tracks.csv`

列は少なくとも次を含む.

- `track_id`
- `frame_index`
- `detection_id`
- `x`
- `y`
- `peak_value`
- `sum_value`
- `area`
- `sigma_estimate`
- `score`
- `track_length`
- `track_score`
- `mean_vx`
- `mean_vy`
- `fit_error`
- `matched_object_id`
- `matched_frame_count`

### 4. `evaluation.json`

- `num_visible_truth_frames`
- `num_matched_tracks`
- `primary_track_id`
- `primary_track_matched_object_id`
- `primary_track_match_count`
- `primary_track_length`

### 5. `results/`

- `result_NNNN.png`
  各フレーム上に主軌跡を重畳した結果画像
- `trajectory_result.png`
  先頭フレーム上に主軌跡全体と開始点, 終了点を描画した要約画像

## CLI仕様

想定コマンドは次とする.

```bash
transient_tracker_analyze \
  --dataset ./output/S-01 \
  --output ./analysis/S-01 \
  --tolerance-radius 1 \
  --threshold-scale 5.0 \
  --max-velocity 3.0 \
  --area-min 1 \
  --area-max 64 \
  --min-track-length 3 \
  --truth-match-distance 2.0 \
  --write-debug-images
```

最低限サポートする引数は次とする.

- `--dataset`
- `--output`
- `--tolerance-radius`
- `--threshold-scale`
- `--max-velocity`
- `--area-min`
- `--area-max`
- `--min-track-length`
- `--truth-match-distance`
- `--write-debug-images`
- `--disable-registration`
- `--help`

## 機能仕様

### F-01 データセット検証

- `dataset_root` の存在を確認する.
- 必須ファイルと必須ディレクトリの存在を確認する.
- `metadata.json` の `num_frames` と `frames/` 内ファイル数が一致することを確認する.

### F-02 フレーム読込

- `frames/frame_NNNN.png` をフレーム番号順に読み込む.
- 読込後は `xt::xtensor<float, 3>` または等価表現に保持する.
- 画素値は `float` へ変換する.

### F-03 フレーム正規化

- 各フレームからロバスト背景値を引けること.
- 必要に応じてフレームごとのスケール差を抑制できること.
- 初期実装では中央値ベース正規化でよい.

### F-04 位置合わせ

- 基準フレームを1枚選び, 各フレームの並進ずれを推定する.
- 初期実装の推定法は位相相関を第一候補とする.
- 推定結果は `Shift2D` 一覧として保持する.

### F-05 フレーム再配置

- 推定した `(dx, dy)` を用いて各フレームを共通座標系へワープする.
- 線形補間を標準とする.
- 端部欠損を扱うため, 有効領域マスクを保持してよい.

### F-06 基準画像生成

- 位置合わせ済みスタックの時間中央値から基準画像 `R` を作る.
- 有効領域外は計算対象から外す.

### F-07 包絡画像生成

- 半径 `e` の近傍最小値から `R_min` を作る.
- 半径 `e` の近傍最大値から `R_max` を作る.
- `e` は `CLI` から変更可能とする.

### F-08 候補抽出

- 残差画像 `D_t = A_t - R_max` を計算する.
- `MAD` からノイズ尺度 `sigma_t` を推定する.
- `D_t > k * sigma_t` を候補マスクとする.
- 連結成分解析で候補領域を抽出する.
- `area_min <= area <= area_max` の候補だけを残す.

### F-09 軌跡連結

- 隣接フレーム間で距離制約により候補を接続する.
- 接続条件は `max_velocity` と方向連続性を基本とする.
- 長さ `min_track_length` 以上の系列を軌跡候補とする.

### F-10 真値評価

- `labels.csv` の可視真値と候補を距離閾値内で照合する.
- 軌跡単位では, 真値軌跡近傍を一定割合以上で通過する候補を一致とみなす.
- 評価結果を `evaluation.json` に保存する.

### F-11 デバッグ出力

- `--write-debug-images` 指定時は, 位置合わせ画像, 残差画像, 候補マスク, 候補重畳画像を保存する.
- 出力しない場合も, 数値結果は `CSV` と `JSON` に保存する.

### F-12 結果画像出力

- 各フレームに対して `results/result_NNNN.png` を保存する.
- `result_NNNN.png` には主軌跡のみを描画する.
- `results/trajectory_result.png` には, 先頭フレーム上へ主軌跡のポリラインと開始点, 終了点を描画する.
- 主軌跡が存在しない場合も, 画像自体は保存し, 主軌跡なしの状態が分かるようにする.

## 非機能仕様

### N-01 再現性

- 同じ入力データセットと同じ実行パラメータなら同一結果を返すこと.

### N-02 保守性

- `読込`, `前処理`, `検出`, `追跡`, `評価`, `出力` をモジュール分離すること.

### N-03 可観測性

- どの段階で失敗したかが標準エラー出力と `summary.json` から分かること.

### N-04 拡張性

- 将来の複数天体対応で, `object_id` を保持した評価へ拡張できること.
- 将来の `shift-and-stack` 追加時に, 検出層を差し替えやすい構成にすること.

## モジュール仕様

### 1. `DatasetReader`

- `dataset_root` の検証とメタデータ読込を行う.

### 2. `FrameLoader`

- `frames/` の画像列を読み込む.

### 3. `FrameNormalizer`

- 背景補正と強度スケール補正を行う.

### 4. `RegistrationEstimator`

- フレーム間の並進ずれを推定する.

### 5. `FrameAligner`

- 推定シフトを用いてフレームを再配置する.

### 6. `ReferenceBuilder`

- 中央値基準画像と包絡画像を構築する.

### 7. `CandidateExtractor`

- 残差, しきい値, 連結成分解析から候補を抽出する.

### 8. `TrackLinker`

- 候補点をフレーム間で連結して軌跡を構築する.

### 9. `TruthEvaluator`

- `labels.csv` と候補, 軌跡を照合して評価する.

### 10. `AnalysisWriter`

- `summary.json`, `detections.csv`, `tracks.csv`, `evaluation.json`, `debug/` を保存する.

## データ構造案

```cpp
struct AnalysisConfig {
    std::filesystem::path dataset_root;
    std::filesystem::path output_root;
    int tolerance_radius;
    double threshold_scale;
    double max_velocity_per_frame;
    std::size_t area_min;
    std::size_t area_max;
    std::size_t min_track_length;
    double truth_match_distance;
    bool write_debug_images;
    bool disable_registration;
};

struct Shift2D {
    std::size_t frame_index;
    double dx;
    double dy;
    double response;
};

struct Detection {
    std::size_t frame_index;
    std::size_t detection_id;
    double x;
    double y;
    double peak_value;
    double sum_value;
    std::size_t area;
    double sigma_estimate;
    double score;
};

struct Track {
    std::size_t track_id;
    std::vector<Detection> detections;
    double mean_vx;
    double mean_vy;
    double fit_error;
    double score;
    int matched_object_id;
    std::size_t matched_frame_count;
};
```

## 処理フロー

1. `CLI` 引数を解析する.
2. データセット構造を検証する.
3. `metadata.json`, `labels.csv`, `frames/` を読み込む.
4. フレームを正規化する.
5. 並進シフトを推定する.
6. フレームを再配置する.
7. 基準画像と包絡画像を構築する.
8. 残差画像と候補マスクを計算する.
9. 候補を連結して軌跡候補を作る.
10. 真値と照合して評価する.
11. 解析結果とデバッグ画像を保存する.

## テスト仕様

### T-01 データセット読込テスト

- `S-01` を読めること.
- ファイル欠損時に失敗理由を返すこと.

### T-02 位置合わせテスト

- `S-02` で推定シフトが真値シフトに近いこと.

### T-03 候補抽出テスト

- `S-01` で可視フレーム上に真値近傍候補が存在すること.

### T-04 ノイズ耐性テスト

- `S-03`, `S-05` で解析が異常終了しないこと.

### T-05 広がり天体テスト

- `S-04` で面積制約内の候補として残ること.

### T-06 軌跡連結テスト

- 連続3フレーム以上の真値近傍候補が1本の軌跡としてまとまること.

### T-07 評価出力テスト

- `evaluation.json` に真値一致数と偽陽性数が保存されること.
  初期実装では, 主軌跡の一致情報が保存されることを確認対象とする.

## 受け入れ条件

- `--dataset` と `--output` だけで基本解析が開始できること.
- `S-01` で少なくとも1本の真値近傍軌跡を返せること.
- `S-02` で位置合わせなしより位置合わせありの方が残差偽陽性が減ること.
- `S-03`, `S-05` で異常終了せず結果ファイルを出力できること.
- `summary.json`, `tracks.csv`, `evaluation.json` が生成されること.
- `results/trajectory_result.png` が生成されること.

## 関連文書

- `010-moving-object-detection-poc.md`
- `020-requirements-definition.md`
- `030-algorithm-specification.md`
- `040-cpp-implementation-notes.md`
- `080-moving-object-analyzer-development-policy.md`
