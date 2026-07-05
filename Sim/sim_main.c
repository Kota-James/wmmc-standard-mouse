/*
 * sim_main.c — 迷路探索シミュレータ（PC用）
 *
 * ファームウェアと同じ Core/Src/app/maze.c, solver.c をそのままリンクし，
 * 仮想マウスに探索走行をさせる。実機と同じく「今いる区画の前・左・右の壁しか
 * 見えない」という制約を再現している。
 *
 * 使い方:
 *   make
 *   ./sim mazes/real/japan2019.maze [goal_x goal_y]
 *   ./sim --lefthand mazes/perfect.maze 15 15 … 左手法で歩く（課題3の実験用）
 *
 * 終了コード: 0=成功（課題の自動判定に使える）
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "app/maze.h"
#include "app/solver.h"
#include "hw/hw_eeprom.h"

#define MAX_STEPS 10000 // これを超えたら暴走とみなす

//----正解の迷路（真の壁情報。仮想マウスは直接見られない）----
static uint8_t truth[MAZE_SIZE][MAZE_SIZE]; // 各バイト下位4bit: bit3=北 bit2=東 bit1=南 bit0=西

/*==========================================================
    hw_eeprom のスタブ（PCではただの配列）
==========================================================*/
static uint16_t fake_eeprom[1024];

int hw_eeprom_enable_write(void) {
    memset(fake_eeprom, 0xff, sizeof(fake_eeprom));
    return 0;
}
int hw_eeprom_disable_write(void) { return 0; }
int hw_eeprom_write_halfword(uint32_t offset, uint16_t data) {
    fake_eeprom[offset] = data;
    return 0;
}
uint16_t hw_eeprom_read_halfword(uint32_t offset) { return fake_eeprom[offset]; }

/*==========================================================
    迷路ファイルの読み込み
    micromouseonline/mazefiles と同じテキスト形式:
      o---o---o ... 柱は'o'，横壁は'---'，縦壁は'|'
      33行 × (16*4+1)列。ファイルの1行目が迷路の北端
==========================================================*/
static int load_maze(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "cannot open %s\n", path);
        return -1;
    }
    char lines[MAZE_SIZE * 2 + 1][256];
    int n = 0;
    while (n < MAZE_SIZE * 2 + 1 && fgets(lines[n], sizeof(lines[0]), fp)) {
        n++;
    }
    fclose(fp);
    if (n < MAZE_SIZE * 2 + 1) {
        fprintf(stderr, "maze file too short (%d lines)\n", n);
        return -1;
    }

    memset(truth, 0, sizeof(truth));
    for (int y = 0; y < MAZE_SIZE; y++) {
        // ファイル上の行: 北壁行 = (15-y)*2, 縦壁行 = (15-y)*2+1, 南壁行 = (15-y)*2+2
        const char *north = lines[(MAZE_SIZE - 1 - y) * 2];
        const char *mid = lines[(MAZE_SIZE - 1 - y) * 2 + 1];
        const char *south = lines[(MAZE_SIZE - 1 - y) * 2 + 2];
        for (int x = 0; x < MAZE_SIZE; x++) {
            if (north[x * 4 + 2] == '-') truth[y][x] |= 0x08;
            if (south[x * 4 + 2] == '-') truth[y][x] |= 0x02;
            if (mid[x * 4] == '|') truth[y][x] |= 0x01;
            if (mid[x * 4 + 4] == '|') truth[y][x] |= 0x04;
        }
    }
    return 0;
}

/*==========================================================
    仮想マウスのセンサと移動
==========================================================*/
// 4bit壁情報を左に回転させる（絶対方位→自機から見た向きへの変換）
static uint8_t rot4_left(uint8_t bits, uint8_t n) {
    n &= 3;
    return ((bits << n) | (bits >> (4 - n))) & 0x0f;
}

// 実機の get_wall_info() 相当: 前・右・左の壁だけ見える（後ろは見えない）
static uint8_t sense_walls(void) {
    uint8_t rel = rot4_left(truth[mouse.y][mouse.x], mouse.dir);
    rel &= 0x0d; // 後壁ビット(bit1)は実機センサでは見えないので落とす
    return rel | (rel << 4); // wall.h と同じ二重ニブル形式にする
}

