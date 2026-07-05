/*
 * hw_motor.h — ステッピングモータ制御の境界API
 *
 * ここより上の層（Core/Src/app/）はこのヘッダの関数だけでモータを扱う。
 * タイマ・GPIO などマイコン固有の実装は Core/Src/platform/hw_motor.c にある。
 * このヘッダに HAL の型や関数を書いてはいけない（マイコン移植性のため）。
 */

#ifndef HW_HW_MOTOR_H_
#define HW_HW_MOTOR_H_

#include <stdint.h>

//----進行方向----
typedef enum {
    MOTOR_DIR_FORWARD,      // 前進
    MOTOR_DIR_BACK,         // 後退
    MOTOR_DIR_ROTATE_LEFT,  // 左超信地旋回（左後退・右前進）
    MOTOR_DIR_ROTATE_RIGHT, // 右超信地旋回（左前進・右後退）
} motor_direction_t;

//----速度プロファイル（パルスごとのパルス間隔の決め方）----
typedef enum {
    MOTOR_PROFILE_ACCEL, // 加減速テーブルを1パルスごとに進む（＝加速）
    MOTOR_PROFILE_DECEL, // 加減速テーブルを1パルスごとに戻る（＝減速）
    MOTOR_PROFILE_CONST, // テーブルの現在位置を維持（＝等速）
    MOTOR_PROFILE_TURN,  // テーブルを使わず旋回用の一定速度で回る
} motor_profile_t;

//----初期化設定----
typedef struct {
    int16_t max_speed_index;   // 加減速テーブルをどこまで進むか（＝最高速度）
    uint16_t turn_interval_us; // MOTOR_PROFILE_TURN時のパルス間隔[µs]
} hw_motor_config_t;

void hw_motor_init(const hw_motor_config_t *config);

void hw_motor_enable(void);  // 励磁ON（トルクが出る。電流も流れる）
void hw_motor_disable(void); // 励磁OFF

void hw_motor_set_direction(motor_direction_t dir);
void hw_motor_set_profile(motor_profile_t profile);
void hw_motor_reset_speed(void); // 速度を最低速（テーブル先頭）に戻す。加速走行の開始前に呼ぶ

void hw_motor_start(void); // パルス数を0にリセットして走行開始
void hw_motor_stop(void);  // 走行停止

// 走行開始からのパルス数（走行距離の計測に使う）
uint16_t hw_motor_pulse_left(void);
uint16_t hw_motor_pulse_right(void);

// 現在の速度から最低速まで減速するのに必要なパルス数（減速開始タイミングの計算に使う）
int16_t hw_motor_decel_pulses(void);

// 壁制御の操作量[µs]。パルス間隔から引かれる（正の値=速くなる）
// 左右で逆符号を与えると機体が曲がる。割り込みから毎周期参照される
void hw_motor_set_steering(int16_t left, int16_t right);

// タイマ割り込みハンドラから呼ばれる（platform内部用。appから呼んではいけない）
void hw_motor_isr_pulse_left(void);
void hw_motor_isr_pulse_right(void);

#endif /* HW_HW_MOTOR_H_ */
