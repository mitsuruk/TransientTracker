# TransientTracker

This work is dedicated to the public domain under [CC0 1.0 Universal](LICENSE).

TransientTracker は, 同一視野で撮影した時系列天体画像から移動天体候補を検出, 追跡するための Proof of Concept(PoC) です.

## このプロジェクトの目的

xtensorの学習を目的として作成した PoC です.

このプロジェクト・リポジトリは，未完成であり，動作に制約があります
作業中の記録として残している状態です


現在は, 次の 2 つのコマンドで構成されています.

- `transient_tracker_generate`
  合成データセットを生成します.
- `transient_tracker_analyze`
  生成済みデータセットを解析し, 軌跡を抽出します.

## 概要

解析器は, おおむね次の流れで処理します.

1. フレーム画像列を読み込みます.
2. フレーム間の位置ずれを推定し, 位置合わせします.
3. 基準画像と残差画像を生成します.
4. 残差から候補点を抽出します.
5. 候補点をフレーム間で連結し, 軌跡を構成します.
6. 軌跡 CSV, 要約 JSON, 可視化画像を出力します.

現時点の標準シナリオは次のとおりです.

- `S-01`: 3 個の移動天体を含む標準シナリオ
- `S-06`: 2 個の移動天体を含む複数天体シナリオ

## 前提環境

- CMake 3.16 以上
- C++20 対応コンパイラ
- OpenCV
- xtensor

Homebrew を使う場合の導入例:

```bash
brew install cmake opencv xtensor
```

## ビルド

プロジェクトルートで次を実行します.

```bash
cd /Users/mitsuruk/devs/@Projects/TransientTracker
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

## 実行方法

### 1. 合成データセットを生成する

`S-01` を生成する例です.

```bash
./build/transient_tracker_generate \
  --scenario S-01 \
  --output ./output/S-01 \
  --seed 20260413 \
  --write-all-preview
```

主な引数:

- `--scenario <name>`
- `--output <dir>`
- `--seed <value>`
- `--width <value>`
- `--height <value>`
- `--num-frames <value>`
- `--num-stars <value>`
- `--object-flux <value>`
- `--object-velocity-x <value>`
- `--object-velocity-y <value>`
- `--write-all-preview`

### 2. データセットを解析する

```bash
./build/transient_tracker_analyze \
  --dataset ./output/S-01 \
  --output ./analysis/S-01-track \
  --threshold-scale 3.5 \
  --tolerance-radius 1 \
  --area-min 1 \
  --area-max 64 \
  --max-velocity 3.0 \
  --min-track-length 3 \
  --truth-match-distance 2.0 \
  --write-debug-images
```

主な引数:

- `--dataset <dir>`
- `--output <dir>`
- `--tolerance-radius <value>`
- `--threshold-scale <value>`
- `--area-min <value>`
- `--area-max <value>`
- `--max-velocity <value>`
- `--min-track-length <value>`
- `--truth-match-distance <value>`
- `--write-debug-images`
- `--disable-registration`

## ディレクトリ構成

```text
TransientTracker/
├── apps/
├── build/
├── design-docs/
├── include/
├── output/
├── src/
└── tests/
```

生成後のデータセット例:

```text
output/S-01/
├── frames/
├── labels.csv
├── metadata.json
└── preview/
```

解析後の出力例:

```text
analysis/S-01-track/
├── detections.csv
├── evaluation.json
├── summary.json
├── tracks.csv
├── results/
└── debug/
```

## 出力結果の見方

### 生成器の出力

#### `metadata.json`

データセット全体の基本情報です. 最初に次を確認します.

- `scenario_name`
- `num_frames`
- `num_objects`
- `seed`

#### `labels.csv`

合成データの真値です. 主に次を確認します.

- `frame_index`
- `object_id`
- `object_x`
- `object_y`
- `object_visible`

#### `preview/`

目視確認用のプレビュー画像です. 合成した移動天体位置の確認に使います.

### 解析器の出力

#### `summary.json`

解析全体の要約です. 主に次を確認します.

- `num_tracks`
- `num_matched_objects`
- `status`

#### `tracks.csv`

検出された軌跡一覧です. 主に次を確認します.

- `track_id`
- `frame_index`
- `matched_object_id`
- `track_score`
- `fit_error`

#### `evaluation.json`

合成データに対する真値照合結果です. 主に次を確認します.

- `num_visible_objects`
- `num_matched_tracks`
- `num_matched_objects`
- `matched_objects`

#### `results/trajectory_result.png`

先頭星図上に検出軌跡を重ねた要約画像です. まず最初に見る画像として有効です.

#### `results/result_XXXX.png`

各フレーム上に軌跡を重ねた結果画像です. フレームごとの追跡状況を確認できます.

#### `debug/`

中間生成物です. 次の画像で処理の切り分けができます.

- `reference/` 相当の基準画像
- `aligned/` の位置合わせ後画像
- `residual/` の残差画像
- `mask/` の候補抽出マスク
- `overlay/` の候補重畳画像

## 最初に確認するファイル

`S-01` を解析した場合は, まず次を見ると全体像を把握しやすくなります.

- `output/S-01/metadata.json`
- `analysis/S-01-track/summary.json`
- `analysis/S-01-track/evaluation.json`
- `analysis/S-01-track/results/trajectory_result.png`

## 関連文書

設計文書は `design-docs/` 配下にあります. 主な参照先は次です.

- [010-moving-object-detection-poc.md](/Users/mitsuruk/devs/@Projects/TransientTracker/design-docs/010-moving-object-detection-poc.md)
- [020-requirements-definition.md](/Users/mitsuruk/devs/@Projects/TransientTracker/design-docs/020-requirements-definition.md)
- [090-moving-object-analyzer-development-specification.md](/Users/mitsuruk/devs/@Projects/TransientTracker/design-docs/090-moving-object-analyzer-development-specification.md)
- [110-analyzer-autonomous-track-detection.md](/Users/mitsuruk/devs/@Projects/TransientTracker/design-docs/110-analyzer-autonomous-track-detection.md)

## 補足

- 生成器は, 真値付き PoC データセットを作るためのプログラムです.
- 解析器は, 現状では真値付きデータセットでの評価に対応しています.
- 次の段階として, 真値 `labels.csv` や `num_objects` に依存しない自律的な軌跡数判定機能を実装予定です.