// 1区画前進する。壁を突き抜けたら異常終了
static int move_forward(void) {
    uint8_t abs_walls = truth[mouse.y][mouse.x];
    if (abs_walls & (0x08 >> mouse.dir)) { // 向いている方角に壁がある
        printf("*** CRASH: (%d,%d) dir=%d で壁に激突 ***\n", mouse.x, mouse.y,
               mouse.dir);
        return -1;
    }
    maze_advance_position();
    return 0;
}

/*==========================================================
    マップの表示
==========================================================*/
static void print_maze(const uint8_t walls[MAZE_SIZE][MAZE_SIZE],
                       const uint8_t steps[MAZE_SIZE][MAZE_SIZE]) {
    for (int y = MAZE_SIZE - 1; y >= 0; y--) {
        // 北壁
        for (int x = 0; x < MAZE_SIZE; x++) {
            printf("o%s", (walls[y][x] & 0x08) ? "---" : "   ");
        }
        printf("o\n");
        // 西壁とセル内容
        for (int x = 0; x < MAZE_SIZE; x++) {
            printf("%c", (walls[y][x] & 0x01) ? '|' : ' ');
            if (steps && steps[y][x] != 0xff) {
                printf("%3d", steps[y][x]);
            } else if (x == (int)mouse.x && y == (int)mouse.y) {
                printf(" M ");
            } else {
                printf("   ");
            }
        }
        printf("%c\n", (walls[y][MAZE_SIZE - 1] & 0x04) ? '|' : ' ');
    }
    for (int x = 0; x < MAZE_SIZE; x++) {
        printf("o%s", (walls[0][x] & 0x02) ? "---" : "   ");
    }
    printf("o\n");
}

/*==========================================================
    探索走行（strategy.c の search_to_goal と同じ流れ。移動は一瞬で終わる）
==========================================================*/
static int run_search(int *steps_taken) {
    int steps = 0;
    uint8_t wall_info;

    if (maze_is_second_run()) {
        maze_load_from_eeprom();
    }

    //====スタート位置の壁情報取得====
    wall_info = sense_walls();
    wall_info &= ~0x88; // スタート区画の前壁は存在しないはず
    maze_write_walls(wall_info);

    //====最初の半区画前進（実機は問答無用で前進する）====
    if (move_forward() != 0) return -1;
    wall_info = sense_walls();
    maze_write_walls(wall_info);
    steps++;

    //====歩数マップ・経路作成====
    r_cnt = 0;
    solver_make_step_map();
    solver_make_route();

    //====探索走行====
    do {
        switch (route[r_cnt++]) {
        case 0x88: // 前進
            break;
        case 0x44: // 右折
            maze_turn(DIR_TURN_R90);
            break;
        case 0x22: // Uターン
            maze_turn(DIR_TURN_180);
            break;
        case 0x11: // 左折
            maze_turn(DIR_TURN_L90);
            break;
        default:
            printf("*** 不正な経路データ 0x%02x（経路が尽きた） ***\n",
                   route[r_cnt - 1]);
            return -1;
        }
        if (move_forward() != 0) return -1;
        steps++;
        if (steps > MAX_STEPS) {
            printf("*** %d 歩を超えても到達できない（暴走） ***\n", MAX_STEPS);
            return -1;
        }
        wall_info = sense_walls();
        solver_update_route(wall_info);
    } while (!maze_is_goal(mouse.x, mouse.y));

    // 実機はゴールで停止後に180度回転してから次の走行に備える（strategy.c参照）
    maze_turn(DIR_TURN_180);

    if (!maze_is_second_run()) {
        maze_store_to_eeprom();
    }
    *steps_taken = steps;
    return 0;
}

/*==========================================================
    左手法（課題3の実験用）
    実機の lefthand_run() と同じ判断を仮想マウスで行う
==========================================================*/
static int run_lefthand(void) {
    int steps = 0;
    printf("左手法で歩きます（優先順位: 左折 > 直進 > 右折 > Uターン）\n");
    while (!maze_is_goal(mouse.x, mouse.y) && steps < MAX_STEPS) {
        uint8_t rel = rot4_left(truth[mouse.y][mouse.x], mouse.dir);
        if (!(rel & 0x01)) { // 左が空いていれば左折
            maze_turn(DIR_TURN_L90);
        } else if (!(rel & 0x08)) { // 前が空いていれば直進
        } else if (!(rel & 0x04)) { // 右が空いていれば右折
            maze_turn(DIR_TURN_R90);
        } else { // 行き止まりならUターン
            maze_turn(DIR_TURN_180);
        }
        if (move_forward() != 0) return -1;
        steps++;
    }
    if (maze_is_goal(mouse.x, mouse.y)) {
        printf("結果: 左手法でゴール到達（%d 歩）\n", steps);
        return 0;
    }
    printf("結果: %d 歩歩いてもゴールに着けなかった。\n"
           "この迷路にはループ（回り道）があり，左手法では中心に入れない。\n"
           "なぜかは docs/exercises/03_左手法.md で考えよう\n",
           steps);
    return 1;
}

