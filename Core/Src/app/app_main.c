/*
 * app_main.c — モード選択ループ（マウスの最上位処理）
 */

#include <stdio.h>
#include "app/app_main.h"
#include "app/params.h"
#include "app/motion.h"
#include "app/wall.h"
#include "app/control.h"
#include "app/maze.h"
#include "app/solver.h"
#include "app/strategy.h"
#include "app/ui.h"
#include "hw/hw_sensor.h"
#include "hw/hw_ui.h"
#include "hw/hw_system.h"

//+++++++++++++++++++++++++++++++++++++++++++++++
// sensor_check_mode
// センサ値と壁判定を表示し続ける（壁判断閾値の調整用）
//+++++++++++++++++++++++++++++++++++++++++++++++
static void sensor_check_mode(void) {
    printf("Sensor Check.\n");
    while (1) {
        get_wall_info();
        hw_led_write(wall_info & WALL_RIGHT, wall_info & WALL_FRONT,
                     wall_info & WALL_LEFT);
        printf(" ad_l : %4lu, ad_fl : %4lu, ad_fr : %4lu, ad_r : %4lu, "
               "ad_batt : %4lu\n",
               hw_sensor_left(), hw_sensor_front_left(),
               hw_sensor_front_right(), hw_sensor_right(),
               hw_sensor_battery());
        printf("dif_l : %4d, dif_r : %4d\n", dif_l, dif_r);
        printf("Left : [%c], Front : [%c], Right : [%c]\n",
               (wall_info & WALL_LEFT) ? 'X' : ' ',
               (wall_info & WALL_FRONT) ? 'X' : ' ',
               (wall_info & WALL_RIGHT) ? 'X' : ' ');

        // バッテリーが消耗するとLED4が点灯する（hw_sensor.c参照）
        hw_delay_ms(333);
    }
}

//+++++++++++++++++++++++++++++++++++++++++++++++
// app_main
// 各モジュールを初期化し，モード選択ループを回す
//+++++++++++++++++++++++++++++++++++++++++++++++
void app_main(void) {
    setbuf(stdout, NULL); // printfを1文字ずつ即時出力する（バッファリング無効化）

    //====各モジュールの初期化====
    hw_sensor_init(); // センサの周期取得を開始（platform層）
    motion_init();    // 走行系（内部でモータも初期化される）
    control_init();   // 壁制御
    wall_init();      // 壁判定
    maze_init();      // 迷路情報
    solver_init();    // 経路探索

    goal_x = GOAL_X;
    goal_y = GOAL_Y; // ゴール座標を設定（params.hで調整）

    printf("***** WMMC Nucleo Mouse 2023 *****\n");

    //====モード選択ループ====
    int mode = 0;
    while (1) {
        mode = select_mode(mode);

        switch (mode) {
        case 0:
            //----基準値を取る----
            // この動作は走行開始時に自動で行われる（strategy.c参照）。
            // このモードは手動で行う場合に使用する
            printf("Mode 0: Get Base Value.\n");
            get_base();
            break;

        case 1:
            //----探索走行----
            printf("Mode 1: 1st Run.\n");
            strategy_run(0);
            break;

        case 2:
            //----二次（最短）走行----
            printf("Mode 2: 2nd Run.\n");
            if (!maze_in_eeprom_is_valid()) {
                // マップ未保存のまま二次走行すると全区画が壁として読まれ
                // 探索がフリーズするため中止する
                printf("No map in EEPROM. Run Mode 1 first.\n");
                break;
            }
            strategy_run(1);
            break;

        case 3:
            //----空きモード----
            // マイクロマウスのルールでは5分間で5回走行できるので，
            // 連続で何回か走行するモードを作っておくと良い。
            // 検索キーワード:「マイクロマウス オートスタート」
            printf("Mode 3: .\n");
            break;

        case 4:
            //----空きモード----
            printf("Mode 4: .\n");
            break;

        case 5:
            //----空きモード----
            printf("Mode 5: .\n");
            break;

        case 6:
            //----テスト走行----
            // このモードを使って区画距離・旋回角度などのパラメータを調整する
            printf("Mode 6: Test Run.\n");
            motion_test_run();
            break;

        case 7:
            //----センサチェック----
            // このモードを使ってセンサで壁の有無を判断するための閾値を調整する
            sensor_check_mode();
            break;
        }
    }
}
