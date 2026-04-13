# TransientTracker 移動天体解析器 実装計画とチェックリスト

## 略語

- CLI: Command Line Interface
- MVP: Minimum Viable Product
- PoC: Proof of Concept

## 目的

解析器 `transient_tracker_analyze` の初期実装へ進むための段階計画と, 実装前に確認すべきチェック項目を整理する.

## 実装対象

初期実装の対象は次とする.

- データセット読込
- 並進位置合わせ
- 差分候補抽出
- 軌跡連結
- 真値評価
- 結果出力

## 実装フェーズ

### Phase 0. 骨組み作成

- `apps/analyze/main.cpp` を追加する.
- `include/TransientTracker/analyze/` と `src/analyze/` を作る.
- `CMakeLists.txt` に `transient_tracker_analyze` を追加する.
- `AnalysisConfig` と `CLI` 解析を用意する.

完了条件:

- `--help` が動く.
- `--dataset`, `--output` の必須引数判定が動く.

### Phase 1. データセット読込

- `DatasetReader`
- `FrameLoader`
- `metadata.json` と `labels.csv` の最小読込
- フレーム列の順序保証

完了条件:

- `S-01` を読み込める.
- 欠損ファイル時に明確なエラーが返る.

### Phase 2. 基礎解析MVP

- フレーム正規化
- 位置合わせなしの中央値基準画像
- 残差計算
- しきい値マスク
- 候補抽出

完了条件:

- `S-01` で真値近傍の候補が少なくとも1フレームで出る.
- `debug/` に残差画像とマスクを保存できる.

### Phase 3. 並進位置合わせ

- `RegistrationEstimator`
- `FrameAligner`
- 推定シフト保存

完了条件:

- `S-02` で位置合わせ後の基準画像が目視で改善する.
- 推定シフトと真値シフトの比較結果を保存できる.

### Phase 4. 包絡差分と軌跡連結

- `ReferenceBuilder`
- `CandidateExtractor`
- `TrackLinker`

完了条件:

- `S-01` で3フレーム以上の軌跡候補が作れる.
- `tracks.csv` を保存できる.

### Phase 5. 真値評価

- `TruthEvaluator`
- `evaluation.json`
- `summary.json`

完了条件:

- `S-01` から `S-05` で評価ファイルを保存できる.
- 真値一致数と偽陽性数を確認できる.

## 推奨ファイル構成

```text
TransientTracker/
├── apps/
│   ├── analyze/
│   │   └── main.cpp
│   └── generate/
├── include/
│   └── TransientTracker/
│       ├── analyze/
│       │   ├── analysis_config.hpp
│       │   ├── analysis_types.hpp
│       │   ├── dataset_reader.hpp
│       │   ├── frame_loader.hpp
│       │   ├── frame_normalizer.hpp
│       │   ├── registration_estimator.hpp
│       │   ├── frame_aligner.hpp
│       │   ├── reference_builder.hpp
│       │   ├── candidate_extractor.hpp
│       │   ├── track_linker.hpp
│       │   ├── truth_evaluator.hpp
│       │   ├── analysis_writer.hpp
│       │   └── analyze_app.hpp
│       └── synthetic/
├── src/
│   ├── analyze/
│   │   ├── dataset_reader.cpp
│   │   ├── frame_loader.cpp
│   │   ├── frame_normalizer.cpp
│   │   ├── registration_estimator.cpp
│   │   ├── frame_aligner.cpp
│   │   ├── reference_builder.cpp
│   │   ├── candidate_extractor.cpp
│   │   ├── track_linker.cpp
│   │   ├── truth_evaluator.cpp
│   │   ├── analysis_writer.cpp
│   │   └── analyze_app.cpp
│   └── synthetic/
└── tests/
    ├── analyze_dataset_reader_test.cpp
    ├── analyze_registration_test.cpp
    ├── analyze_candidate_test.cpp
    ├── analyze_track_linker_test.cpp
    └── analyze_app_smoke_test.cpp
```

## 実装順序