/*==========================================================
    main
==========================================================*/
int main(int argc, char **argv) {
    int lefthand = 0;
    if (argc > 1 && strcmp(argv[1], "--lefthand") == 0) {
        lefthand = 1;
        argv++;
        argc--;
    }
    if (argc < 2) {
        fprintf(stderr,
                "usage: %s [--lefthand] <maze-file> [goal_x goal_y]\n",
                argv[0]);
        return 2;
    }
    if (load_maze(argv[1]) != 0) {
        return 2;
    }
    if (truth[0][0] & 0x08) {
        // 競技ルールではスタート区画は北だけが開いている（実機もその前提で最初に前進する）
        fprintf(stderr, "invalid maze: (0,0) の北が壁になっている\n");
        return 2;
    }

    maze_init();
    solver_init();
    // ゴール既定値は日本のクラシック競技ルール: 中央2×2領域（南西角(7,7)）。
    // 座標を明示指定した場合は1区画ゴール（サイズは第4引数で変更可）
    goal_x = (argc > 2) ? (uint8_t)atoi(argv[2]) : 7;
    goal_y = (argc > 3) ? (uint8_t)atoi(argv[3]) : 7;
    goal_size = (argc > 4) ? (uint8_t)atoi(argv[4]) : ((argc > 2) ? 1 : 2);

    printf("====== 迷路: %s  ゴール: (%d,%d) %d×%d区画 ======\n", argv[1],
           goal_x, goal_y, goal_size, goal_size);
    print_maze((const uint8_t(*)[MAZE_SIZE])truth, NULL);

    //====左手法モード====
    if (lefthand) {
        return (run_lefthand() == 0) ? 0 : 1;
    }

    //====一次走行（探索）====
    int steps1 = 0;
    maze_set_second_run(0);
    if (run_search(&steps1) != 0) {
        printf("結果: 探索走行 失敗\n");
        return 1;
    }
    printf("\n====== 探索走行成功: %d 歩でゴール到達 ======\n", steps1);

    //====帰り探索（スタートへ戻る）====
    int steps_back = 0;
    uint8_t gx = goal_x, gy = goal_y, gs = goal_size;
    goal_x = goal_y = 0;
    goal_size = 1; // スタートは(0,0)の1区画だけ
    if (run_search(&steps_back) != 0) {
        printf("結果: 帰り探索 失敗\n");
        return 1;
    }
    printf("====== 帰り探索成功: %d 歩 ======\n", steps_back);
    goal_x = gx;
    goal_y = gy;
    goal_size = gs;

    //====二次走行（保存済みマップで最短走行）====
    if (!maze_in_eeprom_is_valid()) {
        printf("結果: EEPROMにマップが無い（保存処理の異常）\n");
        return 1;
    }
    int steps2 = 0;
    mouse.x = mouse.y = 0;
    mouse.dir = DIR_NORTH;
    maze_set_second_run(1);
    if (run_search(&steps2) != 0) {
        printf("結果: 二次走行 失敗\n");
        return 1;
    }
    printf("====== 二次走行成功: %d 歩 ======\n", steps2);

    //====歩数マップを表示====
    printf("\n最終的な歩数マップ（既知の壁のみ）:\n");
    maze_set_second_run(0);
    mouse.x = mouse.y = 0;
    mouse.dir = DIR_NORTH;
    solver_make_step_map();
    print_maze((const uint8_t(*)[MAZE_SIZE])truth, (const uint8_t(*)[MAZE_SIZE])smap);

    printf("\n結果: すべて成功（探索 %d 歩 / 帰り %d 歩 / 最短 %d 歩）\n",
           steps1, steps_back, steps2);
    return 0;
}
