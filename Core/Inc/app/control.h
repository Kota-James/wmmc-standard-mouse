/*
 * control.h — 壁沿い制御（壁との距離を一定に保つフィードバック制御）
 *
 * 有効な間，12ms周期でセンサ値と基準値の差(dif)から操作量を計算し，
 * 左右のモータ速度をわずかに変えて機体を区画中央に寄せる。
 */

#ifndef APP_CONTROL_H_
#define APP_CONTROL_H_

#include <stdint.h>

// AD値と基準値の差（センサチェックモードの表示にも使う）
extern volatile int16_t dif_l, dif_r;

void control_init(void);
void control_enable(void);  // 壁制御を有効にする（直進の前に呼ぶ）
void control_disable(void); // 壁制御を無効にする（旋回の前に呼ぶ）

#endif /* APP_CONTROL_H_ */
