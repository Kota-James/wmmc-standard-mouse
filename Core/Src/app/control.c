/*
 * control.c — 壁沿い制御（比例制御）の実装
 *
 * app_control_tick() はタイマ割り込み（12ms周期）から呼ばれる。
 * 割り込み内で動くので，printfや長いループを書いてはいけない。
 */

#include "app/control.h"
#include "app/params.h"
#include "app/wall.h"
#include "hw/hw_motor.h"
#include "hw/hw_sensor.h"
#include "hw/app_hooks.h"

volatile int16_t dif_l, dif_r;

static volatile uint8_t enabled; // 壁制御が有効か

void control_init(void) {
    enabled = 0;
    dif_l = dif_r = 0;
}

void control_enable(void) { enabled = 1; }
void control_disable(void) { enabled = 0; }

//----範囲制限----
static int16_t clamp(int16_t v, int16_t lo, int16_t hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

//+++++++++++++++++++++++++++++++++++++++++++++++
// app_control_tick
// 12ms周期の制御処理（platform層のタイマ割り込みから呼ばれる）
//+++++++++++++++++++++++++++++++++++++++++++++++
void app_control_tick(void) {
    // 基準値からの差を見る（表示にも使うため制御の有無に関わらず更新する）
    dif_l = (int32_t)hw_sensor_left() - base_l;
    dif_r = (int32_t)hw_sensor_right() - base_r;

    if (!enabled) {
        hw_motor_set_steering(0, 0); // 制御が無効なら操作量0
        return;
    }

    // 比例制御: 壁に近づいた（＝基準値より大きい）側と逆へ操作する
    int16_t steer_l = 0, steer_r = 0;
    if (CTRL_BASE_L < dif_l) { // 左壁に近づきすぎた場合
        steer_l += -1 * CTRL_CONT * dif_l;
        steer_r += CTRL_CONT * dif_l;
    }
    if (CTRL_BASE_R < dif_r) { // 右壁に近づきすぎた場合
        steer_l += CTRL_CONT * dif_r;
        steer_r += -1 * CTRL_CONT * dif_r;
    }

    // 操作量が大きくなりすぎないよう制限してモータへ反映する
    hw_motor_set_steering(clamp(steer_l, -CTRL_MAX, CTRL_MAX),
                          clamp(steer_r, -CTRL_MAX, CTRL_MAX));
}
