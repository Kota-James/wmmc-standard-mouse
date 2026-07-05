# C++ 移行ガイド（上級生向け）

対象読者: C 版マウスを一通り理解し、より大きなコード（斜め走行、パス最適化、
複数機体対応など）を書きたくなった上級生。

> **位置づけ（重要）**: C++ 化は**標準プログラム（サークルの共有財産）には適用しない**。
> 標準プログラムは毎年の新入生教育の土台として C のままシンプルに保つ。
> 本ガイドは、テンプレートから作った**自分の個人リポジトリ**で C++ に挑戦する
> 上級生向けの手引きである。

## 0. 移行する価値・しない価値

**価値があるところ**
- 迷路・座標・方向を型で表現でき、`0x88` のようなマジックナンバー地獄から脱出できる
- `constexpr` により**加減速テーブルを Excel からコンパイラに移管**できる（後述、最大の実利）
- クラスで状態と操作をまとめられ、グローバル変数 30 個の管理から解放される
- テンプレートで迷路サイズ（16×16 / 32×32）を型パラメータ化できる

**価値がないところ / やってはいけないところ**
- HAL・CubeMX 生成コードを C++ に書き換えること（生成し直すたびに壊れる。C のまま触らない）
- 例外・RTTI・動的確保・iostream（組込みでは使わない。後述のビルド設定で殺す）
- 継承と仮想関数を使った過剰な抽象化（このプロジェクト規模では不要）

## 1. ビルド環境の変更（STM32CubeIDE）

1. プロジェクト右クリック → **Convert to C++**（既存 C ファイルはそのまま C としてコンパイルされる）
2. アプリケーションコードを `.cc`/`.cpp` に**少しずつ**リネームして移す。
   一括変換はしない。1 ファイルずつ「C++ としてコンパイルが通る」を確認しながら進める
3. C++ コンパイラ設定（Project Properties → C/C++ Build → Settings → G++ Compiler）:
   - 言語規格: `-std=c++17`（gnu++17 でも可）
   - `-fno-exceptions -fno-rtti -fno-use-cxa-atexit`
   - `-fno-threadsafe-statics`（関数内 static のガードコード削減。ISR との併用に注意）
4. リンク後に **Flash/RAM 使用量を毎回確認**する（F303K8 は 64KB/12KB しかない。
   上記フラグを入れ、iostream を使わなければ C とほぼ同サイズに収まる）

### C との接点で必須の作法

- HAL のコールバック（`HAL_TIM_PeriodElapsedCallback` 等）と ISR から呼ばれる関数は
  `extern "C"` で囲む。これを忘れると**名前修飾によりリンクは通るのに割り込みが
  デフォルトハンドラに飛ぶ**事故が起きる（コールバックは weak シンボル上書きのため
  リンクエラーにならない）:

```cpp
extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM16) { g_left_motor.on_pulse(); }
    ...
}
```

- `main.c` は CubeMX 管理のまま残し、`/* USER CODE BEGIN 2 */` から
  `extern "C" void app_main(void);` を 1 行呼ぶだけにする。
  アプリ本体は `app_main.cpp` に書く。CubeMX 再生成の影響を最小化できる。

## 2. 設計の指針（このプロジェクトの場合）

### 2.1 まず enum class と型でドメインを固める

```cpp
enum class Direction : uint8_t { North, East, South, West };
enum class Turn : uint8_t { Straight, Right, UTurn, Left };

struct Position { uint8_t x, y; };

// 現行の 0x88/0x44/0x22/0x11 は WallFlags 型に閉じ込める
struct WallFlags {
    uint8_t bits;   // NESW を下位 4bit で保持
    bool has(Direction d) const { return bits & (1u << (3 - static_cast<int>(d))); }
};
```

これだけで `search.c` の可読性問題の大半が解決する。**クラス設計より先にやる価値がある。**

### 2.2 クラス分割の例

| クラス | 現行対応 | 役割 |
|---|---|---|
| `StepperMotor` | drive.c 基幹 + interrupt.c | 1 輪のパルス生成・加減速テーブル参照 |
| `WallSensor` | sensor.c + interrupt.c | AD 値、基準値、壁判定 |
| `Motion` | drive.c 上位 | 半区画・旋回・尻当て（左右の `StepperMotor` を保持） |
| `Maze<W,H>` | map/smap/write_map | 壁記録と歩数マップ。**ハード非依存・単体テスト対象** |
| `Solver` | make_smap/make_route | 足立法（`Maze` を参照するだけ） |
| `Mouse` | main.c のシナリオ | 上記を束ねる最上位 |

