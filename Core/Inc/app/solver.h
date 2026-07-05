/*
 * solver.h — 経路探索（足立法）
 *
 * ゴールから各区画までの歩数を書き込んだ「歩数マップ」を作り，
 * 現在地から歩数が減る方向へたどることで最短経路を導出する。
 *
 * ---最短経路 route[] のデータ形式---
 * 進行順に「次にとる動作」が入る:
 *   0x88=前進, 0x44=右折, 0x22=Uターン, 0x11=左折, 0x00=それ以外
 * wall.h の壁情報と同じビット割り当てなので，
 * (wall_info & route[r_cnt]) だけで「次の進路に壁があるか」を判定できる。
 *
 * このモジュールはハードウェアに依存しない（PC上でもそのまま動く）。
 */

#ifndef APP_SOLVER_H_
#define APP_SOLVER_H_

#include <stdint.h>
#include "app/maze.h"

extern uint8_t smap[MAZE_SIZE][MAZE_SIZE]; // 歩数マップ
extern uint8_t route[256];                 // 最短経路（動作の列）
extern uint8_t r_cnt;                      // 経路カウンタ（routeの現在位置）

void solver_init(void);

void solver_make_step_map(void); // 歩数マップを作成する
void solver_make_route(void);    // 歩数マップから最短経路route[]を導出する

// 進んだ直後に呼ぶ。壁情報をマップに書き込み，
// 次の進路が壁で塞がれていれば経路を作り直す
void solver_update_route(uint8_t wall_info);

#endif /* APP_SOLVER_H_ */
