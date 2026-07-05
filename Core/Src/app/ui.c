/*
 * ui.c — モード選択の実装
 */

#include <stdio.h>
#include "app/ui.h"
#include "hw/hw_ui.h"
#include "hw/hw_system.h"

//+++++++++++++++++++++++++++++++++++++++++++++++
// select_mode
// スイッチが押されるたびモード番号を増減し，決定スイッチで確定する
// 100ms待ってから離されるのを待つことでチャタリングを除去している
//+++++++++++++++++++++++++++++++++++++++++++++++
int select_mode(int mode) {
    printf("Mode : %d\n", mode);

    while (1) {
        hw_led_write(mode & 0b001, mode & 0b010, mode & 0b100);

        if (hw_switch_pressed(HW_SW1)) {
            hw_delay_ms(100);
            while (hw_switch_pressed(HW_SW1))
                ;
            mode++;
            if (mode > 7) {
                mode = 0;
            }
            printf("Mode : %d\n", mode);
        }
        if (hw_switch_pressed(HW_SW2)) {
            hw_delay_ms(100);
            while (hw_switch_pressed(HW_SW2))
                ;
            mode--;
            if (mode < 0) {
                mode = 7;
            }
            printf("Mode : %d\n", mode);
        }
        if (hw_switch_pressed(HW_SW3)) {
            hw_delay_ms(100);
            while (hw_switch_pressed(HW_SW3))
                ;
            return mode;
        }
    }
}
