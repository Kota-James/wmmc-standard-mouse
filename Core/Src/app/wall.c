/*
 * wall.c — 壁判定の実装
 */

#include "app/wall.h"
#include "app/params.h"
#include "hw/hw_sensor.h"

uint8_t wall_info;
uint16_t base_l, base_r;

void wall_init(void) {
    wall_info = 0x00;
    base_l = base_r = 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++
// get_wall_info
// 壁情報を取得する
// AD値が閾値より大きい（＝壁があって光が跳ね返ってきている）なら壁ありと判断する
//+++++++++++++++++++++++++++++++++++++++++++++++
void get_wall_info(void) {
    wall_info = 0x00;

    // ================= 課題1 =================
    // TODO: センサ値としきい値を比べて壁の有無を判定し，wall_infoにビットを立てる
    //
    // 使うもの:
    //   hw_sensor_front_right(), hw_sensor_front_left(),
    //   hw_sensor_right(), hw_sensor_left()        … 最新のセンサ値
    //   WALL_BASE_FR, WALL_BASE_FL, WALL_BASE_R, WALL_BASE_L … しきい値(params.h)
    //   WALL_FRONT, WALL_RIGHT, WALL_LEFT          … 立てるビット(wall.h)
    //
    // ヒント: 前壁はセンサが2つある。どう組み合わせるべきか？
    // 詳細: docs/exercises/01_壁判定.md
    // =========================================
}

//+++++++++++++++++++++++++++++++++++++++++++++++
// get_base
// 壁制御用の基準値を取得する
// 両側に壁がある区画の中央に機体を置いた状態で呼ぶこと
// 戻り値：理想的な値を取得できたか 1:できた 0:できなかった
//+++++++++++++++++++++++++++++++++++++++++++++++
uint8_t get_base(void) {
    base_l = hw_sensor_left();
    base_r = hw_sensor_right();
    return 1;
}
