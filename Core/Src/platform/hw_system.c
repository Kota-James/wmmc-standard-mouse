/*
 * hw_system.c — 時間待ち・printf出力先（STM32F303K8実装）
 */

#include "main.h"
#include "hw/hw_system.h"

void hw_delay_ms(uint32_t ms) { HAL_Delay(ms); }

//+++++++++++++++++++++++++++++++++++++++++++++++
//__io_putchar
// printfの出力1文字をUART2（ST-LinkのVirtual COM Port）へ送る
//+++++++++++++++++++++++++++++++++++++++++++++++
int __io_putchar(int c) {
    uint8_t ch = (uint8_t)c;
    if (ch == '\n') {
        uint8_t cr = '\r';
        HAL_UART_Transmit(&huart2, &cr, 1, 1);
    }
    HAL_UART_Transmit(&huart2, &ch, 1, 1);
    return c;
}
