/*
 * hw_sensor.c — 壁センサ・バッテリ監視（STM32F303K8実装）
 *
 * TIM6の4ms周期割り込みで3つのフェーズを巡回する:
 *   フェーズ0: 横（左右）センサの取得
 *   フェーズ1: 前センサ + バッテリ電圧の取得
 *   フェーズ2: 制御処理（app層の app_control_tick() を呼ぶ）
 * 赤外線LEDは読む直前だけ点灯する（外乱光の影響を抑え，消費電力も減らすため）。
 */

#include "main.h"
#include "hw/hw_sensor.h"
#include "hw/hw_ui.h"
#include "hw/app_hooks.h"

//----赤外線LED発光から読み取りまでの待機時間[µs]（光が強まるまで待つ）----
#define IR_WAIT_US 15

//----バッテリ低電圧のしきい値----
// 33kΩと10kΩの分圧抵抗を通して測定している → 11.1V*(10/(10+33)/3.3V)*4096 = 3204
#define BATT_LOW_VOL (3204 * 0.88)

//----状態（割り込みが更新するためvolatile）----
static volatile uint8_t tp;                                  // タスクポインタ（フェーズ番号）
static volatile uint32_t ad_l, ad_r, ad_fl, ad_fr, ad_batt;  // 最新のAD値
static volatile uint16_t low_vol_count;                      // 低電圧の連続検出カウント
static volatile uint8_t battery_low;                         // 低電圧フラグ

//+++++++++++++++++++++++++++++++++++++++++++++++
// get_adc_value
// 指定されたチャンネルのアナログ電圧値を取り出す
//+++++++++++++++++++++++++++++++++++++++++++++++
static uint32_t get_adc_value(ADC_HandleTypeDef *hadc, uint32_t channel) {
    ADC_ChannelConfTypeDef sConfig = {0};

    sConfig.Channel = channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.SamplingTime = ADC_SAMPLETIME_19CYCLES_5;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    HAL_ADC_ConfigChannel(hadc, &sConfig);

    HAL_ADC_Start(hadc);                  // AD変換を開始する
    HAL_ADC_PollForConversion(hadc, 100); // AD変換終了まで待機する
    return HAL_ADC_GetValue(hadc);        // AD変換結果を取得する
}

//+++++++++++++++++++++++++++++++++++++++++++++++
// tim6_wait_us
// 1µsごとにカウントアップするTIM6のカウンタを使った待機
// （オーバーフローを跨げないのでTIM6割り込みハンドラ内でのみ使用可）
//+++++++++++++++++++++++++++++++++++++++++++++++
static void tim6_wait_us(uint32_t us) {
    uint32_t dest = __HAL_TIM_GET_COUNTER(&htim6) + us;
    while (__HAL_TIM_GET_COUNTER(&htim6) < dest)
        ;
}

void hw_sensor_init(void) {
    tp = 0;
    ad_l = ad_r = ad_fr = ad_fl = ad_batt = 0;
    low_vol_count = 0;
    battery_low = 0;

    // TIM6の更新割り込みを有効化して周期タスクを開始する
    __HAL_TIM_CLEAR_FLAG(&htim6, TIM_FLAG_UPDATE);
    __HAL_TIM_ENABLE_IT(&htim6, TIM_IT_UPDATE);
    HAL_TIM_Base_Start(&htim6);
}

uint32_t hw_sensor_left(void) { return ad_l; }
uint32_t hw_sensor_right(void) { return ad_r; }
uint32_t hw_sensor_front_left(void) { return ad_fl; }
uint32_t hw_sensor_front_right(void) { return ad_fr; }
uint32_t hw_sensor_battery(void) { return ad_batt; }
uint8_t hw_sensor_battery_is_low(void) { return battery_low; }

/*==========================================================
    TIM6割り込み（4ms周期）からのフェーズ処理
==========================================================*/
void hw_sensor_isr_tick(void) {
    switch (tp) {
    case 0:
        // 左右センサ値の取得
        HAL_GPIO_WritePin(IR_SIDE_GPIO_Port, IR_SIDE_Pin, GPIO_PIN_SET); // 発光
        tim6_wait_us(IR_WAIT_US);
        ad_r = get_adc_value(&hadc2, ADC_CHANNEL_1); // SENSOR_R
        ad_l = get_adc_value(&hadc1, ADC_CHANNEL_1); // SENSOR_L
        HAL_GPIO_WritePin(IR_SIDE_GPIO_Port, IR_SIDE_Pin, GPIO_PIN_RESET); // 消灯
        break;

    case 1:
        // 正面センサ値の取得
        HAL_GPIO_WritePin(IR_FRONT_GPIO_Port, IR_FRONT_Pin, GPIO_PIN_SET); // 発光
        tim6_wait_us(IR_WAIT_US);
        ad_fr = get_adc_value(&hadc1, ADC_CHANNEL_2); // SENSOR_FR
        ad_fl = get_adc_value(&hadc2, ADC_CHANNEL_4); // SENSOR_FL
        HAL_GPIO_WritePin(IR_FRONT_GPIO_Port, IR_FRONT_Pin, GPIO_PIN_RESET); // 消灯

        // バッテリー電圧の取得と低電圧判定（連続して下回ったときだけ警告する）
        ad_batt = get_adc_value(&hadc1, ADC_CHANNEL_12); // VOL_CHECK
        if (ad_batt < BATT_LOW_VOL) {
            low_vol_count++;
            if (low_vol_count > 1000) {
                battery_low = 1;
            }
        } else {
            low_vol_count = 0;
            battery_low = 0;
        }
        hw_led_write4(battery_low);
        break;

    case 2:
        // 制御処理（中身はアルゴリズムなのでapp層に委ねる）
        app_control_tick();
        break;
    }

    tp = (tp + 1) % 3; // 次のフェーズへ
}
