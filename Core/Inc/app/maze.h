/*
 * maze.h — 迷路情報の管理（自機位置・壁マップ）
 *
 * ---座標系---
 * スタート地点が左下(0,0)になるような位置から迷路を見たとき，
 * 上方向を北，右方向を東，下方向を南，左方向を西と定義する。
 * 方角は 北=0x00, 東=0x01, 南=0x02, 西=0x03 で表す。
 *
 * ---壁マップ map[y][x] のデータ形式---
 * 各バイトは上位4bitと下位4bitに分かれ，それぞれ
 *   bit3=北壁, bit2=東壁, bit1=南壁, bit0=西壁（1=壁あり）
 * を表す。下位4bitは一次走行用（未探索の壁は「なし」と扱う），
 * 上位4bitは二次走行用（未探索の壁は「あり」と扱う）。
 * 探索で確認した壁は両方のニブルに同じ値が書かれる。
 *
 * このモジュールはハードウェアに依存しない（PC上でもそのまま動く）。
 * 例外はEEPROMへの保存・読み出しで，hw_eeprom.h（境界API）だけを使う。
 */

#ifndef APP_MAZE_H_
#define APP_MAZE_H_

#include <stdint.h>

#define MAZE_SIZE 16 // 迷路の一辺の区画数（クラシック競技は16）

//----方角----
#define DIR_NORTH 0x00
#define DIR_EAST 0x01
#define DIR_SOUTH 0x02
#define DIR_WEST 0x03

//----方向転換（mouse.dirに加算する値）----
#define DIR_TURN_R90 0x01 // 右90度回転
#define DIR_TURN_L90 0xff // 左90度回転（-1と同じ。&0x03で巻き戻る）
#define DIR_TURN_180 0x02 // 180度回転

//----自機の現在地と方角----
struct mouse_state {
    uint8_t x;
    uint8_t y;
    uint8_t dir;
};

extern struct mouse_state mouse;
extern uint8_t map[MAZE_SIZE][MAZE_SIZE]; // 壁マップ
extern uint8_t goal_x, goal_y; // ゴール領域の南西角の座標
extern uint8_t goal_size; // ゴール領域の一辺の区画数
                          // 日本のクラシック競技はゴールが中央2×2領域
                          // （(7,7)〜(8,8)）に固定されているため通常は2。
                          // スタートに戻る走行では(0,0)の1区画にするため1

// (x,y)がゴール領域内なら1を返す
uint8_t maze_is_goal(uint8_t x, uint8_t y);

void maze_init(void);

// 二次走行モードの切り替え（1: 上位ニブル=未探索壁ありで判断する）
void maze_set_second_run(uint8_t second);
uint8_t maze_is_second_run(void);

// 指定区画の壁情報4bit（NESW）を走行モードに応じたニブルから取り出す
uint8_t maze_wall_bits(uint8_t x, uint8_t y);

// 自機位置の壁情報（wall.hの形式，自機から見た向き）をマップに書き込む
void maze_write_walls(uint8_t wall_info);

void maze_advance_position(void); // 内部位置情報を1区画前進させる
void maze_turn(uint8_t turn);     // 内部方角情報を回転させる（DIR_TURN_*）

//----EEPROMへの保存・読み出し（二次走行のため）----
void maze_store_to_eeprom(void);
void maze_load_from_eeprom(void);
uint8_t maze_in_eeprom_is_valid(void); // 有効な保存データがあるか

#endif /* APP_MAZE_H_ */
