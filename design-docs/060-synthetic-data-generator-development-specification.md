# TransientTracker サンプルデータ生成器 開発仕様書

## 略語

- CLI: Command Line Interface
- CSV: Comma-Separated Values
- JSON: JavaScript Object Notation
- PNG: Portable Network Graphics
- PoC: Proof of Concept
- PSF: Point Spread Function
- RNG: Random Number Generator

## 目的

`050-synthetic-data-generation.md` で定義した方針に従い, 移動天体検出PoCのためのサンプルデータ生成器を実装するための仕様を定義する.

## 実装前提

- 実装言語は `C++20` とする.
- ビルドシステムは `CMake` とする.
- `xtensor` は Homebrew 導入版を前提とし, その要件に合わせて `C++20` を必須とする.

## 対象範囲

- 人工画像スタックの生成
- 真値ラベルの生成
- シナリオ定義の管理
- デバッグ用プレビュー画像の出力
- 再現可能な `CLI` 実行
- 解析器へ受け渡すデータセット形式の確定

対象外は次とする.

- 実画像の読込
- 検出アルゴリズム本体
- 生成済みデータセットの解析実行
- 高精度な物理シミュレーション
- 分散実行や専用アクセラレータ最適化

## 分離方針

- サンプルデータ生成器は独立した実行プログラムとして実装する.
- 解析側は別実行プログラムとして実装し, 生成器の内部モジュールを直接呼び出さない.
- 両プログラムの接点は, 出力ディレクトリ配下のデータセット構造に限定する.
- 生成器は `書き出し責務`, 解析器は `読込と解析責務` に限定する.
- これにより, 生成器の変更が解析器のビルドや実行経路へ波及しにくい構成とする.

## 成果物

生成器は1回の実行で次を出力する.

- 画像フレーム群
- フレーム単位の真値ラベル
- シナリオ全体のメタデータ
- 実行に使ったパラメータ
- プレビュー画像

この成果物一式を, 解析器が読む `1データセット単位の入力` とみなす.

## 想定ディレクトリ

```text
output/
└── S-01/
    ├── metadata.json
    ├── labels.csv
    ├── frames/
    │   ├── frame_0000.png
    │   ├── frame_0001.png
    │   └── ...
    └── preview/
        ├── preview_frame_0000.png
        ├── preview_frame_0001.png
        └── ...
```

解析器は, この `S-01/` のようなディレクトリを1入力単位として受け取る.

## 入力仕様

### 実行入力

- シナリオ名
- 出力先ディレクトリ
- 乱数シード
- 上書きパラメータ

解析器側の入力は本仕様の対象外とするが, 少なくとも `dataset_root` だけで処理開始できる構造を保証する.

### 設定項目

最低限, 次の設定を扱う.

- `width`
- `height`
- `num_frames`
- `num_stars`
- `background_level`
- `read_noise_sigma`
- `use_poisson_noise`
- `shift_sigma_x`
- `shift_sigma_y`
- `object_initial_x`
- `object_initial_y`
- `object_velocity_x`
- `object_velocity_y`
- `object_flux`
- `object_sigma`
- `drop_frame_indices`
- `hot_pixel_count`

## 出力仕様

### 1. 画像フレーム

- フレーム画像は `frames/frame_NNNN.png` の形式で保存する.
- 初期実装ではグレースケール `PNG` を標準とする.
- 画素値は内部では `float` で保持し, 出力時に可視化用へ正規化して保存してよい.

### 2. 真値ラベル

`labels.csv` には少なくとも次の列を含める.

- `frame_index`
- `shift_dx`
- `shift_dy`
- `object_id`
- `object_x`
- `object_y`
- `object_flux`
- `object_visible`

将来の複数天体対応を見据え, `object_id` を必須とする.
解析器はこのファイルを参照して, 検出結果と真値の比較評価を行う.

### 3. メタデータ

`metadata.json` には次を含める.

- シナリオ名
- 乱数シード
- 使用パラメータ一式
- 画像サイズ
- フレーム数
- ノイズモデル
- 生成時刻

解析器はこのファイルを参照して, データセット条件を把握できるようにする.

