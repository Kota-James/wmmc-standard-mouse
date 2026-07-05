/*
 * hw_sensor.h — 壁センサ・バッテリ監視の境界API
 *
 * 赤外線LEDの発光タイミングやADコンバータの扱いは platform/hw_sensor.c に隠蔽し、
 * app層は「最新のセンサ値を読む」ことだけできる。
 * 値は4ms周期のタイマ割り込みで自動更新される（12msで全センサが一巡）。
 */

#ifndef HW_HW_SENSOR_H_
#define HW_HW_SENSOR_H_

#include <stdint.h>

void hw_sensor_init(void);

// 最新のセンサ値（12bit: 0-4095。壁が近いほど大きい）
uint32_t hw_sensor_left(void);
uint32_t hw_sensor_right(void);
uint32_t hw_sensor_front_left(void);
uint32_t hw_sensor_front_right(void);

// バッテリ電圧の分圧AD値と低電圧判定
uint32_t hw_sensor_battery(void);
uint8_t hw_sensor_battery_is_low(void);

// タイマ割り込みハンドラから呼ばれる（platform内部用。appから呼んではいけない）
void hw_sensor_isr_tick(void);

#endif /* HW_HW_SENSOR_H_ */
