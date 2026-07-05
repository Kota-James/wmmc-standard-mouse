/*
 * solver.c — 経路探索（足立法）の実装
 */

#include "app/solver.h"

uint8_t smap[MAZE_SIZE][MAZE_SIZE];
uint8_t route[256];
uint8_t r_cnt;

#define STEP_UNREACHED 0xff // 歩数マップの「未記入」印

void solver_init(void) { r_cnt = 0; }

//+++++++++++++++++++++++++++++++++++++++++++++++
// solver_make_step_map
// 歩数マップを作成する
// ゴールを歩数0とし，壁がない隣の区画に歩数+1を書き込む作業を
// 自分の座標に歩数が書かれるまで繰り返す
//+++++++++++++++++++++++++++++++++++++++++++++++
void solver_make_step_map(void) {
    uint8_t x, y;

    //----歩数マップのクリア----
    for (y = 0; y < MAZE_SIZE; y++) {
        for (x = 0; x < MAZE_SIZE; x++) {
            smap[y][x] = STEP_UNREACHED;
        }
    }

    //----ゴール座標を歩数0にする----
    uint8_t m_step = 0;
    smap[goal_y][goal_x] = 0;

    //----自分の座標にたどり着くまでループ----
    do {
        // マップ全域を走査し，現在の最大歩数m_stepの区画を見つけたら
        // 壁のない未記入の隣接区画に次の歩数を書き込む
        for (y = 0; y < MAZE_SIZE; y++) {
            for (x = 0; x < MAZE_SIZE; x++) {
                if (smap[y][x] != m_step) {
                    continue;
                }
                uint8_t walls = maze_wall_bits(x, y);
                //----北----
                if (!(walls & 0x08) && y != MAZE_SIZE - 1) {
                    if (smap[y + 1][x] == STEP_UNREACHED) {
                        smap[y + 1][x] = m_step + 1;
                    }
                }
                //----東----
                if (!(walls & 0x04) && x != MAZE_SIZE - 1) {
                    if (smap[y][x + 1] == STEP_UNREACHED) {
                        smap[y][x + 1] = m_step + 1;
                    }
                }
                //----南----
                if (!(walls & 0x02) && y != 0) {
                    if (smap[y - 1][x] == STEP_UNREACHED) {
                        smap[y - 1][x] = m_step + 1;
                    }
                }
                //----西----
                if (!(walls & 0x01) && x != 0) {
                    if (smap[y][x - 1] == STEP_UNREACHED) {
                        smap[y][x - 1] = m_step + 1;
                    }
                }
            }
        }
        m_step++;
    } while (smap[mouse.y][mouse.x] == STEP_UNREACHED &&
             m_step < STEP_UNREACHED);
    // 0xffは未記入印なので，そこまで数えたら到達不能と判断して打ち切る
}

//+++++++++++++++++++++++++++++++++++++++++++++++
// solver_make_route
// 歩数マップを歩数が減る方向へたどり，最短経路route[]を導出する
//+++++++++++++++++++++++++++++++++++++++++++++++
void solver_make_route(void) {
    uint8_t x, y;
    uint8_t dir_temp = mouse.dir; // 探索中に内部方角を使うので退避する
    uint16_t i;

    //----最短経路を初期化----
    for (i = 0; i < 256; i++) {
        route[i] = 0xff;
    }

    //----現在座標から出発----
    uint8_t m_step = smap[mouse.y][mouse.x];
    x = mouse.x;
    y = mouse.y;

    //----ゴール（歩数0）にたどり着くまで歩数が減る方向を選ぶ----
    i = 0;
    do {
        uint8_t walls = maze_wall_bits(x, y);

        //----北を見る----
        if (!(walls & 0x08) && (smap[y + 1][x] < m_step)) {
            route[i] = (DIR_NORTH - mouse.dir) & 0x03; // 進行方向を相対方向に変換
            m_step = smap[y + 1][x];
            y++;
        }
        //----東を見る----
        else if (!(walls & 0x04) && (smap[y][x + 1] < m_step)) {
            route[i] = (DIR_EAST - mouse.dir) & 0x03;
            m_step = smap[y][x + 1];
            x++;
        }
        //----南を見る----
        else if (!(walls & 0x02) && (smap[y - 1][x] < m_step)) {
            route[i] = (DIR_SOUTH - mouse.dir) & 0x03;
            m_step = smap[y - 1][x];
            y--;
        }
        //----西を見る----
        else if (!(walls & 0x01) && (smap[y][x - 1] < m_step)) {
            route[i] = (DIR_WEST - mouse.dir) & 0x03;
            m_step = smap[y][x - 1];
            x--;
        }

        //----相対方向を動作形式（壁情報と同じビット割り当て）に変換----
        switch (route[i]) {
        case 0x00: // 前進
            route[i] = 0x88;
            break;
        case 0x01: // 右折
            maze_turn(DIR_TURN_R90); // 内部方角も回しながら経路を組み立てる
            route[i] = 0x44;
            break;
        case 0x02: // Uターン
            maze_turn(DIR_TURN_180);
            route[i] = 0x22;
            break;
        case 0x03: // 左折
            maze_turn(DIR_TURN_L90);
            route[i] = 0x11;
            break;
        default: // どの方向にも進めなかった場合
            route[i] = 0x00;
            break;
        }
        i++;
    } while (smap[y][x] != 0 && i < 256);
    // route[256]の容量を超えないよう打ち切る

    mouse.dir = dir_temp; // 退避した内部方角を復元する
}

//+++++++++++++++++++++++++++++++++++++++++++++++
// solver_update_route
// 壁情報をマップに書き込み，次の進路が壁で塞がれていれば経路を作り直す
//+++++++++++++++++++++++++++++++++++++++++++++++
void solver_update_route(uint8_t wall_info) {
    //----壁情報書き込み----
    maze_write_walls(wall_info);

    //----最短経路上に壁があれば進路変更----
    if (wall_info & route[r_cnt]) {
        solver_make_step_map();
        solver_make_route();
        r_cnt = 0;
    }
}
