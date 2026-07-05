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
    // ================= 課題5-1 =================
    // TODO: 現在位置からgoal座標まで足立法で連続探索走行する
    //
    // 流れ:
    //   1. 二次走行なら maze_load_from_eeprom() で保存済みマップを読み込む
    //   2. スタート区画の壁を読む: get_wall_info() → wall_info &= ~WALL_FRONT
    //      （前壁は無い前提）→ maze_write_walls(wall_info)
    //   3. motion_half_section_accel() で最初の半区画を前進し，
    //      maze_advance_position() と壁の記録を行う
    //   4. r_cnt=0 にして solver_make_step_map() → solver_make_route()
    //   5. ゴールに着くまで繰り返し:
    //        route[r_cnt++] の値で分岐（0x88=前進, 0x44=右折, 0x22=Uターン, 0x11=左折）
    //        - 前進: motion_one_section_const()
    //        - 旋回: motion_half_section_decel() → motion_rotate_*() →
    //                maze_turn(DIR_TURN_*) → motion_half_section_accel()
    //        - Uターンでは turn_around_with_touch_up() を使うと位置誤差をリセットできる
    //        毎回 maze_advance_position() と solver_update_route(wall_info) を呼ぶ
    //   6. ゴールしたら motion_half_section_decel() で停止 →
    //      hw_delay_ms(2200)（2秒以上停止は競技規定）→ motion_rotate_180() +
    //      maze_turn(DIR_TURN_180)
    //   7. 一次走行なら maze_store_to_eeprom() でマップを保存する
    //
    // Sim/sim_main.c の run_search() に同じ流れの動く例がある（移動が一瞬なだけ）。
    // 詳細: docs/exercises/05_走行戦略.md
    // ==========================================
    (void)turn_around_with_touch_up; // 実装したらこの行は消す（未使用警告よけ）
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

    // ================= 課題5-2 =================
    // TODO: 走行前の準備と往復の探索走行を組み立てる
    //
    // 流れ:
    //   1. 尻当てで位置と向きを合わせる:
    //      右を向いて尻当て→左を向き直して尻当てすると，
    //      横方向・縦方向の両方の位置と角度が揃う（なぜか考えること）
    //      使う関数: motion_rotate_right90/left90(), motion_set_position(0),
    //                motion_wait()
    //   2. get_base() で壁制御の基準値を取る
    //   3. search_to_goal() でゴールへ → hw_delay_ms(500)
    //   4. goal_x = goal_y = 0 にして search_to_goal() でスタートへ戻る
    //   5. goal座標を元に戻す
    //
    // 詳細: docs/exercises/05_走行戦略.md
    // ==========================================
    (void)search_to_goal; // 実装したらこの行は消す（未使用警告よけ）

    hw_motor_disable(); // 励磁を切る
}
