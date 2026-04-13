# 解析器の自律的軌跡検出機能

## 文書の目的

次に実装すべき機能として, `真値 labels.csv や num_objects の事前既知に依存せず, 解析器が自動的に移動天体候補数と軌跡を判断する機能` を整理する.

## 背景

- 生成器では, PoC用データセットを作る都合上, `num_objects` や真値 `labels.csv` を明示的に持つ.
- 一方で最終的な解析器は, 実観測データに対して動作するため, 事前に移動天体数を知っている前提は避けたい.
- 現在の解析器は, 候補点抽出, 軌跡連結, `num_tracks` 算出までは自動で行えている.
- 現在の `num_matched_objects` は, 真値照合後の評価値であり, 実運用時の主指標にはできない.

## 目標

- 解析器入力として `labels.csv` を必須にしない.
- `num_objects` を解析器へ与えなくても, 検出された軌跡数を自動判定できる.
- 出力として `num_detected_tracks` または `num_high_confidence_tracks` を正式指標にする.
- 真値が存在する場合のみ, 追加の評価出力を生成する.

## 到達イメージ

解析器は以下の2モードを持つ.

1. 観測モード
   - 入力: `frames/`, `metadata.json`
   - 出力: `summary.json`, `tracks.csv`, `results/trajectory_result.png`
   - 特徴: 真値なしで完走する.
2. 評価モード
   - 入力: `frames/`, `metadata.json`, `labels.csv`
   - 出力: 観測モードの出力 + `evaluation.json`
   - 特徴: PoC検証や回帰テストで使う.

## 実装方針

### 1. 真値依存の分離

- `labels.csv` の存在確認を `任意` に変更する.
- 真値が無い場合は, `EvaluateTracksAgainstTruth()` を呼ばない.
- `evaluation.json` は真値がある場合のみ出力する.

### 2. 解析器の正式な主出力を変更する

- `summary.json` に以下を追加する.
  - `num_detected_tracks`
  - `num_high_confidence_tracks`
  - `track_score_threshold`
- `num_matched_objects` は評価モード専用の補助値に下げる.

### 3. 軌跡の採否を自動判定する

各軌跡について, 少なくとも以下を使って採否を判断する.

- 軌跡長
- 平均速度の安定性
- 直線近似誤差 `fit_error`
- 軌跡スコア `score`
- 検出フレームの連続性

初期実装では, 以下のような単純な閾値方式でよい.

- `track_length >= min_track_length`
- `fit_error <= fit_error_threshold`
- `score >= track_score_threshold`

この条件を満たす軌跡数を `num_high_confidence_tracks` とする.

### 4. 重複軌跡の抑制

- 同じ移動天体に対して近接した軌跡が複数立つ場合がある.
- そのため, 空間距離, 平均速度, 時間重なりを使って重複候補をまとめる.
- 初期実装では, `track_id` 間の代表選択だけでよい.

### 5. 可視化出力の整理

- `trajectory_result.png` には採用軌跡のみ描画する.
- 却下軌跡は `debug/` 側にのみ残す.
- `tracks.csv` には `is_high_confidence` 列を追加する.

## 機能要件

- 真値無しデータセットでも解析器が正常終了する.
- 真値無しでも `summary.json` と `tracks.csv` が生成される.
- 真値有りなら従来どおり `evaluation.json` が生成される.
- 同一データセットで, 真値有り/無しの両モードを切り替えられる.

## 非機能要件

- 既存の `S-01`, `S-06` 回帰テストを壊さない.
- 既存の `tracks.csv` 形式は, 列追加で拡張しても後方互換をできるだけ保つ.
- 初期実装では, 実行時間の増加を大きくしない.

## 変更対象の見込み

- `include/TransientTracker/analyze/analysis_types.hpp`
- `include/TransientTracker/analyze/dataset_reader.hpp`
- `include/TransientTracker/analyze/truth_evaluator.hpp`
- `src/analyze/dataset_reader.cpp`
- `src/analyze/analyze_app.cpp`
- `src/analyze/analysis_writer.cpp`
- `src/analyze/truth_evaluator.cpp`
- `tests/analyze_app_smoke_test.cpp`
- `tests/analyze_dataset_reader_test.cpp`

## 実装順序

1. `labels.csv` を任意入力へ変更する.
2. 真値無しで `RunAnalyzeApp()` が完走するようにする.
3. `summary.json` と `tracks.csv` に自動判定結果を追加する.
4. 軌跡採否の閾値を `AnalysisConfig` へ追加する.
5. 真値有りモードの既存評価を維持する.

## 受け入れ条件

- 真値無しデータセットで `transient_tracker_analyze` が終了コード 0 を返す.
- `summary.json` に `num_detected_tracks` と `num_high_confidence_tracks` が出る.
- `tracks.csv` に採否列が出る.
- 真値有りデータセットでは `evaluation.json` が従来どおり生成される.
- `S-01` と `S-06` の回帰テストが通る.

## 想定コスト

- 最小実装: `2 - 3 人日`
- 実用化を見据えた整理まで含む場合: `4 - 6 人日`

## 備考

- この機能は `num_objects を知らない観測データに対して, 解析器を実用化するための最初の一歩` である.
- 真値照合は今後も PoC評価には有用なので, 削除ではなく `任意化` とする.
