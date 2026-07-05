/*
 * hw_isr.c — タイマ割り込みの振り分け
 *
 * HALはどのタイマの割り込みでも共通のコールバックを呼ぶので，
 * ここでタイマを判別して各モジュールのISR処理に振り分ける。
 */

#include "main.h"
#include "hw/hw_motor.h"
#include "hw/hw_sensor.h"

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == htim16.Instance) {
        hw_motor_isr_pulse_left(); // 左モータの1パルスごとの処理
    } else if (htim->Instance == htim17.Instance) {
        hw_motor_isr_pulse_right(); // 右モータの1パルスごとの処理
    } else if (htim->Instance == htim6.Instance) {
        hw_sensor_isr_tick(); // センサ取得・制御の周期タスク
    }
}
