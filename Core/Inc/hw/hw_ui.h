/*
 * hw_ui.h — LED・スイッチの境界API
 */

#ifndef HW_HW_UI_H_
#define HW_HW_UI_H_

#include <stdint.h>

typedef enum {
    HW_SW1, // モード選択 +
    HW_SW2, // モード選択 -
    HW_SW3, // 決定
} hw_switch_t;

// led1〜led3: 0で消灯、0以外で点灯
void hw_led_write(uint8_t led1, uint8_t led2, uint8_t led3);
void hw_led_write4(uint8_t led4);

// 押されていれば1（チャタリング除去は呼び出し側で行う）
uint8_t hw_switch_pressed(hw_switch_t sw);

#endif /* HW_HW_UI_H_ */
