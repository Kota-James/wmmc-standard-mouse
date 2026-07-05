/*
 * wall.h — 壁判定（センサ値から壁の有無を判断する）
 *
 * 壁情報 wall_info のビット割り当て（機体から見た向き）:
 *   0x88 = 前壁, 0x44 = 右壁, 0x22 = 後壁, 0x11 = 左壁
 * 上位4bitと下位4bitに同じパターンを重ねて持つことで，
 * 進行方向(maze.hのroute形式)とのAND演算だけで壁の有無を判定できる。
 */

#ifndef APP_WALL_H_
#define APP_WALL_H_

#include <stdint.h>

//----壁情報ビット----
#define WALL_FRONT 0x88
#define WALL_RIGHT 0x44
#define WALL_REAR 0x22
#define WALL_LEFT 0x11

extern uint8_t wall_info;          // 最新の壁情報
extern uint16_t base_l, base_r;    // 壁制御の基準値（区画中央にいるときのセンサ値）

void wall_init(void);
void get_wall_info(void); // センサ値と閾値を比べてwall_infoを更新する
uint8_t get_base(void);   // 現在のセンサ値を制御基準値として記憶する

#endif /* APP_WALL_H_ */
