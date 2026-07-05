/*
 * lefthand.c — 左手法（壁伝い走行）の実装
 */

#include <stdio.h>
#include "app/lefthand.h"
#include "app/params.h"
#include "app/motion.h"
#include "app/wall.h"
#include "app/maze.h"
#include "hw/hw_motor.h"
#include "hw/hw_sensor.h"

#define LEFTHAND_MAX_STEPS 999 // これだけ歩いて着かなければループとみなして諦める

//+++++++++++++++++++++++++++++++++++++++++++++++
// lefthand_run
// 左手法: 各区画で立ち止まり，左が空いていれば左折，
// だめなら直進，右折，最後にUターンの順で試して1区画進む
//+++++++++++++++++++++++++++++++++++++++++++++++
void lefthand_run(void) {
#if BATT_CHECK_ENABLED
    if (hw_sensor_battery_is_low()) {
        printf("Battery voltage low. Run aborted.\n");
        return; // 電圧不足のまま走ると脱調や過放電の原因になる（params.h参照）
    }
#endif

    hw_motor_enable();

    //====尻当てで位置と向きを合わせる====
    motion_rotate_right90();
    motion_wait();
    motion_set_position(0);
    motion_wait();
    motion_rotate_left90();
    motion_wait();
    motion_set_position(0);
    motion_wait();

    get_base(); // 壁制御のための基準値取得

    goal_x = GOAL_X;
    goal_y = GOAL_Y;
    goal_size = GOAL_SIZE;
    mouse.x = 0;
    mouse.y = 0;
    mouse.dir = DIR_NORTH;

    //====左手法で1区画ずつ進む====
    uint16_t steps = 0;
    while (!maze_is_goal(mouse.x, mouse.y) && steps < LEFTHAND_MAX_STEPS) {
        get_wall_info(); // 立ち止まった状態で壁を読む

        // ================= 課題3 =================
        // 左手法の優先順位: 左折 > 直進 > 右折 > Uターン
        if (!(wall_info & WALL_LEFT)) { // 左が空いていれば左折
            motion_rotate_left90();
            maze_turn(DIR_TURN_L90); // 内部方角情報も左回転
        } else if (!(wall_info & WALL_FRONT)) { // 前が空いていれば直進
            // 何もしない（そのまま前進する）
        } else if (!(wall_info & WALL_RIGHT)) { // 右が空いていれば右折
            motion_rotate_right90();
            maze_turn(DIR_TURN_R90);
        } else { // 行き止まりならUターン
            motion_rotate_180();
            maze_turn(DIR_TURN_180);
        }
        // =========================================

        motion_one_section();    // 1区画進んで止まる
        maze_advance_position(); // 内部位置情報も前進
        steps++;
    }

    if (maze_is_goal(mouse.x, mouse.y)) {
        printf("Goal! (%u steps)\n", steps);
    } else {
        // ループのある迷路では左手法はゴールに着けないことがある。
        // 特に大会ルールの中央2×2ゴールは周囲がループになるため原理的に到達できない
        printf("Goal not reached in %u steps. (maze has loops?)\n", steps);
    }

    hw_motor_disable();
}
