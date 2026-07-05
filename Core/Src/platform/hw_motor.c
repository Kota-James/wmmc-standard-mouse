/*
 * hw_motor.c — ステッピングモータ制御（STM32F303K8実装）
 *
 * ---走行の仕組み---
 * ステッピングモータの動作制御は16bitタイマで行う。
 *   TIM16: 左モータ（出力 TIM16_CH1 → モータドライバのClock）
 *   TIM17: 右モータ（出力 TIM17_CH1 → モータドライバのClock）
 * 各タイマは，
 *   ・カウント開始からCCR1までの間は出力ピンがLow
 *   ・CCR1に達すると出力ピンがHigh
 *   ・ARRに達すると割り込みを生成しタイマカウンタをリセット
 * となっている。ClockがHighになるたびモータは一定角度回転するので，
 * ARR（＝パルス間隔）を短くするほど回転が速くなる。
 *
 * 加減速は，事前計算した「割り込みごとのARR値」の列 motor_table.inc を
 * 1パルスごとに進む/戻ることで行う（テーブル駆動方式）。
 * 現在の速度（＝テーブルの何番目か）は t_cnt_l / t_cnt_r が保持する。
 */

#include "main.h"
#include "hw/hw_motor.h"

//----加減速テーブル----
static const uint16_t table[] = {
#include "motor_table.inc"
};

//----動作方向のGPIO出力値（配線で決まる。機体固有）----
#define MT_FWD_L GPIO_PIN_SET    // CW/CCWで前に進む出力（左）
#define MT_BACK_L GPIO_PIN_RESET // CW/CCWで後ろに進む出力（左）
#define MT_FWD_R GPIO_PIN_RESET  // CW/CCWで前に進む出力（右）
#define MT_BACK_R GPIO_PIN_SET   // CW/CCWで後ろに進む出力（右）

//----状態（割り込みと共有するためvolatile）----
static volatile motor_profile_t profile = MOTOR_PROFILE_CONST;
static volatile int16_t t_cnt_l, t_cnt_r;     // テーブルカウンタ（現在の速度）
static volatile int16_t min_t_cnt, max_t_cnt; // テーブルカウンタの範囲
static volatile uint16_t pulse_l, pulse_r;    // 走行開始からのパルス数
static volatile int16_t steer_l, steer_r;     // 壁制御の操作量[µs]
static uint16_t turn_interval_us;             // 旋回時のパルス間隔[µs]

//+++++++++++++++++++++++++++++++++++++++++++++++
// hw_motor_init
// 走行系の変数の初期化とモータドライバの初期状態設定
//+++++++++++++++++++++++++++++++++++++++++++++++
void hw_motor_init(const hw_motor_config_t *config) {
    min_t_cnt = 0; // テーブル先頭＝最低速から使う（現状これを変える理由がないため固定）
    max_t_cnt = config->max_speed_index;
    turn_interval_us = config->turn_interval_us;
    t_cnt_l = t_cnt_r = min_t_cnt;
    steer_l = steer_r = 0;
    profile = MOTOR_PROFILE_CONST;

    hw_motor_disable();
    hw_motor_set_direction(MOTOR_DIR_FORWARD);

    __HAL_TIM_SET_AUTORELOAD(&htim16, turn_interval_us);
    __HAL_TIM_SET_AUTORELOAD(&htim17, turn_interval_us);
}

void hw_motor_enable(void) {
    HAL_GPIO_WritePin(M3_GPIO_Port, M3_Pin, GPIO_PIN_RESET); // 励磁ON
    HAL_GPIO_WritePin(M3_2_GPIO_Port, M3_2_Pin, GPIO_PIN_RESET);
}

void hw_motor_disable(void) {
    HAL_GPIO_WritePin(M3_GPIO_Port, M3_Pin, GPIO_PIN_SET); // 励磁OFF
    HAL_GPIO_WritePin(M3_2_GPIO_Port, M3_2_Pin, GPIO_PIN_SET);
}

