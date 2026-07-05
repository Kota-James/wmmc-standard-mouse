/*
 * strategy.c — 走行戦略の実装
 *
 * 足立法探索走行（連続走行）:
 * 「壁を見る → マップに記録 → 経路を作る → 経路に従って動く」を
 * ゴールに着くまで繰り返す。途中で経路上に壁が見つかれば経路を作り直す。
 */

#include <stdio.h>
#include "app/strategy.h"
#include "app/maze.h"
#include "app/solver.h"
#include "app/motion.h"
#include "app/wall.h"
#include "app/params.h"
#include "hw/hw_motor.h"
#include "hw/hw_sensor.h"
#include "hw/hw_system.h"

//+++++++++++++++++++++++++++++++++++++++++++++++
// turn_around_with_touch_up
// Uターンする。前後の壁が確実にあるときは，2回の90度回転の間に
// 尻当てを挟んで位置と角度の誤差をリセットする（探索が長くなるほど効く）
//+++++++++++++++++++++++++++++++++++++++++++++++
static void turn_around_with_touch_up(void) {
    if (hw_sensor_front_right() >= WALL_BASE_FR * 1.5 &&
        hw_sensor_front_left() >= WALL_BASE_FL * 1.5 &&
        hw_sensor_right() >= WALL_BASE_R * 1.5) {
        // 前壁と右壁が確実に有る場合（左90度回転×2の間に右壁→前壁の順で尻当てできる）
        motion_rotate_left90();
        motion_wait();
        motion_set_position(0);
        motion_wait();
        motion_rotate_left90();
        motion_wait();
        motion_set_position(0);
        motion_wait();
    } else if (hw_sensor_front_right() >= WALL_BASE_FR * 1.5 &&
               hw_sensor_front_left() >= WALL_BASE_FL * 1.5 &&
               hw_sensor_left() >= WALL_BASE_L) {
        // それ以外で前壁と左壁が有る場合（右90度回転×2で同様に尻当てする）
        motion_rotate_right90();
        motion_wait();
        motion_set_position(0);
        motion_wait();
        motion_rotate_right90();
        motion_wait();
        motion_set_position(0);
        motion_wait();
    } else {
        // 尻当てに使える壁がなければそのまま180度回転する
        motion_rotate_180();
    }
}

//+++++++++++++++++++++++++++++++++++++++++++++++
// search_to_goal
// 現在位置からgoal座標まで足立法で連続探索走行する
//+++++++++++++++++++++++++++++++++++++++++++++++
static void search_to_goal(void) {
    if (maze_is_second_run()) {
        maze_load_from_eeprom(); // 二次走行時は保存済みマップを使う
    }

    //====スタート位置の壁情報取得====
    get_wall_info();
    wall_info &= ~WALL_FRONT; // スタート区画の前壁は存在しないはずなので消す
    maze_write_walls(wall_info);

    //====前に壁が無い前提で最初の半区画を前進====
    motion_half_section_accel();
    maze_advance_position();
    maze_write_walls(wall_info);

    //====歩数マップ・経路作成====
    r_cnt = 0;
    solver_make_step_map();
    solver_make_route();

    //====探索走行====
    do {
        //----経路の次の動作で進行----
        switch (route[r_cnt++]) {
        case 0x88: //----前進----
            motion_one_section_const();
            break;
        case 0x44: //----右折----
            motion_half_section_decel();
            motion_rotate_right90();
            maze_turn(DIR_TURN_R90); // 内部方角情報も右回転
            motion_half_section_accel();
            break;
        case 0x22: //----Uターン----
            motion_half_section_decel();
            turn_around_with_touch_up();
            maze_turn(DIR_TURN_180); // 内部方角情報も180度回転
            motion_half_section_accel();
            break;
        case 0x11: //----左折----
            motion_half_section_decel();
            motion_rotate_left90();
            maze_turn(DIR_TURN_L90); // 内部方角情報も左回転
            motion_half_section_accel();
            break;
        }

        maze_advance_position();        // 内部位置情報を前進
        solver_update_route(wall_info); // 壁を記録し必要なら経路を作り直す

    } while (!maze_is_goal(mouse.x, mouse.y));
    // ゴール領域（2×2のどの区画でもよい）に入るまで実行

    motion_half_section_decel(); // 区画中央で停止する

    hw_delay_ms(2200); // スタートでは2秒以上停止しなくてはならない（競技規定）
    motion_rotate_180();
    maze_turn(DIR_TURN_180);

    if (!maze_is_second_run()) {
        maze_store_to_eeprom(); // 一次走行で得たマップを保存する
    }
}

//+++++++++++++++++++++++++++++++++++++++++++++++
// strategy_run
// 探索走行の一連のシナリオ（モード1・2の本体）
//+++++++++++++++++++++++++++++++++++++++++++++++
void strategy_run(uint8_t second_run) {
#if BATT_CHECK_ENABLED
    if (hw_sensor_battery_is_low()) {
        printf("Battery voltage low. Run aborted.\n");
        return; // 電圧不足のまま走ると脱調や過放電の原因になる（params.h参照）
    }
#endif

    hw_motor_enable(); // ステッピングモータを励磁する

    maze_set_second_run(second_run);
    goal_x = GOAL_X;
    goal_y = GOAL_Y;
    goal_size = GOAL_SIZE; // 往路のゴールは中央2×2領域

    //====尻当てで位置と向きを合わせる====
    // 右を向いて尻当て→左を向き直して尻当てすると，
    // 横方向・縦方向の両方の位置と角度が揃う
    motion_rotate_right90();
    motion_wait();
    motion_set_position(0);
    motion_wait();
    motion_rotate_left90();
    motion_wait();
    motion_set_position(0);
    motion_wait();

    get_base(); // 壁制御のための基準値取得

    //====往路: ゴールまで探索走行====
    search_to_goal();
    hw_delay_ms(500);

    //====復路: 探索しながらスタート地点へ戻る====
    goal_x = goal_y = 0;
    goal_size = 1; // スタートは(0,0)の1区画だけ
    search_to_goal();

    goal_x = GOAL_X;
    goal_y = GOAL_Y;
    goal_size = GOAL_SIZE; // ゴール設定を元に戻しておく

    hw_motor_disable(); // 励磁を切る
}