### 4. プレビュー画像

- 真値位置を重畳した確認用画像を `preview/` 配下へ保存する.
- 少なくとも先頭, 中央, 末尾のフレームを出力する.
- すべてのフレームを出力できるオプションも持たせる.

## 機能仕様

### F-01 シナリオ選択

- `CLI` 引数からシナリオ名を受け取る.
- 未知のシナリオ名が指定された場合は, 実行を失敗として終了する.
- シナリオは `S-01` から `S-05` を初期実装対象とする.

### F-02 乱数再現性

- 乱数シードが同じなら, 同一画像と同一ラベルを再生成できること.
- シード未指定時も, 使用したシードを必ずログと `metadata.json` に保存すること.

### F-03 静止星群生成

- 星数と星位置を設定に従って生成する.
- 星の明るさは広い分布を持てること.
- 星像は2次元ガウス `PSF` を標準とする.

### F-04 背景とノイズ付与

- 一様背景を加算できること.
- 任意で線形背景勾配を加えられること.
- ガウス雑音を付与できること.
- 任意でポアソン雑音を付与できること.

### F-05 フレームずれ注入

- 各フレームへ真の並進ずれ `(dx_t, dy_t)` を注入できること.
- 注入した真値をラベルへ保存すること.

### F-06 移動天体生成

- 少なくとも1個の移動天体を生成できること.
- 天体位置は `x_t = x_0 + t * v_x`, `y_t = y_0 + t * v_y` に従うこと.
- 天体像は点状と少し広がった像の両方を表現できること.

### F-07 欠損ケース生成

- 指定フレームで天体信号を消せること.
- 指定数のホットピクセルを注入できること.
- 画面端での部分欠損を許容すること.

### F-08 ラベル出力

- フレームごとの真値情報を `CSV` へ保存すること.
- シナリオ全体の構成を `JSON` へ保存すること.
- 生成器内部の設定値と出力値の対応を追跡できること.
- 解析器が追加情報なしで読めるよう, 保存場所と列構成を固定すること.

### F-09 プレビュー出力

- フレーム画像に真値位置を重畳したプレビューを出力できること.
- 少なくとも移動天体中心が識別できる描画を行うこと.

## 非機能仕様

### N-01 再現性

- 同一設定, 同一シード, 同一実装バージョンで同一出力を得られること.

### N-02 保守性

- 星生成, ノイズ生成, 移動天体生成, ラベル出力を別モジュールへ分離すること.

### N-03 拡張性

- 将来の複数移動天体対応を見据えたデータ構造にすること.
- 出力形式追加が容易なように, レンダリングと保存処理を分けること.
- 解析器を別リポジトリや別バイナリに分けても成立するよう, データセット契約に依存する構成にすること.

### N-04 可観測性

- 実行ログからシナリオ名, シード, 出力先, フレーム数を確認できること.
- 異常終了時に入力パラメータが追跡できること.

## モジュール仕様

### 1. `ScenarioConfig`

- シナリオの設定値を保持する.
- デフォルト値と `CLI` 上書き値を統合する.
- 不正値検証を行う.

### 2. `StarFieldGenerator`

- 静止星群を理想座標系上で生成する.
- 星位置, 星強度, 星像幅を返す.

### 3. `MovingObjectGenerator`

- 移動天体の初期位置, 速度, 強度, 像幅を受け取り, 各フレームの真値位置を返す.

### 4. `NoiseModel`

- 背景, ガウス雑音, ポアソン雑音, ホットピクセルを適用する.

### 5. `FrameShiftGenerator`

- フレームごとの真の並進ずれを生成する.
- `labels.csv` 用の真値を返す.

### 6. `FrameRenderer`

- 星群, 移動天体, 背景, ノイズを統合して最終フレームを生成する.
- 内部表現は `xt::xtensor<float, 2>` を標準とする.

### 7. `LabelWriter`

- 真値ラベルを `CSV` と `JSON` へ保存する.

### 8. `PreviewWriter`

- 可視化用画像を生成し, `PNG` で保存する.

### 9. `SyntheticDataApp`