void hw_motor_set_direction(motor_direction_t dir) {
    GPIO_PinState left, right;
    switch (dir) {
    case MOTOR_DIR_BACK:
        left = MT_BACK_L;
        right = MT_BACK_R;
        break;
    case MOTOR_DIR_ROTATE_LEFT: // 左後退・右前進
        left = MT_BACK_L;
        right = MT_FWD_R;
        break;
    case MOTOR_DIR_ROTATE_RIGHT: // 左前進・右後退
        left = MT_FWD_L;
        right = MT_BACK_R;
        break;
    case MOTOR_DIR_FORWARD:
    default:
        left = MT_FWD_L;
        right = MT_FWD_R;
        break;
    }
    HAL_GPIO_WritePin(CW_CCW_L_GPIO_Port, CW_CCW_L_Pin, left);
    HAL_GPIO_WritePin(CW_CCW_R_GPIO_Port, CW_CCW_R_Pin, right);
    HAL_GPIO_WritePin(CW_CCW_R_2_GPIO_Port, CW_CCW_R_2_Pin, right);
}

void hw_motor_set_profile(motor_profile_t p) { profile = p; }

void hw_motor_reset_speed(void) { t_cnt_l = t_cnt_r = min_t_cnt; }

//+++++++++++++++++++++++++++++++++++++++++++++++
// hw_motor_start
// パルスカウンタをリセットしてPWMタイマを有効にする
//+++++++++++++++++++++++++++++++++++++++++++++++
void hw_motor_start(void) {
    pulse_l = pulse_r = 0;

    __HAL_TIM_CLEAR_FLAG(&htim16, TIM_FLAG_UPDATE);
    __HAL_TIM_ENABLE_IT(&htim16, TIM_IT_UPDATE);
    HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);

    __HAL_TIM_CLEAR_FLAG(&htim17, TIM_FLAG_UPDATE);
    __HAL_TIM_ENABLE_IT(&htim17, TIM_IT_UPDATE);
    HAL_TIM_PWM_Start(&htim17, TIM_CHANNEL_1);
}

//+++++++++++++++++++++++++++++++++++++++++++++++
// hw_motor_stop
// PWMタイマを止めてカウント値をリセットする
//+++++++++++++++++++++++++++++++++++++++++++++++
void hw_motor_stop(void) {
    HAL_TIM_PWM_Stop(&htim16, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim17, TIM_CHANNEL_1);
    __HAL_TIM_SET_COUNTER(&htim16, 0);
    __HAL_TIM_SET_COUNTER(&htim17, 0);
}

uint16_t hw_motor_pulse_left(void) { return pulse_l; }
uint16_t hw_motor_pulse_right(void) { return pulse_r; }

int16_t hw_motor_decel_pulses(void) { return t_cnt_l - min_t_cnt; }

void hw_motor_set_steering(int16_t left, int16_t right) {
    steer_l = left;
    steer_r = right;
}

/*==========================================================
    タイマ割り込みからの1パルスごとの処理
    プロファイルに応じてテーブルカウンタを進め，次のパルス間隔をARRに設定する
==========================================================*/
void hw_motor_isr_pulse_left(void) {
    pulse_l++;
    switch (profile) {
    case MOTOR_PROFILE_ACCEL:
        if (t_cnt_l < max_t_cnt) t_cnt_l++;
        break;
    case MOTOR_PROFILE_DECEL:
        if (t_cnt_l > min_t_cnt) t_cnt_l--;
        break;
    default:
        break;
    }
    uint16_t interval =
        (profile == MOTOR_PROFILE_TURN) ? turn_interval_us : table[t_cnt_l];
    __HAL_TIM_SET_AUTORELOAD(&htim16, interval - steer_l);
}

void hw_motor_isr_pulse_right(void) {
    pulse_r++;
    switch (profile) {
    case MOTOR_PROFILE_ACCEL:
        if (t_cnt_r < max_t_cnt) t_cnt_r++;
        break;
    case MOTOR_PROFILE_DECEL:
        if (t_cnt_r > min_t_cnt) t_cnt_r--;
        break;
    default:
        break;
    }
    uint16_t interval =
        (profile == MOTOR_PROFILE_TURN) ? turn_interval_us : table[t_cnt_r];
    __HAL_TIM_SET_AUTORELOAD(&htim17, interval - steer_r);
}
