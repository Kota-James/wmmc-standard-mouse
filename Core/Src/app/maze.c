/*
 * maze.c — 迷路情報の管理の実装
 */

#include <stdio.h>
#include "app/maze.h"
#include "app/wall.h"
#include "hw/hw_eeprom.h"

struct mouse_state mouse;
uint8_t map[MAZE_SIZE][MAZE_SIZE];
uint8_t goal_x, goal_y;

static uint8_t second_run; // 二次走行モードか

// EEPROM上の配置: オフセット0〜255にマップ本体，256に有効性マーカー。
// マーカーが無いままマップを読むと，消去済みFlashの0xFFが「全区画に全壁あり」として
// 読まれ探索がフリーズするため，読み込み前に必ず maze_in_eeprom_is_valid() で確認する
#define MAP_MARKER_OFFSET (MAZE_SIZE * MAZE_SIZE)
#define MAP_VALID_MARKER 0xA5A5

//+++++++++++++++++++++++++++++++++++++++++++++++
// maze_init
// マップと自機位置を初期化する
//+++++++++++++++++++++++++++++++++++++++++++++++
void maze_init(void) {
    uint8_t x, y;

    //----マップのクリア----
    for (y = 0; y < MAZE_SIZE; y++) {
        for (x = 0; x < MAZE_SIZE; x++) {
            map[y][x] = 0xf0; // 上位4bit（二次走行用）を壁あり，
                              // 下位4bit（一次走行用）を壁なしで初期化する
        }
    }

    //----確定壁（外周）の配置----
    for (y = 0; y < MAZE_SIZE; y++) {
        map[y][0] |= 0xf1;             // 最西列に西壁
        map[y][MAZE_SIZE - 1] |= 0xf4; // 最東列に東壁
    }
    for (x = 0; x < MAZE_SIZE; x++) {
        map[0][x] |= 0xf2;             // 最南行に南壁
        map[MAZE_SIZE - 1][x] |= 0xf8; // 最北行に北壁
    }

    //----自機位置の初期化----
    mouse.x = 0;
    mouse.y = 0;
    mouse.dir = DIR_NORTH;

    second_run = 0;
}

void maze_set_second_run(uint8_t second) { second_run = second; }
uint8_t maze_is_second_run(void) { return second_run; }

//+++++++++++++++++++++++++++++++++++++++++++++++
// maze_wall_bits
// 指定区画の壁情報4bit（NESW）を返す
// 一次走行なら下位ニブル（未探索=壁なし），二次走行なら上位ニブル（未探索=壁あり）
//+++++++++++++++++++++++++++++++++++++++++++++++
uint8_t maze_wall_bits(uint8_t x, uint8_t y) {
    uint8_t m = map[y][x];
    if (second_run) {
        m >>= 4;
    }
    return m & 0x0f;
}

//+++++++++++++++++++++++++++++++++++++++++++++++
// maze_write_walls
// 自機から見た壁情報を絶対方位に補正してマップに書き込む。
// 隣の区画から見た壁（例:自分の北壁=北隣の南壁）も同時に更新する
//+++++++++++++++++++++++++++++++++++++++++++++++
void maze_write_walls(uint8_t wall_info) {
    uint8_t m_temp;

    //----壁情報の向き補正----
    m_temp = (wall_info >> mouse.dir) & 0x0f; // 自機方位分シフトして絶対方位に直す
    m_temp |= (m_temp << 4); // 上位ニブルにも同じ値を入れる（探索済み=確定壁）

    //----自機位置に書き込み----
    map[mouse.y][mouse.x] = m_temp;

    //----隣接区画への反映----
    // 北隣: 自分の北壁は隣の南壁
    if (mouse.y != MAZE_SIZE - 1) {
        if (m_temp & 0x88) {
            map[mouse.y + 1][mouse.x] |= 0x22;
        } else {
            map[mouse.y + 1][mouse.x] &= 0xDD;
        }
    }
    // 東隣: 自分の東壁は隣の西壁
    if (mouse.x != MAZE_SIZE - 1) {
        if (m_temp & 0x44) {
            map[mouse.y][mouse.x + 1] |= 0x11;
        } else {
            map[mouse.y][mouse.x + 1] &= 0xEE;
        }
    }
    // 南隣: 自分の南壁は隣の北壁
    if (mouse.y != 0) {
        if (m_temp & 0x22) {
            map[mouse.y - 1][mouse.x] |= 0x88;
        } else {
            map[mouse.y - 1][mouse.x] &= 0x77;
        }
    }
    // 西隣: 自分の西壁は隣の東壁
    if (mouse.x != 0) {
        if (m_temp & 0x11) {
            map[mouse.y][mouse.x - 1] |= 0x44;
        } else {
            map[mouse.y][mouse.x - 1] &= 0xBB;
        }
    }
}

//+++++++++++++++++++++++++++++++++++++++++++++++
// maze_advance_position
// 内部位置情報を向いている方角へ1区画進める
//+++++++++++++++++++++++++++++++++++++++++++++++
void maze_advance_position(void) {
    switch (mouse.dir) {
    case DIR_NORTH:
        mouse.y++;
        break;
    case DIR_EAST:
        mouse.x++;
        break;
    case DIR_SOUTH:
        mouse.y--;
        break;
    case DIR_WEST:
        mouse.x--;
        break;
    }
}

//+++++++++++++++++++++++++++++++++++++++++++++++
// maze_turn
// 内部方角情報を回転させる
//+++++++++++++++++++++++++++++++++++++++++++++++
void maze_turn(uint8_t turn) {
    mouse.dir = (mouse.dir + turn) & 0x03; // 4方位で巻き戻るよう2bitでマスクする
}

/*==========================================================
    EEPROMへの保存・読み出し
==========================================================*/
void maze_store_to_eeprom(void) {
    int failed = 0;
    int i, j;

    if (hw_eeprom_enable_write() != 0) {
        printf("EEPROM: erase failed. Map was not saved.\n");
        return;
    }
    for (i = 0; i < MAZE_SIZE; i++) {
        for (j = 0; j < MAZE_SIZE; j++) {
            if (hw_eeprom_write_halfword(i * MAZE_SIZE + j,
                                         (uint16_t)map[i][j]) != 0) {
                failed = 1;
            }
        }
    }
    if (!failed) {
        // 全データを書けたときだけ有効性マーカーを記録する
        failed = (hw_eeprom_write_halfword(MAP_MARKER_OFFSET,
                                           MAP_VALID_MARKER) != 0);
    }
    hw_eeprom_disable_write();
    if (failed) {
        printf("EEPROM: write failed. Map data is invalid.\n");
    }
}

void maze_load_from_eeprom(void) {
    int i, j;
    for (i = 0; i < MAZE_SIZE; i++) {
        for (j = 0; j < MAZE_SIZE; j++) {
            map[i][j] = (uint8_t)hw_eeprom_read_halfword(i * MAZE_SIZE + j);
        }
    }
}

uint8_t maze_in_eeprom_is_valid(void) {
    return hw_eeprom_read_halfword(MAP_MARKER_OFFSET) == MAP_VALID_MARKER;
}