1. `AnalysisConfig`, `CLI`, `apps/analyze/main.cpp` を実装する.
2. `DatasetReader`, `FrameLoader` を実装する.
3. `FrameNormalizer`, `ReferenceBuilder`, `CandidateExtractor` を実装する.
4. `RegistrationEstimator`, `FrameAligner` を実装する.
5. `TrackLinker`, `TruthEvaluator`, `AnalysisWriter` を実装する.
6. スモークテストとシナリオ別回帰テストを追加する.

## 実装前チェックリスト

- [ ] 目的: `transient_tracker_analyze` を新規実装し, 生成済みデータセットを読んで候補抽出, 軌跡生成, 真値評価まで実行できるようにする.
- [ ] 対象ファイル: `CMakeLists.txt`, `apps/analyze/main.cpp`, `include/TransientTracker/analyze/*`, `src/analyze/*`, `tests/analyze_*` を追加または更新する.
- [ ] 変更方針: まず `読込 -> 基礎候補抽出 -> 出力` のMVPを作り, その後に位置合わせと軌跡連結を積む. 代替案として全機能を一括実装する方法もあるが, 切り分け性が低いため採らない.
- [ ] 影響範囲: `CMakeLists.txt` に新規ターゲット追加が入る. 生成器本体のロジックは変更しない. 共有契約は `dataset_root` 配下のファイル構造に限定する.
- [ ] 未決定事項: 初期位置合わせを `位相相関` と `恒星重心対応` のどちらで始めるか. デフォルトは `位相相関` を推奨する.
- [ ] テスト計画: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`, `cmake --build build`, `ctest --test-dir build --output-on-failure`, `transient_tracker_analyze --dataset ... --output ...` の実行確認を行う.

## フェーズ別チェックリスト

### Phase 0

- [ ] `transient_tracker_analyze` がビルドできる.
- [ ] `--help` が表示できる.
- [ ] 必須引数不足時に失敗できる.

### Phase 1

- [ ] `metadata.json` を読める.
- [ ] `labels.csv` を読める.
- [ ] `frames/` を順番通りに読める.

### Phase 2

- [ ] 中央値基準画像を作れる.
- [ ] 残差画像を作れる.
- [ ] 候補マスクを作れる.

### Phase 3

- [ ] シフト推定結果を保持できる.
- [ ] 位置合わせ済み画像を保存できる.
- [ ] `S-02` で改善確認ができる.

### Phase 4

- [ ] 候補連結ができる.
- [ ] `tracks.csv` を出せる.
- [ ] 最小軌跡長で候補を落とせる.

### Phase 5

- [ ] 真値照合ができる.
- [ ] `evaluation.json` を出せる.
- [ ] `summary.json` を出せる.

## 完了判定

- `S-01` から `S-05` の全データセットで解析コマンドが完走する.
- `tracks.csv`, `evaluation.json`, `summary.json` が生成される.
- 少なくとも `S-01` と `S-02` で真値近傍軌跡を返せる.

## 実装進捗 (2026-04-12)

### 完了済み

- [x] `Phase 0` 骨組み作成
- [x] `Phase 1` データセット読込
- [x] `Phase 2` 基礎解析MVP
- [x] `Phase 3` 並進位置合わせ
- [x] `Phase 4` 包絡差分と軌跡連結
- [x] `Phase 5` 真値評価

### 実装済みの主な出力

- `summary.json`
- `detections.csv`
- `tracks.csv`
- `evaluation.json`
- `results/result_NNNN.png`
- `results/trajectory_result.png`
- `debug/aligned/`, `debug/residual/`, `debug/mask/`, `debug/overlay/`

### 次段階の候補

- [ ] 複数天体対応
- [ ] 軌跡スコア重みの外部化
- [ ] シフト数不一致時の厳格エラー化
- [ ] 中央値計算ユーティリティの共通化
- [ ] 実画像入力への対応

## 関連文書

- `080-moving-object-analyzer-development-policy.md`
- `090-moving-object-analyzer-development-specification.md`
