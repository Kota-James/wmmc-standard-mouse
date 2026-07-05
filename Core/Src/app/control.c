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

    // ================= 課題2 =================
    // TODO: 偏差 dif_l / dif_r から操作量を計算し，
    //       hw_motor_set_steering(left, right) でモータに反映する（比例制御）
    //
    // 使うもの:
    //   dif_l, dif_r … 基準値からの偏差（正 = 壁に近づきすぎ。上で計算済み）
    //   CTRL_BASE_L, CTRL_BASE_R … 不感帯（この偏差までは操作しない）
    //   CTRL_CONT               … 比例ゲイン
    //   CTRL_MAX                … 操作量の上限（clamp関数を使うとよい）
    //
    // 操作量は「正の値を渡した側の車輪が速くなる」。
    // 左壁に近づいたらどちらを速くすべきか，図を書いて考えること。
    // 注意: この関数は割り込み内で動く。printfや長いループは書かないこと。
    // 詳細: docs/exercises/02_壁沿い制御.md
    // =========================================
    hw_motor_set_steering(0, 0); // 実装したらこの行は消す
    (void)clamp;                 // 実装したらこの行は消す（未使用警告よけ）
}