- `CLI` 入口として全処理を統括する.
- 解析処理は持たない.

## 処理フロー

1. `CLI` 引数を解析する.
2. `ScenarioConfig` を構築し, 値を検証する.
3. 乱数シードを確定する.
4. 静止星群の理想シーンを生成する.
5. 移動天体の軌跡真値を生成する.
6. フレームごとの真の並進ずれを生成する.
7. 各フレームをレンダリングする.
8. フレーム画像, ラベル, メタデータ, プレビュー画像を保存する.
9. 実行結果サマリを標準出力へ表示する.

## データ構造仕様

```cpp
struct ScenarioConfig {
    std::string scenario_name;
    std::size_t width;
    std::size_t height;
    std::size_t num_frames;
    std::size_t num_stars;
    double background_level;
    double read_noise_sigma;
    bool use_poisson_noise;
    double shift_sigma_x;
    double shift_sigma_y;
    double object_initial_x;
    double object_initial_y;
    double object_velocity_x;
    double object_velocity_y;
    double object_flux;
    double object_sigma;
    std::vector<std::size_t> drop_frame_indices;
    std::size_t hot_pixel_count;
    std::uint64_t seed;
};

struct FrameTruth {
    std::size_t frame_index;
    double shift_dx;
    double shift_dy;
    double object_x;
    double object_y;
    double object_flux;
    bool object_visible;
};
```

## `CLI` 仕様

想定コマンドは次とする.

```bash
transient_tracker_generate \
  --scenario S-01 \
  --output ./output/S-01 \
  --seed 12345 \
  --width 512 \
  --height 512 \
  --num-frames 16
```

解析器は, 例えば次のような別コマンドとして実装する想定とする.

```bash
transient_tracker_analyze \
  --dataset ./output/S-01
```

最低限サポートする引数は次とする.

- `--scenario`
- `--output`
- `--seed`
- `--width`
- `--height`
- `--num-frames`
- `--num-stars`
- `--object-flux`
- `--object-velocity-x`
- `--object-velocity-y`
- `--write-all-preview`

## エラー処理

- `width`, `height`, `num_frames` が0以下相当ならエラーとする.
- 出力先が作成できない場合はエラーとする.
- シナリオ名不明時は利用可能シナリオ一覧を表示する.
- 画素外のみを通る無効な天体軌跡は警告またはエラーとする.

## テスト仕様

### T-01 再現性テスト

- 同一シードで画像とラベルが一致すること.

### T-02 ラベル整合性テスト

- `labels.csv` の真値座標が設定した速度式と一致すること.

### T-03 画素範囲テスト

- `object_visible` が真のフレームでは, 天体中心が画素範囲内にあること.

### T-04 シナリオ生成テスト

- `S-01` から `S-05` の全シナリオで出力が成功すること.

### T-05 欠損ケーステスト

- 指定した欠損フレームで `object_visible` が偽になること.

### T-06 プレビュー生成テスト

- プレビュー画像が期待枚数だけ保存されること.

## 実装順序

1. `ScenarioConfig` と `CLI` 解析を実装する.
2. `StarFieldGenerator` と `MovingObjectGenerator` を実装する.
3. `FrameShiftGenerator` と `FrameRenderer` を実装する.
4. `NoiseModel` を実装する.
5. `LabelWriter` と `PreviewWriter` を実装する.
6. `S-01` から `S-05` のシナリオを登録する.
7. 単体テストと回帰テストを追加する.

## 受け入れ条件

- `S-01` で画像, ラベル, メタデータ, プレビューが生成されること.
- 同一シード実行で同一 `labels.csv` と `metadata.json` が得られること.
- `S-02` でフレームずれ真値が保存されること.
- `S-04` で広がった天体像を生成できること.
- `S-05` で失敗系シナリオを再現できること.

## 関連文書

- `010-moving-object-detection-poc.md`
- `020-requirements-definition.md`
- `030-algorithm-specification.md`
- `040-cpp-implementation-notes.md`
- `050-synthetic-data-generation.md`
- `070-synthetic-data-generator-detailed-design.md`
