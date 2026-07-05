/*
 * motion.h — 走行動作（半区画走行・旋回・尻当てなど）
 *
 * 探索アルゴリズム側はこの関数を並べるだけで機体を動かせる。
 * すべてブロッキング動作（走り終わるまで関数から戻らない）。
 */

#ifndef APP_MOTION_H_
#define APP_MOTION_H_

#include <stdint.h>

void motion_init(void);

//----区画単位の走行----
void motion_half_section_accel(void); // 半区画分加速しながら走行する（止まらない）
void motion_half_section_decel(void); // 半区画分減速しながら走行し停止する
void motion_one_section(void);        // 1区画分進んで停止する（加速→減速）
                                      // ※連続走行では使わない。1区画ごとに止まる
                                      //   確実な探索を自作したいときに使う
void motion_one_section_const(void);  // 等速のまま1区画分進む（止まらない）

//----旋回----
void motion_rotate_right90(void);
void motion_rotate_left90(void);
void motion_rotate_180(void);

//----位置合わせ----
// 機体の尻を後ろ壁に当てて位置を区画中央に合わせる
// with_get_base: 0以外なら壁に当たった状態で制御基準値も取り直す
void motion_set_position(uint8_t with_get_base);

//----補助----
void motion_wait(void); // 機体が安定するまで待機（動作の間に挟む）

//----テスト走行モード（パラメータ調整用）----
void motion_test_run(void);

#endif /* APP_MOTION_H_ */
