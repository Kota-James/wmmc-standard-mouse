/*
 * motion.c — 走行動作の実装
 *
 * 下位の基幹関数 drive_*()（このファイル内のstatic関数）でパルス数を指定して走り，
 * それを組み合わせて「半区画進む」「90度回る」といった動作を作っている。
 */

#include <stdio.h>
#include "app/motion.h"
#include "app/params.h"
#include "app/wall.h"
#include "app/control.h"
#include "app/ui.h"
#include "hw/hw_motor.h"
#include "hw/hw_system.h"

//+++++++++++++++++++++++++++++++++++++++++++++++
// motion_wait
// 機体が安定するまで待機する（旋回や停止の直後に呼ぶ）
//+++++++++++++++++++++++++++++++++++++++++++++++
void motion_wait(void) { hw_delay_ms(50); }

/*==========================================================
    基幹関数（パルス数を指定して走る）
==========================================================*/
//+++++++++++++++++++++++++++++++++++++++++++++++
// drive_accel
// 指定パルス分加速しながら走行する（走行後は停止しない）
//+++++++++++++++++++++++++++++++++++++++++++++++
static void drive_accel(uint16_t dist) {
    hw_motor_set_profile(MOTOR_PROFILE_ACCEL);
    hw_motor_reset_speed(); // 最低速から加速を開始する
    hw_motor_start();

    // 左右のモータが指定パルス以上進むまで待機
    while ((hw_motor_pulse_left() < dist) || (hw_motor_pulse_right() < dist))
        ;

    hw_motor_stop();
}

//+++++++++++++++++++++++++++++++++++++++++++++++
// drive_decel
// 指定パルス分減速しながら走行して停止する
// （減速に必要な距離になるまでは等速で走る）
//+++++++++++++++++++++++++++++++++++++++++++++++
static void drive_decel(uint16_t dist) {
    hw_motor_set_profile(MOTOR_PROFILE_CONST);
    hw_motor_start();

    // 等速走行距離 = 総距離 - 減速に必要な距離
    int16_t c_pulse = dist - hw_motor_decel_pulses();
    if (c_pulse > 0) {
        while ((hw_motor_pulse_left() < c_pulse) ||
               (hw_motor_pulse_right() < c_pulse))
            ;
    }

    // 減速走行
    hw_motor_set_profile(MOTOR_PROFILE_DECEL);
    while ((hw_motor_pulse_left() < dist) || (hw_motor_pulse_right() < dist))
        ;

    hw_motor_stop();
}

//+++++++++++++++++++++++++++++++++++++++++++++++
// drive_const
// 指定パルス分等速走行する（現在の速度を維持する）
//+++++++++++++++++++++++++++++++++++++++++++++++
static void drive_const(uint16_t dist) {
    hw_motor_set_profile(MOTOR_PROFILE_CONST);
    hw_motor_start();

    while ((hw_motor_pulse_left() < dist) || (hw_motor_pulse_right() < dist))
        ;

    hw_motor_stop();
}

//+++++++++++++++++++++++++++++++++++++++++++++++
// drive_turn_speed
// 指定パルス分を旋回用の一定速度で走行して停止する（旋回・尻当てに使う）
//+++++++++++++++++++++++++++++++++++++++++++++++
static void drive_turn_speed(uint16_t dist) {
    hw_motor_set_profile(MOTOR_PROFILE_TURN);
    hw_motor_start();

    while ((hw_motor_pulse_left() < dist) || (hw_motor_pulse_right() < dist))
        ;

    hw_motor_stop();
}

/*==========================================================
    区画単位の走行
==========================================================*/
void motion_init(void) {
    hw_motor_config_t config = {
        .max_speed_index = MAX_T_CNT,
        .turn_interval_us = DEFAULT_INTERVAL,
    };
    hw_motor_init(&config);
}

//+++++++++++++++++++++++++++++++++++++++++++++++
// motion_half_section_accel
// 半区画分加速しながら走行する。走行後に壁情報を更新する
//+++++++++++++++++++++++++++++++++++++++++++++++
void motion_half_section_accel(void) {
    control_enable(); // 壁制御を有効にする
    drive_accel(PULSE_SEC_HALF);
    get_wall_info(); // 区画の境目で壁情報を取得する
}

//+++++++++++++++++++++++++++++++++++++++++++++++
// motion_half_section_decel
// 半区画分減速しながら走行し停止する
//+++++++++++++++++++++++++++++++++++++++++++++++
void motion_half_section_decel(void) {
    control_enable();
    drive_decel(PULSE_SEC_HALF);
}