原則:
- インスタンスは**静的に**確保する（`static StepperMotor left(htim16);`）。new は使わない
- `Maze`/`Solver` は HAL を一切 include しない → PC でユニットテスト可能
  （[02_教育向け再構成案.md](02_教育向け再構成案.md) のシミュレータと同じ資産になる）
- ISR ↔ クラスの橋渡しは上記 `extern "C"` コールバックからメンバ関数を呼ぶ形にする。
  ISR から触るメンバ変数は `volatile`（または `std::atomic` の relaxed）を維持する

### 2.3 constexpr で加減速テーブルを生成する（Excel の廃止）

現行の `table.h`（Excel から貼り付けた 300 個の ARR 値）はこう置き換えられる:

```cpp
// 等加速度で v = sqrt(v0^2 + 2*a*x) となるパルス間隔を「コンパイル時」に計算
constexpr auto make_accel_table() {
    std::array<uint16_t, kTableSize> t{};
    double v = kStartVelocity;                  // [mm/s]
    for (std::size_t i = 0; i < t.size(); ++i) {
        t[i] = static_cast<uint16_t>(kTimerHz * kMmPerPulse / v);   // ARR 値
        v = std::sqrt(v * v + 2.0 * kAccel * kMmPerPulse);
    }
    return t;
}
inline constexpr auto kAccelTable = make_accel_table();  // Flash (.rodata) に置かれる
```

- 加速度・最高速度・車輪径を `constexpr` 定数として 1 箇所に書けば、
  テーブルは**ビルドのたびに自動再計算**される。実行時コストはゼロ
- 注意: `std::sqrt` の constexpr 対応は処理系依存（GCC は拡張として可）。
  C++17 なら自前のニュートン法 constexpr sqrt を書いてもよい（それ自体が良い課題）
- これにより「Excel の元ファイルが行方不明」という現行の暗黙知が消滅する

### 2.4 テンプレートは 1 箇所だけ

`Maze<uint8_t Width, uint8_t Height>` のサイズパラメータ化程度に留める。
それ以上のテンプレート技巧は後輩が読めなくなる。

## 3. 移行手順（壊さずに進めるロードマップ）

1. **[準備]** C のまま既知バグを修正し、テストコース走行で「基準の挙動」を録っておく
2. **[Convert to C++]** ビルド設定のみ変更。全ファイル C のままビルド・走行確認
3. **[app_main 分離]** main.c から `app_main.cpp` を呼ぶ構造に変更（この時点で
   グローバル変数はそのまま extern で参照してよい）
4. **[型の導入]** enum class / WallFlags / Position を導入し、search 系から置換
5. **[Maze/Solver のクラス化 + PC テスト]** ここで初めてロジックをクラスへ。
   googletest か素朴な assert で迷路データ 10 本を回す
6. **[Motion/Motor のクラス化]** ISR との接続を extern "C" 経由に整理
7. **[constexpr テーブル]** table.h を削除
8. 各段階でコミットし、**実機の走行確認を挟んでから次へ**進む

## 4. ハマりどころ集

| 症状 | 原因 |
|---|---|
| 割り込みが飛ばなくなった | ISR/HAL コールバックの `extern "C"` 忘れ（名前修飾） |
| リンクエラー `undefined reference to __cxa_...` | 例外/RTTI/atexit 系が有効のまま。§1 のフラグを確認 |
| 起動直後に HardFault | グローバルオブジェクトのコンストラクタが HAL 初期化前に走り、未初期化ペリフェラルへアクセス（静的初期化順序問題）。コンストラクタではペリフェラルを触らず、`init()` メソッドを `app_main` から呼ぶ |
| Flash が数 KB 太った | float の printf、`__cxa_atexit`、仮想関数テーブル。map ファイル（`Debug/*.map`）で犯人を特定する習慣をつける |
| たまに値が壊れる | ISR と共有する変数の `volatile` をクラス化の際に落とした |

## 5. 参考にすべき先行事例

マイクロマウス界隈には C++ 実装の公開リポジトリが複数ある（例: kerikun11 氏の
`micromouse-maze-library` は迷路クラス設計と探索アルゴリズムの実装が整理されており、
`Maze`/`Solver` 設計の手本になる）。丸写しではなく設計の読み比べ教材として使うとよい。
