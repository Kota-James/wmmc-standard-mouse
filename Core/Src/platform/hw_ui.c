/*
 * hw_ui.c — LED・スイッチ（STM32F303K8実装）
 */

#include "main.h"
#include "hw/hw_ui.h"

void hw_led_write(uint8_t led1, uint8_t led2, uint8_t led3) {
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin,
                      led1 ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin,
                      led2 ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin,
                      led3 ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void hw_led_write4(uint8_t led4) {
    HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin,
                      led4 ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

uint8_t hw_switch_pressed(hw_switch_t sw) {
    switch (sw) {
    case HW_SW1:
        return HAL_GPIO_ReadPin(SW1_GPIO_Port, SW1_Pin) == GPIO_PIN_RESET;
    case HW_SW2:
        return HAL_GPIO_ReadPin(SW2_GPIO_Port, SW2_Pin) == GPIO_PIN_RESET;
    case HW_SW3:
        return HAL_GPIO_ReadPin(SW3_GPIO_Port, SW3_Pin) == GPIO_PIN_RESET;
    default:
        return 0;
    }
}