//+++++++++++++++++++++++++++++++++++++++++++++++
// motion_one_section
// 1区画分進んで停止する（前半で加速し後半で減速する）
//+++++++++++++++++++++++++++++++++++++++++++++++
void motion_one_section(void) {
    motion_half_section_accel();
    motion_half_section_decel();
}

//+++++++++++++++++++++++++++++++++++++++++++++++
// motion_one_section_const
// 等速のまま1区画分進む（連続走行用）。走行後に壁情報を更新する
//+++++++++++++++++++++++++++++++++++++++++++++++
void motion_one_section_const(void) {
    control_enable();
    drive_const(PULSE_SEC_HALF);
    drive_const(PULSE_SEC_HALF);
    get_wall_info();
}

/*==========================================================
    旋回・位置合わせ
==========================================================*/
//+++++++++++++++++++++++++++++++++++++++++++++++
// motion_rotate_right90 / left90 / 180
// その場で超信地旋回する（壁制御は無効にする）
//+++++++++++++++++++++++++++++++++++++++++++++++
void motion_rotate_right90(void) {
    control_disable();
    hw_motor_set_direction(MOTOR_DIR_ROTATE_RIGHT);
    motion_wait();
    drive_turn_speed(PULSE_ROT_R90);
    motion_wait();
    hw_motor_set_direction(MOTOR_DIR_FORWARD);
}

void motion_rotate_left90(void) {
    control_disable();
    hw_motor_set_direction(MOTOR_DIR_ROTATE_LEFT);
    motion_wait();
    drive_turn_speed(PULSE_ROT_L90);
    motion_wait();
    hw_motor_set_direction(MOTOR_DIR_FORWARD);
}

void motion_rotate_180(void) {
    control_disable();
    hw_motor_set_direction(MOTOR_DIR_ROTATE_RIGHT); // 右回りで180度回る
    motion_wait();
    drive_turn_speed(PULSE_ROT_180);
    motion_wait();
    hw_motor_set_direction(MOTOR_DIR_FORWARD);
}

//+++++++++++++++++++++++++++++++++++++++++++++++
// motion_set_position
// 機体の尻を後ろ壁に当てて位置を区画中央に合わせる（尻当て）
//+++++++++++++++++++++++++++++++++++++++++++++++
void motion_set_position(uint8_t with_get_base) {
    control_disable();
    hw_motor_set_direction(MOTOR_DIR_BACK);
    motion_wait();
    drive_turn_speed(PULSE_SETPOS_BACK); // 尻が壁に当たるまで後退する
    motion_wait();
    if (with_get_base) {
        get_base(); // 壁に正対した状態で制御基準値を取り直す
    }
    hw_motor_set_direction(MOTOR_DIR_FORWARD);
    motion_wait();
    drive_turn_speed(PULSE_SETPOS_SET); // 区画中央まで前進する
    motion_wait();
}

/*==========================================================
    テスト走行モード（パラメータ調整用）
==========================================================*/
//+++++++++++++++++++++++++++++++++++++++++++++++
// motion_test_run
// params.h の各パルス数を調整するためのサブメニュー
// 旋回は16回（8回）連続で行い、誤差を積算させて見えやすくする
//+++++++++++++++++++++++++++++++++++++++++++++++
void motion_test_run(void) {
    int mode = 0;
    int i;

    hw_motor_enable();

    while (1) {
        mode = select_mode(mode);

        switch (mode) {
        case 0:
            //----尻当て----
            printf("Set Position.\n");
            motion_set_position(0);
            break;
        case 1:
            //----1区画等速走行----
            printf("1 Section, Forward, Constant Speed.\n");
            control_disable();
            hw_motor_set_direction(MOTOR_DIR_FORWARD);
            drive_turn_speed(PULSE_SEC_HALF * 2);
            motion_wait();
            break;
        case 2:
            //----右90度回転×16----
            printf("Rotate R90.\n");
            for (i = 0; i < 16; i++) {
                motion_rotate_right90();
            }
            break;
        case 3:
            //----左90度回転×16----
            printf("Rotate L90.\n");
            for (i = 0; i < 16; i++) {
                motion_rotate_left90();
            }
            break;
        case 4:
            //----180度回転×8----
            printf("Rotate 180.\n");
            for (i = 0; i < 8; i++) {
                motion_rotate_180();
            }
            break;
        case 5:
        case 6:
            break;
        case 7:
            //----6区画連続走行----
            printf("6 Section, Forward, Continuous.\n");
            control_disable();
            hw_motor_set_direction(MOTOR_DIR_FORWARD);
            drive_accel(PULSE_SEC_HALF);
            for (i = 0; i < 6 - 1; i++) {
                drive_const(PULSE_SEC_HALF * 2);
            }
            drive_decel(PULSE_SEC_HALF);
            break;
        }
    }
}
