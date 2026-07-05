/*
 * strategy.h — 走行戦略（探索走行・二次走行のシナリオ）
 */

#ifndef APP_STRATEGY_H_
#define APP_STRATEGY_H_

#include <stdint.h>

// 探索走行の一連のシナリオを実行する:
// 尻当てで位置合わせ → 制御基準値取得 → ゴールまで探索走行 →
// 帰り探索 → （一次走行なら）マップをEEPROMに保存
// second_run: 0=一次走行（未探索壁なし扱い） 1=二次走行（保存済みマップ使用）
void strategy_run(uint8_t second_run);

#endif /* APP_STRATEGY_H_ */
